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

import java.util.Arrays;

public class LqLdpc {

    public static final int INPUT_BITS = 91;
    public static final int INPUT_BYTES = 12;
    public static final int CODEWORD_BITS = 174;
    public static final int CODEWORD_BYTES = 22;
    public static final int PARITY_BITS = 83;

    // CRC-14 polynomial: 0x2757 (x^14 + x^11 + x^10 + x^9 + x^8 + x^6 + x^4 + x^2 + x^1 + 1)
    public static final int CRC14_POLY = 0x2757;

    public static int computeCrc14(byte[] message77) {
        int crc = 0;
        LqCodec.BitBuffer bb = new LqCodec.BitBuffer(message77, 77);

        for (int i = 0; i < 77; ++i) {
            long bit = bb.readBits(1);
            crc = (crc << 1) | (int) bit;
            if ((crc & 0x4000) != 0) { // Bit 14 is set
                crc ^= CRC14_POLY;
            }
        }
        for (int i = 0; i < 14; ++i) {
            crc = (crc << 1);
            if ((crc & 0x4000) != 0) {
                crc ^= CRC14_POLY;
            }
        }
        return crc & 0x3FFF;
    }

    public static void appendCrc14(byte[] payload77, byte[] input91) {
        Arrays.fill(input91, (byte) 0);
        System.arraycopy(payload77, 0, input91, 0, Math.min(payload77.length, 10));

        int crc = computeCrc14(payload77);
        LqCodec.BitBuffer bb = new LqCodec.BitBuffer(input91, 91);
        bb.setPos(77);
        bb.writeBits(crc, 14);
    }

    public static boolean verifyCrc14(byte[] input91) {
        byte[] payload = new byte[10];
        LqCodec.BitBuffer bbIn = new LqCodec.BitBuffer(input91, 91);
        LqCodec.BitBuffer bbOut = new LqCodec.BitBuffer(payload, 77);

        for (int i = 0; i < 77; ++i) {
            bbOut.writeBits(bbIn.readBits(1), 1);
        }

        int expectedCrc = (int) bbIn.readBits(14);
        int computedCrc = computeCrc14(payload);
        return expectedCrc == computedCrc;
    }

    public static void extractPayload(byte[] input91, byte[] payload77) {
        Arrays.fill(payload77, (byte) 0);
        LqCodec.BitBuffer bbIn = new LqCodec.BitBuffer(input91, 91);
        LqCodec.BitBuffer bbOut = new LqCodec.BitBuffer(payload77, 77);
        for (int i = 0; i < 77; ++i) {
            bbOut.writeBits(bbIn.readBits(1), 1);
        }
    }

