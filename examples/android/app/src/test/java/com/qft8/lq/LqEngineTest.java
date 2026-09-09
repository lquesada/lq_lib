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

import org.junit.Test;
import static org.junit.Assert.*;

public class LqEngineTest {

    @Test
    public void testEndToEndTonesLQ8() {
        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.CQ_STD;
        msg.call1 = "HB9IPH";
        msg.modifier = "DX";
        msg.locator = "JN47";

        int[] tones = LqEngine.encodeMessageToTones(msg, LqProtocol.Mode.LQ8);
        assertNotNull(tones);
        assertEquals(79, tones.length);

        LqMessage decoded = LqEngine.decodeTonesToMessage(tones, LqProtocol.Mode.LQ8);
        assertNotNull(decoded);
        assertEquals(msg.type, decoded.type);
        assertEquals("HB9IPH", decoded.call1);
        assertEquals("DX", decoded.modifier);
        assertEquals("JN47", decoded.locator);
    }

    @Test
    public void testEndToEndTonesLQ4() {
        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.CALL_STD;
        msg.call1 = "HB9IPH";
        msg.call2 = "YO1YO";
        msg.locator = "KL22";
        msg.rstDb = -3;

        int[] tones = LqEngine.encodeMessageToTones(msg, LqProtocol.Mode.LQ4);
        assertNotNull(tones);
        assertEquals(105, tones.length);

        LqMessage decoded = LqEngine.decodeTonesToMessage(tones, LqProtocol.Mode.LQ4);
        assertNotNull(decoded);
        assertEquals(msg.type, decoded.type);
        assertEquals("HB9IPH", decoded.call1);
        assertEquals("YO1YO", decoded.call2);
        assertEquals("KL22", decoded.locator);
        assertEquals(-3, decoded.rstDb);
    }

    @Test
    public void testEndToEndTonesLQ2() {
        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.REPLY73_STD;
        msg.call1 = "HB9IPH";
        msg.call2 = "YO1YO";
        msg.rstDb = 5;

        int[] tones = LqEngine.encodeMessageToTones(msg, LqProtocol.Mode.LQ2);
        assertNotNull(tones);
        assertEquals(105, tones.length);

        LqMessage decoded = LqEngine.decodeTonesToMessage(tones, LqProtocol.Mode.LQ2);
        assertNotNull(decoded);
        assertEquals(msg.type, decoded.type);
        assertEquals("HB9IPH", decoded.call1);
        assertEquals("YO1YO", decoded.call2);
        assertEquals(5, decoded.rstDb);
    }

    @Test
    public void testAudioWaveformLoopback() {
        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.CQ_STD;
        msg.call1 = "HB9IPH";
        msg.locator = "JN47";

        int[] tones = LqEngine.encodeMessageToTones(msg, LqProtocol.Mode.LQ8);
        assertNotNull(tones);

        float[] audio = LqEngine.generateAudio(tones, LqProtocol.Mode.LQ8, 1500.0f, 12000.0f);
        assertTrue(audio.length > 0);

        int[] detectedTones = LqEngine.demodulateAudio(audio, 0, 1500.0f, 12000.0f, LqProtocol.Mode.LQ8);
        assertNotNull(detectedTones);
        assertArrayEquals(tones, detectedTones);

        LqMessage decoded = LqEngine.attemptDecodeFromAudio(audio, 0, 1500.0f, 12000.0f, LqProtocol.Mode.LQ8);
        assertNotNull(decoded);
        assertEquals("HB9IPH", decoded.call1);
        assertEquals("JN47", decoded.locator);
    }

    @Test
    public void testFastFftPeakDetection() {
        FastFft fft = new FastFft(1024);
        float sampleRate = 12000.0f;
        float targetFreq = 1500.0f;

        float[] input = new float[1024];
        for (int i = 0; i < 1024; i++) {
            input[i] = (float) Math.sin(2.0 * Math.PI * targetFreq * (i / sampleRate));
        }

        float[] real = new float[1024];
        float[] imag = new float[1024];
        float[] powerDb = new float[512];

        fft.computeSpectrum(input, real, imag, powerDb);

        float maxPower = -999.0f;
        int peakBin = 0;
        for (int i = 0; i < 512; i++) {
            if (powerDb[i] > maxPower) {
                maxPower = powerDb[i];
                peakBin = i;
            }
        }

        float binWidth = sampleRate / 1024.0f;
        float detectedFreq = peakBin * binWidth;
        assertEquals(targetFreq, detectedFreq, binWidth * 1.5f);
    }

    @Test
    public void testSlidingWindowAudioDecodeWithOffset() {
        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.FREE_TEXT;
        msg.text = "73 DE HB9IP";

        int[] tones = LqEngine.encodeMessageToTones(msg, LqProtocol.Mode.LQ8);
        assertNotNull(tones);

        float[] txAudio = LqEngine.generateAudio(tones, LqProtocol.Mode.LQ8, 1500.0f, 12000.0f);
        assertTrue(txAudio.length > 0);

        // Prepend 1.0 second of silence (12,000 samples)
        int silenceSamples = 12000;
        float[] delayedAudio = new float[silenceSamples + txAudio.length + 6000];
        System.arraycopy(txAudio, 0, delayedAudio, silenceSamples, txAudio.length);

        // Attempt decode over delayed stream
        LqMessage decoded = LqEngine.attemptDecodeFromAudio(delayedAudio, 0, 1500.0f, 12000.0f, LqProtocol.Mode.LQ8);
        assertNotNull(decoded);
        assertEquals(LqMessage.Type.FREE_TEXT, decoded.type);
        assertEquals("73 DE HB9IP", decoded.text);
    }

