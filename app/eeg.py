import struct
import serial
from util import *
import matplotlib.pyplot as plt
import numpy as np

EEG_CHANNELS = 4

EEG_ID = bytearray('EEG \0'.encode())

class EegCommand:
    ADS_CONFIG_REQUEST_CHANNEL = 0x01
    ADS_CONFIG_REQUEST_BIAS_GAIN = 0x02
    ADS_CONFIG_REQUEST_LEAD_OFF = 0x03

class EegGain:
	ADS_GAIN_1 = 0x0
	ADS_GAIN_2 = 0x1
	ADS_GAIN_4 = 0x2
	ADS_GAIN_6 = 0x3
	ADS_GAIN_8 = 0x4
	ADS_GAIN_12 = 0x5
	ADS_GAIN_24 = 0x6

class EegMode:
    ADS_CHANNEL_NORMAL = 0x0 #connect to actual inputs
    ADS_CHANNEL_SHORTED = 0x1 #in+ and in- shorted to each other and to vref
    ADS_CHANNEL_BIAS_MEASUREMENT = 0x2 #connect to bias
    ADS_CHANNEL_SUPPLY_MEASUREMENT = 0x3 #connect to the supply
    ADS_CHANNEL_TEMPERATURE = 0x4 #connect to temperature measurement diodes
    ADS_CHANNEL_TEST = 0x5 #connect to test signal
    ADS_CHANNEL_BIAS_POS = 0x6 #positive electrode as bias output
    ADS_CHANNEL_BIAS_NEG = 0x7 #negative elecrode as bias output - do not use

class EegBiasGain:
    ADS_BIAS_GAIN_0 = 0x0
    ADS_BIAS_GAIN_5 = 0x1
    ADS_BIAS_GAIN_9 = 0x2
    ADS_BIAS_GAIN_14 = 0x3
    ADS_BIAS_GAIN_18 = 0x4

class EegLeadOffThreshold:
	ADS_LO_P95_N5 = 0x0
	ADS_LO_P92_N7 = 0x1
	ADS_LO_P90_N10 = 0x2
	ADS_LO_P87_N12 = 0x03
	ADS_LO_P85_N15 = 0x04
	ADS_LO_P80_N20 = 0x05
	ADS_LO_P75_N25 = 0x06
	ADS_LO_P70_N30 = 0x07

class EegLeadOffCurrent:
    ADS_LO_6NA = 0x0
    ADS_LO_24NA = 0x1
    ADS_LO_6UA = 0x2
    ADS_LO_24UA = 0x3

class EegLeadOffFrequency:
	ADS_LO_DC = 0x0
	ADS_LO_7 = 0x1
	ADS_LO_31 = 0x2
	ADS_LO_DR = 0x3

def EegSetChannel(ser: serial.Serial, number: int, enable: bool, mode: EegMode, gain: EegGain, biasDerivation: bool, leadOffDetection: bool):
    ser.write(struct.pack('<5sI7B', EEG_ID, 7, EegCommand.ADS_CONFIG_REQUEST_CHANNEL, number, btoi(enable), mode, gain, btoi(biasDerivation), btoi(leadOffDetection)))

def EegSetBiasGain(ser: serial.Serial, gain: EegBiasGain):
    ser.write(struct.pack('<5sI2B', EEG_ID, 2, EegCommand.ADS_CONFIG_REQUEST_BIAS_GAIN, gain))

def EegSetLeadOffConfig(ser: serial.Serial, threshold: EegLeadOffThreshold, current: EegLeadOffCurrent, freq: EegLeadOffFrequency):
    ser.write(struct.pack('<5sI4B', EEG_ID, 4, EegCommand.ADS_CONFIG_REQUEST_LEAD_OFF, threshold, current, freq))

def EegSetExampleConfig(ser):
    #set channel 1 to enabled, shorted inputs mode, 4x gain, bias derivation disabled, lead off detection enabled
    EegSetChannel(ser, 1, True, EegMode.ADS_CHANNEL_SHORTED, EegGain.ADS_GAIN_4, False, True)
    #set bias gain to 5x
    EegSetBiasGain(ser, EegBiasGain.ADS_BIAS_GAIN_5)
    #set lead-off detection threshold to 90%/10%, source current to 24 nA, DC
    EegSetLeadOffConfig(ser, EegLeadOffThreshold.ADS_LO_P90_N10, EegLeadOffCurrent.ADS_LO_24NA, EegLeadOffFrequency.ADS_LO_DC)

SAMPLE_COUNT = 100
y1 = [0] * SAMPLE_COUNT
y2 = [0] * SAMPLE_COUNT
y3 = [0] * SAMPLE_COUNT
y4 = [0] * SAMPLE_COUNT
c = 0

def EegParse(payload, size):
    if size < (3 + 3 * EEG_CHANNELS):
        return
    
    #unpack structure
    data = struct.unpack('<2Bx' + str(EEG_CHANNELS * 3) + 'B', payload)
    #extract lead status bitmap: 0 - not connected, 1 - connected
    leadState = ~(((data[0] & 0xF) << 4) | (data[1] >> 4)) & 0xF
    #prepare sample buffer
    samples = [0] * EEG_CHANNELS
    
    i = 0
    while i < EEG_CHANNELS:
        # parse each 3 bytes MSB first as 24-bit signed integers
        samples[i] = float(int.from_bytes(payload[3 * i + 3:3 * i + 6], 'big', signed=True)) / 8388608.0
        i += 1
    
    print('Lead state: ' + str(bin(leadState)))
    print('Samples: ' + str(samples))

    #just some debug plotting below
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