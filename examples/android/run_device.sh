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

echo "=== 1. Building Core C++ Reference Library (Root) ==="
"$ROOT_DIR/build.sh"

echo "=== 2. Checking Device & Building APK ==="
# Define paths
SDK_DIR=$(grep "^sdk.dir" "local.properties" 2>/dev/null | cut -d'=' -f2)
if [ -z "$SDK_DIR" ]; then
    SDK_DIR="$HOME/Android/Sdk"
fi
ADB="$SDK_DIR/platform-tools/adb"
if [ ! -x "$ADB" ]; then
    ADB="adb"
fi

echo "📱 Checking for connected physical device..."
DEVICE_CHECK=$($ADB devices -l | grep -v "emulator" | grep -v "List of devices attached" | grep "device")

if [ -z "$DEVICE_CHECK" ]; then
    echo "❌ No physical device found or authorized. Please connect your phone and enable USB Debugging."
    exit 1
fi

echo "✅ Device found: $DEVICE_CHECK"

echo "🔨 Building APK (NDK & Gradle)..."
./gradlew assembleDebug

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

APK_PATH="app/build/outputs/apk/debug/app-debug.apk"

echo "📦 Installing to device..."
if ! $ADB -d install -t -r "$APK_PATH"; then
    echo "⚠️ Re-install failed (possible signature mismatch). Reinstalling cleanly..."
    $ADB -d uninstall com.qft8.lq 2>/dev/null || true
    $ADB -d install -t -r "$APK_PATH"
fi

echo "🚀 Launching application..."
$ADB -d shell am start -n com.qft8.lq/.MainActivity
