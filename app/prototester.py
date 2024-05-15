import serial
import struct
import sys

ser = serial.Serial("COM10", timeout=1)
while True:
    try:
        rawHeader = ser.read(9) #read at least 9 bytes: 5-byte ID and 4-byte payload size
        if len(rawHeader) == 0:
            continue

        header = struct.unpack('<4sxI', rawHeader)
        payload = ser.read(header[1])
        
        print('Received packet - type: ' + str(header[0]) + ', size: ' + str(header[1]) + ', payload: ' + str(payload))
    except KeyboardInterrupt:
        sys.exit()
