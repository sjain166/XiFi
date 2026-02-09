#!/usr/bin/env python3

import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button
from collections import deque
import numpy as np
import time

WINDOW_SIZE = 500
UPDATE_INTERVAL = 50

class LivePlotter:
    def __init__(self, port=None, baud=115200):
        if port is None:
            port = self.find_esp32_port()

        print(f"Connecting to {port} at {baud} baud...")
        self.ser = serial.Serial(port, baud, timeout=1)
        time.sleep(2)

        # rolling window for display
        self.times = deque(maxlen=WINDOW_SIZE)
        self.currents = deque(maxlen=WINDOW_SIZE)
        self.powers = deque(maxlen=WINDOW_SIZE)
        self.voltages = deque(maxlen=WINDOW_SIZE)
        self.ble_markers = deque(maxlen=WINDOW_SIZE)
        self.wifi_markers = deque(maxlen=WINDOW_SIZE)
        self.sleep_markers = deque(maxlen=WINDOW_SIZE)

        # storage for saving data
        self.stored_times = []
        self.stored_currents = []
        self.stored_powers = []
        self.stored_voltages = []
        self.stored_ble_markers = []
        self.stored_wifi_markers = []
        self.stored_sleep_markers = []

        self.logging = False
        self.start_time = None

        self.setup_plot()

    def find_esp32_port(self):
        ports = serial.tools.list_ports.comports()
        for port in ports:
            if any(x in port.description.lower() for x in ['cp210', 'ch340', 'usb', 'serial']):
                print(f"Found ESP32 at: {port.device}")
                return port.device

        if ports:
            return ports[0].device

        raise Exception("No serial ports found!")

    def setup_plot(self):
        self.fig = plt.figure(figsize=(14, 8))
        self.fig.suptitle('Live Power Tracking', fontsize=16, fontweight='bold')

        # create subplots
        self.ax1 = plt.subplot(3, 1, 1)
        self.ax2 = plt.subplot(3, 1, 2)
        self.ax3 = plt.subplot(3, 1, 3)

        # current
        self.ax1.set_ylabel('Current (mA)', fontweight='bold')
        self.ax1.grid(True, alpha=0.3)
        self.line_current, = self.ax1.plot([], [], 'b-', linewidth=2, label='Current')
        self.ax1.legend(loc='upper right')

        # power
        self.ax2.set_ylabel('Power (mW)', fontweight='bold')
        self.ax2.grid(True, alpha=0.3)
        self.line_power, = self.ax2.plot([], [], 'r-', linewidth=2, label='Power')
        self.ax2.legend(loc='upper right')

        # voltage + phase markers
        self.ax3.set_ylabel('Voltage (V)', fontweight='bold')
        self.ax3.set_xlabel('Time (seconds)', fontweight='bold')
        self.ax3.grid(True, alpha=0.3)
        self.line_voltage, = self.ax3.plot([], [], 'g-', linewidth=2, label='Voltage')

        self.ax3_twin = self.ax3.twinx()
        self.ax3_twin.set_ylabel('Phase', fontweight='bold')
        self.ax3_twin.set_ylim(-0.5, 3.5)
        self.ax3_twin.set_yticks([0, 1, 2, 3])
        self.ax3_twin.set_yticklabels(['Idle', 'BLE', 'WiFi', 'Sleep'])

        self.ax3.legend(loc='upper left')

        # buttons
        plt.subplots_adjust(bottom=0.15)

        ax_start = plt.axes([0.15, 0.02, 0.08, 0.05])
        ax_stop = plt.axes([0.24, 0.02, 0.08, 0.05])
        ax_clear = plt.axes([0.33, 0.02, 0.08, 0.05])
        ax_stats = plt.axes([0.42, 0.02, 0.11, 0.05])
        ax_csv = plt.axes([0.54, 0.02, 0.11, 0.05])

        self.btn_start = Button(ax_start, 'START', color='lightgreen')
        self.btn_stop = Button(ax_stop, 'STOP', color='lightcoral')
        self.btn_clear = Button(ax_clear, 'CLEAR', color='lightyellow')
        self.btn_stats = Button(ax_stats, 'AVG POWER', color='lightblue')
        self.btn_csv = Button(ax_csv, 'SAVE CSV', color='lightpink')

        self.btn_start.on_clicked(self.on_start)
        self.btn_stop.on_clicked(self.on_stop)
        self.btn_clear.on_clicked(self.on_clear)
        self.btn_stats.on_clicked(self.on_stats)
        self.btn_csv.on_clicked(self.on_save_csv)

        self.status_text = self.fig.text(0.02, 0.02, 'Status: IDLE', fontsize=12,
                                         bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

        self.stats_text = self.fig.text(0.70, 0.02, 'Avg Power: N/A', fontsize=11,
                                        bbox=dict(boxstyle='round', facecolor='lightcyan', alpha=0.5))

    def on_start(self, event):
        self.ser.write(b'start\n')
        self.logging = True
        self.start_time = time.time()

        # clear old data
        self.stored_times.clear()
        self.stored_currents.clear()
        self.stored_powers.clear()
        self.stored_voltages.clear()
        self.stored_ble_markers.clear()
        self.stored_wifi_markers.clear()
        self.stored_sleep_markers.clear()

        self.status_text.set_text('Status: LOGGING')
        self.status_text.set_bbox(dict(boxstyle='round', facecolor='lightgreen', alpha=0.5))
        self.stats_text.set_text('Avg Power: N/A')

    def on_stop(self, event):
        self.ser.write(b'stop\n')
        self.logging = False
        self.status_text.set_text('Status: STOPPED')
        self.status_text.set_bbox(dict(boxstyle='round', facecolor='lightcoral', alpha=0.5))
        print(f"Stopped - {len(self.stored_times)} samples recorded")

    def on_clear(self, event):
        self.times.clear()
        self.currents.clear()
        self.powers.clear()
        self.voltages.clear()
        self.ble_markers.clear()
        self.wifi_markers.clear()
        self.sleep_markers.clear()

        self.stored_times.clear()
        self.stored_currents.clear()
        self.stored_powers.clear()
        self.stored_voltages.clear()
        self.stored_ble_markers.clear()
        self.stored_wifi_markers.clear()
        self.stored_sleep_markers.clear()

        self.stats_text.set_text('Avg Power: N/A')

    def on_stats(self, event):
        if len(self.stored_powers) == 0:
            self.stats_text.set_text('Avg Power: NO DATA')
            return

        avg_power = np.mean(self.stored_powers)
        max_power = np.max(self.stored_powers)
        min_power = np.min(self.stored_powers)
        avg_current = np.mean(self.stored_currents)
        duration = self.stored_times[-1] - self.stored_times[0] if len(self.stored_times) > 1 else 0

        # calculate total energy
        if len(self.stored_times) > 1:
            time_interval = (self.stored_times[-1] - self.stored_times[0]) / (len(self.stored_times) - 1)
            total_energy_mJ = sum(self.stored_powers) * time_interval
        else:
            total_energy_mJ = 0

        self.stats_text.set_text(
            f'Avg: {avg_power:.2f}mW | {avg_current:.2f}mA\n'
            f'Max: {max_power:.2f}mW | Energy: {total_energy_mJ:.0f}mJ'
        )

        print("\n" + "="*50)
        print("POWER STATISTICS")
        print("="*50)
        print(f"Duration:        {duration:.2f} seconds")
        print(f"Samples:         {len(self.stored_powers)}")
        print(f"Average Power:   {avg_power:.2f} mW")
        print(f"Average Current: {avg_current:.2f} mA")
        print(f"Max Power:       {max_power:.2f} mW")
        print(f"Min Power:       {min_power:.2f} mW")
        print(f"Total Energy:    {total_energy_mJ:.2f} mJ ({total_energy_mJ/1000:.3f} J)")
        print("="*50 + "\n")

    def on_save_csv(self, event):
        if len(self.stored_times) == 0:
            print("No data to save")
            return

        import os
        from datetime import datetime

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"power_log_{timestamp}.csv"

        try:
            with open(filename, 'w') as f:
                f.write("timestamp_ms,voltage_V,current_mA,power_mW,marker_ble,marker_wifi,marker_sleep\n")

                for i in range(len(self.stored_times)):
                    f.write(f"{self.stored_times[i]*1000:.0f},"
                           f"{self.stored_voltages[i]:.3f},"
                           f"{self.stored_currents[i]:.2f},"
                           f"{self.stored_powers[i]:.2f},"
                           f"{self.stored_ble_markers[i]},"
                           f"{self.stored_wifi_markers[i]},"
                           f"{self.stored_sleep_markers[i]}\n")

            abs_path = os.path.abspath(filename)
            print(f"\nCSV saved: {abs_path}")
            print(f"Samples: {len(self.stored_times)}, Duration: {self.stored_times[-1] - self.stored_times[0]:.2f}s\n")

        except Exception as e:
            print(f"Error saving CSV: {e}")

    def update_plot(self, frame):
        while self.ser.in_waiting:
            try:
                line = self.ser.readline().decode('utf-8').strip()

                if line.startswith('DATA,'):
                    parts = line.split(',')
                    if len(parts) == 8:
                        timestamp = float(parts[1]) / 1000.0
                        voltage = float(parts[2])
                        current = float(parts[3])
                        power = float(parts[4])
                        ble = int(parts[5])
                        wifi = int(parts[6])
                        sleep = int(parts[7])

                        # update display
                        self.times.append(timestamp)
                        self.voltages.append(voltage)
                        self.currents.append(current)
                        self.powers.append(power)
                        self.ble_markers.append(ble)
                        self.wifi_markers.append(wifi)
                        self.sleep_markers.append(sleep)

                        # store if logging
                        if self.logging:
                            self.stored_times.append(timestamp)
                            self.stored_voltages.append(voltage)
                            self.stored_currents.append(current)
                            self.stored_powers.append(power)
                            self.stored_ble_markers.append(ble)
                            self.stored_wifi_markers.append(wifi)
                            self.stored_sleep_markers.append(sleep)

            except Exception as e:
                print(f"Parse error: {e}")

        if self.logging:
            self.status_text.set_text(f'Status: LOGGING ({len(self.stored_times)} samples)')

        # update plots
        if len(self.times) > 0:
            times_array = np.array(self.times)

            self.line_current.set_data(times_array, np.array(self.currents))
            self.ax1.relim()
            self.ax1.autoscale_view()

            self.line_power.set_data(times_array, np.array(self.powers))
            self.ax2.relim()
            self.ax2.autoscale_view()

            self.line_voltage.set_data(times_array, np.array(self.voltages))
            self.ax3.relim()
            self.ax3.autoscale_view()

            # update phase markers
            self.ax3_twin.clear()
            self.ax3_twin.set_ylabel('Phase', fontweight='bold')
            self.ax3_twin.set_ylim(-0.5, 3.5)
            self.ax3_twin.set_yticks([0, 1, 2, 3])
            self.ax3_twin.set_yticklabels(['Idle', 'BLE', 'WiFi', 'Sleep'])

            for i in range(len(times_array)):
                if i == 0:
                    continue

                t_start = times_array[i-1]
                t_end = times_array[i]

                if self.ble_markers[i]:
                    self.ax3_twin.axvspan(t_start, t_end, alpha=0.3, color='blue')
                elif self.wifi_markers[i]:
                    self.ax3_twin.axvspan(t_start, t_end, alpha=0.3, color='orange')
                elif self.sleep_markers[i]:
                    self.ax3_twin.axvspan(t_start, t_end, alpha=0.3, color='green')

        return self.line_current, self.line_power, self.line_voltage

    def run(self):
        print("Starting plotter...")

        ani = animation.FuncAnimation(
            self.fig,
            self.update_plot,
            interval=UPDATE_INTERVAL,
            blit=False,
            cache_frame_data=False
        )

        plt.show()
        self.ser.close()

def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--port',  type=str)
    parser.add_argument('--baud',  type=int, default=115200)
    args = parser.parse_args()

    try:
        plotter = LivePlotter(port=args.port, baud=args.baud)
        plotter.run()
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == '__main__':
    main()
