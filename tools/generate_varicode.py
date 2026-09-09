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
# =============================================================================

"""
LQ Varicode generator and LaTeX table exporter.

Generates the sequential canonical Varicode table for the LQ Digital Mode Family.
"""

import math
import sys
import os

CHARSET = [
    # Top letters & sentinels
    ' ', 'E', 'T', 'A', 'O', 'N', 'I', 'S', 'R', 'H',
    'D', 'L', 'C', 'U', 'M', 'F', 'W', 'P', 'G', 'Y',
    'B', 'V', 'K', '⌁', 'X', 'J', '1', 'Q', '!', '?', 'Z',
    # , and . before 0
    ',', '.',
    # Digits
    '0', '2', '8', '3', '4', '5', '9', '6', '7',
    # ", -, ', _, ; after 7
    '"', '-', "'", '_', ';',
    # Symbols
    ':', '=', '(', ')', '#', '*', '[', ']', '|', '/', '$', '+',
    # \n after +
    '\n',
    # Rare symbols & Latin-1
    '%', '&', '<', '>', '@', '\\', '^', '`', '{', '}', '~',
    '¡', '¿', 'À', 'Á', 'Â', 'Ã', 'Ä', 'Å', 'Æ', 'Ç', 'È', 'É',
    'Ê', 'Ë', 'Ì', 'Í', 'Î', 'Ï', 'Ð', 'Ñ', 'Ò', 'Ó', 'Ô', 'Õ',
    'Ö', 'Ø', 'Ù', 'Ú', 'Û', 'Ü', 'Ý', 'Þ',
    # [FILL] at absolute end
    '⌁FILL'
]

assert len(CHARSET) == 104, f"Expected 104 characters, got {len(CHARSET)}"
assert len(set(CHARSET)) == 104, "Duplicate characters in CHARSET"

RAW_FREQUENCIES = {
    ' ': 16.2347,
    'E': 9.9274,
    'T': 7.2165,
    'A': 6.3918,
    'O': 6.0594,
    'N': 5.7799,
    'I': 5.7269,
    'S': 5.2454,
    'R': 4.8485,
    'H': 4.6155,
    'D': 3.3786,
    'L': 3.1114,
    'C': 2.2707,
    'U': 2.1729,
    'M': 1.9902,
    'F': 1.8932,
    'W': 1.5793,
    'P': 1.5493,
    'G': 1.5180,
    'Y': 1.4172,
    'B': 1.1460,
    'V': 0.8204,
    'K': 0.5137,
    '⌁': 0.3300,
    'X': 0.1537,
    'J': 0.1008,
    '1': 0.0873,
    'Q': 0.0716,
    '!': 0.0681,
    '?': 0.0651,
    'Z': 0.0595,
    ',': 0.0560,
    '.': 0.0520,
    '0': 0.0480,
    '2': 0.0478,
    '8': 0.0396,
    '3': 0.0390,
    '4': 0.0379,
    '5': 0.0343,
    '9': 0.0313,
    '6': 0.0312,
    '7': 0.0296,
    '"': 0.0270,
    '-': 0.0250,
    "'": 0.0230,
    '_': 0.0210,
    ';': 0.0190,
    ':': 0.0170,
    '=': 0.0150,
    '(': 0.0140,
    ')': 0.0130,
    '#': 0.0116,
    '*': 0.0077,
    '[': 0.0068,
    ']': 0.0068,
    '|': 0.0064,
    '/': 0.0021,
    '$': 0.0017,
    '+': 0.0014,
    '\n': 0.0012,
}

FREQUENCIES = {}
rare_base = 0.0010
for i, ch in enumerate(CHARSET):
    if ch in RAW_FREQUENCIES:
        FREQUENCIES[ch] = RAW_FREQUENCIES[ch]
    else:
        if ch == '⌁FILL':
            FREQUENCIES[ch] = 0.00001
        else:
            offset = (i - 60) * 0.00001
            FREQUENCIES[ch] = max(0.00005, rare_base - offset)

# Normalize
_tot = sum(FREQUENCIES.values())
for ch in FREQUENCIES:
    FREQUENCIES[ch] = (FREQUENCIES[ch] / _tot) * 100.0


