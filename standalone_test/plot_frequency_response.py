#!/usr/bin/env python3
"""
Frequency Response Plotter for Filter Engines 8-14

This script reads CSV data from frequency response tests and generates
professional plots showing:
- Individual frequency response curves per engine
- Combined overlay plot of all filters
- Detailed analysis annotations
- Export to PNG and PDF formats
"""

import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np
import pandas as pd
import os
from pathlib import Path

# Professional plot styling
plt.style.use('seaborn-v0_8-darkgrid')
plt.rcParams['figure.figsize'] = (14, 8)
plt.rcParams['font.size'] = 10
plt.rcParams['axes.labelsize'] = 11
plt.rcParams['axes.titlesize'] = 12
plt.rcParams['legend.fontsize'] = 9
plt.rcParams['xtick.labelsize'] = 9
plt.rcParams['ytick.labelsize'] = 9

# Engine names and colors
ENGINES = {
    8: {'name': 'VintageConsoleEQ_Studio', 'color': '#FF6B6B', 'linestyle': '-'},
    9: {'name': 'LadderFilter', 'color': '#4ECDC4', 'linestyle': '-'},
    10: {'name': 'StateVariableFilter', 'color': '#45B7D1', 'linestyle': '-'},
    11: {'name': 'FormantFilter', 'color': '#96CEB4', 'linestyle': '-'},
    12: {'name': 'EnvelopeFilter', 'color': '#FFEAA7', 'linestyle': '-'},
    13: {'name': 'CombResonator', 'color': '#DFE6E9', 'linestyle': '-'},
    14: {'name': 'VocalFormantFilter', 'color': '#A29BFE', 'linestyle': '-'}
}

def load_frequency_response(engine_id):
    """Load frequency response data from CSV file."""
    filename = f'frequency_response_engine_{engine_id}.csv'

    if not os.path.exists(filename):
        print(f"Warning: File not found: {filename}")
        return None

    try:
        df = pd.read_csv(filename)
        print(f"Loaded {len(df)} data points for Engine {engine_id}")
        return df
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def plot_single_response(engine_id, df, save_path='plots'):
    """Plot frequency response for a single engine."""

    Path(save_path).mkdir(exist_ok=True)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

    engine_info = ENGINES[engine_id]

    # Plot 1: Frequency Response (Gain vs Frequency)
    ax1.semilogx(df['Frequency_Hz'], df['Gain_dB'],
                 color=engine_info['color'],
                 linewidth=2,
                 label=engine_info['name'],
                 marker='o',
                 markersize=3,
                 alpha=0.8)

    ax1.set_xlabel('Frequency (Hz)')
    ax1.set_ylabel('Gain (dB)')
    ax1.set_title(f"Frequency Response: Engine {engine_id} - {engine_info['name']}")
    ax1.grid(True, which='both', alpha=0.3)
    ax1.legend(loc='best')
    ax1.axhline(y=0, color='gray', linestyle='--', alpha=0.5, linewidth=1)
    ax1.axhline(y=-3, color='red', linestyle='--', alpha=0.3, linewidth=1, label='-3dB')
    ax1.set_xlim(20, 20000)

    # Add vertical lines for key frequencies
    key_freqs = [100, 1000, 10000]
    for freq in key_freqs:
        ax1.axvline(x=freq, color='gray', linestyle=':', alpha=0.2)

    # Plot 2: Output Level (Linear)
    ax2.semilogx(df['Frequency_Hz'], df['Output_Level'],
                 color=engine_info['color'],
                 linewidth=2,
                 marker='o',
                 markersize=3,
                 alpha=0.8)

    ax2.set_xlabel('Frequency (Hz)')
    ax2.set_ylabel('Output Level (Linear)')
    ax2.set_title(f"Output Amplitude vs Frequency")
    ax2.grid(True, which='both', alpha=0.3)
    ax2.set_xlim(20, 20000)

    # Add analysis annotations
    max_gain_idx = df['Gain_dB'].idxmax()
    min_gain_idx = df['Gain_dB'].idxmin()

    max_gain = df.loc[max_gain_idx, 'Gain_dB']
    max_freq = df.loc[max_gain_idx, 'Frequency_Hz']
    min_gain = df.loc[min_gain_idx, 'Gain_dB']
    min_freq = df.loc[min_gain_idx, 'Frequency_Hz']

    # Annotate peaks
    ax1.annotate(f'Peak: {max_gain:.2f}dB @ {max_freq:.1f}Hz',
                xy=(max_freq, max_gain),
                xytext=(max_freq*2, max_gain+5),
                arrowprops=dict(arrowstyle='->', color='green', lw=1.5),
                fontsize=9,
                color='green',
                bbox=dict(boxstyle='round,pad=0.5', facecolor='white', alpha=0.8))

    ax1.annotate(f'Min: {min_gain:.2f}dB @ {min_freq:.1f}Hz',
                xy=(min_freq, min_gain),
                xytext=(min_freq*2, min_gain-5),
                arrowprops=dict(arrowstyle='->', color='red', lw=1.5),
                fontsize=9,
                color='red',
                bbox=dict(boxstyle='round,pad=0.5', facecolor='white', alpha=0.8))

    plt.tight_layout()

    # Save plot
    output_file = f'{save_path}/frequency_response_engine_{engine_id}.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Saved: {output_file}")

    plt.close()