    @Test
    public void testSilenceAndAllZerosRejection() {
        // 1. All zero audio samples
        float[] silence = new float[12000 * 13];
        LqMessage decoded = LqEngine.attemptDecodeFromAudio(silence, 0, 1500.0f, 12000.0f, LqProtocol.Mode.LQ8);
        assertNull("Silence must not decode as a message", decoded);

        // 2. All zero tones
        int[] allZeros = new int[79];
        LqMessage decodedTones = LqEngine.decodeTonesToMessage(allZeros, LqProtocol.Mode.LQ8);
        assertNull("All zeros tone array must not decode as CQ 00 AA00", decodedTones);
    }

    @Test
    public void testRandomCallsignAndGridGeneration() {
        java.util.Random rnd = new java.util.Random(12345);
        for (int i = 0; i < 50; i++) {
            String call = MainActivity.generateRandomCallsign(rnd);
            assertNotNull(call);
            assertTrue("Callsign must have standard length 3-6: " + call, call.length() >= 3 && call.length() <= 6);
            assertTrue("Callsign must encode to standard 28-bit: " + call, LqCodec.encodeStandardCallsign(call) > 0);

            String grid = MainActivity.generateRandomGrid(rnd);
            assertNotNull(grid);
            assertEquals("Grid must be 4 characters: " + grid, 4, grid.length());
            assertTrue("Grid must encode: " + grid, LqCodec.encodeLocator(grid) >= 0);
        }
    }

    @Test
    public void testAllModesAudioDurationAndLoopback() {
        LqProtocol.Mode[] modes = {
            LqProtocol.Mode.LQ8,
            LqProtocol.Mode.LQ4,
            LqProtocol.Mode.LQ2,
            LqProtocol.Mode.LQ16
        };

        for (LqProtocol.Mode mode : modes) {
            LqMessage msg = new LqMessage();
            msg.type = LqMessage.Type.CQ_STD;
            msg.call1 = "HB9IPH";
            msg.locator = "JN47";

            int[] tones = LqEngine.encodeMessageToTones(msg, mode);
            assertNotNull("Tones must not be null for mode: " + mode.name, tones);
            assertEquals("Total symbols must match mode definition: " + mode.name, mode.totalSymbols, tones.length);

            float[] audio = LqEngine.generateAudio(tones, mode, 1500.0f, 12000.0f);
            assertNotNull("Audio must not be null for mode: " + mode.name, audio);

            float expectedSeconds = mode.totalSymbols * mode.symbolPeriod;
            float actualSeconds = audio.length / 12000.0f;
            assertEquals("Audio duration must match symbol period * total symbols for " + mode.name,
                    expectedSeconds, actualSeconds, 0.05f);

            LqMessage decoded = LqEngine.attemptDecodeFromAudio(audio, 0, 1500.0f, 12000.0f, mode);
            assertNotNull("Decode must succeed for mode: " + mode.name, decoded);
            assertEquals("HB9IPH", decoded.call1);
            assertEquals("JN47", decoded.locator);
        }
    }

    @Test
    public void testCallMessageAudioLoopback() {
        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.CALL_STD;
        msg.call1 = "YO1YO";
        msg.call2 = "TU2TU";
        msg.locator = "KL22";
        msg.rstDb = -3;

        int[] tones = LqEngine.encodeMessageToTones(msg, LqProtocol.Mode.LQ8);
        assertNotNull(tones);

        float[] audio = LqEngine.generateAudio(tones, LqProtocol.Mode.LQ8, 1500.0f, 12000.0f);
        assertNotNull(audio);

        LqMessage decoded = LqEngine.attemptDecodeFromAudio(audio, 0, 1500.0f, 12000.0f, LqProtocol.Mode.LQ8);
        assertNotNull("CALL message must decode from audio", decoded);
        assertEquals(LqMessage.Type.CALL_STD, decoded.type);
        assertEquals("YO1YO", decoded.call1);
        assertEquals("TU2TU", decoded.call2);
        assertEquals("KL22", decoded.locator);
        assertEquals(-3, decoded.rstDb);
        assertEquals("YO1YO TU2TU KL22 -03", decoded.format());
    }

    @Test
    public void testReply73AudioLoopback() {
        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.REPLY73_STD;
        msg.call1 = "YO1YO";
        msg.call2 = "TU2TU";
        msg.rstDb = 5;

        int[] tones = LqEngine.encodeMessageToTones(msg, LqProtocol.Mode.LQ8);
        assertNotNull(tones);

        float[] audio = LqEngine.generateAudio(tones, LqProtocol.Mode.LQ8, 1500.0f, 12000.0f);
        assertNotNull(audio);

        LqMessage decoded = LqEngine.attemptDecodeFromAudio(audio, 0, 1500.0f, 12000.0f, LqProtocol.Mode.LQ8);
        assertNotNull("REPLY73 message must decode from audio", decoded);
        assertEquals(LqMessage.Type.REPLY73_STD, decoded.type);
        assertEquals("YO1YO", decoded.call1);
        assertEquals("TU2TU", decoded.call2);
        assertEquals(5, decoded.rstDb);
        assertEquals("YO1YO TU2TU R+05", decoded.format());
    }
}
