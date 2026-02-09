#!/usr/bin/env python3
"""
BLE_ADV_2x vs WiFi_BRD Comparison
Compares power consumption and loss rate for:
- BLE_ADV_2x: 20 bytes every 40ms (2x repetition, 20ms intervals)
- WiFi_BRD: 20 bytes every 40ms (single transmission)
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

# Loss rates (measured)
LOSS_RATE_BLE = 1.0  # Average of 0.8-1.2%
LOSS_RATE_WIFI = 4.0  # Average of 3-5%

def load_and_process(filepath):
    """Load CSV and cap at 15 seconds"""
    df = pd.read_csv(filepath)
    df = df[df['timestamp_ms'] <= MAX_TIME_MS].copy()

    # Calculate metrics
    avg_power = df['power_mW'].mean()
    avg_current = df['current_mA'].mean()
    total_energy = (df['power_mW'] * np.diff(df['timestamp_ms'], prepend=0)).sum() / 1000.0

    return {
        'avg_power_mW': avg_power,
        'avg_current_mA': avg_current,
        'total_energy_mJ': total_energy
    }

def create_comparison_plot():
    """Create a single comparison plot with power and loss rate"""

    # Load data
    ble_file = os.path.join(RESULTS_DIR, "BLE_ADV_2X", "20ms.csv")
    wifi_file = os.path.join(RESULTS_DIR, "WiFi_BRD", "40ms.csv")

    ble_metrics = load_and_process(ble_file)
    wifi_metrics = load_and_process(wifi_file)

    print(f"BLE_ADV_2x (40ms effective):")
    print(f"  Avg Power: {ble_metrics['avg_power_mW']:.2f} mW")
    print(f"  Total Energy: {ble_metrics['total_energy_mJ']:.2f} mJ")
    print(f"  Loss Rate: {LOSS_RATE_BLE:.1f}%")
    print()
    print(f"WiFi_BRD (40ms):")
    print(f"  Avg Power: {wifi_metrics['avg_power_mW']:.2f} mW")
    print(f"  Total Energy: {wifi_metrics['total_energy_mJ']:.2f} mJ")
    print(f"  Loss Rate: {LOSS_RATE_WIFI:.1f}%")
    print()

    power_savings = (1 - ble_metrics['avg_power_mW'] / wifi_metrics['avg_power_mW']) * 100
    print(f"Power Savings: {power_savings:.1f}%")
    print(f"Loss Improvement: {LOSS_RATE_WIFI - LOSS_RATE_BLE:.1f}% reduction")

    # Create figure with dual y-axis
    fig, ax1 = plt.subplots(figsize=(12, 7))

    x = np.arange(2)
    width = 0.4

    # Plot power consumption on primary axis
    power_values = [ble_metrics['avg_power_mW'], wifi_metrics['avg_power_mW']]
    bars1 = ax1.bar(x, power_values, width,
                    color=['#2E86AB', '#A23B72'], alpha=0.8,
                    label='Average Power')

    ax1.set_ylabel('Average Power (mW)', fontweight='bold', fontsize=13)
    ax1.set_xlabel('Transmission Method', fontweight='bold', fontsize=13)
    ax1.set_title('BLE 2x Repetition vs WiFi Broadcast: Power & Reliability\n' +
                  '(Both sending 20 bytes every 40ms, 15s measurement)',
                  fontsize=15, fontweight='bold', pad=20)
    ax1.set_xticks(x)
    ax1.set_xticklabels(['BLE ADV 2x\n(2x repetition @20ms)', 'WiFi UDP Broadcast\n(single @40ms)'],
                        fontsize=11)
    ax1.tick_params(axis='y', labelsize=11)
    ax1.grid(True, alpha=0.3, axis='y')

    # Add power value labels on bars
    for i, (bar, val) in enumerate(zip(bars1, power_values)):
        ax1.text(bar.get_x() + bar.get_width()/2, val + 5,
                f'{val:.1f} mW',
                ha='center', va='bottom', fontsize=11, fontweight='bold')

    # Create secondary y-axis for loss rate
    ax2 = ax1.twinx()
    loss_values = [LOSS_RATE_BLE, LOSS_RATE_WIFI]

    # Plot loss rate as line with markers
    line = ax2.plot(x, loss_values, 'o-', color='#F18F01',
                    linewidth=3, markersize=12, label='Packet Loss Rate')

    ax2.set_ylabel('Packet Loss Rate (%)', fontweight='bold', fontsize=13, color='#F18F01')
    ax2.tick_params(axis='y', labelcolor='#F18F01', labelsize=11)
    ax2.set_ylim(0, max(loss_values) * 1.5)

    # Add loss rate labels
    for i, (xval, loss) in enumerate(zip(x, loss_values)):
        ax2.text(xval, loss + 0.3, f'{loss:.1f}%',
                ha='center', va='bottom', fontsize=11, fontweight='bold',
                color='#F18F01')

    # Add annotations highlighting advantages
    ax1.annotate('', xy=(0, ble_metrics['avg_power_mW']),
                xytext=(1, ble_metrics['avg_power_mW']),
                arrowprops=dict(arrowstyle='<->', color='green', lw=2))
    ax1.text(0.5, ble_metrics['avg_power_mW'] - 15,
            f'{power_savings:.1f}% Power Savings',
            ha='center', fontsize=10, fontweight='bold',
            bbox=dict(boxstyle='round,pad=0.5', facecolor='lightgreen', alpha=0.7))

    # Add legend
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2,
              loc='upper left', fontsize=11, framealpha=0.9)

    # Add text box with key findings
    # textstr = '\n'.join([
    #     'Key Findings:',
    #     f'• BLE uses {power_savings:.1f}% less power',
    #     f'• BLE has {LOSS_RATE_WIFI - LOSS_RATE_BLE:.1f}% lower loss rate',
    #     '• BLE sends each packet 2x (redundancy)',
    #     '• WiFi sends each packet once',
    #     '',
    #     'Conclusion: BLE 2x repetition is superior',
    #     'in both power efficiency AND reliability!'
    # ])

    # props = dict(boxstyle='round', facecolor='wheat', alpha=0.8)
    # ax1.text(0.98, 0.97, textstr, transform=ax1.transAxes, fontsize=10,
    #         verticalalignment='top', horizontalalignment='right',
    #         bbox=props, family='monospace')

    plt.tight_layout()

    output_file = os.path.join(GRAPHS_DIR, 'ble2x_vs_wifi_comparison.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: {output_file}")
    plt.close()

def main():
    print("BLE_ADV_2x vs WiFi_BRD Comparison")
    print("="*50)
    print("Comparing 20 bytes every 40ms effective rate")
    print(f"Data capped at {MAX_TIME_MS/1000:.0f} seconds\n")

    os.makedirs(GRAPHS_DIR, exist_ok=True)
    create_comparison_plot()

    print("\nAnalysis complete!")

if __name__ == "__main__":
    main()
