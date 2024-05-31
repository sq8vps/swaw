from util import *
import struct

SPO2_ID = bytearray('SPO2\0'.encode())

def Spo2Parse(payload, size):
    if size < 11:
        return
    
    data = struct.unpack('<BiBiB', payload)
    if itob(data[0]):
        if itob(data[2]):
            print('SpO2: ' + str(data[1]) + '%')
        if itob(data[4]):
            print('HR: ' + str(data[3]) + ' bpm')
    else:
        print('Finger not present')