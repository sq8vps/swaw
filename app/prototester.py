import serial
import struct
import sys

EEG_CHANNELS = 4
SERIAL = 'COM10'

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


ser = serial.Serial(SERIAL, timeout=1)
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
        else:
            print('Unknown packet type: ' + str(header[0]))
    
    except KeyboardInterrupt:
        sys.exit()
