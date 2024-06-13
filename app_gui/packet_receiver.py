
import sys
from app.app_gui.sensor_parser.eeg import *
import threading
import numpy as np



class PacketReceiver(threading.Thread):
    def __init__(self,EEG_SAMPLES_QUEUE, CO2_SAMPLES_QUEUE, SPO2_SAMPLES_QUEUE,EEG_BUFFER_SIZE,ser):
        super(PacketReceiver, self).__init__()

        self.ser = ser

        self.EEG_ID = bytearray('EEG \0'.encode())
        self.CO2_ID = bytearray('CO2 \0'.encode())
        self.SPO2_ID = bytearray('SPO2\0'.encode())

        self.EEG_SAMPLES_QUEUE = EEG_SAMPLES_QUEUE      # Queue to store separate channel values
        self.CO2_SAMPLES_QUEUE = CO2_SAMPLES_QUEUE      # Queue to store separate channel values
        self.SPO2_SAMPLES_QUEUE = SPO2_SAMPLES_QUEUE    # Queue to store separate channel values

        self.EEG_BUFFER_SIZE = EEG_BUFFER_SIZE
        self.EEG_BUFFER = np.zeros((EEG_CHANNELS, EEG_BUFFER_SIZE))
        self.buffer_index = 0
        self.running = True

    def run(self):

        # ser = serial.Serial(self.SERIA, timeout=1)
        self.ser.reset_input_buffer()

        while self.running:
            try:
                rawHeader = self.ser.read(9)  # read at least 9 bytes: 5-byte ID and 4-byte payload size
                if len(rawHeader) == 0:
                    continue

                header = struct.unpack('<5sI', rawHeader)
                payload = self.ser.read(header[1])

                if header[0] == self.EEG_ID:
                   eeg_samples, leadstate = self.EegParse(payload, header[1])

                   self.EEG_BUFFER[:, self.buffer_index] = eeg_samples
                   self.buffer_index += 1

                   if self.buffer_index == self.EEG_BUFFER_SIZE:

                        self.EEG_SAMPLES_QUEUE.put((self.EEG_BUFFER,leadstate))
                        self.EEG_BUFFER = np.zeros((EEG_CHANNELS, self.EEG_BUFFER_SIZE))
                        self.buffer_index = 0

                elif header[0] == self.CO2_ID:
                    co2_data = self.Co2Parse(payload, header[1])
                    self.CO2_SAMPLES_QUEUE.put(co2_data)


                elif header[0] == self.SPO2_ID:
                    hr_spo2_data = self.Spo2Parse(payload, header[1])
                    self.SPO2_SAMPLES_QUEUE.put(hr_spo2_data)

                else:
                    print('Unknown packet type: ' + str(header[0]))

            except KeyboardInterrupt:
                sys.exit()

    def EegParse(self,payload, size):
        if size < (3 + 3 * EEG_CHANNELS):
            return

        # unpack structure
        data = struct.unpack('<2Bx' + str(EEG_CHANNELS * 3) + 'B', payload)
        # extract lead status bitmap: 0 - not connected, 1 - connected
        leadState = ~(((data[0] & 0xF) << 4) | (data[1] >> 4)) & 0xF

        samples = [0] * EEG_CHANNELS  # prepare sample buffer

        i = 0
        while i < EEG_CHANNELS:
            # parse each 3 bytes MSB first as 24-bit signed integers
            samples[i] = float(int.from_bytes(payload[3 * i + 3:3 * i + 6], 'big', signed=True)) / 8388608.0
            i += 1

        return samples, leadState

    def Co2Parse(self, payload, size):
        if size < 6:
            return
        data = struct.unpack('<HhH', payload)
        return np.array(data)

    def Spo2Parse(self,payload, size):
        if size < 11:
            return
        data = struct.unpack('<BiBiB', payload)
        hr_spo2 = np.array(data)
        return hr_spo2


    def stop(self):
        self.running = False
        self.join()  # Wait for the thread to finish


