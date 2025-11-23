#!/usr/bin/env python3
"""Plot power measurement data from XiFi Logger CSV files."""

import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys
import os
from pathlib import Path

BASE_DIR = Path(__file__).parent.parent
IMAGES_DIR = BASE_DIR / "images"

def plot_single_test(csv_path):
    """Plot a single test CSV file."""
    df = pd.read_csv(csv_path)

    fig, axes = plt.subplots(3, 1, figsize=(12, 8), sharex=True)
    fig.suptitle(os.path.basename(csv_path), fontsize=14)

    time_s = df['timestamp_ms'] / 1000

    # Current plot
    axes[0].plot(time_s, df['current_mA'], 'b-', linewidth=0.5)
    axes[0].fill_between(time_s, 0, df['current_mA'],
                         where=df['marker']==1, alpha=0.3, color='green', label='Transfer')
    axes[0].set_ylabel('Current (mA)')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    # Power plot
    axes[1].plot(time_s, df['power_mW'], 'r-', linewidth=0.5)
    axes[1].fill_between(time_s, 0, df['power_mW'],
                         where=df['marker']==1, alpha=0.3, color='green')
    axes[1].set_ylabel('Power (mW)')
    axes[1].grid(True, alpha=0.3)

    # Voltage plot
    axes[2].plot(time_s, df['voltage_V'], 'g-', linewidth=0.5)
    axes[2].set_ylabel('Voltage (V)')
    axes[2].set_xlabel('Time (s)')
    axes[2].grid(True, alpha=0.3)

    plt.tight_layout()

    # Calculate stats
    idle = df[df['marker'] == 0]
    transfer = df[df['marker'] == 1]

    print(f"\n=== {os.path.basename(csv_path)} ===")
    print(f"Total samples: {len(df)}")
    print(f"Duration: {df['timestamp_ms'].max()/1000:.2f}s")
    print(f"\nIdle (marker=0): {len(idle)} samples")
    print(f"  Current: {idle['current_mA'].mean():.2f} ± {idle['current_mA'].std():.2f} mA")
    print(f"  Power:   {idle['power_mW'].mean():.2f} ± {idle['power_mW'].std():.2f} mW")
    print(f"\nTransfer (marker=1): {len(transfer)} samples")
    print(f"  Current: {transfer['current_mA'].mean():.2f} ± {transfer['current_mA'].std():.2f} mA")
    print(f"  Power:   {transfer['power_mW'].mean():.2f} ± {transfer['power_mW'].std():.2f} mW")
    print(f"\nDelta (Transfer - Idle):")
    print(f"  Current: +{transfer['current_mA'].mean() - idle['current_mA'].mean():.2f} mA")
    print(f"  Power:   +{transfer['power_mW'].mean() - idle['power_mW'].mean():.2f} mW")

    return df

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_power.py <csv_file>")
        print("       python plot_power.py <csv_file1> <csv_file2> ...")
        sys.exit(1)

    for csv_path in sys.argv[1:]:
        if os.path.exists(csv_path):
            plot_single_test(csv_path)
            # Save plot to images directory
            basename = os.path.splitext(os.path.basename(csv_path))[0]
            output_path = IMAGES_DIR / f"{basename}_plot.png"
            plt.savefig(output_path, dpi=150)
            print(f"\nSaved: {output_path}")
        else:
            print(f"File not found: {csv_path}")
