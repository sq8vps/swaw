import numpy as np

MAX30102_SAMPLES_PER_SECOND = 50
MAX30102_BUFFER_LENGTH = 6 * MAX30102_SAMPLES_PER_SECOND
FS = MAX30102_SAMPLES_PER_SECOND  # MAX30102_SAMPLES_PER_SECOND
BUFFER_SIZE = (MAX30102_BUFFER_LENGTH - MAX30102_SAMPLES_PER_SECOND)
HR_FIFO_SIZE = 7
MA4_SIZE = 4  # DO NOT CHANGE
HAMMING_SIZE = 5  # DO NOT CHANGE

auw_hamm = np.array([41, 276, 512, 276, 41])  # Hamm = long16(512* hamming(5)')
# uch_spo2_table is computed as  -45.060*ratioAverage* ratioAverage + 30.354 *ratioAverage + 94.845 ;
uch_spo2_table = np.array([95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99,
                           99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                           100, 100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97,
                           97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91,
                           90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81,
                           80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67,
                           66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50,
                           49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29,
                           28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5,
                           3, 2, 1])
an_dx = np.zeros((BUFFER_SIZE - MA4_SIZE), dtype=int)  # delta
an_x = np.zeros(BUFFER_SIZE, dtype=int)  # ir
an_y = np.zeros(BUFFER_SIZE, dtype=int)  # red


def min(x, y):
    return x if x < y else y


def max30102_init_param(param):
    """
        MAX30102 initialization
        Parameters:
        param (int): MAX30102_SAMPLES_PER_SECOND value from sensor configuration

    """
    global MAX30102_SAMPLES_PER_SECOND
    global MAX30102_BUFFER_LENGTH
    global FS
    global BUFFER_SIZE
    MAX30102_SAMPLES_PER_SECOND = param
    MAX30102_BUFFER_LENGTH = 6 * MAX30102_SAMPLES_PER_SECOND
    FS = MAX30102_SAMPLES_PER_SECOND
    BUFFER_SIZE = (MAX30102_BUFFER_LENGTH - MAX30102_SAMPLES_PER_SECOND)


