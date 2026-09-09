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

set -euo pipefail
umask 000

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "================================================================================"
echo "  The LQ Digital Mode Family — Full System & Verification Build (build_all.sh)  "
echo "================================================================================"
echo ""

# 1. Run Complete Automated Test Suite (run_full_tests.sh)
echo ">>> [1/3] Running Comprehensive Test & Verification Suite (run_full_tests.sh)..."
./run_full_tests.sh
echo "✓ All test suites and standalone examples completed successfully."
echo ""

# 2. Build Specification Paper PDF
echo ">>> [2/3] Building Academic Specification Paper PDF (paper/build.sh)..."
(cd paper && ./build.sh > /dev/null 2>&1 || ./build.sh)
if [ -f "paper/lq_digitalmode.pdf" ] || [ -f "paper/build/lq_digitalmode.pdf" ]; then
    PDF_FILE="paper/lq_digitalmode.pdf"
    [ -f "$PDF_FILE" ] || PDF_FILE="paper/build/lq_digitalmode.pdf"
    PDF_SIZE="$(ls -lh "$PDF_FILE" | awk '{print $5}')"
    echo "✓ Specification paper built successfully: ${PDF_FILE} (${PDF_SIZE})"
else
    echo "❌ Error: paper/lq_digitalmode.pdf was not generated."
    exit 1
fi
echo ""

# 3. Measure Reference Library Code Coverage
echo ">>> [3/3] Measuring Reference Library Code Coverage..."
./build.sh --coverage > /dev/null
if [ -f "tools/measure_coverage.py" ]; then
    python3 tools/measure_coverage.py
fi
echo "✓ Code coverage measurement completed."
echo ""

chmod -R a+rwX . 2>/dev/null || true

echo "================================================================================"
echo "  ✓ ALL BUILDS, EXAMPLES, COMPATIBILITY TESTS, AND PAPER GENERATION SUCCEEDED!  "
echo "================================================================================"
