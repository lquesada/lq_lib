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
# ==============================================================================
# LQ8 Project — Global Clean Script
# Cleans all build directories, intermediate files, and caches repository-wide.
# ==============================================================================
set -euo pipefail
umask 000

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

echo "========================================"
echo "Cleaning all LQ project build artifacts"
echo "========================================"

# 1. Root CMake build directory and Coverage directory
if [ -d "build" ]; then
    echo " -> Removing root build directory..."
    if [ -d "build/_deps/googletest-src" ]; then
        mkdir -p .deps_cache
        cp -r build/_deps/googletest-src .deps_cache/ 2>/dev/null || true
    fi
    rm -rf build
fi
if [ -d "build_coverage" ]; then
    echo " -> Removing coverage build directory..."
    rm -rf build_coverage
fi
rm -f *.gcov src/*.gcov tests/*.gcov *.gcno src/*.gcno tests/*.gcno *.gcda src/*.gcda tests/*.gcda tones_data.json build/tones_data.json

# 2. Paper LaTeX build directory and preview artifacts (preserves rendered specification PDF paper/lq_digitalmode.pdf and paper/build/lq_digitalmode.pdf)
if [ -d "paper/build" ]; then
    echo " -> Cleaning paper LaTeX auxiliary files (preserving rendered paper/lq_digitalmode.pdf and paper/build/lq_digitalmode.pdf)..."
    if [ -f "paper/build/lq_digitalmode.pdf" ] && [ ! -f "paper/lq_digitalmode.pdf" ]; then
        cp "paper/build/lq_digitalmode.pdf" "paper/lq_digitalmode.pdf"
    elif [ -f "paper/lq_digitalmode.pdf" ] && [ ! -f "paper/build/lq_digitalmode.pdf" ]; then
        cp "paper/lq_digitalmode.pdf" "paper/build/lq_digitalmode.pdf"
    fi
    find paper/build -type f ! -name "lq_digitalmode.pdf" -delete
fi

if [ -d "paper/preview" ]; then
    echo " -> Removing paper preview directory..."
    rm -rf paper/preview
fi
rm -rf preview
rm -f paper/*.aux paper/*.log paper/*.out paper/*.toc paper/*.synctex.gz paper/*.fls paper/*.fdb_latexmk paper/*.bbl paper/*.blg paper/*.run.xml paper/*-blx.bib
rm -f paper/*.png paper/figures/*.png page-*.png page_*.png paper/page-*.png paper/page_*.png
rm -f test_lightning.aux test_lightning.log test_lightning.pdf test_lightning.tex paper/table.pdf paper/table.tex paper/table.aux paper/table.log paper/table_preview*.png

# Remove unneeded temporary directories and redundant duplicates
if [ -d "exampledocs" ]; then
    echo " -> Removing unneeded exampledocs directory..."
    rm -rf exampledocs
fi
rm -f tests/data/golden_vectors*.json

# 3. C++ Examples and Tools local build outputs
echo " -> Removing C++ demo and tool binaries..."
rm -f examples/demo/lq_demo
rm -rf examples/demo/build
rm -f examples/transceiver/lq_transceiver
rm -rf examples/transceiver/build
rm -f examples/matrix/lq_matrix examples/matrix/lq_demo
rm -rf examples/matrix/build
rm -f examples/tutorial/lq_tutorial
rm -rf examples/tutorial/build
rm -f examples/c_api_demo/lq_c_api_demo
rm -rf examples/c_api_demo/build
rm -f tools/verify_paper_examples
rm -f tools/export_tones
rm -f tools/generate_golden_vectors
rm -f tools/verify_golden_vectors
rm -rf tools/build

# 4. External FT8 reference library artifacts and test audio fixtures
echo " -> Cleaning ft8_lib build directories, git history, and large test wav fixtures..."
rm -rf tests/test_ft8_lib/build
rm -rf tests/test_ft8_lib/ft8_lib/build
rm -rf tests/test_ft8_lib/ft8_lib/.git
rm -rf tests/test_ft8_lib/ft8_lib/test/wav
rm -rf tests/test_ft8_lib/ft8_lib/test
rm -f tests/test_ft8_lib/test_ft8_compat

# 5. Android Example build outputs
if [ -d "examples/android" ]; then
    echo " -> Cleaning Android example..."
    rm -rf examples/android/build
    rm -rf examples/android/app/build
    rm -rf examples/android/.gradle
    rm -rf examples/android/app/.cxx
    rm -rf examples/android/bin
    rm -f examples/android/crash.log
    rm -f examples/android/emulator_log.txt
fi

# 6. Temp directory build outputs (if present)
if [ -d "temp" ]; then
    echo " -> Cleaning temp directory build artifacts..."
    rm -rf temp/build
    rm -rf temp/app/build
    rm -rf temp/.gradle
    rm -rf temp/app/.cxx
    rm -rf temp/bin
fi

# 7. Python bytecode caches
echo " -> Cleaning Python __pycache__ and .pyc files..."
find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
find . -type f -name "*.pyc" -delete 2>/dev/null || true

# 8. Stray object or archive files
find . -maxdepth 4 -type f \( -name "*.o" -o -name "*.a" -o -name "*.so" -o -name "*.dylib" \) -not -path "*/.git/*" -delete 2>/dev/null || true

chmod -R a+rwX . 2>/dev/null || true

echo "========================================"
echo "✓ All build files cleaned successfully."
echo "========================================"
