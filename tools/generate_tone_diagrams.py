#!/usr/bin/env python3
# =============================================================================
# The LQ Digital Mode Family — Reference Implementation (lq_lib)
#
# Author:  Luis Quesada (HB9IPH)
# Web:     https://luisquesada.com
# Portal:  https://lquesada.github.io/lq_lib/
# GitHub:  https://github.com/lquesada/lq_lib
# App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)
#
# License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)
#
# Copyright (c) 2026 Luis Quesada (HB9IPH)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# =============================================================================

import json
import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches

def generate_lq8_diagram(data, sample_msg_key="msg2", grayscale=True):
    """
    Generates the standalone LQ8 physical channel frame spectrogram (horizontal orientation).
    Horizontal layout: X-axis is transmission time (0..12.64 s), Y-axis is frequency shift (0..43.75 Hz, T0..T7).
    Grayscale version is optimized for IEEE paper publication.
    Color version is optimized for web portal and online documentation.
    """
    fig, ax = plt.subplots(figsize=(7.2, 2.5))

    lq8_data = data["modes"]["LQ8"][sample_msg_key]
    tones = lq8_data["tones"]
    ts = lq8_data["symbol_period"]   # 0.160 s
    df = lq8_data["tone_spacing"]     # 6.25 Hz
    num_tones = lq8_data["num_tones"] # 8
    tx_dur = lq8_data["tx_duration"]  # 12.64 s
    bw = num_tones * df               # 50.0 Hz

    if grayscale:
        c_sync = "#1A1A1A"      # Solid dark charcoal / black for Costas
        c_data = "#FFFFFF"      # White for data
        edge_col = "#1A1A1A"
        title_col = "#000000"
        ann_sync_col = "#000000"
        ann_data_col = "#1F2937"
        border_col = "#9CA3AF"
        grid_col = "#D1D5DB"
    else:
        c_sync = "#E74C3C"      # Red
        c_data = "#2980B9"      # Blue
        edge_col = "#1A202C"
        title_col = "#0E3A5D"
        ann_sync_col = "#C0392B"
        ann_data_col = "#2471A3"
        border_col = "#CBD5E1"
        grid_col = "#E2E8F0"

    for i, t in enumerate(tones):
        t_start = i * ts
        f_center = t * df
        f_min = f_center - df * 0.44
        is_sync = (0 <= i < 7) or (36 <= i < 43) or (72 <= i < 79)
        face_col = c_sync if is_sync else c_data

        rect = patches.Rectangle(
            (t_start, f_min), ts, df * 0.88,
            facecolor=face_col, edgecolor=edge_col, linewidth=0.6, alpha=1.0, zorder=3
        )
        ax.add_patch(rect)

    ax.set_xlim(-0.1, 12.74)
    ax.set_ylim(-df * 0.6, 58.0)

    ax.set_xticks(np.arange(0, 14, 1))
    tone_freqs = [k * df for k in range(num_tones)]
    ax.set_yticks(tone_freqs)
    ax.set_yticklabels([f"T{k} ({tone_freqs[k]:.1f})" for k in range(num_tones)], fontsize=7.5)

    ax.grid(True, which='both', linestyle=':', alpha=0.6, color=grid_col, zorder=1)

    # Frame structure span annotations at top (vertically centered between divider line at 46.2 and top border at 58.0)
    ann_y = 52.1
    ax.text(0.56, ann_y, "Sync 1\n(7 sym)", fontsize=6.8, color=ann_sync_col, fontweight='bold', ha='center', va='center')
    ax.text(3.44, ann_y, "LDPC Data Block 1 (29 sym, 87 bits)", fontsize=6.8, color=ann_data_col, fontweight='bold', ha='center', va='center')
    ax.text(6.32, ann_y, "Sync 2\n(7 sym)", fontsize=6.8, color=ann_sync_col, fontweight='bold', ha='center', va='center')
    ax.text(9.20, ann_y, "LDPC Data Block 2 (29 sym, 87 bits)", fontsize=6.8, color=ann_data_col, fontweight='bold', ha='center', va='center')
    ax.text(12.08, ann_y, "Sync 3\n(7 sym)", fontsize=6.8, color=ann_sync_col, fontweight='bold', ha='center', va='center')

    # Top boundary line for spans
    ax.plot([0.0, 1.12], [46.2, 46.2], color=border_col, linewidth=1.0)
    ax.plot([1.12, 5.76], [46.2, 46.2], color=border_col, linewidth=1.0)
    ax.plot([5.76, 6.88], [46.2, 46.2], color=border_col, linewidth=1.0)
    ax.plot([6.88, 11.52], [46.2, 46.2], color=border_col, linewidth=1.0)
    ax.plot([11.52, 12.64], [46.2, 46.2], color=border_col, linewidth=1.0)

    ax.set_xlabel("Transmission Time (seconds elapsed) $\\rightarrow$", fontweight='bold', fontsize=8.5)
    ax.set_ylabel("Frequency Shift (Hz)", fontweight='bold', fontsize=8.5)

    legend_handles = [
        patches.Patch(facecolor=c_sync, edgecolor=edge_col, label='Costas Synchronization Arrays (3 × 7 symbols)'),
        patches.Patch(facecolor=c_data, edgecolor=edge_col, label='LDPC(174,91) Data Payloads (2 × 29 symbols)'),
    ]
    ax.legend(handles=legend_handles, loc='upper center', bbox_to_anchor=(0.5, -0.22), ncol=2, frameon=True, fontsize=7.8)
    fig.tight_layout()
    return fig


