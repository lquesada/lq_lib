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
# LQ8 Project — C++ Library Build Script
# ==============================================================================
set -euo pipefail
umask 000

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

BUILD_TYPE="Release"
RUN_FULL_TESTS=true
BUILD_ALL=true

for arg in "$@"; do
    case "$arg" in
        --debug|-d)
            BUILD_TYPE="Debug"
            ;;
        --tests|-t)
            RUN_FULL_TESTS=true
            BUILD_ALL=true
            ;;
        --no-tests)
            RUN_FULL_TESTS=false
            ;;
        --coverage|-k)
            python3 tools/measure_coverage.py
            exit 0
            ;;
        --clean|-c)
            ./clean.sh
            ;;
        --all|-a)
            BUILD_ALL=true
            ;;
        --lib-only|-l)
            BUILD_ALL=false
            RUN_FULL_TESTS=false
            ;;
        --help|-h)
            echo "Usage: ./build.sh [OPTIONS]"
            echo "Options:"
            echo "  --debug, -d     Build with Debug configuration"
            echo "  --all, -a       Build all targets (library, demo tools, test suites) [default]"
            echo "  --tests, -t     Run full test suite via ./run_full_tests.sh [default]"
            echo "  --no-tests      Skip running test suite"
            echo "  --coverage, -k  Run gcov code coverage and print detailed analysis"
            echo "  --clean, -c     Clean all build artifacts before building"
            echo "  --lib-only, -l  Build only the static library (liblq.a) without tests"
            echo "  --help, -h      Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown argument: $arg (use --help for options)"
            exit 1
            ;;
    esac
done

echo "========================================"
echo "Building LQ C++17/C++20 Library ($BUILD_TYPE)"
echo "========================================"

# Detect if build directory contains CMake cache from another directory/machine
if [ -f "build/CMakeCache.txt" ]; then
    CACHED_SRC="$(grep -m 1 "^CMAKE_HOME_DIRECTORY:INTERNAL=" build/CMakeCache.txt 2>/dev/null | cut -d'=' -f2 || true)"
    CACHED_BUILD="$(grep -m 1 "^# For build in directory:" build/CMakeCache.txt 2>/dev/null | sed 's/# For build in directory: //' || true)"
    STALE=false
    if [ -n "$CACHED_SRC" ] && { [ ! -e "$CACHED_SRC" ] || [ ! "$CACHED_SRC" -ef "$ROOT_DIR" ]; }; then
        STALE=true
    elif [ -n "$CACHED_BUILD" ] && { [ ! -e "$CACHED_BUILD" ] || [ ! "$CACHED_BUILD" -ef "$ROOT_DIR/build" ]; }; then
        STALE=true
    fi
    if [ "$STALE" = true ]; then
        echo " -> Detected CMake cache from another directory/system."
        echo " -> Automatically clearing stale cache for clean configuration..."
        rm -rf build/CMakeCache.txt build/CMakeFiles
    fi
fi

mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

if [ "$BUILD_ALL" = true ] || [ "$RUN_FULL_TESTS" = true ]; then
    echo " -> Compiling liblq, demo tools, and test suites..."
    cmake --build build -j"$(nproc)"
else
    echo " -> Compiling static library liblq.a..."
    cmake --build build --target lq -j"$(nproc)"
fi

echo ""
echo "========================================"
if [ -f "build/liblq.a" ]; then
    echo "✓ Static library built: build/liblq.a ($(du -h build/liblq.a | cut -f1))"
fi
if [ "$BUILD_ALL" = true ] || [ "$RUN_FULL_TESTS" = true ]; then
    if [ -f "build/lq_demo" ]; then
        echo "✓ Demo binary built:    build/lq_demo"
    fi
    if [ -f "build/lq_transceiver" ]; then
        echo "✓ Transceiver binary:   build/lq_transceiver"
    fi
fi
echo "========================================"

if [ "$RUN_FULL_TESTS" = true ]; then
    echo ""
    echo "Running full test suite via ./run_full_tests.sh..."
    ./run_full_tests.sh
fi
