import serial
import struct
import sys
import spo2_algorithm as spo2

EEG_CHANNELS = 4
SERIAL = 'COM10'
SPO2_NUM_OF_SAMPLE = 100
SPO2_SAMPLE_SIZE = 3
CO2_DATA_SIZE = 6


def ParseEeg(payload, size):
    if size < (3 + 3 * EEG_CHANNELS):
        return

    data = struct.unpack('<2Bx' + str(EEG_CHANNELS * 3) + 'B', payload)
    leadState = ((data[0] & 0xF) << 4) | (data[1] >> 4)
    samples = [0] * EEG_CHANNELS

    i = 0
    while i < EEG_CHANNELS:
        # parse each 3 bytes MSB first as 24-bit signed integers
        samples[i] = int.from_bytes(payload[3 * i + 3:3 * i + 6], 'big', signed=True)
        i += 1

    print('Lead state: ' + str(bin(leadState)))
    print('Samples: ' + str(samples))


def ParseSpo2(payload, size):
    #payload format should be:
    # SAMPL_1_IR_MSB .. SAMPL_1_IR_LSB ... SAMPL_N_IR_MSB .. SAMPL_N_IR_LSB, SAMPL_1_RED_MSB .. SAMPL_1_RED_LSB
    #                                                                    ... SAMPL_N_RED_MSB .. SAMPL_N_RED_LSB
    if size < 3*2*SPO2_NUM_OF_SAMPLE:
        return
    spo2_val = [0] * 2  # [0] SPO2_Value [1] SP02_VALID
    hr_val = [0] * 2  # [0] HR_Value [1] HR_VALID
    ir_recv = [0] * SPO2_NUM_OF_SAMPLE
    red_recv = [0] * SPO2_NUM_OF_SAMPLE
    for i in range(SPO2_NUM_OF_SAMPLE-1):
        ir_recv[i] = int.from_bytes(payload[i * SPO2_SAMPLE_SIZE:(i * SPO2_SAMPLE_SIZE) + 3], 'big', signed=True)
    for i in range(SPO2_NUM_OF_SAMPLE-1):
        red_recv[i] = int.from_bytes(payload[(i * SPO2_SAMPLE_SIZE) + SPO2_NUM_OF_SAMPLE:
                                             (i * SPO2_SAMPLE_SIZE) + 3 + SPO2_NUM_OF_SAMPLE], 'big', signed=True)

    spo2.maxim_heart_rate_and_oxygen_saturation(ir_recv, red_recv, ir_recv.size(), 0, spo2_val, hr_val)
    if spo2_val[1] == 1:
        print('Spo2 value: ' + spo2_val[0])
    else:
        print('Spo2 value is not valid')
    if hr_val[1] == 1:
        print('HR value: ' + hr_val[0])
    else:
        print('HR value is not valid')


def ParseCo2(payload, size):
    if size < CO2_DATA_SIZE:
        return
    scd_data = [0] * (CO2_DATA_SIZE/2)
    i = 0
    while i < (CO2_DATA_SIZE/2):
        scd_data[i] = int.from_bytes(payload[2 * i:2 * i + 2], 'big', signed=False)
        i += 1
    print('CO2 level: ' + str(scd_data[0]))
    print('Temperature: ' + str(scd_data[1]))
    print('Humidity: ' + str(scd_data[2]))


ser = serial.Serial(SERIAL, timeout=1)
while True:
    try:
        rawHeader = ser.read(9)  # read at least 9 bytes: 5-byte ID and 4-byte payload size
        if len(rawHeader) == 0:
            continue

        header = struct.unpack('<4sxI', rawHeader)
        payload = ser.read(header[1])

        # print('Received packet - type: ' + str(header[0]) + ', size: ' + str(header[1]) + ', payload: ' + str(payload))
        if header[0] == bytearray('EEG '.encode()):
            ParseEeg(payload, header[1])
        if header[0] == bytearray('SPO2 '.encode()):
            ParseSpo2(payload, header[1])
        if header[0] == bytearray('CO2 '.encode()):
            ParseCo2(payload, header[1])
        else:
            print('Unknown packet type: ' + str(header[0]))

    except KeyboardInterrupt:
        sys.exit()