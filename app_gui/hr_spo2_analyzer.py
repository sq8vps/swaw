import customtkinter as ctk
import threading
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

def btoi(b: bool) -> int:
    return 1 if b == True else 0

def itob(i: int) -> bool:
    return True if i > 0 else False
class HR_SPO2_Analyzer(ctk.CTkToplevel):
    def __init__(self, master, SPO2_SAMPLES_QUEUE):
        super().__init__(master)
        self.geometry(f"{800}x{800}")
        self.title("MAX30102 Analyzer")
        # Top-level frame
        # Top-level frame
        self.frame = ctk.CTkFrame(master=self, width=400, height=400)
        self.frame.pack(pady=20, padx=20, fill="both", expand=True)

        # Saturation Label
        self.sat_frame = ctk.CTkFrame(master=self.frame)
        self.sat_frame.pack(pady=10, fill="x", expand=True)
        self.sat_label = ctk.CTkLabel(master=self.sat_frame, text="Saturation: -- %", font=ctk.CTkFont(size=16, weight='bold'), compound="left")

        self.sat_label.pack(padx=10, pady=10, fill="x", expand=True)

        # Heart Rate Label
        self.hr_frame = ctk.CTkFrame(master=self.frame)
        self.hr_frame.pack(pady=10, fill="x", expand=True)
        self.hr_label = ctk.CTkLabel(master=self.hr_frame, text="Heart Rate: -- ", font=ctk.CTkFont(size=16, weight='bold'), compound="left")

        self.hr_label.pack(padx=10, pady=10, fill="x", expand=True)

        # Finger Status Label

        self.finger_status_frame = ctk.CTkFrame(master=self.frame)
        self.finger_status_frame.pack(pady=10, fill="x", expand=True)
        self.finger_status_label = ctk.CTkLabel(master=self.finger_status_frame, text="Finger Not On", compound="left", font=ctk.CTkFont(size=16, weight='bold'))

        self.finger_status_label.pack(padx=10, pady=10, fill="x", expand=True)

        self.SPO2_SAMPLES_QUEUE = SPO2_SAMPLES_QUEUE

        self.fig, self.ax = plt.subplots(2, 1, figsize=(5, 5),facecolor='#2c2c2c')
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.frame)
        self.canvas.get_tk_widget().pack(fill="both",anchor='center', expand=True)

        self.saturation_data = []
        self.heart_rate_data = []

        self.ax[0].set_title("Saturation",  color='White')
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
        self.ax[0].set_ylim([60, 101])

        self.ax[1].set_title("Heart Rate", color='white')
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
        self.ax[1].set_ylim([30, 250])


        self.ax[0].plot(self.saturation_data, 'r-')
        self.ax[1].plot(self.heart_rate_data, 'b-')
        plt.tight_layout()

        self.update_thread = threading.Thread(target=self.update_sensor_data)
        self.update_thread.daemon = True  # Ensure the thread exits when the main program does
        self.update_thread.start()

    def update_sensor_data(self):
        while True:
            hr_spo2_data = self.SPO2_SAMPLES_QUEUE.get()

            if hr_spo2_data[0]:  # Finger is on
                self.finger_status_label.configure(text="Finger Detected")
                self.finger_status_label.configure(bg_color="green")
                if itob(hr_spo2_data[2]):
                    self.sat_label.configure(text=f"Saturation: {hr_spo2_data[1]} %")
                    self.saturation_data.append(hr_spo2_data[1])

                    self.ax[0].plot(self.saturation_data)
                    self.canvas.draw()

                if itob(hr_spo2_data[4]):
                    self.hr_label.configure(text=f"Heart Rate: {hr_spo2_data[3]}")
                    self.heart_rate_data.append(hr_spo2_data[3])
                    self.ax[1].plot(self.heart_rate_data)
                    self.canvas.draw()


            else:  # Finger is not on
                self.sat_label.configure(text="Saturation: -- %")
                self.hr_label.configure(text="Heart Rate: --")
                self.finger_status_label.configure(text="Finger not present")
                self.finger_status_label.configure(bg_color="red")

