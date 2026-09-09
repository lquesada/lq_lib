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

public class LqEngine {

    public static final float DEFAULT_SAMPLE_RATE = 12000.0f;
    public static final float DEFAULT_BASE_FREQ = 1500.0f;
    private static final double TWO_PI = 2.0 * Math.PI;

    public static int[] encodeMessageToTones(LqMessage msg, LqProtocol.Mode mode) {
        if (LqNative.isAvailable()) {
            int modeId = (mode == LqProtocol.Mode.LQ8) ? 1 : (mode == LqProtocol.Mode.LQ4 ? 2 : (mode == LqProtocol.Mode.LQ2 ? 3 : 4));
            int[] nativeTones = LqNative.encodeTones(msg, modeId);
            if (nativeTones != null) return nativeTones;
        }

        byte[] payload = new byte[10];
        if (!LqCodec.encodeMessage(msg, payload)) return null;

        byte[] input91 = new byte[12];
        LqLdpc.appendCrc14(payload, input91);

        byte[] codeword174 = new byte[22];
        LqLdpc.encodeLdpc(input91, codeword174);

        int[] tones = new int[mode.totalSymbols];
        LqCodec.BitBuffer bb = new LqCodec.BitBuffer(codeword174, 174);

        if (mode == LqProtocol.Mode.LQ8 || mode == LqProtocol.Mode.LQ16) {
            int[] dataTones = new int[58];
            for (int i = 0; i < 58; ++i) {
                int val3 = (int) bb.readBits(3);
                dataTones[i] = LqProtocol.GRAY_MAP_8[val3 & 7];
            }
            // Sync 1 (0..6)
            System.arraycopy(LqProtocol.COSTAS_ARRAY_8, 0, tones, 0, 7);
            // Data Block 1 (7..35)
            System.arraycopy(dataTones, 0, tones, 7, 29);
            // Sync 2 (36..42)
            System.arraycopy(LqProtocol.COSTAS_ARRAY_8, 0, tones, 36, 7);
            // Data Block 2 (43..71)
            System.arraycopy(dataTones, 29, tones, 43, 29);
            // Sync 3 (72..78)
            System.arraycopy(LqProtocol.COSTAS_ARRAY_8, 0, tones, 72, 7);
        } else { // LQ4 or LQ2
            int[] dataTones = new int[87];
            for (int i = 0; i < 87; ++i) {
                int val2 = (int) bb.readBits(2);
                dataTones[i] = LqProtocol.GRAY_MAP_4[val2 & 3];
            }
            tones[0] = 0; // Ramp
            System.arraycopy(LqProtocol.COSTAS_SYNC1_4, 0, tones, 1, 4);
            System.arraycopy(dataTones, 0, tones, 5, 29);
            System.arraycopy(LqProtocol.COSTAS_SYNC2_4, 0, tones, 34, 4);
            System.arraycopy(dataTones, 29, tones, 38, 29);
            System.arraycopy(LqProtocol.COSTAS_SYNC3_4, 0, tones, 67, 4);
            System.arraycopy(dataTones, 58, tones, 71, 29);
            System.arraycopy(LqProtocol.COSTAS_SYNC4_4, 0, tones, 100, 4);
            tones[104] = 0; // Ramp
        }

        return tones;
    }

    public static boolean verifySyncTones(int[] tones, LqProtocol.Mode mode) {
        if (tones == null) return false;
        if (mode == LqProtocol.Mode.LQ8 || mode == LqProtocol.Mode.LQ16) {
            if (tones.length < 79) return false;
            int match = 0;
            for (int i = 0; i < 7; ++i) {
                if (tones[i] == LqProtocol.COSTAS_ARRAY_8[i]) match++;
                if (tones[36 + i] == LqProtocol.COSTAS_ARRAY_8[i]) match++;
                if (tones[72 + i] == LqProtocol.COSTAS_ARRAY_8[i]) match++;
            }
            return match >= 10;
        } else if (mode == LqProtocol.Mode.LQ4 || mode == LqProtocol.Mode.LQ2) {
            if (tones.length < 105) return false;
            int match = 0;
            for (int i = 0; i < 4; ++i) {
                if (tones[1 + i] == LqProtocol.COSTAS_SYNC1_4[i]) match++;
                if (tones[34 + i] == LqProtocol.COSTAS_SYNC2_4[i]) match++;
                if (tones[67 + i] == LqProtocol.COSTAS_SYNC3_4[i]) match++;
                if (tones[100 + i] == LqProtocol.COSTAS_SYNC4_4[i]) match++;
            }
            return match >= 8;
        }
        return false;
    }