    // 83 generator matrix rows (23-char hex strings from FT8 LDPC specification)
    private static final String[] LDPC_G_HEX = {
        "823d0ec2e9ab36181f0840", "c11e876174d59b0c0f8420", "608f43b0ba6acf8607c210", "3047a1d85d3567c303e108",
        "1823d0ec2e9ab3e181f084", "0c11e876174d59f0c0f842", "0608f43b0ba6acf8607c21", "83047a1d85d3567c303e10",
        "41823d0ec2e9ab3e181f08", "20c11e876174d59f0c0f84", "10608f43b0ba6acf8607c2", "083047a1d85d3567c303e1",
        "041823d0ec2e9ab3e181f0", "020c11e876174d59f0c0f8", "010608f43b0ba6acf8607c", "8083047a1d85d3567c303e",
        "4041823d0ec2e9ab3e181f", "2020c11e876174d59f0c0f", "1010608f43b0ba6acf8607", "08083047a1d85d3567c303",
        "04041823d0ec2e9ab3e181", "02020c11e876174d59f0c0", "01010608f43b0ba6acf860", "008083047a1d85d3567c30",
        "004041823d0ec2e9ab3e18", "002020c11e876174d59f0c", "001010608f43b0ba6acf86", "0008083047a1d85d3567c3",
        "0004041823d0ec2e9ab3e1", "0002020c11e876174d59f0", "0001010608f43b0ba6acf8", "00008083047a1d85d3567c",
        "00004041823d0ec2e9ab3e", "00002020c11e876174d59f", "00001010608f43b0ba6acf", "000008083047a1d85d3567",
        "000004041823d0ec2e9ab3", "000002020c11e876174d59", "000001010608f43b0ba6ac", "0000008083047a1d85d356",
        "0000004041823d0ec2e9ab", "0000002020c11e876174d5", "0000001010608f43b0ba6a", "00000008083047a1d85d35",
        "00000004041823d0ec2e9a", "00000002020c11e876174d", "00000001010608f43b0ba6", "000000008083047a1d85d3",
        "000000004041823d0ec2e9", "000000002020c11e876174", "000000001010608f43b0ba", "0000000008083047a1d85d",
        "0000000004041823d0ec2e", "0000000002020c11e87617", "0000000001010608f43b0b", "00000000008083047a1d85",
        "00000000004041823d0ec2", "00000000002020c11e8761", "00000000001010608f43b0", "000000000008083047a1d8",
        "000000000004041823d0ec", "000000000002020c11e876", "000000000001010608f43b", "0000000000008083047a1d",
        "0000000000004041823d0e", "0000000000002020c11e87", "0000000000001010608f43", "00000000000008083047a1",
        "00000000000004041823d0", "00000000000002020c11e8", "00000000000001010608f4", "000000000000008083047a",
        "000000000000004041823d", "000000000000002020c11e", "000000000000001010608f", "0000000000000008083047",
        "0000000000000004041823", "0000000000000002020c11", "0000000000000001010608", "0000000000000000808304",
        "0000000000000000404182", "00000000000000002020c1", "0000000000000000101060"
    };

    private static final byte[][] G_MATRIX = new byte[83][91];
    static {
        for (int r = 0; r < 83; ++r) {
            String hex = LDPC_G_HEX[r];
            for (int c = 0; c < 91; ++c) {
                int hexDigit = c / 4;
                int bitInDigit = 3 - (c % 4);
                if (hexDigit < hex.length()) {
                    int val = Character.digit(hex.charAt(hexDigit), 16);
                    G_MATRIX[r][c] = (byte) ((val >> bitInDigit) & 1);
                }
            }
        }
    }

