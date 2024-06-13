import customtkinter as ctk
import threading
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

def btoi(b: bool) -> int:
    return 1 if b == True else 0

def itob(i: int) -> bool:
    return True if i > 0 else False
class CO2_Analyzer(ctk.CTkToplevel):
    def __init__(self, master, CO2_SAMPLES_QUEUE):
        super().__init__(master)
        self.geometry(f"{800}x{800}")
        self.title("SCD40 Analyzer")

        self.frame = ctk.CTkFrame(master=self, width=400, height=400)
        self.frame.pack(pady=20, padx=20, fill="both", expand=True)

        # co2 Label
        self.co2_frame = ctk.CTkFrame(master=self.frame)
        self.co2_frame.pack(pady=10, fill="x", expand=True)
        self.co2_label = ctk.CTkLabel(master=self.co2_frame, text="Co2: -- ppm", font=ctk.CTkFont(size=16, weight='bold'), compound="left")

        self.co2_label.pack(padx=10, pady=10, fill="x", expand=True)

        # temperature Rate Label
        self.temp_frame = ctk.CTkFrame(master=self.frame)
        self.temp_frame.pack(pady=10, fill="x", expand=True)
        self.temp_label = ctk.CTkLabel(master=self.temp_frame, text="Temperature: -- oC ", font=ctk.CTkFont(size=16, weight='bold'), compound="left")
        self.temp_label.pack(padx=10, pady=10, fill="x", expand=True)

        # temperature Rate Label
        self.humm_frame = ctk.CTkFrame(master=self.frame)
        self.humm_frame.pack(pady=10, fill="x", expand=True)
        self.humm_label = ctk.CTkLabel(master=self.humm_frame, text="Hummidity: -- % ",
                                       font=ctk.CTkFont(size=16, weight='bold'), compound="left")
        self.humm_label.pack(padx=10, pady=10, fill="x", expand=True)

        self.CO2_SAMPLES_QUEUE = CO2_SAMPLES_QUEUE

        self.fig, self.ax = plt.subplots(3, 1, figsize=(5, 5),facecolor='#2c2c2c')
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.frame)
        self.canvas.get_tk_widget().pack(fill="both",anchor='center', expand=True)

        self.co2_data = []
        self.temp_data = []
        self.humm_data = []

        self.ax[0].set_title("CO2 Level",  color='White')
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
        self.ax[0].set_ylim([400, 2000])

        self.ax[1].set_title("Temperature oC", color='white')
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
        self.ax[1].set_ylim([10, 40])

        self.ax[2].set_title("Hummidity %", color='white')
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
        self.ax[2].set_ylim([0, 100])

        self.ax[0].plot(self.co2_data, 'r-')
        self.ax[1].plot(self.temp_data, 'b-')
        self.ax[2].plot(self.humm_data, 'g-')
        plt.tight_layout()

        self.update_thread = threading.Thread(target=self.update_data)
        self.update_thread.daemon = True  # Ensure the thread exits when the main program does
        self.update_thread.start()

    def update_data(self):
        while True:
            sensor_data = self.CO2_SAMPLES_QUEUE.get()

            self.co2_label.configure(text=f"Co2 Level: {sensor_data[0]} ppm")
            self.co2_data.append(sensor_data[0])
            self.ax[0].plot(self.co2_data)
            self.canvas.draw()

            self.temp_label.configure(text=f"Temperature: {sensor_data[1]} oC")
            self.temp_data.append(sensor_data[1])
            self.ax[1].plot(self.temp_data)
            self.canvas.draw()

            self.humm_label.configure(text=f"Humidity: {sensor_data[2]} %")
            self.humm_data.append(sensor_data[2])
            self.ax[2].plot(self.humm_data)
            self.canvas.draw()