    public static LqMessage decodeTonesToMessage(int[] tones, LqProtocol.Mode mode) {
        if (tones == null) return null;
        if (!verifySyncTones(tones, mode)) return null;

        if (LqNative.isAvailable()) {
            int modeId = (mode == LqProtocol.Mode.LQ8) ? 1 : (mode == LqProtocol.Mode.LQ4 ? 2 : (mode == LqProtocol.Mode.LQ2 ? 3 : 4));
            LqMessage nativeMsg = new LqMessage();
            if (LqNative.decodeTones(tones, modeId, nativeMsg)) {
                return nativeMsg;
            }
        }

        byte[] codeword174 = new byte[22];
        LqCodec.BitBuffer bb = new LqCodec.BitBuffer(codeword174, 174);

        if (mode == LqProtocol.Mode.LQ8 || mode == LqProtocol.Mode.LQ16) {
            for (int i = 0; i < 29; ++i) {
                int tone = tones[7 + i] & 7;
                bb.writeBits(LqProtocol.GRAY_INV_MAP_8[tone], 3);
            }
            for (int i = 0; i < 29; ++i) {
                int tone = tones[43 + i] & 7;
                bb.writeBits(LqProtocol.GRAY_INV_MAP_8[tone], 3);
            }
        } else { // LQ4 or LQ2
            for (int i = 0; i < 29; ++i) {
                int tone = tones[5 + i] & 3;
                bb.writeBits(LqProtocol.GRAY_INV_MAP_4[tone], 2);
            }
            for (int i = 0; i < 29; ++i) {
                int tone = tones[38 + i] & 3;
                bb.writeBits(LqProtocol.GRAY_INV_MAP_4[tone], 2);
            }
            for (int i = 0; i < 29; ++i) {
                int tone = tones[71 + i] & 3;
                bb.writeBits(LqProtocol.GRAY_INV_MAP_4[tone], 2);
            }
        }

        byte[] output91 = new byte[12];
        if (LqLdpc.decodeLdpcHard(codeword174, output91) < 0) return null;
        if (!LqLdpc.verifyCrc14(output91)) return null;

        byte[] payload77 = new byte[10];
        LqLdpc.extractPayload(output91, payload77);

        LqMessage msg = new LqMessage();
        if (!LqCodec.decodeMessage(payload77, msg)) return null;
        return msg;
    }

    public static float[] generateAudio(int[] tones, LqProtocol.Mode mode, float baseFreq, float sampleRate) {
        if (tones == null) return new float[0];
        if (LqNative.isAvailable()) {
            int modeId = (mode == LqProtocol.Mode.LQ8) ? 1 : (mode == LqProtocol.Mode.LQ4 ? 2 : (mode == LqProtocol.Mode.LQ2 ? 3 : 4));
            float[] nativeAudio = LqNative.generateAudio(tones, modeId, baseFreq, sampleRate);
            if (nativeAudio != null) return nativeAudio;
        }

        int samplesPerSym = Math.round(mode.symbolPeriod * sampleRate);
        int totalSamples = samplesPerSym * tones.length;
        float[] audio = new float[totalSamples];

        double phase = 0.0;
        double twoPi = 2.0 * Math.PI;

        for (int i = 0; i < tones.length; ++i) {
            int tone = tones[i];
            double freq = (double) baseFreq + (double) tone * (double) mode.toneSpacing;
            double phaseInc = twoPi * freq / (double) sampleRate;

            int startIdx = i * samplesPerSym;
            for (int n = 0; n < samplesPerSym; ++n) {
                audio[startIdx + n] = (float) Math.sin(phase);
                phase += phaseInc;
                if (phase >= twoPi) phase -= twoPi;
            }
        }

        return audio;
    }

    public static int[] demodulateAudio(float[] samples, int offset, float baseFreq, float sampleRate, LqProtocol.Mode mode) {
        int samplesPerSym = Math.round(mode.symbolPeriod * sampleRate);
        if (samplesPerSym <= 0) return null;
        int totalSamplesNeeded = samplesPerSym * mode.totalSymbols;
        if (samples.length - offset < totalSamplesNeeded) return null;

        // Reject silence / zero buffer
        double totalEnergy = 0.0;
        for (int i = 0; i < totalSamplesNeeded; ++i) {
            float s = samples[offset + i];
            totalEnergy += s * s;
        }
        if (totalEnergy < 1e-5) return null;

        double dt = 1.0 / (double) sampleRate;
        int[] tones = new int[mode.totalSymbols];

        for (int sym = 0; sym < mode.totalSymbols; ++sym) {
            int startSample = offset + sym * samplesPerSym;
            double maxEnergy = -1.0;
            int bestTone = 0;

            for (int tone = 0; tone < mode.numTones; ++tone) {
                double toneFreq = (double) baseFreq + (double) tone * (double) mode.toneSpacing;
                double omega = TWO_PI * toneFreq * dt;

                double sumCos = 0.0;
                double sumSin = 0.0;

                for (int n = 0; n < samplesPerSym; ++n) {
                    float s = samples[startSample + n];
                    double angle = omega * (double) n;
                    sumCos += s * Math.cos(angle);
                    sumSin += s * Math.sin(angle);
                }

                double energy = sumCos * sumCos + sumSin * sumSin;
                if (energy > maxEnergy) {
                    maxEnergy = energy;
                    bestTone = tone;
                }
            }
            tones[sym] = bestTone;
        }

        return tones;
    }

