#!/usr/bin/env python3
"""
XiFi Demo Visualizer
Real-time visualization of BLE/WiFi hybrid health monitoring system
Shows transmission mode, buffer state, rates, and energy consumption
"""

import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import numpy as np
import time
from datetime import datetime

# Configuration
WINDOW_SIZE_SECONDS = 30  # Display last 30 seconds
UPDATE_INTERVAL = 50  # ms between plot updates

class DemoVisualizer:
    def __init__(self, dut_port, baud=115200):
        """
        Initialize demo visualizer

        Args:
            dut_port: Serial port for DUT (ESP32-C5 sender)
            baud: Baud rate
        """
        print(f"Connecting to DUT: {dut_port}")

        self.dut_ser = serial.Serial(dut_port, baud, timeout=0.1)
        time.sleep(2)  # Wait for connections to stabilize

        # Display buffers (rolling window for 30 seconds)
        self.times = deque(maxlen=1000)

        # DUT data
        self.buffer_levels = deque(maxlen=1000)
        self.thresholds = deque(maxlen=1000)
        self.modes = deque(maxlen=1000)  # 0=BLE, 1=WiFi
        self.input_rates = deque(maxlen=1000)
        self.output_rates = deque(maxlen=1000)
        self.buffer_fill_rates = deque(maxlen=1000)

        # Config data (phase info)
        self.current_phase = 1
        self.test_mode = 2
        self.delay_tolerance = 5000
        self.prob_20_bytes = 70
        self.prob_60_bytes = 30

        # State
        self.start_time = None

        # Setup plot
        self.setup_plot()

    def setup_plot(self):
        """Create matplotlib figure with subplots"""
        self.fig = plt.figure(figsize=(16, 12))
        self.fig.suptitle('Demo - Hybrid BLE/WiFi Visualizer',
                         fontsize=18, fontweight='bold')

        # Create 4 subplots
        gs = self.fig.add_gridspec(4, 1, height_ratios=[1, 1.5, 0.8, 0.8],
                                   hspace=0.35, top=0.93, bottom=0.08)

        self.ax1 = self.fig.add_subplot(gs[0])  # Transmission Mode
        self.ax2 = self.fig.add_subplot(gs[1])  # Buffer Level + Threshold
        self.ax3 = self.fig.add_subplot(gs[2])  # Rates (as text overlay)
        self.ax4 = self.fig.add_subplot(gs[3])  # Phase Configuration

        # Panel 1: Transmission Mode (Binary: BLE vs UDP)
        self.ax1.set_ylabel('Mode', fontweight='bold', fontsize=11)
        self.ax1.set_ylim(-0.5, 1.5)
        self.ax1.set_yticks([0, 1])
        self.ax1.set_yticklabels(['BLE', 'UDP/WiFi'])
        self.ax1.grid(True, alpha=0.3, axis='x')
        self.ax1.set_title('Transmission Mode (Last 30s)', fontsize=12)
        self.mode_line, = self.ax1.plot([], [], 'o', markersize=2, color='blue', markeredgewidth=0)

        # Panel 2: Buffer Level + Threshold
        self.ax2.set_ylabel('Buffer (bytes)', fontweight='bold', fontsize=11)
        self.ax2.grid(True, alpha=0.3)
        self.ax2.set_title('Buffer Level & Dynamic Threshold', fontsize=12)
        self.buffer_line, = self.ax2.plot([], [], 'g-', linewidth=2.5, label='Buffer Level')
        self.threshold_line, = self.ax2.plot([], [], 'r--', linewidth=2, label='Threshold')
        self.ax2.legend(loc='upper right', fontsize=10)

        # Panel 3: Rates Display (numbers + formula)
        self.ax3.set_xlim(0, 1)
        self.ax3.set_ylim(0, 1)
        self.ax3.axis('off')
        self.ax3.set_title('Data Rates (10-second average)', fontsize=12)

        # Rate text displays
        self.rate_text = self.ax3.text(0.5, 0.7, '', fontsize=14,
                                       ha='center', va='center',
                                       bbox=dict(boxstyle='round',
                                                facecolor='lightyellow', alpha=0.8))
        self.formula_text = self.ax3.text(0.5, 0.3,
                                          'Buffer Fill Rate = Input Rate - Output Rate',
                                          fontsize=11, ha='center', va='center',
                                          style='italic', color='blue')

        # Panel 4: Phase Configuration (text display)
        self.ax4.set_xlim(0, 1)
        self.ax4.set_ylim(0, 1)
        self.ax4.axis('off')
        self.ax4.set_title('Current Phase Configuration', fontsize=12, fontweight='bold', pad=10)
        self.phase_text = self.ax4.text(0.5, 0.5, '', fontsize=13,
                                        ha='center', va='center',
                                        bbox=dict(boxstyle='round',
                                                 facecolor='lightblue', alpha=0.9),
                                        family='monospace')

    def parse_serial_data(self):
        """Read and parse data from DUT serial port"""
        current_time = time.time()
        if self.start_time is None:
            self.start_time = current_time

        elapsed = current_time - self.start_time

        dut_data = None

        # Read DUT serial
        while self.dut_ser.in_waiting:
            try:
                line = self.dut_ser.readline().decode('utf-8').strip()
                if line.startswith('SENDER,'):
                    parts = line.split(',')
                    if len(parts) >= 8:  # Support both old (8) and new (13) format
                        dut_data = {
                            'timestamp': elapsed,
                            'buffer': int(parts[2]),
                            'threshold': int(parts[3]),
                            'mode': int(parts[4]),
                            'input_rate': float(parts[5]),
                            'output_rate': float(parts[6]),
                            'buffer_fill_rate': float(parts[7])
                        }
                        # Parse phase info if available (new format)
                        if len(parts) == 13:
                            dut_data['phase'] = int(parts[8])
                            dut_data['test_mode'] = int(parts[9])
                            dut_data['delay_tolerance'] = int(parts[10])
                            dut_data['prob_20'] = int(parts[11])
                            dut_data['prob_60'] = int(parts[12])
            except:
                pass

        return dut_data

    def update_plot(self, frame):
        """Animation update function"""
        dut_data = self.parse_serial_data()

        current_time = time.time()
        if self.start_time is None:
            self.start_time = current_time
        elapsed = current_time - self.start_time

        # Update data buffers
        if dut_data is not None:
            self.times.append(elapsed)
            self.buffer_levels.append(dut_data['buffer'])
            self.thresholds.append(dut_data['threshold'])
            self.modes.append(dut_data['mode'])
            self.input_rates.append(dut_data['input_rate'])
            self.output_rates.append(dut_data['output_rate'])
            self.buffer_fill_rates.append(dut_data['buffer_fill_rate'])

            # Update phase info if available
            if 'phase' in dut_data:
                self.current_phase = dut_data['phase']
                self.test_mode = dut_data['test_mode']
                self.delay_tolerance = dut_data['delay_tolerance']
                self.prob_20_bytes = dut_data['prob_20']
                self.prob_60_bytes = dut_data['prob_60']

        # Update plots if we have data
        if len(self.times) > 0:
            times_array = np.array(self.times)

            # Filter to last 30 seconds
            cutoff_time = elapsed - WINDOW_SIZE_SECONDS
            valid_indices = times_array >= cutoff_time

            if np.any(valid_indices):
                display_times = times_array[valid_indices]

                # Panel 1: Transmission Mode
                display_modes = np.array(self.modes)[valid_indices]
                self.mode_line.set_data(display_times, display_modes)
                self.ax1.set_xlim(cutoff_time, elapsed)
                # Major ticks (with labels) every 1 second, minor ticks every 0.1 seconds
                self.ax1.xaxis.set_major_locator(plt.MultipleLocator(1.0))
                self.ax1.xaxis.set_minor_locator(plt.MultipleLocator(0.1))

                # Panel 2: Buffer + Threshold
                display_buffer = np.array(self.buffer_levels)[valid_indices]
                display_threshold = np.array(self.thresholds)[valid_indices]
                self.buffer_line.set_data(display_times, display_buffer)
                self.threshold_line.set_data(display_times, display_threshold)
                self.ax2.set_xlim(cutoff_time, elapsed)
                # Major ticks (with labels) every 2 seconds, minor ticks every 0.5 seconds
                self.ax2.xaxis.set_major_locator(plt.MultipleLocator(2.0))
                self.ax2.xaxis.set_minor_locator(plt.MultipleLocator(0.5))
                self.ax2.relim()
                self.ax2.autoscale_view(scalex=False)

                # Panel 3: Rates (text display)
                # Just use the most recent values - ESP32 already does averaging
                if len(self.input_rates) > 0:
                    # Use latest values directly (no Python-side averaging)
                    current_input = self.input_rates[-1]
                    current_output = self.output_rates[-1]
                    current_fill = self.buffer_fill_rates[-1]

                    rate_str = f"Input Rate: {current_input:.2f} B/s  |  "
                    rate_str += f"Output Rate: {current_output:.2f} B/s\n"
                    rate_str += f"Buffer Fill Rate: {current_fill:.2f} B/s"
                    self.rate_text.set_text(rate_str)

                # Panel 4: Update phase configuration display
                phase_descriptions = {
                    1: "WiFi Only (Baseline) - Moderate Delay (5s)",
                    2: "Hybrid - Moderate Delay (5s)",
                    3: "Hybrid - High Delay Tolerance (12s)",
                    4: "Hybrid - Low Delay Tolerance (2s)"
                }
                mode_str = "WiFi Only" if self.test_mode == 2 else "Hybrid BLE+WiFi"
                phase_str = f"PHASE {self.current_phase}: {phase_descriptions.get(self.current_phase, 'Unknown')}\n\n"
                phase_str += f"Mode: {mode_str}\n"
                if self.test_mode == 1:
                    phase_str += f"Delay Tolerance: {self.delay_tolerance}ms\n"
                phase_str += f"Prob(20B): {self.prob_20_bytes}%  |  Prob(60B): {self.prob_60_bytes}%"
                self.phase_text.set_text(phase_str)

        return (self.mode_line, self.buffer_line, self.threshold_line,
                self.rate_text, self.phase_text)

    def run(self):
        """Start the visualizer"""
        print("\n" + "="*60)
        print("XiFi Demo Visualizer - Running")
        print("="*60)
        print("Real-time visualization of hybrid BLE/WiFi transmission")
        print("Close window to stop")
        print("="*60 + "\n")

        # Start animation
        ani = animation.FuncAnimation(
            self.fig,
            self.update_plot,
            interval=UPDATE_INTERVAL,
            blit=False,
            cache_frame_data=False
        )

        plt.show()

        # Cleanup
        self.dut_ser.close()
        print("\nVisualizer closed.")

def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(
        description='XiFi Demo Visualizer - Hybrid BLE/WiFi Monitor',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Example usage:
  python demo_visualizer.py --dut /dev/ttyUSB0
  python demo_visualizer.py --dut COM3
        """
    )

    parser.add_argument('--dut', '-d', type=str, required=True,
                       help='Serial port for DUT (ESP32-C5 sender)')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                       help='Baud rate (default: 115200)')

    args = parser.parse_args()

    try:
        visualizer = DemoVisualizer(
            dut_port=args.dut,
            baud=args.baud
        )
        visualizer.run()
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == '__main__':
    main()
