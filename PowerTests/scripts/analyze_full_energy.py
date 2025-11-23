#!/usr/bin/env python3
"""Analyze full energy cost for XiFi algorithm comparison."""

import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

BASE_DIR = Path(__file__).parent.parent
DATA_DIR = BASE_DIR / "Data"
RESULTS_DIR = BASE_DIR / "results"
IMAGES_DIR = BASE_DIR / "images"

def analyze_full_energy():
    """Analyze complete energy breakdown for each test."""
    results = []

    for size_dir in DATA_DIR.iterdir():
        if not size_dir.is_dir():
            continue

        size_str = size_dir.name.split()[0]
        try:
            payload_size = int(size_str)
        except:
            continue

        for csv_file in size_dir.glob("*.csv"):
            mode = csv_file.stem.replace(" ", "")

            try:
                df = pd.read_csv(csv_file)

                # Time step (ms between samples)
                dt = df['timestamp_ms'].diff().median()

                # Split into phases based on marker
                transfer = df[df['marker'] == 1]
                non_transfer = df[df['marker'] == 0]

                # Total test metrics
                total_duration_ms = df['timestamp_ms'].max() - df['timestamp_ms'].min()
                total_samples = len(df)

                # Energy calculation: sum of (power × dt) for each sample
                total_energy_mJ = (df['power_mW'] * dt).sum() / 1000

                # Transfer phase
                transfer_duration_ms = len(transfer) * dt if len(transfer) > 0 else 0
                transfer_energy_mJ = (transfer['power_mW'] * dt).sum() / 1000 if len(transfer) > 0 else 0
                transfer_avg_power = transfer['power_mW'].mean() if len(transfer) > 0 else 0

                # Non-transfer phase (init + teardown + idle)
                overhead_duration_ms = len(non_transfer) * dt if len(non_transfer) > 0 else 0
                overhead_energy_mJ = (non_transfer['power_mW'] * dt).sum() / 1000 if len(non_transfer) > 0 else 0
                overhead_avg_power = non_transfer['power_mW'].mean() if len(non_transfer) > 0 else 0

                # Throughput
                throughput_bps = (payload_size * 8 * 100) / (transfer_duration_ms / 1000) if transfer_duration_ms > 0 else 0  # 100 iterations

                results.append({
                    'mode': mode,
                    'payload_bytes': payload_size,
                    'total_duration_s': total_duration_ms / 1000,
                    'transfer_duration_s': transfer_duration_ms / 1000,
                    'overhead_duration_s': overhead_duration_ms / 1000,
                    'total_energy_mJ': total_energy_mJ,
                    'transfer_energy_mJ': transfer_energy_mJ,
                    'overhead_energy_mJ': overhead_energy_mJ,
                    'transfer_avg_power_mW': transfer_avg_power,
                    'overhead_avg_power_mW': overhead_avg_power,
                    'throughput_kbps': throughput_bps / 1000,
                    'energy_per_byte_uJ': (total_energy_mJ * 1000) / (payload_size * 100) if payload_size > 0 else 0  # 100 iterations
                })

            except Exception as e:
                print(f"Error: {csv_file}: {e}")

    return pd.DataFrame(results)