    public static LqMessage attemptDecodeFromAudio(float[] samples, int offset, float baseFreq, float sampleRate, LqProtocol.Mode mode) {
        if (samples == null) return null;
        int samplesPerSym = Math.round(mode.symbolPeriod * sampleRate);
        if (samplesPerSym <= 0) return null;
        int totalSamplesNeeded = samplesPerSym * mode.totalSymbols;
        if (samples.length < totalSamplesNeeded) return null;

        if (LqNative.isAvailable()) {
            try {
                int modeId = (mode == LqProtocol.Mode.LQ8) ? 1 : (mode == LqProtocol.Mode.LQ4 ? 2 : (mode == LqProtocol.Mode.LQ2 ? 3 : 4));
                LqMessage outMsg = new LqMessage();
                if (LqNative.decodeAudio(samples, baseFreq, sampleRate, modeId, outMsg)) {
                    return outMsg;
                }
            } catch (Throwable ignored) {}
            return null;
        }

        // 1. Try given offset first
        if (offset >= 0 && samples.length - offset >= totalSamplesNeeded) {
            int[] tones = demodulateAudio(samples, offset, baseFreq, sampleRate, mode);
            if (tones != null) {
                LqMessage msg = decodeTonesToMessage(tones, mode);
                if (msg != null) return msg;
            }
        }

        // 2. Scan across time offsets and frequency search grid
        float[] freqGrid;
        if (baseFreq > 0.0f) {
            float fStep = Math.max(1.0f, mode.toneSpacing / 4.0f);
            int count = (int) (100.0f / fStep) + 1;
            freqGrid = new float[count];
            for (int i = 0; i < count; ++i) {
                freqGrid[i] = (baseFreq - 50.0f) + i * fStep;
            }
        } else {
            float fStep = Math.max(2.0f, mode.toneSpacing / 2.0f);
            int count = (int) (2350.0f / fStep) + 1;
            freqGrid = new float[count];
            for (int i = 0; i < count; ++i) {
                freqGrid[i] = 250.0f + i * fStep;
            }
        }

        int step = Math.max(1, samplesPerSym / 4);
        int maxOffset = samples.length - totalSamplesNeeded;

        int[] syncPattern = (mode == LqProtocol.Mode.LQ8 || mode == LqProtocol.Mode.LQ16) ? LqProtocol.COSTAS_ARRAY_8 : LqProtocol.COSTAS_SYNC1_4;
        int syncLen = syncPattern.length;
        int syncStart = (mode == LqProtocol.Mode.LQ8 || mode == LqProtocol.Mode.LQ16) ? 0 : 1;

        for (float fCand : freqGrid) {
            if (fCand < 100.0f || fCand > (sampleRate / 2.0f - 100.0f)) continue;

            for (int off = 0; off <= maxOffset; off += step) {
                int matchCount = 0;
                for (int s = 0; s < syncLen; ++s) {
                    int startSample = off + (syncStart + s) * samplesPerSym;
                    int dominant = getDominantTone(samples, startSample, samplesPerSym, fCand, sampleRate, mode);
                    if (dominant == syncPattern[s]) {
                        matchCount++;
                    }
                }

                if (matchCount >= (syncLen - 1)) {
                    int[] tones = demodulateAudio(samples, off, fCand, sampleRate, mode);
                    if (tones != null) {
                        LqMessage msg = decodeTonesToMessage(tones, mode);
                        if (msg != null) return msg;
                    }
                }
            }
        }

        return null;
    }

    private static int getDominantTone(float[] samples, int startSample, int samplesPerSym, float baseFreq, float sampleRate, LqProtocol.Mode mode) {
        if (startSample < 0 || startSample + samplesPerSym > samples.length) return -1;
        double dt = 1.0 / (double) sampleRate;
        double maxEnergy = -1.0;
        int bestTone = -1;

        for (int tone = 0; tone < mode.numTones; ++tone) {
            double toneFreq = (double) baseFreq + (double) tone * (double) mode.toneSpacing;
            double omega = TWO_PI * toneFreq * dt;

            double sumCos = 0.0;
            double sumSin = 0.0;

            for (int n = 0; n < samplesPerSym; ++n) {
                float s = samples[startSample + n];
                double angle = omega * (double) n;
                sumCos += (double) s * Math.cos(angle);
                sumSin += (double) s * Math.sin(angle);
            }

            double energy = sumCos * sumCos + sumSin * sumSin;
            if (energy > maxEnergy) {
                maxEnergy = energy;
                bestTone = tone;
            }
        }
        return bestTone;
    }
}
