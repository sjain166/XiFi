#!/usr/bin/env python3
"""Analyze XiFi power measurement data for protocol comparison."""

import pandas as pd
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
import matplotlib.pyplot as plt
import numpy as np
import os
from pathlib import Path

BASE_DIR = Path(__file__).parent.parent
DATA_DIR = BASE_DIR / "Data"
RESULTS_DIR = BASE_DIR / "results"
IMAGES_DIR = BASE_DIR / "images"

def load_all_data():
    """Load all CSV files and extract metrics."""
    results = []

    for size_dir in DATA_DIR.iterdir():
        if not size_dir.is_dir():
            continue

        # Parse payload size from folder name
        size_str = size_dir.name.split()[0]
        try:
            payload_size = int(size_str)
        except:
            continue

        for csv_file in size_dir.glob("*.csv"):
            mode = csv_file.stem.replace(" ", "")  # BLE_ADV, BLE_CONN, WiFi

            try:
                df = pd.read_csv(csv_file)

                # Get transfer phase (marker=1)
                transfer = df[df['marker'] == 1]
                idle = df[df['marker'] == 0]

                if len(transfer) == 0:
                    print(f"Warning: No transfer data in {csv_file}")
                    continue

                # Calculate metrics
                duration_ms = transfer['timestamp_ms'].max() - transfer['timestamp_ms'].min()
                avg_current_mA = transfer['current_mA'].mean()
                avg_power_mW = transfer['power_mW'].mean()
                idle_power_mW = idle['power_mW'].mean() if len(idle) > 0 else 0

                # Energy = Power × Time (mW × ms = µJ)
                energy_uJ = avg_power_mW * duration_ms
                energy_mJ = energy_uJ / 1000

                # Energy per byte (using transfer phase only)
                energy_per_byte_uJ = (energy_mJ * 1000) / payload_size if payload_size > 0 else 0

                # Also calculate total energy for full test (init + transfer)
                total_energy_mJ = df['power_mW'].mean() * (df['timestamp_ms'].max() - df['timestamp_ms'].min()) / 1000

                results.append({
                    'mode': mode,
                    'payload_bytes': payload_size,
                    'duration_ms': duration_ms,
                    'avg_current_mA': avg_current_mA,
                    'avg_power_mW': avg_power_mW,
                    'energy_mJ': energy_mJ,
                    'energy_per_byte_uJ': energy_per_byte_uJ,
                    'total_test_energy_mJ': total_energy_mJ
                })

            except Exception as e:
                print(f"Error loading {csv_file}: {e}")

    return pd.DataFrame(results)

def plot_comparison(df):
    """Create comparison plots."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))

    modes = ['BLE_ADV', 'BLE_CONN', 'WiFi']
    colors = {'BLE_ADV': 'blue', 'BLE_CONN': 'green', 'WiFi': 'red'}

    # Sort by payload size
    df = df.sort_values('payload_bytes')

    # 1. Energy vs Payload Size
    ax = axes[0, 0]
    for mode in modes:
        data = df[df['mode'] == mode]
        ax.plot(data['payload_bytes'], data['energy_mJ'], 'o-',
                label=mode, color=colors[mode], linewidth=2, markersize=8)
    ax.set_xlabel('Payload Size (bytes)')
    ax.set_ylabel('Transfer Energy (mJ)')
    ax.set_title('Energy During Transfer Phase')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')

    # 2. Energy per Byte vs Payload Size
    ax = axes[0, 1]
    for mode in modes:
        data = df[df['mode'] == mode]
        ax.plot(data['payload_bytes'], data['energy_per_byte_uJ'], 'o-',
                label=mode, color=colors[mode], linewidth=2, markersize=8)
    ax.set_xlabel('Payload Size (bytes)')
    ax.set_ylabel('Energy per Byte (µJ/byte)')
    ax.set_title('Efficiency: Energy per Byte')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')
    ax.set_yscale('log')

    # 3. Average Power vs Payload Size
    ax = axes[1, 0]
    for mode in modes:
        data = df[df['mode'] == mode]
        ax.plot(data['payload_bytes'], data['avg_power_mW'], 'o-',
                label=mode, color=colors[mode], linewidth=2, markersize=8)
    ax.set_xlabel('Payload Size (bytes)')
    ax.set_ylabel('Avg Power (mW)')
    ax.set_title('Power Draw During Transfer')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')

    # 4. Duration vs Payload Size
    ax = axes[1, 1]
    for mode in modes:
        data = df[df['mode'] == mode]
        ax.plot(data['payload_bytes'], data['duration_ms']/1000, 'o-',
                label=mode, color=colors[mode], linewidth=2, markersize=8)
    ax.set_xlabel('Payload Size (bytes)')
    ax.set_ylabel('Transfer Duration (s)')
    ax.set_title('Time to Complete Transfer')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')

    plt.tight_layout()
    plt.savefig(IMAGES_DIR / 'analysis_comparison.png', dpi=150)
    print(f"Saved: analysis_comparison.png")

def find_crossover(df):
    """Find crossover points between protocols."""
    print("\n=== CROSSOVER ANALYSIS ===")

    sizes = sorted(df['payload_bytes'].unique())

    print("\nEnergy comparison (mJ) at each payload size:")
    print("-" * 60)
    print(f"{'Size':<10} {'BLE_ADV':<12} {'BLE_CONN':<12} {'WiFi':<12} {'Best':<10}")
    print("-" * 60)

    for size in sizes:
        row = df[df['payload_bytes'] == size]
        energies = {}
        for mode in ['BLE_ADV', 'BLE_CONN', 'WiFi']:
            e = row[row['mode'] == mode]['energy_mJ']
            energies[mode] = e.values[0] if len(e) > 0 else float('inf')

        best = min(energies, key=energies.get)
        print(f"{size:<10} {energies.get('BLE_ADV', 0):<12.2f} {energies.get('BLE_CONN', 0):<12.2f} {energies.get('WiFi', 0):<12.2f} {best:<10}")

def print_summary(df):
    """Print summary statistics."""
    print("\n=== SUMMARY TABLE ===")
    print(df.to_string(index=False))

    print("\n=== KEY INSIGHTS ===")

    # Find most efficient protocol for small vs large payloads
    small = df[df['payload_bytes'] <= 10]
    large = df[df['payload_bytes'] >= 512]

    if len(small) > 0:
        best_small = small.loc[small['energy_per_byte_uJ'].idxmin()]
        print(f"\nBest for small payloads (≤10 bytes): {best_small['mode']}")
        print(f"  Energy per byte: {best_small['energy_per_byte_uJ']:.2f} µJ/byte")

    if len(large) > 0:
        best_large = large.loc[large['energy_per_byte_uJ'].idxmin()]
        print(f"\nBest for large payloads (≥512 bytes): {best_large['mode']}")
        print(f"  Energy per byte: {best_large['energy_per_byte_uJ']:.2f} µJ/byte")

def main():
    print("Loading data from:", DATA_DIR)
    df = load_all_data()

    if len(df) == 0:
        print("No data found!")
        return

    print(f"Loaded {len(df)} test results")

    print_summary(df)
    find_crossover(df)
    plot_comparison(df)

    # Save results to CSV
    output_file = RESULTS_DIR / 'analysis_results.csv'
    df.to_csv(output_file, index=False)
    print(f"\nSaved: {output_file}")

    # plt.show()  # Uncomment to display interactively

if __name__ == "__main__":
    main()