def print_analysis(df):
    """Print detailed analysis."""
    print("\n" + "="*80)
    print("FULL ENERGY ANALYSIS - XiFi Power Measurements")
    print("="*80)

    print("\n### SUMMARY TABLE ###\n")
    summary = df[['mode', 'payload_bytes', 'total_duration_s', 'total_energy_mJ',
                  'transfer_energy_mJ', 'overhead_energy_mJ', 'throughput_kbps']].copy()
    summary = summary.sort_values(['payload_bytes', 'mode'])
    print(summary.to_string(index=False))

    print("\n### ENERGY COMPARISON BY PAYLOAD SIZE ###\n")
    print(f"{'Size':<8} {'BLE_ADV':<20} {'BLE_CONN':<20} {'WiFi':<20} {'Best':<10}")
    print(f"{'(bytes)':<8} {'Energy(mJ)/Time(s)':<20} {'Energy(mJ)/Time(s)':<20} {'Energy(mJ)/Time(s)':<20}")
    print("-"*80)

    for size in sorted(df['payload_bytes'].unique()):
        row_data = {}
        for mode in ['BLE_ADV', 'BLE_CONN', 'WiFi']:
            r = df[(df['payload_bytes'] == size) & (df['mode'] == mode)]
            if len(r) > 0:
                row_data[mode] = (r['total_energy_mJ'].values[0], r['total_duration_s'].values[0])
            else:
                row_data[mode] = (float('inf'), float('inf'))

        best = min(row_data, key=lambda x: row_data[x][0])
        print(f"{size:<8} {row_data['BLE_ADV'][0]:<8.1f}/{row_data['BLE_ADV'][1]:<8.1f}   "
              f"{row_data['BLE_CONN'][0]:<8.1f}/{row_data['BLE_CONN'][1]:<8.1f}   "
              f"{row_data['WiFi'][0]:<8.1f}/{row_data['WiFi'][1]:<8.1f}   {best}")

    print("\n### ENERGY BREAKDOWN (Transfer vs Overhead) ###\n")
    print(f"{'Mode':<10} {'Size':<8} {'Total(mJ)':<12} {'Transfer(mJ)':<14} {'Overhead(mJ)':<14} {'Overhead%':<10}")
    print("-"*70)
    for _, r in df.sort_values(['mode', 'payload_bytes']).iterrows():
        overhead_pct = (r['overhead_energy_mJ'] / r['total_energy_mJ'] * 100) if r['total_energy_mJ'] > 0 else 0
        print(f"{r['mode']:<10} {r['payload_bytes']:<8} {r['total_energy_mJ']:<12.1f} "
              f"{r['transfer_energy_mJ']:<14.1f} {r['overhead_energy_mJ']:<14.1f} {overhead_pct:<10.1f}%")

def analyze_hybrid_scenario(df):
    """Analyze hybrid WiFi + BLE_ADV scenario."""
    print("\n" + "="*80)
    print("HYBRID SCENARIO ANALYSIS")
    print("="*80)
    print("""
Scenario: Video streaming with adaptive bursts
- Sender sleeps between bursts
- Receiver advertises via BLE when buffer low
- Sender wakes, scans BLE, sends WiFi burst, sleeps
    """)

    # For hybrid, we need:
    # 1. WiFi burst cost (full init + transfer + deinit)
    # 2. BLE scan cost (approximated from BLE_ADV overhead - similar radio usage)

    print("\n### COST MODEL ###\n")
    print("WiFi Burst Cost = WiFi total_energy_mJ (includes init + transfer + deinit)")
    print("BLE Scan Cost ≈ BLE_ADV overhead power × scan_duration")
    print("Sleep Cost ≈ ~0.1 mW (deep sleep, negligible)")

    print("\n### COMPARISON: Sending 10KB of video data ###\n")

    # Calculate for 10KB (10240 bytes) scenario
    target_bytes = 10240

    for size in sorted(df['payload_bytes'].unique()):
        bursts_needed = target_bytes / (size * 100)  # 100 iterations per test

        wifi = df[(df['payload_bytes'] == size) & (df['mode'] == 'WiFi')]
        ble_conn = df[(df['payload_bytes'] == size) & (df['mode'] == 'BLE_CONN')]
        ble_adv = df[(df['payload_bytes'] == size) & (df['mode'] == 'BLE_ADV')]

        if len(wifi) == 0 or len(ble_conn) == 0:
            continue

        wifi_energy = wifi['total_energy_mJ'].values[0]
        wifi_time = wifi['total_duration_s'].values[0]

        ble_conn_energy = ble_conn['total_energy_mJ'].values[0]
        ble_conn_time = ble_conn['total_duration_s'].values[0]

        # For hybrid: WiFi burst + minimal BLE scan (assume 100ms scan at ~60mW)
        ble_scan_energy = 0.1 * 60  # 100ms at 60mW = 6mJ per scan
        hybrid_energy = wifi_energy + ble_scan_energy
        hybrid_time = wifi_time + 0.1  # 100ms scan overhead

        print(f"Payload: {size} bytes")
        print(f"  WiFi only:     {wifi_energy:.1f} mJ, {wifi_time:.2f} s")
        print(f"  BLE_CONN only: {ble_conn_energy:.1f} mJ, {ble_conn_time:.2f} s")
        print(f"  Hybrid (WiFi+BLE scan): {hybrid_energy:.1f} mJ, {hybrid_time:.2f} s")
        print(f"  → Best: {'Hybrid' if hybrid_energy < min(wifi_energy, ble_conn_energy) else 'WiFi' if wifi_energy < ble_conn_energy else 'BLE_CONN'}")
        print()

