#!/usr/bin/env python3
"""
BLE vs WiFi Energy Comparison Script - Broadcast vs Connected
Compares total energy consumption for:
- BLE_ADV (BLE Advertising - broadcast)
- BLE_CONN (BLE Connected - point-to-point)
- WiFi_BRD (WiFi UDP Broadcast)
- WiFi_CONN (WiFi UDP Unicast - point-to-point)
Across transmission intervals: 20ms, 50ms, 100ms
Data capped at 15 seconds for consistent comparison.
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# Configuration
RESULTS_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results"
GRAPHS_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results/Graphs"
INTERVALS = ['20ms', '50ms', '100ms']
MAX_TIME_MS = 15000  # Cap at 15 seconds

# Create graphs directory if it doesn't exist
os.makedirs(GRAPHS_DIR, exist_ok=True)

def load_data(mode, interval):
    """Load CSV data for a given mode and interval, capped at 15 seconds"""
    filepath = os.path.join(RESULTS_DIR, mode, f"{interval}.csv")
    df = pd.read_csv(filepath)
    # Filter to first 15 seconds
    df = df[df['timestamp_ms'] <= MAX_TIME_MS].copy()
    return df

def calculate_metrics(df):
    """Calculate key metrics from the dataframe"""
    metrics = {
        'avg_power_mW': df['power_mW'].mean(),
        'avg_current_mA': df['current_mA'].mean(),
        'avg_voltage_V': df['voltage_V'].mean(),
        'total_energy_mJ': (df['power_mW'] * np.diff(df['timestamp_ms'], prepend=0)).sum() / 1000.0,
        'duration_s': (df['timestamp_ms'].max() - df['timestamp_ms'].min()) / 1000.0,
        'peak_power_mW': df['power_mW'].max(),
        'peak_current_mA': df['current_mA'].max()
    }
    return metrics

def plot_energy_comparison():
    """Plot total energy consumption comparison"""
    metrics_data = {'BLE_ADV': {}, 'BLE_CONN': {}, 'WiFi_BRD': {}, 'WiFi_CONN': {}}

    for mode in ['BLE_ADV', 'BLE_CONN', 'WiFi_BRD', 'WiFi_CONN']:
        for interval in INTERVALS:
            df = load_data(mode, interval)
            metrics_data[mode][interval] = calculate_metrics(df)

    fig, ax = plt.subplots(figsize=(14, 6))

    x = np.arange(len(INTERVALS))
    width = 0.20  # Width for 4 bars

    ble_adv_energy = [metrics_data['BLE_ADV'][i]['total_energy_mJ'] for i in INTERVALS]
    ble_conn_energy = [metrics_data['BLE_CONN'][i]['total_energy_mJ'] for i in INTERVALS]
    wifi_brd_energy = [metrics_data['WiFi_BRD'][i]['total_energy_mJ'] for i in INTERVALS]
    wifi_conn_energy = [metrics_data['WiFi_CONN'][i]['total_energy_mJ'] for i in INTERVALS]

    # Plot four bars with different colors
    ax.bar(x - 1.5*width, ble_adv_energy, width, label='BLE ADV', color='#2E86AB', alpha=0.8)
    ax.bar(x - 0.5*width, ble_conn_energy, width, label='BLE CONN', color='#06A77D', alpha=0.8)
    ax.bar(x + 0.5*width, wifi_brd_energy, width, label='WiFi BRD', color='#A23B72', alpha=0.8)
    ax.bar(x + 1.5*width, wifi_conn_energy, width, label='WiFi CONN', color='#F18F01', alpha=0.8)

    ax.set_ylabel('Total Energy (mJ)', fontweight='bold', fontsize=12)
    ax.set_xlabel('Transmission Interval', fontweight='bold', fontsize=12)
    ax.set_title('Energy Consumption: BLE vs WiFi - Broadcast vs Connected (15s, 20 bytes)',
                 fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(INTERVALS)
    ax.legend(fontsize=10, loc='upper right')
    ax.grid(True, alpha=0.3, axis='y')

    # Add value labels on bars
    for i, (adv, conn, brd, uni) in enumerate(zip(ble_adv_energy, ble_conn_energy, wifi_brd_energy, wifi_conn_energy)):
        ax.text(i - 1.5*width, adv + 50, f'{adv:.0f}', ha='center', va='bottom', fontsize=8)
        ax.text(i - 0.5*width, conn + 50, f'{conn:.0f}', ha='center', va='bottom', fontsize=8)
        ax.text(i + 0.5*width, brd + 50, f'{brd:.0f}', ha='center', va='bottom', fontsize=8)
        ax.text(i + 1.5*width, uni + 50, f'{uni:.0f}', ha='center', va='bottom', fontsize=8)

    plt.tight_layout()
    plt.savefig(os.path.join(GRAPHS_DIR, 'energy_comparison.png'), dpi=300, bbox_inches='tight')
    print("✓ Saved: energy_comparison.png")
    plt.close()

def main():
    print("Starting BLE vs WiFi (Broadcast/Connected) energy comparison analysis...")
    print("Modes: BLE_ADV, BLE_CONN, WiFi_BRD, WiFi_CONN")
    print(f"Data capped at {MAX_TIME_MS/1000.0:.0f} seconds\n")

    # Generate energy comparison plot
    print("Generating energy comparison plot...")
    plot_energy_comparison()

    print(f"\nPlot saved to: {GRAPHS_DIR}/energy_comparison.png")
    print("Analysis complete!")

if __name__ == "__main__":
    main()
