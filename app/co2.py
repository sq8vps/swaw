import struct

CO2_ID = bytearray('CO2 \0'.encode())

def Co2Parse(payload, size):
    if size < 6:
        return
    
    data = struct.unpack('<HhH', payload)
    print('CO2 level: ' + str(data[0]) + ' ppm, temperature: ' + str(data[1]) + ' oC, humidity: ' + str(data[2]) + '%')