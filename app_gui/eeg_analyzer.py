
import customtkinter as ctk
import numpy as np

import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


class EEGAnalyzer(ctk.CTkToplevel):
    def __init__(self, master, EEG_BUFFER_SIZE, EEG_SAMPLES_QUEUE,eeg_analyzer_flag):
        super().__init__(master)
        self.time = np.linspace(0, EEG_BUFFER_SIZE, EEG_BUFFER_SIZE)
        self.amplitude = np.linspace(-0.2,    0.2, EEG_BUFFER_SIZE)

        # title
        self.title("EEG ANALYZER")
        # Geometry
        self.geometry(f"{1600}x{800}")

        # Grid Layout
        self.grid_columnconfigure(0, weight=1)
        self.grid_rowconfigure(0, weight=1)

        # Frame For Plot's
        self.eeg_frame = ctk.CTkFrame(self)
        self.eeg_frame.grid(row=0, column=0, padx=5, pady=5, sticky="nsew")


        self.eeg_status_frame = ctk.CTkFrame(master=self.eeg_frame)
        self.eeg_status_frame.pack(pady=1, fill="x")

        # Utworzenie czterech etykiet w jednym wierszu
        self.eeg_status_label1 = ctk.CTkLabel(master=self.eeg_status_frame, text="Electrode 1", compound="left",
                                              font=ctk.CTkFont(size=16, weight='bold'))
        self.eeg_status_label1.grid(row=0, column=0, padx=5, pady=10)

        self.eeg_status_label2 = ctk.CTkLabel(master=self.eeg_status_frame, text="Electrode 2", compound="left",
                                              font=ctk.CTkFont(size=16, weight='bold'))
        self.eeg_status_label2.grid(row=0, column=1, padx=5, pady=10)

        self.eeg_status_label3 = ctk.CTkLabel(master=self.eeg_status_frame, text="Electrode 3", compound="left",
                                              font=ctk.CTkFont(size=16, weight='bold'))
        self.eeg_status_label3.grid(row=0, column=2, padx=5, pady=10)

        self.eeg_status_label4 = ctk.CTkLabel(master=self.eeg_status_frame, text="Electrode 4", compound="left",
                                              font=ctk.CTkFont(size=16, weight='bold'))
        self.eeg_status_label4.grid(row=0, column=3, padx=5, pady=10)

        # Wycentrowanie etykiet w ramce
        for i in range(4):
            self.eeg_status_frame.grid_columnconfigure(i, weight=1)

        # Create plot
        self.fig, self.ax = plt.subplots(nrows=4, ncols=1, figsize=(4, 2),facecolor='#2c2c2c')
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.eeg_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

        # Plot Setting's
        self.line_CH1, = self.ax[0].plot(self.time, self.amplitude, 'green', linewidth=2)
        self.ax[0].set_title("Channel 1 ",color='White')
        self.ax[0].tick_params(axis='x', colors='white')
        self.ax[0].tick_params(axis='y', colors='white')
        self.ax[0].spines['bottom'].set_color('#2c2c2c')  # Set the color of the bottom spine
        self.ax[0].spines['bottom'].set_linestyle('--')
        self.ax[0].spines['bottom'].set_linewidth(0.5)
        self.ax[0].spines['top'].set_color('#2c2c2c')  # Set the color of the top spine
        self.ax[0].spines['top'].set_linestyle('--')
        self.ax[0].spines['top'].set_linewidth(0.5)
        self.ax[0].spines['right'].set_color('#2c2c2c')  # Set the color of the right spine
        self.ax[0].spines['right'].set_linestyle('--')
        self.ax[0].spines['right'].set_linewidth(0.5)
        self.ax[0].spines['left'].set_color('#2c2c2c')  # Set the color of the left spine
        self.ax[0].spines['left'].set_linestyle('--')
        self.ax[0].spines['left'].set_linewidth(0.5)
        self.ax[0].grid(color='#2c2c2c', linestyle='--', linewidth=0.5)

        self.line_CH2, = self.ax[1].plot(self.time, self.amplitude, 'green', linewidth=2)
        self.ax[1].set_title("Channel 2 ", color='White')
        self.ax[1].tick_params(axis='x', colors='white')
        self.ax[1].tick_params(axis='y', colors='white')
        self.ax[1].spines['bottom'].set_color('#2c2c2c')  # Set the color of the bottom spine
        self.ax[1].spines['bottom'].set_linestyle('--')
        self.ax[1].spines['bottom'].set_linewidth(0.5)
        self.ax[1].spines['top'].set_color('#2c2c2c')  # Set the color of the top spine
        self.ax[1].spines['top'].set_linestyle('--')
        self.ax[1].spines['top'].set_linewidth(0.5)
        self.ax[1].spines['right'].set_color('#2c2c2c')  # Set the color of the right spine
        self.ax[1].spines['right'].set_linestyle('--')
        self.ax[1].spines['right'].set_linewidth(0.5)
        self.ax[1].spines['left'].set_color('#2c2c2c')  # Set the color of the left spine
        self.ax[1].spines['left'].set_linestyle('--')
        self.ax[1].spines['left'].set_linewidth(0.5)
        self.ax[1].grid(color='#2c2c2c', linestyle='--', linewidth=0.5)

        self.line_CH3, = self.ax[2].plot(self.time, self.amplitude, 'green', linewidth=2)
        self.ax[2].set_title("Channel 3 ", color='White')
        self.ax[2].tick_params(axis='x', colors='white')
        self.ax[2].tick_params(axis='y', colors='white')
        self.ax[2].spines['bottom'].set_color('#2c2c2c')  # Set the color of the bottom spine
        self.ax[2].spines['bottom'].set_linestyle('--')
        self.ax[2].spines['bottom'].set_linewidth(0.5)
        self.ax[2].spines['top'].set_color('#2c2c2c')  # Set the color of the top spine
        self.ax[2].spines['top'].set_linestyle('--')
        self.ax[2].spines['top'].set_linewidth(0.5)
        self.ax[2].spines['right'].set_color('#2c2c2c')  # Set the color of the right spine
        self.ax[2].spines['right'].set_linestyle('--')
        self.ax[2].spines['right'].set_linewidth(0.5)
        self.ax[2].spines['left'].set_color('#2c2c2c')  # Set the color of the left spine
        self.ax[2].spines['left'].set_linestyle('--')
        self.ax[2].spines['left'].set_linewidth(0.5)
        self.ax[2].grid(color='#2c2c2c', linestyle='--', linewidth=0.5)

        self.line_CH4, = self.ax[3].plot(self.time, self.amplitude, 'green', linewidth=2)
        self.ax[3].set_title("Channel 3 ", color='White')
        self.ax[3].tick_params(axis='x', colors='white')
        self.ax[3].tick_params(axis='y', colors='white')
        self.ax[3].spines['bottom'].set_color('#2c2c2c')  # Set the color of the bottom spine
        self.ax[3].spines['bottom'].set_linestyle('--')
        self.ax[3].spines['bottom'].set_linewidth(0.5)
        self.ax[3].spines['top'].set_color('#2c2c2c')  # Set the color of the top spine
        self.ax[3].spines['top'].set_linestyle('--')
        self.ax[3].spines['top'].set_linewidth(0.5)
        self.ax[3].spines['right'].set_color('#2c2c2c')  # Set the color of the right spine
        self.ax[3].spines['right'].set_linestyle('--')
        self.ax[3].spines['right'].set_linewidth(0.5)
        self.ax[3].spines['left'].set_color('#2c2c2c')  # Set the color of the left spine
        self.ax[3].spines['left'].set_linestyle('--')
        self.ax[3].spines['left'].set_linewidth(0.5)
        self.ax[3].grid(color='#2c2c2c', linestyle='--', linewidth=0.5)


        self.EEG_SAMPLES_QUEUE = EEG_SAMPLES_QUEUE
        self.EEG_BUFFER = np.zeros((4, EEG_BUFFER_SIZE,))
        self.buffer_index = 0


        self.animation_run = True



        self.bind("<Destroy>", self.on_window_close)
        self.protocol("WM_DELETE_WINDOW", self.on_window_close)


    def init_plot(self):

        self.line_CH1.set_data(self.time, self.amplitude)
        self.line_CH2.set_data(self.time, self.amplitude)
        self.line_CH3.set_data(self.time, self.amplitude)
        self.line_CH4.set_data(self.time, self.amplitude)

        return self.line_CH1, self.line_CH2, self.line_CH3, self.line_CH4,

    def update_plot(self, frame):
        if self.animation_run:
            self.EEG_BUFFER, leadstate = self.EEG_SAMPLES_QUEUE.get()

            self.update_status(leadstate)

            self.line_CH1.set_ydata(self.EEG_BUFFER[0])
            self.line_CH2.set_ydata(self.EEG_BUFFER[1])
            self.line_CH3.set_ydata(self.EEG_BUFFER[2])
            self.line_CH4.set_ydata(self.EEG_BUFFER[3])

            return self.line_CH1, self.line_CH2, self.line_CH3, self.line_CH4,

    def update_status(self, leadstate):
        labels = [self.eeg_status_label1, self.eeg_status_label2, self.eeg_status_label3, self.eeg_status_label4]

        if (leadstate & 1 ):
            labels[0].configure(text="Channel 1 Connected")
            labels[0].configure(bg_color="green")
        else:
            labels[0].configure(text="Channel 1 Not Connected")
            labels[0].configure(bg_color="red")

        if (leadstate & 2):
            labels[1].configure(text="Channel 2 Connected")
            labels[1].configure(bg_color="green")
        else:
            labels[1].configure(text="Channel 2 Not Connected")
            labels[1].configure(bg_color="red")

        if (leadstate & 4):
            labels[2].configure(text="Channel 3 Connected")
            labels[2].configure(bg_color="green")
        else:
            labels[2].configure(text="Channel 3 Not Connected")
            labels[2].configure(bg_color="red")


        if (leadstate & 8):
            labels[3].configure(text="Channel 4 Connected")
            labels[3].configure(bg_color="green")
        else:
            labels[3].configure(text="Channel 4 Not Connected")
            labels[3].configure(bg_color="red")


    def on_window_close(self, event=None):
        print("EEGAnalyzer window is closed")
        self.destroy()  # This ensures the window is properly destroyed









