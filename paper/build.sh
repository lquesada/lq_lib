#!/usr/bin/env bash
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
# Build script for the LQ digital mode specification paper
#
# Usage:
#   ./build.sh          Build the PDF (regenerates Varicode table first)
#   ./build.sh clean    Remove all build artifacts
#   ./build.sh quick    Build without regenerating Varicode table
#   ./build.sh table    Build standalone table.pdf (Table 3 message catalogue)

set -euo pipefail

PAPER="lq_digitalmode"
BUILDDIR="build"

cd "$(dirname "$0")"

if [[ "${1:-}" == "clean" ]]; then
    echo "Cleaning build artifacts..."
    rm -rf "$BUILDDIR"
    rm -f "table.aux" "table.log"
    echo "Done."
    exit 0
fi

if [[ "${1:-}" == "table" ]]; then
    echo "=== Building standalone table.pdf ==="
    pdflatex -interaction=nonstopmode table.tex
    echo ""
    echo "============================================"
    echo "  Build complete: table.pdf"
    echo "  $(wc -c < "table.pdf" | tr -d ' ') bytes"
    echo "============================================"
    exit 0
fi

# --- Step 0: Generate Varicode table (unless --quick) ---
if [[ "${1:-}" != "quick" ]]; then
    if [ -f "../tools/generate_varicode.py" ]; then
        echo "=== Generating Varicode table ==="
        python3 ../tools/generate_varicode.py --latex -o tables/varicode_table.tex
    fi
fi

# --- Step 1-3: pdflatex (three passes for references) ---
mkdir -p "$BUILDDIR"

echo "=== Building $PAPER.tex ==="
pdflatex -interaction=nonstopmode -output-directory="$BUILDDIR" "$PAPER.tex" || true
pdflatex -interaction=nonstopmode -output-directory="$BUILDDIR" "$PAPER.tex" || true
pdflatex -interaction=nonstopmode -output-directory="$BUILDDIR" "$PAPER.tex" || true
cp "$BUILDDIR/$PAPER.pdf" "./$PAPER.pdf"

# --- Step 4: Validate Page Budget & Column Flush Bottom Alignment ---
PAGES=$(pdfinfo "$PAPER.pdf" | awk '/Pages:/ {print $2}')
if [[ "$PAGES" -ne 8 ]]; then
    echo "ERROR: Page count is $PAGES (must be strictly 8 pages)!"
    exit 1
fi

VBOX_WARNINGS=$(grep -E "(Underfull \\\\vbox|Overfull \\\\vbox)" "$BUILDDIR/$PAPER.log" || true)
if [[ -n "$VBOX_WARNINGS" ]]; then
    echo "WARNING: Vertical column alignment mismatch detected on pages 1-7:"
    echo "$VBOX_WARNINGS"
else
    echo "✓ Layout verification passed: Strictly 8 pages, all columns vertically flush to bottom margin."
fi

echo ""
echo "============================================"
echo "  Build complete: $PAPER.pdf"
echo "  $(wc -c < "$PAPER.pdf" | tr -d ' ') bytes"
echo "============================================"
