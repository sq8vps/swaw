import serial
import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import scipy

EEG_CHANNELS = 4
SERIAL = 'COM12'

SAMPLE_COUNT = 100
#y = [[0] * SAMPLE_COUNT] * 4
y1 = [0] * SAMPLE_COUNT
y2 = [0] * SAMPLE_COUNT
y3 = [0] * SAMPLE_COUNT
y4 = [0] * SAMPLE_COUNT
c = 0
fir = scipy.signal.firwin(63, 100, fs=1000)

def ParseEeg(payload, size):
    if size < (3 + 3 * EEG_CHANNELS):
        return
    
    data = struct.unpack('<2Bx' + str(EEG_CHANNELS * 3) + 'B', payload)
    leadState = ~(((data[0] & 0xF) << 4) | (data[1] >> 4)) & 0xF
    samples = [0] * EEG_CHANNELS
    
    i = 0
    while i < EEG_CHANNELS:
        # parse each 3 bytes MSB first as 24-bit signed integers
        samples[i] = float(int.from_bytes(payload[3 * i + 3:3 * i + 6], 'big', signed=True)) / 8388608.0
        i += 1
    
    print('Lead state: ' + str(bin(leadState)))
    print('Samples: ' + str(samples))

    global c
    if c >= SAMPLE_COUNT:
        plt.figure()
        ymin = min(min(y1), min(y2), min(y3), min(y4))
        if ymin > 0:
            ymin = ymin * 0.8
        else:
            ymin = ymin * 1.2
        
        ymax = max(max(y1), max(y2), max(y3), max(y4))
        if ymax > 0:
            ymax = ymax * 1.2
        else:
            ymax = ymax * 0.8

        plt.ylim(ymin, ymax)
        # plt.plot(x, scipy.signal.convolve(np.array(y1), fir)[50:SAMPLE_COUNT-50], label='ch1')
        # plt.plot(x, scipy.signal.convolve(np.array(y2), fir)[50:SAMPLE_COUNT-50], label='ch2')
        # plt.plot(x, scipy.signal.convolve(np.array(y3), fir)[50:SAMPLE_COUNT-50], label='ch3')
        # plt.plot(x, scipy.signal.convolve(np.array(y4), fir)[50:SAMPLE_COUNT-50], label='ch4')
        plt.plot(np.array(y1), label='ch1')
        plt.plot(np.array(y2), label='ch2')
        plt.plot(np.array(y3), label='ch3')
        plt.plot(np.array(y4), label='ch4')
        plt.legend()
        plt.show()
        c = 0
    else:
        y1[c] = samples[0]
        y2[c] = samples[1]
        y3[c] = samples[2]
        y4[c] = samples[3]
        c = c + 1

def ParseCo2(payload, size):
    if size < 6:
        return
    
    data = struct.unpack('<HhH', payload)
    print('CO2 level: ' + str(data[0]) + ' ppm, temperature: ' + str(data[1]) + ' oC, humidity: ' + str(data[2]) + '%')

ser = serial.Serial(SERIAL, timeout=1)
ser.reset_input_buffer()
while True:
    try:
        rawHeader = ser.read(9) #read at least 9 bytes: 5-byte ID and 4-byte payload size
        if len(rawHeader) == 0:
            continue

        header = struct.unpack('<4sxI', rawHeader)
        payload = ser.read(header[1])
        
        #print('Received packet - type: ' + str(header[0]) + ', size: ' + str(header[1]) + ', payload: ' + str(payload))
        if header[0] == bytearray('EEG '.encode()):
            ParseEeg(payload, header[1])
        elif header[0] == bytearray('CO2 '.encode()):
            ParseCo2(payload, header[1])
        else:
            print('Unknown packet type: ' + str(header[0]))
    
    except KeyboardInterrupt:
        sys.exit()