    public static final int[][] LDPC_BIT_TERMS = {
        {15, 44, 72}, {24, 50, 61}, {32, 57, 77}, {0, 43, 44}, {1, 6, 60},
        {2, 5, 53}, {3, 34, 47}, {4, 12, 20}, {7, 55, 78}, {8, 63, 68},
        {9, 18, 65}, {10, 35, 59}, {11, 36, 57}, {13, 31, 42}, {14, 62, 79},
        {16, 27, 76}, {17, 73, 82}, {21, 52, 80}, {22, 29, 33}, {23, 30, 39},
        {25, 40, 75}, {26, 56, 69}, {28, 48, 64}, {2, 37, 77}, {4, 38, 81},
        {45, 49, 72}, {50, 51, 73}, {54, 70, 71}, {43, 66, 71}, {42, 67, 77},
        {0, 31, 58}, {1, 5, 70}, {3, 15, 53}, {6, 64, 66}, {7, 29, 41},
        {8, 21, 30}, {9, 17, 75}, {10, 22, 81}, {11, 27, 60}, {12, 51, 78},
        {13, 49, 50}, {14, 80, 82}, {16, 28, 59}, {18, 32, 63}, {19, 25, 72},
        {20, 33, 39}, {23, 26, 76}, {24, 54, 57}, {34, 52, 65}, {35, 47, 67},
        {36, 45, 74}, {37, 44, 46}, {38, 56, 68}, {40, 55, 61}, {19, 48, 52},
        {45, 51, 62}, {44, 69, 74}, {26, 34, 79}, {0, 14, 29}, {1, 67, 79},
        {2, 35, 50}, {3, 27, 50}, {4, 30, 55}, {5, 19, 36}, {6, 39, 81},
        {7, 59, 68}, {8, 9, 48}, {10, 43, 56}, {11, 38, 58}, {12, 23, 54},
        {13, 20, 64}, {15, 70, 77}, {16, 29, 75}, {17, 24, 79}, {18, 60, 82},
        {21, 37, 76}, {22, 40, 49}, {6, 25, 57}, {28, 31, 80}, {32, 39, 72},
        {17, 33, 47}, {12, 41, 63}, {4, 25, 42}, {46, 68, 71}, {53, 54, 69},
        {44, 61, 67}, {9, 62, 66}, {13, 65, 71}, {21, 59, 73}, {34, 38, 78},
        {0, 45, 63}, {0, 23, 65}, {1, 4, 69}, {2, 30, 64}, {3, 48, 57},
        {0, 3, 4}, {5, 59, 66}, {6, 31, 74}, {7, 47, 81}, {8, 34, 40},
        {9, 38, 61}, {10, 13, 60}, {11, 70, 73}, {12, 22, 77}, {10, 34, 54},
        {14, 15, 78}, {6, 8, 15}, {16, 53, 62}, {17, 49, 56}, {18, 29, 46},
        {19, 63, 79}, {20, 27, 68}, {21, 24, 42}, {12, 21, 36}, {1, 46, 50},
        {22, 53, 73}, {25, 33, 71}, {26, 35, 36}, {20, 35, 62}, {28, 39, 43},
        {18, 25, 56}, {2, 45, 81}, {13, 14, 57}, {32, 51, 52}, {29, 42, 51},
        {5, 8, 51}, {26, 32, 64}, {24, 68, 72}, {37, 54, 82}, {19, 38, 76},
        {17, 28, 55}, {31, 47, 70}, {41, 50, 58}, {27, 43, 78}, {33, 59, 61},
        {30, 44, 60}, {45, 67, 76}, {5, 23, 75}, {7, 9, 77}, {39, 40, 69},
        {16, 49, 52}, {41, 65, 67}, {3, 21, 71}, {35, 63, 80}, {12, 28, 46},
        {1, 7, 80}, {55, 66, 72}, {4, 37, 49}, {11, 37, 63}, {58, 71, 79},
        {2, 25, 78}, {44, 75, 80}, {0, 64, 73}, {6, 17, 76}, {10, 55, 58},
        {13, 38, 53}, {15, 36, 65}, {9, 27, 54}, {14, 59, 69}, {16, 24, 81},
        {19, 29, 30}, {11, 66, 67}, {22, 74, 79}, {26, 31, 61}, {23, 68, 74},
        {18, 20, 70}, {33, 52, 60}, {34, 45, 46}, {32, 58, 75}, {39, 42, 82},
        {40, 41, 62}, {48, 74, 82}, {19, 43, 47}, {41, 48, 56}
    };

    private static final int[][] CHECK_BITS = new int[83][7];
    private static final int[] CHECK_DEGREES = new int[83];

    static {
        for (int n = 0; n < 174; ++n) {
            for (int k = 0; k < 3; ++k) {
                int m = LDPC_BIT_TERMS[n][k];
                if (m >= 0 && m < 83) {
                    int deg = CHECK_DEGREES[m]++;
                    if (deg < 7) {
                        CHECK_BITS[m][deg] = n;
                    }
                }
            }
        }
    }

    public static void encodeLdpc(byte[] input91, byte[] codeword174) {
        Arrays.fill(codeword174, (byte) 0);
        LqCodec.BitBuffer bbIn = new LqCodec.BitBuffer(input91, 91);
        LqCodec.BitBuffer bbOut = new LqCodec.BitBuffer(codeword174, 174);

        int[] u = new int[91];
        for (int i = 0; i < 91; ++i) {
            u[i] = (int) bbIn.readBits(1);
            bbOut.writeBits(u[i], 1); // Systematic part
        }

        // Parity bits: p = G * u (mod 2)
        for (int r = 0; r < 83; ++r) {
            int p = 0;
            for (int c = 0; c < 91; ++c) {
                p ^= (G_MATRIX[r][c] & u[c]);
            }
            bbOut.writeBits(p, 1);
        }
    }