def generate_comparison_diagram(data, sample_msg_key="msg2", grayscale=True):
    """
    Generates the comprehensive multi-mode transport comparison figure.
    All 4 profiles are drawn to exact proportional scale:
      - X-axis: Bandwidth (25.0 Hz, 50.0 Hz, 83.3 Hz, 166.7 Hz) with proportional subplot widths.
      - Y-axis: Transmission Time Elapsed (0 to 26.5 s) shared across all modes.
    """
    fig = plt.figure(figsize=(13.0, 5.0))
    gs = fig.add_gridspec(1, 4, width_ratios=[1.0, 2.0, 3.333, 6.667], wspace=0.18,
                          left=0.06, right=0.98, bottom=0.12, top=0.80)

    modes = ["LQ16", "LQ8", "LQ4", "LQ2"]
    snrs = ["-24.0 dB", "-21.0 dB", "-17.5 dB", "-14.0 dB"]
    slots = [30.0, 15.0, 7.5, 3.75]

    if grayscale:
        c_sync = "#111827"      # Solid black
        c_data = "#FFFFFF"      # White with black outline
        c_ramp = "#E2E8F0"      # Light hatched
        hatch_ramp = "///"
        edge_col = "#111827"
        title_col = "#000000"
        slot_bg = "#F8FAFC"
        slot_border = "#64748B"
    else:
        c_sync = "#E74C3C"      # Red
        c_data = "#2980B9"      # Blue
        c_ramp = "#27AE60"      # Green
        hatch_ramp = ""
        edge_col = "#1A202C"
        title_col = "#0E3A5D"
        slot_bg = "#F0FDF4"
        slot_border = "#22C55E"

    for idx, (mode, snr, slot_dur) in enumerate(zip(modes, snrs, slots)):
        ax = fig.add_subplot(gs[0, idx])
        mdata = data["modes"][mode][sample_msg_key]
        tones = mdata["tones"]
        ts = mdata["symbol_period"]
        df = mdata["tone_spacing"]
        num_tones = mdata["num_tones"]
        tx_dur = mdata["tx_duration"]
        bw = num_tones * df
        total_syms = len(tones)

        # 1. UTC Slot allocation box
        rect_slot = patches.Rectangle(
            (0, 0), bw, min(slot_dur, 26.5),
            facecolor=slot_bg, edgecolor=slot_border, linestyle="--", linewidth=1.0, zorder=1
        )
        ax.add_patch(rect_slot)

        # 2. Tone Symbols
        for i, t in enumerate(tones):
            t_start = i * ts
            f_min = t * df + df * 0.05
            w_sym = df * 0.90

            if mode in ("LQ16", "LQ8"):
                is_sync = (0 <= i < 7) or (36 <= i < 43) or (72 <= i < 79)
                is_ramp = False
            else:
                is_ramp = (i == 0 or i == total_syms - 1)
                is_sync = (1 <= i <= 4) or (34 <= i <= 37) or (67 <= i <= 70) or (100 <= i <= 103)

            face_c = c_ramp if is_ramp else (c_sync if is_sync else c_data)
            hatch = hatch_ramp if is_ramp else ""

            rect = patches.Rectangle(
                (f_min, t_start), w_sym, ts,
                facecolor=face_c, edgecolor=edge_col, linewidth=0.5, hatch=hatch, zorder=3
            )
            ax.add_patch(rect)

        ax.set_xlim(0, bw)
        ax.set_ylim(31.5, -0.2)  # Inverted time: 0 at top, down to 31.5s

        # Ticks
        if mode == "LQ16":
            ax.set_xticks([0, 12.5, 25.0])
            ax.set_xticklabels(["0", "12.5", "25 Hz"], fontsize=7.2)
        elif mode == "LQ8":
            ax.set_xticks([0, 25.0, 50.0])
            ax.set_xticklabels(["0", "25", "50 Hz"], fontsize=7.5)
        elif mode == "LQ4":
            ax.set_xticks([0, 20.8, 41.7, 62.5, 83.3])
            ax.set_xticklabels(["0", "21", "42", "63", "83 Hz"], fontsize=7.2)
        elif mode == "LQ2":
            ax.set_xticks([0, 41.7, 83.3, 125.0, 166.7])
            ax.set_xticklabels(["0", "42", "83", "125", "167 Hz"], fontsize=7.5)

        ax.set_yticks(np.arange(0, 32, 5))
        if idx == 0:
            ax.set_yticklabels([f"{t:.0f}s" for t in np.arange(0, 32, 5)], fontsize=8.5)
            ax.set_ylabel("Transmission Time Elapsed (s) $\\downarrow$", fontsize=9.2, fontweight="bold")
        else:
            ax.set_yticklabels([])

        ax.grid(True, which="both", linestyle=":", alpha=0.5, color="#CBD5E1", zorder=0)

        # Title
        ax.set_title(f"{mode} ({num_tones}-GFSK)\n{bw:.1f} Hz · {tx_dur:.2f} s\nSensitivity: {snr}",
                     fontsize=7.8, fontweight="bold", pad=8, color=title_col, linespacing=1.2)
        ax.set_xlabel("Tone Freq (Hz)", fontsize=8.0, fontweight="bold")

        # Tone labels below burst
        if mode in ("LQ16", "LQ8"):
            for k in range(num_tones):
                ax.text((k + 0.5)*df, tx_dur + 0.4, f"T{k}", fontsize=5.6, ha="center", va="top", color="#64748B")
        else:
            for k in range(num_tones):
                ax.text((k + 0.5)*df, tx_dur + 0.4, f"T{k}", fontsize=6.8, ha="center", va="top", color="#64748B")

        # Slot boundary annotation
        ax.text(bw * 0.5, slot_dur + 0.4, f"UTC Slot: {slot_dur:.1f}s", fontsize=7.0, ha="center", va="top", color="#475569", style="italic")

    legend_handles = [
        patches.Patch(facecolor=c_sync, edgecolor=edge_col, label="Costas Synchronization Arrays"),
        patches.Patch(facecolor=c_data, edgecolor=edge_col, label="LDPC(174,91) Coded Data Symbols"),
        patches.Patch(facecolor=c_ramp, edgecolor=edge_col, hatch=hatch_ramp, label="PA Ramp Guard Symbols (LQ4/LQ2)"),
        patches.Patch(facecolor=slot_bg, edgecolor=slot_border, linestyle="--", label="UTC Time-Slot Allocation"),
    ]
    fig.legend(handles=legend_handles, loc="upper center", bbox_to_anchor=(0.5, 0.99), ncol=4, frameon=True, fontsize=8.2)

    return fig