def maxim_heart_rate_and_oxygen_saturation(pun_ir_buffer, pun_red_buffer, n_buffer_length, un_offset, spo2, heart_rate):
    """
    Calculate the heart rate and SpO2 level

    By detecting peaks of PPG cycle and corresponding AC/DC of red/infra-red signal, the ratio for the SPO2 is computed.
    Since this algorithm is aiming for Arm M0/M3, formula for SPO2 did not achieve the accuracy due to register overflow
    Thus, accurate SPO2 is precalculated and saved in uch_spo2_table[] per each ratio.

    Parameters:
    pun_ir_buffer (numpy.ndarray): IR sensor data buffer
    n_buffer_length (int): IR sensor data buffer length
    pun_red_buffer (numpy.ndarray): Red sensor data buffer
    spo2 (numpy.ndarray): [0] Calculated SpO2 value, [1] 1 if the calculated SpO2 value is valid
    heart_rate (numpy.ndarray): [0] Calculated heart rate value, [1] 1 if the calculated heart rate value is valid

    Returns:
    None
    """
    n_npks_list = np.zeros(1)
    an_ir_valley_locs = np.zeros(15, dtype=np.int32)
    an_exact_ir_valley_locs = np.zeros(15, dtype=np.int32)
    an_dx_peak_locs = np.zeros(15, dtype=np.int32)
    n_peak_interval_sum = 0
    n_y_dc_max_idx = 0
    n_x_dc_max_idx = 0
    an_ratio = np.zeros(5, dtype=np.int32)
    un_offset_tmp = un_offset

    # remove DC of ir signal
    un_ir_mean = 0
    for k in range(n_buffer_length):
        un_ir_mean += pun_ir_buffer[un_offset_tmp]
        un_offset_tmp = (un_offset_tmp + 1) % MAX30102_BUFFER_LENGTH
    un_ir_mean /= n_buffer_length
    un_offset_tmp = un_offset

    for k in range(n_buffer_length):
        an_x[k] = pun_ir_buffer[un_offset_tmp] - un_ir_mean
        un_offset_tmp = (un_offset_tmp + 1) % MAX30102_BUFFER_LENGTH

    # 4 pt Moving Average
    for k in range(BUFFER_SIZE - MA4_SIZE):
        n_denom = an_x[k] + an_x[k + 1] + an_x[k + 2] + an_x[k + 3]
        an_x[k] = n_denom / 4

    # get difference of smoothed IR signal
    for k in range(BUFFER_SIZE - MA4_SIZE-1):
        an_dx[k] = (an_x[k+1] - an_x[k])
    # 2-pt Moving Average to an_dx
    for k in range(BUFFER_SIZE - MA4_SIZE-2):
        an_dx[k] = (an_dx[k] + an_dx[k + 1]) / 2

    # hamming window
    # flip wave form so that we can detect valley with peak detector
    for i in range(BUFFER_SIZE-HAMMING_SIZE-MA4_SIZE-2):
        s = 0
        for k in range(i + HAMMING_SIZE):
            s -= an_dx[k] * auw_hamm[k - i]
        an_dx[i] = s / 1146  # divide by sum of auw_hamm

    n_th1 = 0  # threshold calculation
    for k in range(BUFFER_SIZE-HAMMING_SIZE):
        n_th1 += an_dx[k] if an_dx[k] > 0 else -an_dx[k]
    n_th1 = n_th1 / (BUFFER_SIZE - HAMMING_SIZE)
    # peak location is acutally index for sharpest location of raw signal since we flipped the signal
    # peak_height, peak_distance, max_num_peaks
    maxim_find_peaks(an_dx_peak_locs, n_npks_list, an_dx, BUFFER_SIZE - HAMMING_SIZE, n_th1, 8, 5)
    n_npks = n_npks_list[0]

    if n_npks >= 2:
        for k in range(1, n_npks):
            n_peak_interval_sum += (an_dx_peak_locs[k] - an_dx_peak_locs[k - 1])
        n_peak_interval_sum = n_peak_interval_sum / (n_npks - 1)
        heart_rate[0] = int(6000 / float(n_peak_interval_sum) * (FS / 100.0))
        heart_rate[1] = 1
    else:
        heart_rate[0] = -999
        heart_rate[1] = 0

    for k in range(n_npks):
        an_ir_valley_locs[k] = an_dx_peak_locs[k] + HAMMING_SIZE / 2

    # raw value : RED(=y) and IR(=X)
    # we need to assess DC and AC value of ir and red PPG.
    un_offset_tmp = un_offset
    for k in range(n_buffer_length):
        an_x[k] = pun_ir_buffer[un_offset_tmp]
        an_y[k] = pun_red_buffer[un_offset_tmp]
        un_offset_tmp = (un_offset_tmp + 1) % MAX30102_BUFFER_LENGTH

    # find precise min near an_ir_valley_locs
    n_exact_ir_valley_locs_count = 0
    for k in range(n_npks):
        un_only_once = 1
        m = an_ir_valley_locs[k]
        n_c_min = 16777216  # 2 ^ 24
        if m+5 < BUFFER_SIZE-HAMMING_SIZE and m-5 > 0:
            for i in range(m-5, m+5):
                if an_x[i] < n_c_min:
                    if un_only_once > 0:
                        un_only_once = 0
                    n_c_min = an_x[i]
                    an_exact_ir_valley_locs[k] = i
            if un_only_once == 0:
                n_exact_ir_valley_locs_count += 1
    if n_exact_ir_valley_locs_count < 2:
        spo2[0] = -999
        spo2[1] = 0
        return
    # 4 pt MA
    for k in range(BUFFER_SIZE-MA4_SIZE):
        an_x[k] = (an_x[k] + an_x[k+1] + an_x[k+2] + an_x[k+3]) / 4
        an_y[k] = (an_y[k] + an_y[k+1] + an_y[k+2] + an_y[k+3]) / 4
    # using an_exact_ir_valley_locs , find ir-red DC andir-red AC for SPO2 calibration ratio
    # finding AC/DC maximum of raw ir * red between two valley locations
    n_i_ratio_count = 0
    for k in range(5):
        an_ratio[k] = 0
    for k in range(n_exact_ir_valley_locs_count):
        if an_exact_ir_valley_locs[k] > BUFFER_SIZE:
            spo2[0] = -999
            spo2[1] = 0
            return
    # find max between two valley locations
    # and use ratio betwen AC compoent of Ir & Red and DC compoent of Ir & Red for SPO2

    for k in range(n_exact_ir_valley_locs_count-1):
        n_y_dc_max = -16777216
        n_x_dc_max = - 16777216
        if an_exact_ir_valley_locs[k+1]-an_exact_ir_valley_locs[k] > 10:
            for i in range(an_exact_ir_valley_locs[k], an_exact_ir_valley_locs[k+1]):
                if an_x[i] > n_x_dc_max:
                    n_x_dc_max = an_x[i]
                    n_x_dc_max_idx = i
                if an_y[i] > n_y_dc_max:
                    n_y_dc_max = an_y[i]
                    n_y_dc_max_idx = i
            n_y_ac = (an_y[an_exact_ir_valley_locs[k + 1]] - an_y[an_exact_ir_valley_locs[k]]) * (
                    n_y_dc_max_idx - an_exact_ir_valley_locs[k])  # red
            n_y_ac = an_y[an_exact_ir_valley_locs[k]] + n_y_ac / (
                    an_exact_ir_valley_locs[k + 1] - an_exact_ir_valley_locs[k])
            n_y_ac = an_y[n_y_dc_max_idx] - n_y_ac    # subracting linear DC compoenents from raw
            n_x_ac = (an_x[an_exact_ir_valley_locs[k+1]] - an_x[an_exact_ir_valley_locs[k]]) * \
                     (n_x_dc_max_idx - an_exact_ir_valley_locs[k])  # ir
            n_x_ac = an_x[an_exact_ir_valley_locs[k]] + n_x_ac / \
                     (an_exact_ir_valley_locs[k+1] - an_exact_ir_valley_locs[k])
            n_x_ac = an_x[n_y_dc_max_idx] - n_x_ac      # subracting linear DC compoenents from raw
            n_nume = (n_y_ac * n_x_dc_max) >> 7     # prepare X100 to preserve floating value
            n_denom = (n_x_ac * n_y_dc_max) >> 7
            if n_denom > 0 and n_i_ratio_count < 5 and n_nume != 0:
                # formular is (n_y_ac * n_x_dc_max) / (n_x_ac * n_y_dc_max);
                an_ratio[n_i_ratio_count] = (n_nume * 100) / n_denom
                n_i_ratio_count += 1

    maxim_sort_ascend(an_ratio, n_i_ratio_count)
    n_middle_idx = n_i_ratio_count/2

    if n_middle_idx > 1:
        n_ratio_average = (an_ratio[n_middle_idx - 1] + an_ratio[n_middle_idx]) / 2  # use median
    else:
        n_ratio_average = an_ratio[n_middle_idx]
    if n_ratio_average > 2 and n_ratio_average < 184:
        n_spo2_calc = uch_spo2_table[n_ratio_average]
        spo2[0] = n_spo2_calc
        # float_SPO2 =  -45.060*n_ratio_average* n_ratio_average/10000 + 30.354 *n_ratio_average/100 + 94.845
        spo2[1] = 1
    else:
        spo2[0] = -999
        spo2[1] = 0