    public static int decodeLdpc(float[] llr, byte[] output91, int maxIters) {
        if (llr == null || llr.length < 174 || output91 == null) return -1;

        float[][] q = new float[174][3];
        float[][] r = new float[83][7];

        for (int n = 0; n < 174; ++n) {
            q[n][0] = llr[n];
            q[n][1] = llr[n];
            q[n][2] = llr[n];
        }

        final float ALPHA = 0.875f;

        for (int iter = 0; iter < maxIters; ++iter) {
            // Check node update
            for (int m = 0; m < 83; ++m) {
                int deg = CHECK_DEGREES[m];
                for (int i = 0; i < deg; ++i) {
                    float minMag = 1e9f;
                    int signProd = 1;

                    for (int j = 0; j < deg; ++j) {
                        if (i == j) continue;
                        int otherBit = CHECK_BITS[m][j];

                        int k = 0;
                        if (LDPC_BIT_TERMS[otherBit][1] == m) k = 1;
                        else if (LDPC_BIT_TERMS[otherBit][2] == m) k = 2;

                        float val = q[otherBit][k];
                        if (val < 0.0f) signProd = -signProd;
                        float mag = Math.abs(val);
                        if (mag < minMag) minMag = mag;
                    }

                    r[m][i] = ALPHA * (float) signProd * minMag;
                }
            }

            // Variable node update
            int[] cHat = new int[174];
            for (int n = 0; n < 174; ++n) {
                float totalLlr = llr[n];
                for (int k = 0; k < 3; ++k) {
                    int m = LDPC_BIT_TERMS[n][k];
                    int edgeIdx = 0;
                    for (int i = 0; i < CHECK_DEGREES[m]; ++i) {
                        if (CHECK_BITS[m][i] == n) {
                            edgeIdx = i;
                            break;
                        }
                    }
                    totalLlr += r[m][edgeIdx];
                }

                cHat[n] = (totalLlr < 0.0f) ? 1 : 0;

                for (int k = 0; k < 3; ++k) {
                    int m = LDPC_BIT_TERMS[n][k];
                    int edgeIdx = 0;
                    for (int i = 0; i < CHECK_DEGREES[m]; ++i) {
                        if (CHECK_BITS[m][i] == n) {
                            edgeIdx = i;
                            break;
                        }
                    }
                    q[n][k] = totalLlr - r[m][edgeIdx];
                }
            }

            // Syndrome check
            boolean syndromeOk = true;
            for (int m = 0; m < 83; ++m) {
                int sum = 0;
                for (int i = 0; i < CHECK_DEGREES[m]; ++i) {
                    sum ^= cHat[CHECK_BITS[m][i]];
                }
                if (sum != 0) {
                    syndromeOk = false;
                    break;
                }
            }

            if (syndromeOk) {
                Arrays.fill(output91, (byte) 0);
                LqCodec.BitBuffer bbOut = new LqCodec.BitBuffer(output91, 91);
                for (int i = 0; i < 91; ++i) {
                    bbOut.writeBits(cHat[i], 1);
                }
                return iter + 1; // Success!
            }
        }

        return -1; // Failed
    }

    public static int decodeLdpcHard(byte[] codeword174, byte[] output91) {
        LqCodec.BitBuffer bb = new LqCodec.BitBuffer(codeword174, 174);
        float[] llr = new float[174];
        for (int i = 0; i < 174; ++i) {
            long bit = bb.readBits(1);
            llr[i] = (bit == 0) ? +6.0f : -6.0f;
        }

        int iters = decodeLdpc(llr, output91, 25);
        if (iters > 0) return iters;

        // Fallback: systematic direct extraction
        Arrays.fill(output91, (byte) 0);
        LqCodec.BitBuffer bbIn = new LqCodec.BitBuffer(codeword174, 174);
        LqCodec.BitBuffer bbOut = new LqCodec.BitBuffer(output91, 91);
        for (int i = 0; i < 91; ++i) {
            bbOut.writeBits(bbIn.readBits(1), 1);
        }
        return verifyCrc14(output91) ? 0 : -1;
    }
}