def main():
    tones_json = "build/tones_data.json"
    if not os.path.exists(tones_json):
        if os.path.exists("../build/tones_data.json"):
            tones_json = "../build/tones_data.json"
        else:
            raise FileNotFoundError(f"Could not find {tones_json}. Run build/export_tones first.")

    with open(tones_json, "r") as f:
        data = json.load(f)

    os.makedirs("paper/figures", exist_ok=True)
    if os.path.exists("docs"):
        os.makedirs("docs", exist_ok=True)

    plt.rcParams.update({
        'font.family': 'sans-serif',
        'font.size': 8.5,
        'axes.labelsize': 9,
        'axes.titlesize': 10,
        'xtick.labelsize': 7.5,
        'ytick.labelsize': 7.5,
    })

    # 1. Generate Grayscale Vector Figures for LaTeX Paper
    fig_lq8_gray = generate_lq8_diagram(data, grayscale=True)
    fig_lq8_gray.savefig("paper/figures/tones_waterfall_lq8.pdf", bbox_inches='tight', dpi=300)
    plt.close(fig_lq8_gray)

    fig_cmp_gray = generate_comparison_diagram(data, grayscale=True)
    fig_cmp_gray.savefig("paper/figures/tones_waterfall_comparison.pdf", bbox_inches='tight', dpi=300)
    plt.close(fig_cmp_gray)

    # 2. Generate Color Figures for Web Portal / Online Docs
    fig_lq8_color = generate_lq8_diagram(data, grayscale=False)
    if os.path.exists("docs"):
        fig_lq8_color.savefig("docs/tones_waterfall_lq8.png", bbox_inches='tight', dpi=300)
    plt.close(fig_lq8_color)

    print("✓ Successfully generated paper grayscale diagrams & web portal color assets.")

if __name__ == "__main__":
    main()