def plot_combined_response(all_data, save_path='plots'):
    """Plot all frequency responses on one graph for comparison."""

    Path(save_path).mkdir(exist_ok=True)

    fig = plt.figure(figsize=(16, 10))
    gs = gridspec.GridSpec(2, 1, height_ratios=[2, 1])

    ax1 = plt.subplot(gs[0])
    ax2 = plt.subplot(gs[1])

    # Plot all responses
    for engine_id, df in all_data.items():
        if df is not None:
            engine_info = ENGINES[engine_id]
            ax1.semilogx(df['Frequency_Hz'], df['Gain_dB'],
                        color=engine_info['color'],
                        linewidth=2.5,
                        label=f"Eng {engine_id}: {engine_info['name']}",
                        linestyle=engine_info['linestyle'],
                        alpha=0.8)

    ax1.set_xlabel('Frequency (Hz)', fontsize=12)
    ax1.set_ylabel('Gain (dB)', fontsize=12)
    ax1.set_title('Frequency Response Comparison: All Filter Engines (8-14)', fontsize=14, fontweight='bold')
    ax1.grid(True, which='both', alpha=0.3)
    ax1.legend(loc='upper right', ncol=2, framealpha=0.9)
    ax1.axhline(y=0, color='black', linestyle='-', alpha=0.5, linewidth=1)
    ax1.axhline(y=-3, color='red', linestyle='--', alpha=0.3, linewidth=1, label='-3dB cutoff')
    ax1.axhline(y=-6, color='orange', linestyle='--', alpha=0.3, linewidth=1)
    ax1.set_xlim(20, 20000)
    ax1.set_ylim(-40, 20)

    # Add vertical lines for octave markers
    octaves = [31.25, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
    for freq in octaves:
        ax1.axvline(x=freq, color='gray', linestyle=':', alpha=0.15)

    # Plot 2: Normalized comparison (all starting at 0dB at 1kHz)
    for engine_id, df in all_data.items():
        if df is not None:
            engine_info = ENGINES[engine_id]

            # Find gain at 1kHz (or closest frequency)
            idx_1k = (df['Frequency_Hz'] - 1000).abs().idxmin()
            gain_1k = df.loc[idx_1k, 'Gain_dB']

            # Normalize to 0dB at 1kHz
            normalized_gain = df['Gain_dB'] - gain_1k

            ax2.semilogx(df['Frequency_Hz'], normalized_gain,
                        color=engine_info['color'],
                        linewidth=2,
                        label=f"Eng {engine_id}",
                        linestyle=engine_info['linestyle'],
                        alpha=0.8)

    ax2.set_xlabel('Frequency (Hz)', fontsize=12)
    ax2.set_ylabel('Relative Gain (dB)', fontsize=12)
    ax2.set_title('Normalized Response (0dB @ 1kHz)', fontsize=12)
    ax2.grid(True, which='both', alpha=0.3)
    ax2.legend(loc='best', ncol=4, framealpha=0.9, fontsize=8)
    ax2.axhline(y=0, color='black', linestyle='-', alpha=0.5, linewidth=1)
    ax2.set_xlim(20, 20000)

    plt.tight_layout()

    # Save combined plot
    output_file = f'{save_path}/frequency_response_combined.png'
    plt.savefig(output_file, dpi=200, bbox_inches='tight')
    print(f"Saved: {output_file}")

    plt.close()

def plot_filter_comparison_grid(all_data, save_path='plots'):
    """Create a grid of subplots for easy comparison."""

    Path(save_path).mkdir(exist_ok=True)

    fig, axes = plt.subplots(2, 4, figsize=(18, 10))
    axes = axes.flatten()

    for idx, (engine_id, df) in enumerate(all_data.items()):
        if df is not None and idx < len(axes):
            ax = axes[idx]
            engine_info = ENGINES[engine_id]

            ax.semilogx(df['Frequency_Hz'], df['Gain_dB'],
                       color=engine_info['color'],
                       linewidth=2,
                       alpha=0.8)

            ax.set_title(f"Eng {engine_id}: {engine_info['name']}", fontsize=10)
            ax.grid(True, which='both', alpha=0.3)
            ax.axhline(y=0, color='gray', linestyle='--', alpha=0.5)
            ax.axhline(y=-3, color='red', linestyle='--', alpha=0.3)
            ax.set_xlim(20, 20000)

            if idx >= 4:  # Bottom row
                ax.set_xlabel('Frequency (Hz)', fontsize=9)
            if idx % 4 == 0:  # Left column
                ax.set_ylabel('Gain (dB)', fontsize=9)

    # Hide unused subplot
    if len(all_data) < len(axes):
        axes[-1].axis('off')

    plt.suptitle('Frequency Response Grid: Engines 8-14', fontsize=14, fontweight='bold', y=0.995)
    plt.tight_layout()

    output_file = f'{save_path}/frequency_response_grid.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Saved: {output_file}")

    plt.close()

def analyze_filter_characteristics(all_data):
    """Analyze and print filter characteristics."""

    print("\n" + "="*80)
    print("FILTER CHARACTERISTICS ANALYSIS")
    print("="*80 + "\n")

    for engine_id, df in all_data.items():
        if df is None:
            continue

        engine_info = ENGINES[engine_id]
        print(f"\nEngine {engine_id}: {engine_info['name']}")
        print("-" * 60)

        # Calculate statistics
        max_gain = df['Gain_dB'].max()
        min_gain = df['Gain_dB'].min()
        gain_range = max_gain - min_gain

        max_idx = df['Gain_dB'].idxmax()
        min_idx = df['Gain_dB'].idxmin()

        max_freq = df.loc[max_idx, 'Frequency_Hz']
        min_freq = df.loc[min_idx, 'Frequency_Hz']

        print(f"  Max Gain: {max_gain:.2f} dB @ {max_freq:.1f} Hz")
        print(f"  Min Gain: {min_gain:.2f} dB @ {min_freq:.1f} Hz")
        print(f"  Gain Range: {gain_range:.2f} dB")

        # Find -3dB cutoff
        cutoff_3db = None
        for idx, row in df.iterrows():
            if row['Gain_dB'] < (max_gain - 3.0):
                cutoff_3db = row['Frequency_Hz']
                break

        if cutoff_3db:
            print(f"  -3dB Cutoff: {cutoff_3db:.1f} Hz")

        # Determine filter type
        if gain_range > 20:
            filter_type = "Strong filtering (>20dB range)"
        elif gain_range > 10:
            filter_type = "Moderate filtering (10-20dB range)"
        elif gain_range > 6:
            filter_type = "Gentle filtering (6-10dB range)"
        else:
            filter_type = "Minimal filtering (<6dB range)"

        print(f"  Filter Type: {filter_type}")

        # Check for resonance peaks
        mean_gain = df['Gain_dB'].mean()
        if max_gain > (mean_gain + 6):
            print(f"  Resonance: Detected (peak {max_gain - mean_gain:.1f}dB above average)")

def main():
    """Main execution function."""

    print("\n" + "="*80)
    print("FREQUENCY RESPONSE PLOTTER")
    print("Filter & EQ Engines 8-14")
    print("="*80 + "\n")

    # Load all data
    all_data = {}
    for engine_id in ENGINES.keys():
        df = load_frequency_response(engine_id)
        if df is not None:
            all_data[engine_id] = df

    if not all_data:
        print("Error: No frequency response data found!")
        print("Please run the test_frequency_response_8_14 executable first.")
        return

    print(f"\nLoaded data for {len(all_data)} engines\n")

    # Create plots directory
    plot_dir = 'frequency_response_plots'
    Path(plot_dir).mkdir(exist_ok=True)

    # Generate individual plots
    print("\nGenerating individual plots...")
    for engine_id, df in all_data.items():
        plot_single_response(engine_id, df, plot_dir)

    # Generate combined plot
    print("\nGenerating combined comparison plot...")
    plot_combined_response(all_data, plot_dir)

    # Generate grid plot
    print("\nGenerating comparison grid...")
    plot_filter_comparison_grid(all_data, plot_dir)

    # Analyze characteristics
    analyze_filter_characteristics(all_data)

    print("\n" + "="*80)
    print(f"COMPLETE! All plots saved to: {plot_dir}/")
    print("="*80 + "\n")

if __name__ == '__main__':
    main()
