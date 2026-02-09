#!/usr/bin/env python3
"""
WiFi Connect/Disconnect Power Analysis
Simple visualization of power consumption during WiFi connection cycles.
10 cycles, ~1060ms average connection time
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# Configuration
RESULTS_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results/WIFI_CONN_DISCONNECT"
GRAPHS_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results/Graphs"

# Connection statistics from serial output
AVG_CONNECT_TIME_MS = 1060
NUM_CYCLES = 10

def analyze_data():
    """Load and analyze WiFi connect/disconnect data"""
    filepath = os.path.join(RESULTS_DIR, "Wifi_Connect_Disconnect.csv")
    df = pd.read_csv(filepath)

    # Calculate overall metrics
    avg_power = df['power_mW'].mean()
    avg_current = df['current_mA'].mean()
    avg_voltage = df['voltage_V'].mean()
    max_power = df['power_mW'].max()

    total_time_sec = df['timestamp_ms'].max() / 1000.0
    total_energy_mJ = (df['power_mW'] * np.diff(df['timestamp_ms'], prepend=0)).sum() / 1000.0

    # Energy per connection cycle
    energy_per_cycle = total_energy_mJ / NUM_CYCLES

    return {
        'avg_power_mW': avg_power,
        'avg_current_mA': avg_current,
        'avg_voltage_V': avg_voltage,
        'max_power_mW': max_power,
        'total_energy_mJ': total_energy_mJ,
        'energy_per_cycle_mJ': energy_per_cycle,
        'total_time_sec': total_time_sec
    }

def create_simple_plot(metrics):
    """Create a simple, clean power visualization"""

    print("WiFi Connect/Disconnect Test Results")
    print("="*50)
    print(f"Number of cycles:        {NUM_CYCLES}")
    print(f"Avg connection time:     {AVG_CONNECT_TIME_MS} ms")
    print(f"Total test duration:     {metrics['total_time_sec']:.1f} sec")
    print()
    print(f"Average Power:           {metrics['avg_power_mW']:.2f} mW")
    print(f"Average Current:         {metrics['avg_current_mA']:.2f} mA")
    print(f"Peak Power:              {metrics['max_power_mW']:.2f} mW")
    print(f"Total Energy:            {metrics['total_energy_mJ']:.2f} mJ")
    print(f"Energy per Cycle:        {metrics['energy_per_cycle_mJ']:.2f} mJ")
    print("="*50)

    # Create simple bar chart
    fig, ax = plt.subplots(figsize=(10, 7))

    # Single bar for average power
    bar = ax.bar([0], [metrics['avg_power_mW']],
                  width=0.6, color='#A23B72', alpha=0.8,
                  edgecolor='black', linewidth=2)

    # Styling
    ax.set_ylabel('Average Power (mW)', fontweight='bold', fontsize=14)
    ax.set_title(f'WiFi Connect/Disconnect Power Consumption\n',
                 fontsize=16, fontweight='bold', pad=20)
    ax.set_xticks([0])
    ax.set_xticklabels(['WiFi Connect/Disconnect\nCycles'], fontsize=12)
    ax.set_xlim(-0.8, 0.8)
    ax.grid(True, alpha=0.3, axis='y')

    # Add power value in center of bar
    ax.text(0, metrics['avg_power_mW'] * 0.5,
            f"{metrics['avg_power_mW']:.1f} mW",
            ha='center', va='center', fontsize=16, fontweight='bold',
            color='white')

    # Add key metrics in top left
    textstr = '\n'.join([
        f'Avg Power (Overall): {metrics["avg_power_mW"]:.1f} mW',
        f'Avg Time/Cycle:      {AVG_CONNECT_TIME_MS} ms',
        f'Energy/Cycle:        {metrics["energy_per_cycle_mJ"]:.1f} mJ',
        f'Total Cycles:        {NUM_CYCLES}'
    ])

    props = dict(boxstyle='round', facecolor='lightblue', alpha=0.85,
                 edgecolor='black', linewidth=1.5)
    ax.text(0.03, 0.97, textstr, transform=ax.transAxes, fontsize=12,
            verticalalignment='top', horizontalalignment='left',
            bbox=props, family='monospace', fontweight='bold')

    plt.tight_layout()

    output_file = os.path.join(GRAPHS_DIR, 'wifi_connect_disconnect_power.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: {output_file}")
    plt.close()

def main():
    print("\n" + "="*50)
    print("WiFi Connect/Disconnect Power Analysis")
    print("="*50 + "\n")

    os.makedirs(GRAPHS_DIR, exist_ok=True)

    metrics = analyze_data()
    create_simple_plot(metrics)

    print("\n" + "="*50)
    print("Analysis complete!")
    print("="*50 + "\n")

if __name__ == "__main__":
    main()
