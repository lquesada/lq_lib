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
IFS=$'\n\t'
umask 000

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

echo "================================================================================"
echo "  The LQ Digital Mode Family — Full Test & Verification Suite (run_full_tests.sh)"
echo "================================================================================"

# 1. Build library, demo tools, and all test targets
echo ""
echo ">>> [1/5] Building library, tools, and complete test suite..."

if [ -f "build/CMakeCache.txt" ]; then
    CACHED_SRC="$(grep -m 1 "^CMAKE_HOME_DIRECTORY:INTERNAL=" build/CMakeCache.txt 2>/dev/null | cut -d'=' -f2 || true)"
    CACHED_BUILD="$(grep -m 1 "^# For build in directory:" build/CMakeCache.txt 2>/dev/null | sed 's/# For build in directory: //' || true)"
    STALE=false
    if [ -n "$CACHED_SRC" ] && { [ ! -e "$CACHED_SRC" ] || [ ! "$CACHED_SRC" -ef "$SCRIPT_DIR" ]; }; then
        STALE=true
    elif [ -n "$CACHED_BUILD" ] && { [ ! -e "$CACHED_BUILD" ] || [ ! "$CACHED_BUILD" -ef "$SCRIPT_DIR/build" ]; }; then
        STALE=true
    fi
    if [ "$STALE" = true ]; then
        echo " -> Detected CMake cache from another directory. Clearing stale cache..."
        rm -rf build/CMakeCache.txt build/CMakeFiles
    fi
fi

mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

# 2. Run complete automated CTest test suite
echo ""
echo ">>> [2/5] Running complete automated CTest test suite (23 test targets)..."
ctest --test-dir build --output-on-failure

# 3. Run standalone golden vector and paper worked example verifiers
echo ""
echo ">>> [3/5] Executing standalone golden vector & paper example verifiers..."
./build/verify_golden_vectors
./build/verify_paper_examples

# 4. Verify standalone demo and transceiver execution
echo ""
echo ">>> [4/5] Testing standalone demo and transceiver execution..."
./build/lq_demo > /dev/null
./build/lq_c_api_demo > /dev/null
./build/lq_tutorial > /dev/null
./build/lq_matrix > /dev/null
./build/lq_transceiver > /dev/null
echo "    ✓ All 5 standalone demonstration binaries executed successfully."

# 5. Specification PDF & Web Portal Verification
echo ""
echo ">>> [5/7] Validating specification paper integrity & web translations..."
if [ -f "paper/build/lq_digitalmode.pdf" ] || [ -f "paper/lq_digitalmode.pdf" ]; then
    echo "    ✓ Specification PDF exists and is preserved."
else
    echo "    Compiling specification PDF..."
    (cd paper && ./build.sh)
fi
if [ -f "tools/test_translations.py" ]; then
    python3 tools/test_translations.py
fi

# 6. Java Test Battery Validation
echo ""
echo ">>> [6/7] Running standalone Java test battery harness..."
./tests/java/run_java_tests.sh

# 7. ft8_lib Cross-Compatibility Verification
echo ""
echo ">>> [7/7] Running ft8_lib transport cross-compatibility tests..."
./tests/test_ft8_lib/build.sh

echo ""
echo "================================================================================"
echo "  ✓ ALL FULL SYSTEM TESTS PASSED SUCCESSFULLY (0 FAILURES)"
echo "================================================================================"
