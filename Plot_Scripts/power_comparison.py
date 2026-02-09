#!/usr/bin/env python3
"""
Power Consumption Comparison: WiFi-Only vs Hybrid BLE Solution
Shows average power consumption across different delay tolerance settings
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# Configuration
DATA_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results/Main Application"
OUTPUT_DIR = "/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Results/Graphs"

baseline_file = os.path.join(DATA_DIR, "Baseline.csv")
improved_file = os.path.join(DATA_DIR, "Improved.csv")

# Create output directory if it doesn't exist
os.makedirs(OUTPUT_DIR, exist_ok=True)

def load_data():
    """Load both CSV files"""
    print("Loading data...")
    baseline_df = pd.read_csv(baseline_file)
    improved_df = pd.read_csv(improved_file)
    return baseline_df, improved_df

def calculate_phase_power(df, start_ms, end_ms):
    """Calculate average power for a specific time window"""
    phase_data = df[(df['timestamp_ms'] >= start_ms) & (df['timestamp_ms'] < end_ms)]
    return phase_data['power_mW'].mean()

def main():
    print("Starting power comparison analysis...\n")

    # Load data
    baseline_df, improved_df = load_data()

    # Phase timing (each phase is 30 seconds = 30000 ms)
    phases = {
        'Low\n(2000ms)': (60000, 90000),     # Phase 4
        'Medium\n(5000ms)': (0, 30000),      # Phase 2
        'High\n(12000ms)': (30000, 60000),   # Phase 3
    }

    # Calculate average power for each phase
    wifi_power = []
    hybrid_power = []
    phase_labels = []

    for label, (start_ms, end_ms) in phases.items():
        wifi_avg = calculate_phase_power(baseline_df, start_ms, end_ms)
        hybrid_avg = calculate_phase_power(improved_df, start_ms, end_ms)

        wifi_power.append(wifi_avg)
        hybrid_power.append(hybrid_avg)
        phase_labels.append(label)

        savings = ((wifi_avg - hybrid_avg) / wifi_avg) * 100
        print(f"{label.replace(chr(10), ' ')}: WiFi={wifi_avg:.1f}mW, Hybrid={hybrid_avg:.1f}mW, Savings={savings:.1f}%")

    # Create the comparison graph
    fig, ax = plt.subplots(figsize=(14, 7))

    x = np.arange(len(phase_labels))
    width = 0.35

    # Create bars - consistent color scheme with other graphs
    bars_hybrid = ax.bar(x - width/2, hybrid_power, width, label='Hybrid BLE',
                         color='#2E86AB', alpha=0.8, edgecolor='black', linewidth=1.2)
    bars_wifi = ax.bar(x + width/2, wifi_power, width, label='WiFi Only',
                       color='#A23B72', alpha=0.8, edgecolor='black', linewidth=1.2)

    # Customize the plot
    ax.set_xlabel('Delay Tolerance', fontsize=14, fontweight='bold')
    ax.set_ylabel('Average Power Consumption (mW)', fontsize=14, fontweight='bold')
    ax.set_title('Power Consumption: WiFi-Only vs Hybrid BLE\nAcross Different Delay Tolerances',
                 fontsize=16, fontweight='bold', pad=20)
    ax.set_xticks(x)
    ax.set_xticklabels(phase_labels, fontsize=13, fontweight='bold')
    ax.legend(fontsize=12, loc='upper right')
    ax.grid(axis='y', alpha=0.3, linestyle='--')

    # Add power values on top of bars
    for bar in bars_hybrid:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height + 5,
                f'{height:.1f} mW',
                ha='center', va='bottom', fontsize=11, fontweight='bold')

    for bar in bars_wifi:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height + 5,
                f'{height:.1f} mW',
                ha='center', va='bottom', fontsize=11, fontweight='bold')

    # Add prominent savings box in the MIDDLE between the bars
    for i, (wifi, hybrid) in enumerate(zip(wifi_power, hybrid_power)):
        savings = ((wifi - hybrid) / wifi) * 100

        # Position in the middle vertically between the two bars
        y_middle = (wifi + hybrid) / 2

        # Create prominent green box with savings percentage (consistent with other graphs)
        ax.text(i, y_middle, f'{savings:.1f}% Power Savings',
                ha='center', va='center', fontsize=12, color='black', fontweight='bold',
                bbox=dict(boxstyle='round,pad=0.6', facecolor='lightgreen',
                         edgecolor='black', linewidth=2, alpha=0.8))

    plt.tight_layout()

    # Save the figure
    output_file = os.path.join(OUTPUT_DIR, "power_comparison.png")
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\n✓ Graph saved to: {output_file}")

    print("\nAnalysis complete!")

if __name__ == "__main__":
    main()
