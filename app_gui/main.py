
from sensor_parser.eeg import *
import customtkinter as ctk
import time

from packet_receiver import PacketReceiver
import serial
from eeg_analyzer import EEGAnalyzer
from hr_spo2_analyzer import HR_SPO2_Analyzer
from co2_analyzer import  CO2_Analyzer

import serial.tools.list_ports
from queue import Queue
from matplotlib.animation import FuncAnimation
import threading
ctk.set_default_color_theme("green")


class SleepMonitorApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        # App window Init
        self.title("SLEEP MONITOR")
        self.geometry(f"{1250}x{700}")

        # ---------------------------------------------------------------------Layout Configuration---------------------------------------------------------------------
        self.grid_columnconfigure(0, weight=0)
        self.grid_columnconfigure(1, weight=1)
        self.grid_columnconfigure(2, weight=1)
        self.grid_columnconfigure(3, weight=1)
        self.grid_columnconfigure(4, weight=1)
        self.grid_columnconfigure(5, weight=1)
        self.grid_rowconfigure(0, weight=1)

        # Menu Widget Frame Label's / Button's
        self.sidebar_frame = ctk.CTkFrame(self, width=140)
        self.sidebar_frame.grid(row=0, column=0, rowspan=7, padx=5, pady=5, sticky="nsew")
        self.sidebar_frame.grid_rowconfigure(4, weight=0)

        # Menu label
        self.menu_label = ctk.CTkLabel(self.sidebar_frame, text="Menu", font=ctk.CTkFont(size=20, weight="bold"))
        self.menu_label.grid(row=0, column=0, padx=20, pady=10)

        # COM label
        self.COM_label = ctk.CTkLabel(self.sidebar_frame, text="Select COM Device:")
        self.COM_label.grid(row=1, column=0, padx=20, pady=10)

        # COM list
        self.COM_combo_box = ctk.CTkComboBox(self.sidebar_frame, values=self.com_devices())
        self.COM_combo_box.grid(row=2, column=0, padx=20, pady=10)

        # Read Data button
        self.button_receive_packets = ctk.CTkButton(self.sidebar_frame, text="Start Receive Data",
                                                    font=ctk.CTkFont(size=14, weight='bold'),
                                                    command=self.start_receive_packets)
        self.button_receive_packets.grid(row=3, column=0, padx=15, pady=10)

        # Panel label
        self.panel_label = ctk.CTkLabel(self.sidebar_frame, text="Panel", font=ctk.CTkFont(size=20, weight="bold"))
        self.panel_label.grid(row=4, column=0, padx=20, pady=10)

        # EEG panel button
        self.button_eeg_analyzer = ctk.CTkButton(self.sidebar_frame, text="EEG Analyzer",
                                                 font=ctk.CTkFont(size=14, weight='bold'),
                                                 command=self.start_eeg_analyzer)
        self.button_eeg_analyzer.grid(row=5, column=0, padx=20, pady=10)

        # HR/SPO2 panel button
        self.button_max_analyzer = ctk.CTkButton(self.sidebar_frame, text="HR/SPO2 Analyzer",
                                                 font=ctk.CTkFont(size=14, weight='bold'),
                                                 command=self.start_hr_spo2_analyzer)
        self.button_max_analyzer.grid(row=6, column=0, padx=20, pady=10)

        # CO2 pannel button
        self.button_co2_analyzer = ctk.CTkButton(self.sidebar_frame, text="CO2/Temp/Humm Analyzer",
                                                 font=ctk.CTkFont(size=14, weight='bold'),
                                                 command= self.start_co2_analyzer)
        self.button_co2_analyzer.grid(row=7, column=0, padx=20, pady=10)



        # Calibration frame
        self.calibration_frame = ctk.CTkFrame(self)
        self.calibration_frame.grid(row=0, column=1, columnspan=5, padx=5, pady=5, sticky="nsew")

        # Configure grid in calibration frame
        self.calibration_frame.grid_columnconfigure(0, weight=1)
        self.calibration_frame.grid_columnconfigure(1, weight=1)
        self.calibration_frame.grid_columnconfigure(2, weight=1)
        self.calibration_frame.grid_columnconfigure(3, weight=1)
        self.calibration_frame.grid_rowconfigure(0, weight=0)
        self.calibration_frame.grid_rowconfigure(1, weight=0)
        self.calibration_frame.grid_rowconfigure(2, weight=0)
        self.calibration_frame.grid_rowconfigure(3, weight=0)
        self.calibration_frame.grid_rowconfigure(4, weight=0)
        self.calibration_frame.grid_rowconfigure(5, weight=0)
        self.calibration_frame.grid_rowconfigure(8, weight=0)
        self.calibration_frame.grid_rowconfigure(9, weight=0)
        self.calibration_frame.grid_rowconfigure(10, weight=0)
        self.calibration_frame.grid_rowconfigure(11, weight=1)

        # Calibration label
        self.calibration_label = ctk.CTkLabel(self.calibration_frame, text="Calibration Panel", font=ctk.CTkFont(size=20, weight="bold"))
        self.calibration_label.grid(row=0, column=0, columnspan=4, padx=20, pady=20, sticky="ew")

        # EEG Channel 1 Gain Label
        self.eeg_gain_label1 = ctk.CTkLabel(self.calibration_frame, text="EEG Channel 1 Gain",font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_gain_label1.grid(row=1, column=1, padx=5, pady=5,sticky='w')
        self.eeg_gain_box1 = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_gain_values())
        self.eeg_gain_box1.grid(row=2, column=1, padx=5, pady=5, sticky="w")

        # EEG Channel 2 Gain Label
        self.eeg_gain_label2 = ctk.CTkLabel(self.calibration_frame, text="EEG Channel 2 Gain", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_gain_label2.grid(row=3, column=1, padx=5, pady=5, sticky='w')
        self.eeg_gain_box2 = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_gain_values())
        self.eeg_gain_box2.grid(row=4, column=1, padx=5, pady=5, sticky="w")

        # EEG Channel 3 Gain Label
        self.eeg_gain_label3 = ctk.CTkLabel(self.calibration_frame, text="EEG Channel 3 Gain",font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_gain_label3.grid(row=5, column=1, padx=5, pady=5, sticky='w')
        self.eeg_gain_box3 = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_gain_values())
        self.eeg_gain_box3.grid(row=6, column=1, padx=5, pady=5, sticky="w")

        # EEG Channel 4 Gain Label
        self.eeg_gain_label4 = ctk.CTkLabel(self.calibration_frame, text="EEG Channel 4 Gain", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_gain_label4.grid(row=7, column=1, padx=5, pady=5, sticky='w')
        self.eeg_gain_box4 = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_gain_values())
        self.eeg_gain_box4.grid(row=8, column=1, padx=5, pady=5, sticky="w")


        # EEG Channel 1 mode Label
        self.eeg_mode_label1 = ctk.CTkLabel(self.calibration_frame, text="EEG Channel 1 Mode", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_mode_label1.grid(row=1, column=2, padx=5, pady=5, sticky='w')
        self.eeg_mode_box1 = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_mode_values())
        self.eeg_mode_box1.grid(row=2, column=2, padx=5, pady=5, sticky="w")

        # EEG Channel 2 mode Label
        self.eeg_mode_label2 = ctk.CTkLabel(self.calibration_frame, text="EEG Channel 2 Mode", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_mode_label2.grid(row=3, column=2, padx=5, pady=5, sticky='w')
        self.eeg_mode_box2 = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_mode_values())
        self.eeg_mode_box2.grid(row=4, column=2, padx=5, pady=5, sticky="w")

        # EEG Channel 3 mode Label
        self.eeg_mode_label3 = ctk.CTkLabel(self.calibration_frame, text="EEG Channel 3 Mode", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_mode_label3.grid(row=5, column=2, padx=5, pady=5, sticky='w')
        self.eeg_mode_box3 = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_mode_values())
        self.eeg_mode_box3.grid(row=6, column=2, padx=5, pady=5, sticky="w")


        # EEG Channel 4 mode Label
        self.eeg_mode_label4 = ctk.CTkLabel(self.calibration_frame, text="EEG Channel 4 Mode", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_mode_label4.grid(row=7, column=2, padx=5, pady=5, sticky='w')
        self.eeg_mode_box4 = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_mode_values())
        self.eeg_mode_box4.grid(row=8, column=2, padx=5, pady=5, sticky="w")


        # EEG Bias Gain Label
        self.eeg_mode_label = ctk.CTkLabel(self.calibration_frame, text="EEG Bias Gain", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_mode_label.grid(row=1, column=3, padx=5, pady=5, sticky='w')
        self.eeg_bias_box = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_bias_gain_values())
        self.eeg_bias_box.grid(row=2, column=3, padx=5, pady=5, sticky="w")

        # EEG LeadOffThreshold
        self.eeg_lead_label = ctk.CTkLabel(self.calibration_frame, text="EEG Lead off Threshold", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_lead_label.grid(row=3, column=3, padx=5, pady=5, sticky='w')
        self.eeg_lead_box = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_lead_off_threshold_values())
        self.eeg_lead_box.grid(row=4, column=3, padx=5, pady=5, sticky="w")

        # EEG LeadOffCurrent
        self.eeg_current_label = ctk.CTkLabel(self.calibration_frame, text="EEG Lead off Current", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_current_label.grid(row=5, column=3, padx=5, pady=5, sticky='w')
        self.eeg_current_box = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_lead_off_current_values())
        self.eeg_current_box.grid(row=6, column=3, padx=5, pady=5, sticky="w")

        # EEG LeadOffFrequency
        self.eeg_current_label = ctk.CTkLabel(self.calibration_frame, text="EEG Lead off Frequency", font=ctk.CTkFont(size=14, weight="bold"))
        self.eeg_current_label.grid(row=7, column=3, padx=5, pady=5, sticky='w')
        self.eeg_current_box = ctk.CTkComboBox(self.calibration_frame, values=get_eeg_lead_off_frequency_values())
        self.eeg_current_box.grid(row=8, column=3, padx=5, pady=5, sticky="w")

        # SendConfig
        self.button_send = ctk.CTkButton(self.calibration_frame, text="Send Configuration",  font=ctk.CTkFont(size=14, weight='bold'),command=self.send_config)
        self.button_send.grid(row=9, column=2, padx=5, pady=20,sticky='w')

        # Console label
        self.console_label = ctk.CTkLabel(self.calibration_frame, text="Console Output", font=ctk.CTkFont(size=20, weight="bold"))
        self.console_label.grid(row=10, column=0, columnspan=4, padx=20, pady=20, sticky="ew")

        # Console Output
        self.textbox = ctk.CTkTextbox(self.calibration_frame, width=1000, height=100)
        self.textbox.grid(row=11, column=0, columnspan=4, padx=20, pady=5, sticky="nsew")
        self.textbox.configure(state="disabled")
        # ---------------------------------------------------------------------Layout Configuration---------------------------------------------------------------------

        # Class Object's Init
        self.packet_receiver = None
        self.eeg_analyzer = None
        self.hr_spo2_analyzer = None
        self.co2_analyzer = None

        # Buffer Size for EEG plot
        self.EEG_BUFFER_SIZE = 500  # Default Value

        #
        self.EEG_SAMPLES_QUEUE = Queue()
        self.CO2_SAMPLES_QUEUE = Queue()
        self.SPO2_SAMPLES_QUEUE = Queue()

        #
        self.packet_receiver_flag = False
        self.eeg_analyzer_flag = False
        self.ser = None

    def start_receive_packets(self):


        global ser
        if self.packet_receiver_flag is False:
            self.SERIAL = self.COM_combo_box.get()
            self.ser = serial.Serial(self.SERIAL, timeout=1)
            self.insert_text_txtbox("Selected COM is " + self.SERIAL)
            self.packet_receiver = PacketReceiver(self.EEG_SAMPLES_QUEUE, self.CO2_SAMPLES_QUEUE, self.SPO2_SAMPLES_QUEUE,self.EEG_BUFFER_SIZE,self.ser)
            self.packet_receiver.start()
            self.insert_text_txtbox("Load Default Configuration")
            self.packet_receiver_flag = True
            self.insert_text_txtbox("Start Reading Data")

        else:
           self.insert_text_txtbox("DATA is already readed")


    def start_eeg_analyzer(self):
        if self.eeg_analyzer_flag is False:
            self.eeg_analyzer = EEGAnalyzer(self, self.EEG_BUFFER_SIZE, self.EEG_SAMPLES_QUEUE,self.eeg_analyzer_flag)
            self.ani = FuncAnimation(self.eeg_analyzer.fig, self.eeg_analyzer.update_plot, init_func=self.eeg_analyzer.init_plot, frames=20, interval=100, blit=True)
            self.insert_text_txtbox("Launching EEG Analyzer")
            self.eeg_analyzer_flag = True
        else:
            self.insert_text_txtbox("EEG ANALYZER IS WORKING")


    def start_hr_spo2_analyzer(self):
        self.hr_spo2_analyzer = HR_SPO2_Analyzer(self, self.SPO2_SAMPLES_QUEUE)


    def start_co2_analyzer(self):
        self.co2_analyzer = CO2_Analyzer(self, self.CO2_SAMPLES_QUEUE)

    def com_devices(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        return ports

    def insert_text_txtbox(self,text):
        self.textbox.configure(state="normal")
        self.textbox.insert("1.0", text +"\n")
        self.textbox.configure(state="disabled")

    def send_config(self):
        eeg_gain1 = self.eeg_gain_box1.get()
        eeg_gain1 = get_eeg_gain_value(eeg_gain1)
        eeg_gain2 = self.eeg_gain_box2.get()
        eeg_gain2 = get_eeg_gain_value(eeg_gain2)
        eeg_gain3 = self.eeg_gain_box3.get()
        eeg_gain3 = get_eeg_gain_value(eeg_gain3)
        eeg_gain4 = self.eeg_gain_box4.get()
        eeg_gain4 = get_eeg_gain_value(eeg_gain4)

        eeg_mode1 = self.eeg_mode_box1.get()
        eeg_mode1 = get_eeg_mode_value(eeg_mode1)
        eeg_mode2 = self.eeg_mode_box2.get()
        eeg_mode2 = get_eeg_mode_value(eeg_mode2)
        eeg_mode3 = self.eeg_mode_box3.get()
        eeg_mode3 = get_eeg_mode_value(eeg_mode3)
        eeg_mode4 = self.eeg_mode_box4.get()
        eeg_mode4 = get_eeg_mode_value(eeg_mode4)

        eeg_bias_gain = self.eeg_bias_box.get()
        eeg_bias_gain = get_eeg_bias_gain_value(eeg_bias_gain)

        eeg_lead_off_threshold = self.eeg_lead_box.get()
        eeg_lead_off_threshold = get_eeg_lead_off_threshold_value(eeg_lead_off_threshold)

        eeg_lead_off_current = self.eeg_current_box.get()
        eeg_lead_off_current = get_eeg_lead_off_current_value(eeg_lead_off_current)

        eeg_lead_off_frequency = self.eeg_current_box.get()
        eeg_lead_off_frequency = get_eeg_lead_off_frequency_value(eeg_lead_off_frequency)

        # To do add sending



        # global ser
        EegSetChannel(self.ser, 0, True, eeg_mode1, eeg_gain1, False, True)
        time.sleep(0.1)
        EegSetChannel(self.ser, 1, True, eeg_mode2, eeg_gain2, False, True)
        time.sleep(0.1)
        EegSetChannel(self.ser, 2, True, eeg_mode3, eeg_gain3, False, True)
        time.sleep(0.1)
        EegSetChannel(self.ser, 3, True, eeg_mode4, eeg_gain4, False, True)
        time.sleep(0.1)
        EegSetBiasGain(self.ser, eeg_bias_gain)
        time.sleep(0.1)
        #EegSetLeadOffConfig(self.ser, eeg_lead_off_threshold, eeg_lead_off_current, eeg_lead_off_frequency)

    def on_close(self):
        self.destroy()


if __name__ == "__main__":
    app = SleepMonitorApp()
    app.protocol("WM_DELETE_WINDOW", app.on_close)
    app.mainloop()

