#!/usr/bin/env python3
"""
WiFi Idle vs BLE Scanning Comparison
Compares power consumption for:
- WiFi Idle: WiFi connected but no activity
- BLE Scanning: Scanning for BLE devices at 1000ms intervals
Data capped at 15 seconds.
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# Configuration
RESULTS_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results"
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
        'avg_power_mW': avg_power,
        'avg_current_mA': avg_current,
        'avg_voltage_V': avg_voltage,
        'total_energy_mJ': total_energy
    }

def create_comparison_plot():
    """Create a comparison plot showing power difference"""

    # Load data
    wifi_idle_file = os.path.join(RESULTS_DIR, "WIFI_IDLE", "wifi_idle.csv")
    ble_scan_file = os.path.join(RESULTS_DIR, "BLE_SCANNING", "ble_scanning.csv")

    wifi_metrics = load_and_process(wifi_idle_file)
    ble_metrics = load_and_process(ble_scan_file)

    print("WiFi Idle vs BLE Scanning")
    print("="*60)
    print()
    print("WiFi Idle (Connected, No Activity):")
    print(f"  Avg Power:   {wifi_metrics['avg_power_mW']:.2f} mW")
    print(f"  Avg Current: {wifi_metrics['avg_current_mA']:.2f} mA")
    print(f"  Total Energy: {wifi_metrics['total_energy_mJ']:.2f} mJ")
    print()
    print("BLE Scanning (1000ms intervals):")
    print(f"  Avg Power:   {ble_metrics['avg_power_mW']:.2f} mW")
    print(f"  Avg Current: {ble_metrics['avg_current_mA']:.2f} mA")
    print(f"  Total Energy: {ble_metrics['total_energy_mJ']:.2f} mJ")
    print()

    power_diff = wifi_metrics['avg_power_mW'] - ble_metrics['avg_power_mW']
    power_diff_pct = (power_diff / ble_metrics['avg_power_mW']) * 100

    if power_diff > 0:
        print(f"WiFi Idle uses {power_diff:.2f} mW MORE ({power_diff_pct:.1f}%) than BLE Scanning")
    else:
        print(f"BLE Scanning uses {abs(power_diff):.2f} mW MORE ({abs(power_diff_pct):.1f}%) than WiFi Idle")

    # Create figure
    fig, ax1 = plt.subplots(figsize=(12, 7))

    x = np.arange(2)
    width = 0.5

    # Plot power consumption
    power_values = [wifi_metrics['avg_power_mW'], ble_metrics['avg_power_mW']]
    bars = ax1.bar(x, power_values, width,
                   color=['#A23B72', '#2E86AB'], alpha=0.8,
                   edgecolor='black', linewidth=1.5)

    ax1.set_ylabel('Average Power (mW)', fontweight='bold', fontsize=13)
    ax1.set_xlabel('Operation Mode', fontweight='bold', fontsize=13)
    ax1.set_title('WiFi Idle vs BLE Scanning: Power Consumption\n' +
                  '(15s measurement)',
                  fontsize=15, fontweight='bold', pad=20)
    ax1.set_xticks(x)
    ax1.set_xticklabels(['WiFi Idle\n(Connected)', 'BLE Scanning\n(1000ms intervals)'],
                        fontsize=11)
    ax1.tick_params(axis='y', labelsize=11)
    ax1.grid(True, alpha=0.3, axis='y')

    # Add power values in center of bars
    current_values = [wifi_metrics['avg_current_mA'], ble_metrics['avg_current_mA']]
    for i, (bar, val, curr) in enumerate(zip(bars, power_values, current_values)):
        ax1.text(bar.get_x() + bar.get_width()/2, val * 0.5,
                f'{val:.1f} mW',
                ha='center', va='center', fontsize=12,
                color='white', fontweight='bold')

    # Add power difference annotation
    y_arrow = max(power_values) * 0.85
    if abs(power_diff) > 1:  # Only show if difference is significant
        if power_diff > 0:
            ax1.annotate('', xy=(1, y_arrow), xytext=(0, y_arrow),
                        arrowprops=dict(arrowstyle='<->', color='#F18F01', lw=3))
            ax1.text(0.5, y_arrow + 10,
                    f'{abs(power_diff):.1f} mW\n({abs(power_diff_pct):.1f}%)',
                    ha='center', fontsize=11, fontweight='bold',
                    bbox=dict(boxstyle='round,pad=0.6', facecolor='#F18F01',
                             alpha=0.7, edgecolor='black'))
        else:
            ax1.annotate('', xy=(0, y_arrow), xytext=(1, y_arrow),
                        arrowprops=dict(arrowstyle='<->', color='#F18F01', lw=3))
            ax1.text(0.5, y_arrow + 10,
                    f'{abs(power_diff):.1f} mW\n({abs(power_diff_pct):.1f}%)',
                    ha='center', fontsize=11, fontweight='bold',
                    bbox=dict(boxstyle='round,pad=0.6', facecolor='#F18F01',
                             alpha=0.7, edgecolor='black'))

    output_file = os.path.join(GRAPHS_DIR, 'wifi_idle_vs_ble_scanning.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: {output_file}")
    plt.close()

def main():
    print("\n" + "="*60)
    print("WiFi Idle vs BLE Scanning Comparison")
    print("="*60)
    print(f"Data capped at {MAX_TIME_MS/1000:.0f} seconds\n")

    os.makedirs(GRAPHS_DIR, exist_ok=True)
    create_comparison_plot()

    print("\n" + "="*60)
    print("Analysis complete!")
    print("="*60 + "\n")

if __name__ == "__main__":
    main()