# Target optimal lengths per character based on empirical entropy
TARGET_LENGTHS = {
    ' ': 3, 'E': 3,
    'T': 4, 'A': 4, 'O': 4, 'N': 4, 'I': 4, 'S': 4, 'R': 4, 'H': 4,
    'D': 5, 'L': 5, 'C': 5,
    'U': 6, 'M': 6, 'F': 6, 'W': 6, 'P': 6, 'G': 6, 'Y': 6, 'B': 6,
    'V': 7,
    'K': 8, '⌁': 8,
    'X': 9,
    'J': 10, '1': 10, 'Q': 10, '!': 10, '?': 10, 'Z': 10,
    ',': 11, '.': 11, '0': 11, '2': 11, '8': 11, '3': 11, '4': 11, '5': 11, '9': 11, '6': 11,
    '7': 12, '"': 12, '-': 12, "'": 12, '_': 12, ';': 12, ':': 12,
    '=': 13, '(': 13, ')': 13, '#': 13, '*': 13,
    '[': 14, ']': 14, '|': 14,
    '/': 15,
    '$': 16, '+': 16, '\n': 16, '%': 16, '&': 16, '<': 16,
    '>': 17, '@': 17, '\\': 17, '^': 17, '`': 17, '{': 17, '}': 17, '~': 17,
    '¡': 17, '¿': 17, 'À': 17, 'Á': 17, 'Â': 17, 'Ã': 17, 'Ä': 17, 'Å': 17,
    'Æ': 17, 'Ç': 17, 'È': 17, 'É': 17, 'Ê': 17, 'Ë': 17, 'Ì': 17, 'Í': 17,
    'Î': 17, 'Ï': 17, 'Ð': 17, 'Ñ': 17, 'Ò': 17, 'Ó': 17, 'Ô': 17, 'Õ': 17,
    'Ö': 17, 'Ø': 17, 'Ù': 17, 'Ú': 17, 'Û': 17, 'Ü': 17, 'Ý': 17,
    'Þ': 18, '⌁FILL': 18
}


def allocate_sequential_varicode(order, target_lengths):
    """
    Allocate sequential canonical Varicode codewords with zero-fill absorption.
    The 000...0 branch is reserved down to length 18 for ⌁FILL (padding fill).
    All other codewords are assigned sequentially in ascending binary order.
    """
    by_len = {}
    for ch in order:
        L = target_lengths[ch]
        by_len.setdefault(L, []).append(ch)

    available_nonzero = list(range(1, 8)) # 001 to 111 at L=3 (000 reserved for FILL branch)
    codes = {}

    for L in range(3, 19):
        chars = by_len.get(L, [])
        if L == 18:
            codes['⌁FILL'] = '0' * 18
            remaining = [c for c in chars if c != '⌁FILL']
            for c in remaining:
                val = available_nonzero.pop(0) if available_nonzero else 1
                codes[c] = format(val, f'0{L}b')
            break

        for c in chars:
            if available_nonzero:
                val = available_nonzero.pop(0)
                codes[c] = format(val, f'0{L}b')
            else:
                codes[c] = format(1, f'0{L}b')

        next_nonzero = []
        for val in available_nonzero:
            next_nonzero.append(val << 1)
            next_nonzero.append((val << 1) | 1)
        if L < 18:
            next_nonzero.append(1) # 00...01 at L+1 from 0-branch
        available_nonzero = next_nonzero

    return codes


def verify_kraft_mcmillan(codes: dict):
    s = sum(2 ** -len(c) for c in codes.values())
    assert abs(s - 1.0) < 1e-9, f"Kraft-McMillan inequality violated: {s} != 1"


DISPLAY_NAMES = {
    ' ':     '␣ (space)',
    '⌁':     '⌁ (EOM)',
    '⌁FILL': '[FILL] (padding fill)',
    '\n':    '\\n (Enter)',
}

LATEX_ESCAPES = {
    ' ':     r'{\textvisiblespace}',
    '⌁':     r'{\ensuremath{\lightning}}',
    '0':     r'{\szero}',
    '⌁FILL': '[FILL]',
    '\n':    r'\textbackslash{}n',
    '&':     r'\&',
    '$':     r'\$',
    '{':     r'\{',
    '}':     r'\}',
    '^':     r'\^{}',
    '~':     r'\~{}',
    '_':     r'\_',
    '#':     r'\#',
    '%':     r'\%',
    '<':     r'$<$',
    '>':     r'$>$',
    '|':     r'$|$',
    '\\':    r'\textbackslash{}',
    '"':     r'\textquotedbl{}',
    "'":     r"\textquotesingle{}",
}


def display_char(ch: str) -> str:
    return DISPLAY_NAMES.get(ch, ch)


def latex_char(ch: str) -> str:
    return LATEX_ESCAPES.get(ch, ch)


def latex_code(code: str) -> str:
    return code.replace('0', r'\szero{}')


