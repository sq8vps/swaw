import serial
import struct
import sys
from eeg import *
from co2 import *

SERIAL = 'COM12'

ser = serial.Serial(SERIAL, timeout=1)
ser.reset_input_buffer()
EegSetExampleConfig(ser)
while True:
    try:
        
        rawHeader = ser.read(9) #read at least 9 bytes: 5-byte ID and 4-byte payload size
        if len(rawHeader) == 0:
            continue

        header = struct.unpack('<5sI', rawHeader)
        payload = ser.read(header[1])
        
        #print('Received packet - type: ' + str(header[0]) + ', size: ' + str(header[1]) + ', payload: ' + str(payload))
        if header[0] == EEG_ID:
            EegParse(payload, header[1])
        elif header[0] == CO2_ID:
            Co2Parse(payload, header[1])
        else:
           print('Unknown packet type: ' + str(header[0]))
    
    except KeyboardInterrupt:
        sys.exit()
