/*
 * =============================================================================
 * The LQ Digital Mode Family — Reference Implementation (lq_lib)
 *
 * Author:  Luis Quesada (HB9IPH)
 * Web:     https://luisquesada.com
 * Portal:  https://lquesada.github.io/lq_lib/
 * GitHub:  https://github.com/lquesada/lq_lib
 * App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)
 *
 * License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)
 *
 * Copyright (c) 2026 Luis Quesada (HB9IPH)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * =============================================================================
 */

package com.qft8.lq;

public class LqNative {

    private static boolean nativeAvailable = false;

    static {
        try {
            System.loadLibrary("lq_native");
            nativeAvailable = true;
        } catch (UnsatisfiedLinkError e) {
            nativeAvailable = false;
        }
    }

    public static boolean isAvailable() {
        return nativeAvailable;
    }

    // Native method declarations
    public static native boolean isNativeLoaded();
    public static native int computeHash(String callsign);
    public static native int computeHash14(String callsign);
    public static native boolean encodeMessage(LqMessage msg, byte[] outPayload);
    public static native boolean decodeMessage(byte[] inPayload, LqMessage outMsg);
    public static native int[] encodeTones(LqMessage msg, int modeId);
    public static native boolean decodeTones(int[] inTones, int modeId, LqMessage outMsg);
    public static native float[] generateAudio(int[] inTones, int modeId, float centerFreq, float sampleRate);
    public static native boolean decodeAudio(float[] inAudio, float centerFreq, float sampleRate, int modeId, LqMessage outMsg);
}