def maxim_find_peaks(pn_locs, pn_npks, pn_x, n_size, n_min_height, n_min_distance, n_max_num):
    """
        Find peaks
        Parameters: Details
        Find at most MAX_NUM peaks above MIN_HEIGHT separated by at least MIN_DISTANCE

    """
    maxim_peaks_above_min_height(pn_locs, pn_npks, pn_x, n_size, n_min_height)
    maxim_remove_close_peaks(pn_locs, pn_npks, pn_x, n_min_distance)
    pn_npks[0] = min(pn_npks[0], n_max_num)


def maxim_peaks_above_min_height(pn_locs, pn_npks, pn_x, n_size, n_min_height):
    """
        Find peaks above n_min_height
        Parameters: Details
        Find all peaks above MIN_HEIGHT

    """
    i = 0
    pn_npks[0] = 0
    while i < n_size-1:
        if pn_x[i] > n_min_height and pn_x[i] > pn_x[i-1]:
            n_width = 1
            while i+n_width < n_size and pn_x[i] == pn_x[i+n_width]:
                n_width += 1
            if pn_x[i] > pn_x[i+n_width] and pn_npks[0] < 15:
                pn_locs[pn_npks[0]] = i
                pn_npks[0] += 1
                i += n_width + 1
            else:
                i += n_width
        else:
            i += 1


def maxim_remove_close_peaks(pn_locs, pn_npks, pn_x, n_min_distance):
    """
        Remove peaks
        Parameters: Details
        Remove peaks separated by less than MIN_DISTANCE

    """
    maxim_sort_indices_descend(pn_x, pn_locs, pn_npks)
    for i in range(-1, pn_npks[0]):
        n_old_npks = pn_npks[0]
        pn_npks[0] = i+1
        for j in range(i+1, n_old_npks):
            n_dist = pn_locs[j] - (-1 if i == -1 else pn_locs[i])
            if n_dist > n_min_distance or n_dist < -n_min_distance:
                pn_locs[pn_npks[0]] = pn_locs[j]
                pn_npks[0] += 1
    # Resort indices longo ascending order
    maxim_sort_ascend(pn_locs, pn_npks)


def maxim_sort_ascend(pn_x, n_size):
    """
        Sort array
        Parameters: Details
        Sort array in ascending order (insertion sort algorithm)

    """
    for i in range(1, n_size):
        n_temp = pn_x[i]
        j = i
        while j > 0 and n_temp < pn_x[j - 1]:
            pn_x[j] = pn_x[j - 1]
            j -= 1
        pn_x[j] = n_temp


def maxim_sort_indices_descend(pn_x, pn_indx, n_size):
    """
        Sort indices
        Parameters: Details
        Sort indices according to descending order (insertion sort algorithm)

    """
    for i in range(1, n_size):
        n_temp = pn_indx[i]
        j = i
        while j > 0 and pn_x[n_temp] > pn_x[pn_indx[j-1]]:
            pn_indx[j] = pn_indx[j - 1]
            j -= 1
        pn_indx[j] = n_temp
