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

public class LqProtocol {

    public enum Mode {
        LQ8("LQ8", 0.160f, 6.25f, 12.64f, 15.0f, 79, 58, 8, -21.0f, 16.0f),
        LQ4("LQ4", 0.048f, 20.833334f, 5.04f, 7.5f, 105, 87, 4, -17.5f, 8.0f),
        LQ2("LQ2", 0.024f, 41.666668f, 2.52f, 3.75f, 105, 87, 4, -14.0f, 4.0f),
        LQ16("LQ16", 0.320f, 3.125f, 25.28f, 30.0f, 79, 58, 8, -24.0f, 28.0f);

        public final String name;
        public final float symbolPeriod;
        public final float toneSpacing;
        public final float txDuration;
        public final float slotDuration;
        public final int totalSymbols;
        public final int dataSymbols;
        public final int numTones;
        public final float minSnrDb;
        public final float decodeWindowSeconds;

        Mode(String name, float symbolPeriod, float toneSpacing, float txDuration,
             float slotDuration, int totalSymbols, int dataSymbols, int numTones,
             float minSnrDb, float decodeWindowSeconds) {
            this.name = name;
            this.symbolPeriod = symbolPeriod;
            this.toneSpacing = toneSpacing;
            this.txDuration = txDuration;
            this.slotDuration = slotDuration;
            this.totalSymbols = totalSymbols;
            this.dataSymbols = dataSymbols;
            this.numTones = numTones;
            this.minSnrDb = minSnrDb;
            this.decodeWindowSeconds = decodeWindowSeconds;
        }
    }

    // Unique 8-GFSK Costas synchronization array for LQ8 / LQ16 (3 bursts of length 7)
    public static final int[] COSTAS_ARRAY_8 = {2, 5, 6, 1, 3, 0, 4};

    // Unique 4-GFSK Costas synchronization arrays for LQ4 / LQ2 (4 bursts of length 4)
    public static final int[] COSTAS_SYNC1_4 = {0, 2, 3, 1};
    public static final int[] COSTAS_SYNC2_4 = {1, 3, 2, 0};
    public static final int[] COSTAS_SYNC3_4 = {2, 0, 1, 3};
    public static final int[] COSTAS_SYNC4_4 = {3, 1, 0, 2};

    // 8-GFSK Gray Code Mapping (3 bits <-> 8 tones)
    public static final int[] GRAY_MAP_8 = {0, 1, 3, 2, 5, 6, 4, 7};
    public static final int[] GRAY_INV_MAP_8 = {0, 1, 3, 2, 6, 4, 5, 7};

    // 4-GFSK Gray Code Mapping (2 bits <-> 4 tones)
    public static final int[] GRAY_MAP_4 = {0, 1, 3, 2};
    public static final int[] GRAY_INV_MAP_4 = {0, 1, 3, 2};
}
