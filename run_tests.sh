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
echo "  The LQ Digital Mode Family — Golden Vector Test Runner (run_tests.sh)"
echo "================================================================================"

# 1. Build library and golden vector test targets
echo ">>> Building liblq and golden vector verification targets..."

if [ -f "build/CMakeCache.txt" ]; then
    CACHED_SRC="$(grep -m 1 "^CMAKE_HOME_DIRECTORY:INTERNAL=" build/CMakeCache.txt 2>/dev/null | cut -d'=' -f2- || true)"
    CACHED_BUILD="$(grep -m 1 "^CMAKE_CACHEFILE_DIR:INTERNAL=" build/CMakeCache.txt 2>/dev/null | cut -d'=' -f2- || true)"
    if [ -z "$CACHED_BUILD" ]; then
        CACHED_BUILD="$(grep -m 1 "^# For build in directory:" build/CMakeCache.txt 2>/dev/null | sed 's/# For build in directory: //' || true)"
    fi
    STALE=false
    if [ -n "$CACHED_SRC" ]; then
        CACHED_SRC_REAL="$(realpath "$CACHED_SRC" 2>/dev/null || echo "$CACHED_SRC")"
        SCRIPT_REAL="$(realpath "$SCRIPT_DIR" 2>/dev/null || echo "$SCRIPT_DIR")"
        if [ "$CACHED_SRC_REAL" != "$SCRIPT_REAL" ]; then
            STALE=true
        fi
    fi
    if [ -n "$CACHED_BUILD" ]; then
        CACHED_BUILD_REAL="$(realpath "$CACHED_BUILD" 2>/dev/null || echo "$CACHED_BUILD")"
        BUILD_REAL="$(realpath "$SCRIPT_DIR/build" 2>/dev/null || echo "$SCRIPT_DIR/build")"
        if [ "$CACHED_BUILD_REAL" != "$BUILD_REAL" ]; then
            STALE=true
        fi
    fi
    if [ "$STALE" = true ]; then
        echo " -> Detected CMake cache from another directory/system (was $CACHED_BUILD). Clearing stale cache..."
        rm -rf build/CMakeCache.txt build/CMakeFiles
    fi
fi

mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target lq test_golden_vectors verify_golden_vectors -j"$(nproc)"

# 2. Run golden vector tests against the reference library
echo ""
echo ">>> Running golden vector test suite against liblq..."
./build/verify_golden_vectors
./build/test_golden_vectors

echo ""
echo "================================================================================"
echo "  ✓ GOLDEN VECTOR TESTS PASSED SUCCESSFULLY (0 FAILURES)"
echo "================================================================================"
