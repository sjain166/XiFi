#!/usr/bin/env python3
"""
BLE Advertising: WiFi Connected vs Disconnected Comparison
Compares power consumption for BLE advertising at 20ms intervals with:
- WiFi Connected: WiFi maintains connection in background
- WiFi Disconnected: WiFi disabled
Data capped at 15 seconds.
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# Configuration
RESULTS_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results/BLE_ADV_WIFI"
GRAPHS_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results/Graphs"
MAX_TIME_MS = 15000  # Cap at 15 seconds

def load_and_process(filepath):
    """Load CSV and cap at 15 seconds"""
    df = pd.read_csv(filepath)
    df = df[df['timestamp_ms'] <= MAX_TIME_MS].copy()

    # Calculate metrics
    avg_power = df['power_mW'].mean()
    avg_current = df['current_mA'].mean()
    avg_voltage = df['voltage_V'].mean()
    total_energy = (df['power_mW'] * np.diff(df['timestamp_ms'], prepend=0)).sum() / 1000.0

    return {
        'df': df,
        'avg_power_mW': avg_power,
        'avg_current_mA': avg_current,
        'avg_voltage_V': avg_voltage,
        'total_energy_mJ': total_energy
    }

def create_comparison_plot():
    """Create a single comparison plot showing power difference"""

    # Load data
    wifi_connected_file = os.path.join(RESULTS_DIR, "BLE_ADV_WiFi_CONNECTED.csv")
    wifi_disconnected_file = os.path.join(RESULTS_DIR, "BLE_AVD_WIFI_DISCONNECTED.csv")

    wifi_on = load_and_process(wifi_connected_file)
    wifi_off = load_and_process(wifi_disconnected_file)

    print("BLE Advertising @ 20ms Interval")
    print("="*60)
    print()
    print("WiFi Connected (Background):")
    print(f"  Avg Power:   {wifi_on['avg_power_mW']:.2f} mW")
    print(f"  Avg Current: {wifi_on['avg_current_mA']:.2f} mA")
    print(f"  Total Energy: {wifi_on['total_energy_mJ']:.2f} mJ")
    print()
    print("WiFi Disconnected:")
    print(f"  Avg Power:   {wifi_off['avg_power_mW']:.2f} mW")
    print(f"  Avg Current: {wifi_off['avg_current_mA']:.2f} mA")
    print(f"  Total Energy: {wifi_off['total_energy_mJ']:.2f} mJ")
    print()

    power_increase = ((wifi_on['avg_power_mW'] - wifi_off['avg_power_mW']) / wifi_off['avg_power_mW']) * 100
    energy_increase = ((wifi_on['total_energy_mJ'] - wifi_off['total_energy_mJ']) / wifi_off['total_energy_mJ']) * 100

    print(f"WiFi Overhead:")
    print(f"  Power Increase:  {power_increase:.1f}%")
    print(f"  Energy Increase: {energy_increase:.1f}%")
    print(f"  Power Delta:     {wifi_on['avg_power_mW'] - wifi_off['avg_power_mW']:.2f} mW")

    # Create figure
    fig, ax1 = plt.subplots(figsize=(12, 7))

    x = np.arange(2)
    width = 0.5

    # Plot power consumption
    power_values = [wifi_off['avg_power_mW'], wifi_on['avg_power_mW']]
    bars = ax1.bar(x, power_values, width,
                   color=['#2E86AB', '#A23B72'], alpha=0.8,
                   edgecolor='black', linewidth=1.5)

    ax1.set_ylabel('Average Power (mW)', fontweight='bold', fontsize=13)
    ax1.set_xlabel('WiFi State', fontweight='bold', fontsize=13)
    ax1.set_title('BLE Advertising Power Consumption: WiFi Impact\n' +
                  '(BLE @ 20ms intervals, 15s measurement)',
                  fontsize=15, fontweight='bold', pad=20)
    ax1.set_xticks(x)
    ax1.set_xticklabels(['WiFi Disconnected\n(BLE Only)', 'WiFi Connected\n(Background)'],
                        fontsize=11)
    ax1.tick_params(axis='y', labelsize=11)
    ax1.grid(True, alpha=0.3, axis='y')

    # Add power value labels on bars
    for i, (bar, val) in enumerate(zip(bars, power_values)):
        ax1.text(bar.get_x() + bar.get_width()/2, val + 5,
                f'{val:.1f} mW',
                ha='center', va='bottom', fontsize=12, fontweight='bold')

    # Add current labels below power
    current_values = [wifi_off['avg_current_mA'], wifi_on['avg_current_mA']]
    for i, (bar, curr) in enumerate(zip(bars, current_values)):
        ax1.text(bar.get_x() + bar.get_width()/2, val * 0.5,
                f'{curr:.1f} mA',
                ha='center', va='center', fontsize=10,
                color='white', fontweight='bold')

    # Add annotation for power increase
    y_arrow = max(power_values) * 0.85
    ax1.annotate('', xy=(0, y_arrow), xytext=(1, y_arrow),
                arrowprops=dict(arrowstyle='<->', color='#F18F01', lw=3))
    ax1.text(0.5, y_arrow + 10,
            f'+{power_increase:.1f}% Power\n(+{wifi_on["avg_power_mW"] - wifi_off["avg_power_mW"]:.1f} mW)',
            ha='center', fontsize=11, fontweight='bold',
            bbox=dict(boxstyle='round,pad=0.6', facecolor='#F18F01', alpha=0.7, edgecolor='black'))


    plt.tight_layout()

    output_file = os.path.join(GRAPHS_DIR, 'ble_wifi_connected_vs_disconnected.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: {output_file}")
    plt.close()

def main():
    print("\n" + "="*60)
    print("BLE Advertising: WiFi Connected vs Disconnected")
    print("="*60)
    print(f"Data capped at {MAX_TIME_MS/1000:.0f} seconds\n")

    os.makedirs(GRAPHS_DIR, exist_ok=True)
    create_comparison_plot()

    print("\n" + "="*60)
    print("Analysis complete!")
    print("="*60 + "\n")

if __name__ == "__main__":
    main()
