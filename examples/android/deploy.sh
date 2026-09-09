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

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$DIR/../.." && pwd)"
cd "$DIR"

VERSION_FILE="$DIR/VERSION"
BUILD_DIR="$DIR/build"

echo "========================================"
echo "LQ Transceiver Android Build Script"
echo "========================================"

echo "Cleaning up previous builds..."
"$DIR/clean.sh"

# 1. Extract Version
if [ -f "$VERSION_FILE" ]; then
    VERSION=$(cat "$VERSION_FILE" | xargs)
else
    VERSION="0.1"
fi

if [ -f "$DIR/CODE_VERSION" ]; then
    CODE_VERSION=$(cat "$DIR/CODE_VERSION" | xargs)
else
    CODE_VERSION="1"
fi

TIMESTAMP=$(date +"%Y%m%d-%H%M%S")
APK_NAME="LQTransceiver-v${VERSION}-b${CODE_VERSION}-${TIMESTAMP}.apk"
AAB_NAME="LQTransceiver-v${VERSION}-b${CODE_VERSION}-${TIMESTAMP}.aab"

echo "Version: $VERSION (Code $CODE_VERSION)"
echo "Timestamp: $TIMESTAMP"
echo ""

# 2. Run Unit Tests
echo "Running Unit Tests..."
"$DIR/run_tests.sh"
if [ $? -ne 0 ]; then
    echo "Unit Tests Failed! Aborting build."
    exit 1
fi
echo ""

# 3. Build Release Artifacts
echo "Building Release Packages..."
bash gradlew assembleRelease
if [ $? -ne 0 ]; then
    echo "Build Failed!"
    exit 1
fi

# 4. Verify 16KB Page Size Compatibility
echo "Verifying 16KB Compatibility..."
./verify_16k.sh release || true
echo ""

# 5. Copy Artifacts
echo "Copying artifacts..."
mkdir -p "$BUILD_DIR"

SRC_APK="app/build/outputs/apk/release/app-release-unsigned.apk"
if [ ! -f "$SRC_APK" ]; then
    SRC_APK="app/build/outputs/apk/release/app-release.apk"
fi

if [ -f "$SRC_APK" ]; then
    cp "$SRC_APK" "$BUILD_DIR/$APK_NAME"
    echo "Created: $BUILD_DIR/$APK_NAME"
fi

echo ""
echo "========================================"
echo "Build Complete Successfully"
echo "========================================"
