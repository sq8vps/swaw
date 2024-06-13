import struct
import serial
from app.app_gui.sensor_parser.util import *


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


def get_eeg_gain_values():
    return [attr for attr in dir(EegGain) if not attr.startswith("__")]


def get_eeg_mode_values():
    return [attr for attr in dir(EegMode) if not attr.startswith("__")]


def get_eeg_bias_gain_values():
    return [attr for attr in dir(EegBiasGain) if not attr.startswith("__")]


def get_eeg_lead_off_threshold_values():
    return [attr for attr in dir(EegLeadOffThreshold) if not attr.startswith("__")]


def get_eeg_lead_off_current_values():
    return [attr for attr in dir(EegLeadOffCurrent) if not attr.startswith("__")]


def get_eeg_lead_off_frequency_values():
    return [attr for attr in dir(EegLeadOffFrequency) if not attr.startswith("__")]



def get_eeg_gain_value(gain_string):
    # Switch statement to map the gain string to the corresponding value
    if gain_string == 'ADS_GAIN_1':
        return EegGain.ADS_GAIN_1
    elif gain_string == 'ADS_GAIN_2':
        return EegGain.ADS_GAIN_2
    elif gain_string == 'ADS_GAIN_4':
        return EegGain.ADS_GAIN_4
    elif gain_string == 'ADS_GAIN_6':
        return EegGain.ADS_GAIN_6
    elif gain_string == 'ADS_GAIN_8':
        return EegGain.ADS_GAIN_8
    elif gain_string == 'ADS_GAIN_12':
        return EegGain.ADS_GAIN_12
    elif gain_string == 'ADS_GAIN_24':
        return EegGain.ADS_GAIN_24


def get_eeg_mode_value(mode_string):
    # Convert the mode string to uppercase to handle case-insensitivity
    mode_string = mode_string.upper()

    # Switch statement to map the mode string to the corresponding value
    if mode_string == 'ADS_CHANNEL_NORMAL':
        return EegMode.ADS_CHANNEL_NORMAL
    elif mode_string == 'ADS_CHANNEL_SHORTED':
        return EegMode.ADS_CHANNEL_SHORTED
    elif mode_string == 'ADS_CHANNEL_BIAS_MEASUREMENT':
        return EegMode.ADS_CHANNEL_BIAS_MEASUREMENT
    elif mode_string == 'ADS_CHANNEL_SUPPLY_MEASUREMENT':
        return EegMode.ADS_CHANNEL_SUPPLY_MEASUREMENT
    elif mode_string == 'ADS_CHANNEL_TEMPERATURE':
        return EegMode.ADS_CHANNEL_TEMPERATURE
    elif mode_string == 'ADS_CHANNEL_TEST':
        return EegMode.ADS_CHANNEL_TEST
    elif mode_string == 'ADS_CHANNEL_BIAS_POS':
        return EegMode.ADS_CHANNEL_BIAS_POS
    elif mode_string == 'ADS_CHANNEL_BIAS_NEG':
        return EegMode.ADS_CHANNEL_BIAS_NEG

def get_eeg_bias_gain_value(gain_string):
    # Convert the gain string to uppercase to handle case-insensitivity
    gain_string = gain_string.upper()

    # Switch statement to map the gain string to the corresponding value
    if gain_string == 'ADS_BIAS_GAIN_0':
        return EegBiasGain.ADS_BIAS_GAIN_0
    elif gain_string == 'ADS_BIAS_GAIN_5':
        return EegBiasGain.ADS_BIAS_GAIN_5
    elif gain_string == 'ADS_BIAS_GAIN_9':
        return EegBiasGain.ADS_BIAS_GAIN_9
    elif gain_string == 'ADS_BIAS_GAIN_14':
        return EegBiasGain.ADS_BIAS_GAIN_14
    elif gain_string == 'ADS_BIAS_GAIN_18':
        return EegBiasGain.ADS_BIAS_GAIN_18

def get_eeg_lead_off_current_value(current_string):
    # Convert the current string to uppercase to handle case-insensitivity
    current_string = current_string.upper()

    # Switch statement to map the current string to the corresponding value
    if current_string == 'ADS_LO_6NA':
        return EegLeadOffCurrent.ADS_LO_6NA
    elif current_string == 'ADS_LO_24NA':
        return EegLeadOffCurrent.ADS_LO_24NA
    elif current_string == 'ADS_LO_6UA':
        return EegLeadOffCurrent.ADS_LO_6UA
    elif current_string == 'ADS_LO_24UA':
        return EegLeadOffCurrent.ADS_LO_24UA
def get_eeg_lead_off_threshold_value(threshold_string):
    # Convert the threshold string to uppercase to handle case-insensitivity
    threshold_string = threshold_string.upper()

    # Switch statement to map the threshold string to the corresponding value
    if threshold_string == 'ADS_LO_P95_N5':
        return EegLeadOffThreshold.ADS_LO_P95_N5
    elif threshold_string == 'ADS_LO_P92_N7':
        return EegLeadOffThreshold.ADS_LO_P92_N7
    elif threshold_string == 'ADS_LO_P90_N10':
        return EegLeadOffThreshold.ADS_LO_P90_N10
    elif threshold_string == 'ADS_LO_P87_N12':
        return EegLeadOffThreshold.ADS_LO_P87_N12
    elif threshold_string == 'ADS_LO_P85_N15':
        return EegLeadOffThreshold.ADS_LO_P85_N15
    elif threshold_string == 'ADS_LO_P80_N20':
        return EegLeadOffThreshold.ADS_LO_P80_N20
    elif threshold_string == 'ADS_LO_P75_N25':
        return EegLeadOffThreshold.ADS_LO_P75_N25
    elif threshold_string == 'ADS_LO_P70_N30':
        return EegLeadOffThreshold.ADS_LO_P70_N30

def get_eeg_lead_off_frequency_value(frequency_string):
    # Convert the frequency string to uppercase to handle case-insensitivity
    frequency_string = frequency_string.upper()

    # Switch statement to map the frequency string to the corresponding value
    if frequency_string == 'ADS_LO_DC':
        return EegLeadOffFrequency.ADS_LO_DC
    elif frequency_string == 'ADS_LO_7':
        return EegLeadOffFrequency.ADS_LO_7
    elif frequency_string == 'ADS_LO_31':
        return EegLeadOffFrequency.ADS_LO_31
    elif frequency_string == 'ADS_LO_DR':
        return EegLeadOffFrequency.ADS_LO_DR
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


