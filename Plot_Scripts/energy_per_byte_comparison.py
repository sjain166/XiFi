#!/usr/bin/env python3
"""
Energy Per Byte Comparison
Compares energy efficiency (mJ/byte) across different communication modes
at various sending rates (20ms, 50ms, 100ms intervals).
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# Configuration
RESULTS_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results"
GRAPHS_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results/Graphs"
MAX_TIME_MS = 15000  # Cap at 15 seconds
PAYLOAD_SIZE = 20    # bytes per packet

# Modes and intervals to compare
MODES = ['BLE_ADV', 'BLE_CONN', 'WiFi_BRD', 'WiFi_CONN']
INTERVALS = [20, 50, 100]  # milliseconds

def calculate_total_bytes(interval_ms):
    """Calculate total bytes sent in 15 seconds for given interval"""
    num_packets = MAX_TIME_MS // interval_ms
    return num_packets * PAYLOAD_SIZE

def load_and_calculate_energy_per_byte(mode, interval_ms):
    """Load CSV and calculate energy per byte"""
    filepath = os.path.join(RESULTS_DIR, mode, f"{interval_ms}ms.csv")

    if not os.path.exists(filepath):
        print(f"Warning: {filepath} not found, skipping...")
        return None

    # Load data
    df = pd.read_csv(filepath)
    df = df[df['timestamp_ms'] <= MAX_TIME_MS].copy()

    # Calculate total energy (mJ)
    time_deltas = np.diff(df['timestamp_ms'], prepend=0) / 1000.0  # convert to seconds
    total_energy_mJ = (df['power_mW'] * time_deltas).sum()

    # Calculate total bytes
    total_bytes = calculate_total_bytes(interval_ms)

    # Calculate energy per byte
    energy_per_byte = total_energy_mJ / total_bytes if total_bytes > 0 else 0

    return {
        'mode': mode,
        'interval_ms': interval_ms,
        'total_energy_mJ': total_energy_mJ,
        'total_bytes': total_bytes,
        'energy_per_byte_mJ': energy_per_byte,
        'avg_power_mW': df['power_mW'].mean()
    }

def create_comparison_plot():
    """Create grouped bar chart comparing energy per byte"""

    # Collect all data
    results = []
    for mode in MODES:
        for interval in INTERVALS:
            result = load_and_calculate_energy_per_byte(mode, interval)
            if result:
                results.append(result)

    # Convert to structured format for plotting
    data_by_mode = {mode: [] for mode in MODES}
    for result in results:
        data_by_mode[result['mode']].append(result['energy_per_byte_mJ'])

    # Print summary table
    print("\n" + "="*80)
    print("Energy Per Byte Comparison (15 second measurement)")
    print("="*80)
    print(f"{'Mode':<12} {'Interval':<10} {'Total Energy':<15} {'Total Bytes':<12} {'Energy/Byte':<15} {'Avg Power'}")
    print(f"{'':12} {'(ms)':<10} {'(mJ)':<15} {'(bytes)':<12} {'(mJ/byte)':<15} {'(mW)'}")
    print("-"*80)
    for result in results:
        print(f"{result['mode']:<12} {result['interval_ms']:<10} "
              f"{result['total_energy_mJ']:<15.2f} {result['total_bytes']:<12} "
              f"{result['energy_per_byte_mJ']:<15.4f} {result['avg_power_mW']:.2f}")
    print("="*80)

    # Create plot
    fig, ax = plt.subplots(figsize=(14, 8))

    x = np.arange(len(INTERVALS))
    width = 0.2
    colors = ['#2E86AB', '#A23B72', '#F18F01', '#C73E1D']

    # Plot bars for each mode
    for i, mode in enumerate(MODES):
        if mode in data_by_mode and len(data_by_mode[mode]) == len(INTERVALS):
            offset = (i - len(MODES)/2 + 0.5) * width
            bars = ax.bar(x + offset, data_by_mode[mode], width,
                         label=mode, color=colors[i], alpha=0.8)

            # Add value labels on bars
            for bar in bars:
                height = bar.get_height()
                ax.text(bar.get_x() + bar.get_width()/2, height,
                       f'{height:.4f}',
                       ha='center', va='bottom', fontsize=9, rotation=0)

    # Customize plot
    ax.set_xlabel('Sending Interval (ms)', fontweight='bold', fontsize=13)
    ax.set_ylabel('Energy per Byte (mJ/byte)', fontweight='bold', fontsize=13)
    ax.set_title('Energy Efficiency Comparison: Energy per Byte\n' +
                '(Lower is better - 15 second measurement, 20 byte payload)',
                fontsize=15, fontweight='bold', pad=20)
    ax.set_xticks(x)
    ax.set_xticklabels([f'{interval}ms' for interval in INTERVALS], fontsize=11)
    ax.legend(loc='upper left', fontsize=11, framealpha=0.9)
    ax.grid(True, alpha=0.3, axis='y')
    ax.tick_params(axis='y', labelsize=11)
    plt.tight_layout()

    # Save plot
    output_file = os.path.join(GRAPHS_DIR, 'energy_per_byte_comparison.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: {output_file}")
    plt.close()

def main():
    print("Energy Per Byte Comparison")
    print("="*50)
    print(f"Payload size: {PAYLOAD_SIZE} bytes")
    print(f"Measurement duration: {MAX_TIME_MS/1000:.0f} seconds")
    print(f"Intervals: {INTERVALS} ms")
    print(f"Modes: {MODES}\n")

    os.makedirs(GRAPHS_DIR, exist_ok=True)
    create_comparison_plot()

    print("\nAnalysis complete!")

if __name__ == "__main__":
    main()