def generate_latex_table(codes: dict) -> str:
    order = CHARSET
    n = len(order)
    col_len = (n + 3) // 4

    lines = []
    lines.append(r"% =============================================================================")
    lines.append(r"% The LQ Digital Mode Family — Reference Implementation (lq_lib)")
    lines.append(r"%")
    lines.append(r"% Author:  Luis Quesada (HB9IPH)")
    lines.append(r"% Web:     https://luisquesada.com")
    lines.append(r"% Portal:  https://lquesada.github.io/lq_lib/")
    lines.append(r"% GitHub:  https://github.com/lquesada/lq_lib")
    lines.append(r"% App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)")
    lines.append(r"%")
    lines.append(r"% License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)")
    lines.append(r"%")
    lines.append(r"% Copyright (c) 2026 Luis Quesada (HB9IPH)")
    lines.append(r"%")
    lines.append(r"% Permission is hereby granted, free of charge, to any person obtaining a copy")
    lines.append(r'% of this software and associated documentation files (the "Software"), to deal')
    lines.append(r"% in the Software without restriction, including without limitation the rights")
    lines.append(r"% to use, copy, modify, merge, publish, distribute, sublicense, and/or sell")
    lines.append(r"% copies of the Software, and to permit persons to whom the Software is")
    lines.append(r"% furnished to do so, subject to the following conditions:")
    lines.append(r"%")
    lines.append(r"% The above copyright notice and this permission notice shall be included in")
    lines.append(r"% all copies or substantial portions of the Software.")
    lines.append(r"%")
    lines.append(r'% THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR')
    lines.append(r"% IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,")
    lines.append(r"% FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE")
    lines.append(r"% AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER")
    lines.append(r"% LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,")
    lines.append(r"% OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE")
    lines.append(r"% SOFTWARE.")
    lines.append(r"% =============================================================================")
    lines.append(r"% Auto-generated by tools/generate_varicode.py — do not edit by hand")
    lines.append(r"% Regenerate with: python3 tools/generate_varicode.py --latex -o tables/varicode_table.tex")
    lines.append(r"")
    lines.append(r"\caption{LQ Varicode assignments for the " + str(n) + r"-character free-text alphabet.}")
    lines.append(r"\label{tab:varicode}")
    lines.append(r"\resizebox{\textwidth}{!}{%")
    lines.append(r"\scriptsize")
    lines.append(r"\setlength{\tabcolsep}{1.2pt}")
    lines.append(r"\begin{tabular}{@{}cl c r | cl c r | cl c r | cl c r@{}}")
    lines.append(r"\toprule")
    lines.append(r"\textbf{Char} & \textbf{Varicode} & \textbf{Len} & \textbf{Freq} & " * 3)
    lines.append(r"\textbf{Char} & \textbf{Varicode} & \textbf{Len} & \textbf{Freq} \\")
    lines.append(r"\midrule")

    for i in range(col_len):
        row_cells = []
        for col in range(4):
            idx = i + col * col_len
            if idx < n:
                ch = order[idx]
                c = f"\\texttt{{{latex_char(ch)}}}"
                v = f"\\texttt{{{latex_code(codes[ch])}}}"
                l = str(len(codes[ch]))
                f = f"{FREQUENCIES[ch]:.2f}\\%"
                if f == "0.00\\%":
                    f = "$<$0.01\\%"
                row_cells.append(f"{c} & {v} & {l} & {f}")
            else:
                row_cells.append(r"& & &")
        lines.append(" & ".join(row_cells) + r" \\")

    lines.append(r"\bottomrule")
    lines.append(r"\end{tabular}%")
    lines.append(r"}")

    return "\n".join(lines)


def print_human_table(codes: dict):
    lengths = [len(codes[c]) for c in CHARSET]
    total_freq = sum(FREQUENCIES.values())
    avg_len = sum(FREQUENCIES[c] * len(codes[c]) for c in CHARSET) / total_freq
    entropy = -sum((FREQUENCIES[c]/total_freq) *
                    math.log2(FREQUENCIES[c]/total_freq) for c in CHARSET)

    print(f"LQ8 Sequential Canonical Varicode — {len(CHARSET)} characters")
    print(f"Code lengths : {min(lengths)}–{max(lengths)} bits")
    print(f"Mean code len: {avg_len:.3f} bits/char")
    print(f"Entropy      : {entropy:.3f} bits/char")
    print(f"Efficiency   : {entropy/avg_len*100:.1f}%")
    print(f"Approx chars in 77 bits: {int(77 / avg_len)} (avg)")
    print()

    print(f"{'Idx':>3}  {'Char':>12}  {'Code':<32}  {'Len':>3}  {'Freq':>8}")
    print("─" * 68)
    for i, ch in enumerate(CHARSET):
        print(f"{i:>3}  {display_char(ch):>12}  "
              f"{codes[ch]:<32}  {len(codes[ch]):>3}  "
              f"{FREQUENCIES[ch]:>8.4f}%")


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Generate and display sequential canonical LQ Varicode table.")
    parser.add_argument('--latex', action='store_true', help="Output LaTeX table instead of text")
    parser.add_argument('-o', '--output', type=str, help="Output file path (default stdout)")
    args = parser.parse_args()

    codes = allocate_sequential_varicode(CHARSET, TARGET_LENGTHS)
    verify_kraft_mcmillan(codes)

    if args.latex:
        out = generate_latex_table(codes)
    else:
        print_human_table(codes)
        return

    if args.output:
        with open(args.output, 'w', encoding='utf-8') as f:
            f.write(out)
        print(f"Wrote {args.output}")
    else:
        print(out)

if __name__ == '__main__':
    main()
