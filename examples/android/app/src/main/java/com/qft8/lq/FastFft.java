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

public class FastFft {

    private final int n;
    private final float[] window;
    private final float[] cosTable;
    private final float[] sinTable;
    private final int[] bitRev;

    public FastFft(int n) {
        if ((n & (n - 1)) != 0 || n <= 0) {
            throw new IllegalArgumentException("FFT size must be a power of 2: " + n);
        }
        this.n = n;
        this.window = new float[n];
        for (int i = 0; i < n; i++) {
            // Periodic Hann window
            this.window[i] = (float) (0.5 * (1.0 - Math.cos(2.0 * Math.PI * i / n)));
        }

        // Bit-reversal table
        this.bitRev = new int[n];
        int log2n = Integer.numberOfTrailingZeros(n);
        for (int i = 0; i < n; i++) {
            int rev = 0;
            for (int j = 0; j < log2n; j++) {
                if ((i & (1 << j)) != 0) {
                    rev |= (1 << (log2n - 1 - j));
                }
            }
            this.bitRev[i] = rev;
        }

        // Twiddle factors
        this.cosTable = new float[n / 2];
        this.sinTable = new float[n / 2];
        for (int i = 0; i < n / 2; i++) {
            double angle = -2.0 * Math.PI * i / n;
            this.cosTable[i] = (float) Math.cos(angle);
            this.sinTable[i] = (float) Math.sin(angle);
        }
    }

    public int getSize() {
        return n;
    }

    public void computeSpectrum(float[] input, float[] outReal, float[] outImag, float[] outPowerDb) {
        if (input.length < n) return;

        // Apply window and bit reversal
        for (int i = 0; i < n; i++) {
            int target = bitRev[i];
            outReal[target] = input[i] * window[i];
            outImag[target] = 0.0f;
        }

        // In-place Cooley-Tukey Radix-2 FFT
        for (int len = 2; len <= n; len <<= 1) {
            int half = len >> 1;
            int step = n / len;
            for (int i = 0; i < n; i += len) {
                for (int j = 0; j < half; j++) {
                    int k = j * step;
                    float c = cosTable[k];
                    float s = sinTable[k];

                    int uIdx = i + j;
                    int vIdx = i + j + half;

                    float uR = outReal[uIdx];
                    float uI = outImag[uIdx];

                    float vR = outReal[vIdx] * c - outImag[vIdx] * s;
                    float vI = outReal[vIdx] * s + outImag[vIdx] * c;

                    outReal[uIdx] = uR + vR;
                    outImag[uIdx] = uI + vI;

                    outReal[vIdx] = uR - vR;
                    outImag[vIdx] = uI - vI;
                }
            }
        }

        // Positive frequency magnitudes in dB
        int halfN = n / 2;
        float norm = 2.0f / (float) n;
        for (int i = 0; i < halfN; i++) {
            float r = outReal[i] * norm;
            float im = outImag[i] * norm;
            float power = r * r + im * im;
            outPowerDb[i] = (float) (10.0 * Math.log10(power + 1e-12f));
        }
    }
}
