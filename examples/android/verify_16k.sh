#!/bin/bash
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
# Script to verify 16KB page size alignment support (Zip + ELF Segment Alignment)
set -e

BUILD_TYPE="${1:-debug}"
APK_PATH="app/build/outputs/apk/${BUILD_TYPE}/app-${BUILD_TYPE}.apk"
if [ ! -f "$APK_PATH" ]; then
    APK_PATH="app/build/outputs/apk/${BUILD_TYPE}/app-${BUILD_TYPE}-unsigned.apk"
fi

if [ ! -f "$APK_PATH" ]; then
    echo "❌ APK not found: $APK_PATH"
    echo "Please build the project first (e.g. ./gradlew assembleDebug or assembleRelease)"
    exit 1
fi

echo "Targeting Build Type: $BUILD_TYPE"
echo "Checking APK: $APK_PATH"

SDK_DIR=$(grep "^sdk.dir" "local.properties" 2>/dev/null | cut -d'=' -f2)
if [ -z "$SDK_DIR" ]; then
    SDK_DIR="$HOME/Android/Sdk"
fi

echo "========================================"
echo "16KB Support Verification"
echo "========================================"

# 1. Check Zip Alignment (16KB page alignment inside APK)
ZIPALIGN=$(find "$SDK_DIR/build-tools" -name "zipalign" 2>/dev/null | sort -r | head -n 1)

if [ -x "$ZIPALIGN" ]; then
    if "$ZIPALIGN" -c -v -P 16 4 "$APK_PATH" > /dev/null; then
         echo "✅ APK is 16KB Zip-Aligned."
    else
         echo "❌ APK is NOT 16KB Zip-Aligned."
         exit 1
    fi
else
    echo "⚠️  zipalign not found. Skipping Zip check."
fi

# 2. Check ELF Segment Alignment of Shared Libraries (.so) inside the APK
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

unzip -q -o "$APK_PATH" "lib/*" -d "$TMP_DIR" 2>/dev/null || true

SO_FILES=$(find "$TMP_DIR" -name "*.so" 2>/dev/null)

if [ -z "$SO_FILES" ]; then
    echo "⚠️  No shared libraries found inside APK."
else
    echo "Verifying ELF LOAD segment alignment of native libraries inside APK:"
    FAIL=0
    for so in $SO_FILES; do
        REL_NAME=$(echo "$so" | sed "s|$TMP_DIR/||")
        # Check alignment of all LOAD segments using readelf
        # 16KB alignment is 0x4000 (16384) or 0x10000 (65536)
        NON_16K=$(readelf -l "$so" | grep -A 1 "LOAD" | grep -E "Align" | grep -v "0x4000" | grep -v "0x10000" || true)
        if [ -n "$NON_16K" ]; then
            echo "  ❌ $REL_NAME is NOT aligned to 16KB (found: $NON_16K)"
            FAIL=1
        else
            echo "  ✅ $REL_NAME is aligned to 16KB (0x4000)."
        fi
    done

    if [ "$FAIL" -ne 0 ]; then
        echo ""
        echo "❌ ELF segment alignment check failed!"
        exit 1
    fi
fi

echo ""
echo "🎉 Verification Passed: Ready for Android 15 (16KB Page Size)"