def plot_full_analysis(df):
    """Create visualization plots."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))

    modes = ['BLE_ADV', 'BLE_CONN', 'WiFi']
    colors = {'BLE_ADV': 'blue', 'BLE_CONN': 'green', 'WiFi': 'red'}

    df = df.sort_values('payload_bytes')

    # 1. Total Energy vs Payload
    ax = axes[0, 0]
    for mode in modes:
        data = df[df['mode'] == mode]
        ax.plot(data['payload_bytes'], data['total_energy_mJ'], 'o-',
                label=mode, color=colors[mode], linewidth=2, markersize=8)
    ax.set_xlabel('Payload Size (bytes)')
    ax.set_ylabel('Total Energy (mJ)')
    ax.set_title('Total Energy (Init + Transfer + Teardown)')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')
    ax.set_yscale('log')

    # 2. Total Time vs Payload
    ax = axes[0, 1]
    for mode in modes:
        data = df[df['mode'] == mode]
        ax.plot(data['payload_bytes'], data['total_duration_s'], 'o-',
                label=mode, color=colors[mode], linewidth=2, markersize=8)
    ax.set_xlabel('Payload Size (bytes)')
    ax.set_ylabel('Total Time (s)')
    ax.set_title('Total Duration (Init + Transfer + Teardown)')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')
    ax.set_yscale('log')

    # 3. Energy Breakdown (stacked bar)
    ax = axes[1, 0]
    sizes = sorted(df['payload_bytes'].unique())
    x = np.arange(len(sizes))
    width = 0.25

    for i, mode in enumerate(modes):
        data = df[df['mode'] == mode].sort_values('payload_bytes')
        transfer = data['transfer_energy_mJ'].values
        overhead = data['overhead_energy_mJ'].values
        ax.bar(x + i*width, transfer, width, label=f'{mode} Transfer', color=colors[mode], alpha=0.7)
        ax.bar(x + i*width, overhead, width, bottom=transfer, label=f'{mode} Overhead', color=colors[mode], alpha=0.3)

    ax.set_xlabel('Payload Size (bytes)')
    ax.set_ylabel('Energy (mJ)')
    ax.set_title('Energy Breakdown: Transfer vs Overhead')
    ax.set_xticks(x + width)
    ax.set_xticklabels(sizes)
    ax.legend(loc='upper left', fontsize=8)
    ax.grid(True, alpha=0.3)

    # 4. Throughput comparison
    ax = axes[1, 1]
    for mode in modes:
        data = df[df['mode'] == mode]
        ax.plot(data['payload_bytes'], data['throughput_kbps'], 'o-',
                label=mode, color=colors[mode], linewidth=2, markersize=8)
    ax.set_xlabel('Payload Size (bytes)')
    ax.set_ylabel('Throughput (kbps)')
    ax.set_title('Effective Throughput')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')
    ax.set_yscale('log')

    plt.tight_layout()
    plt.savefig(IMAGES_DIR / 'full_energy_analysis.png', dpi=150)
    print(f"\nSaved: full_energy_analysis.png")

def main():
    print("Loading data from:", DATA_DIR)
    df = analyze_full_energy()

    if len(df) == 0:
        print("No data found!")
        return

    print(f"Loaded {len(df)} test results")

    print_analysis(df)
    analyze_hybrid_scenario(df)
    plot_full_analysis(df)

    # Save results
    output_file = RESULTS_DIR / 'full_energy_results.csv'
    df.to_csv(output_file, index=False)
    print(f"\nSaved: {output_file}")

if __name__ == "__main__":
    main()
