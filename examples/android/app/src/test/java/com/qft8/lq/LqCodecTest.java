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

import java.math.BigInteger;

public class LqCodecTest {

    @Test
    public void testStandardCallsign() {
        String[] calls = {"W1AW", "K1JT", "HB9IPH", "YO1YO", "3D2AG", "9A1A", "GB2USA", "DE", "QRZ", "CQ"};
        for (String c : calls) {
            long enc = LqCodec.encodeStandardCallsign(c);
            String dec = LqCodec.decodeStandardCallsign(enc);
            assertEquals(c, dec);
        }
    }

    @Test
    public void testNonStandardCallsigns() {
        String[] calls = {"YO1YO/P", "EA6/W1AW", "EA6/W1AW/P", "3B9/HB9IPH/P"};
        int[] lengths = {7, 9, 10, 14};

        for (int i = 0; i < calls.length; ++i) {
            BigInteger enc = LqCodec.encodeNonStdCallsign(calls[i], lengths[i]);
            String dec = LqCodec.decodeNonStdCallsign(enc, lengths[i]);
            assertEquals(calls[i], dec);
        }
    }

    @Test
    public void testCallsignHash() {
        int h1 = LqCodec.hashCallsign("W1AW");
        int h2 = LqCodec.hashCallsign("w1aw");
        assertEquals(h1, h2);
        assertTrue(h1 > 0);
    }

    @Test
    public void testMaidenheadLocator() {
        String[] locs = {"JN47", "KL22", "FN31", "JM19", "RR99", "AA00"};
        for (String loc : locs) {
            int enc = LqCodec.encodeLocator(loc);
            String dec = LqCodec.decodeLocator(enc);
            assertEquals(loc, dec);
        }
    }

    @Test
    public void testRstSignalReport() {
        for (int rst = -26; rst <= 5; ++rst) {
            int enc = LqCodec.encodeRst(rst);
            int dec = LqCodec.decodeRst(enc);
            assertEquals(rst, dec);
        }
    }

    @Test
    public void testVaricodeWithInvalidFiller() {
        byte[] buffer = new byte[10];
        LqCodec.BitBuffer bb = new LqCodec.BitBuffer(buffer, 75);
        LqCodec.encodeVaricode("73 DE HB9IP", bb, 75);

        LqCodec.BitBuffer bbDec = new LqCodec.BitBuffer(buffer, 75);
        String dec = LqCodec.decodeVaricode(bbDec, 75);
        assertEquals("73 DE HB9IP", dec);

        // Test @ and . characters
        byte[] buf2 = new byte[10];
        LqCodec.BitBuffer bb2 = new LqCodec.BitBuffer(buf2, 75);
        LqCodec.encodeVaricode("@CQ HI.", bb2, 75);
        LqCodec.BitBuffer bbDec2 = new LqCodec.BitBuffer(buf2, 75);
        String dec2 = LqCodec.decodeVaricode(bbDec2, 75);
        assertEquals("@CQ HI.", dec2);
    }

    @Test
    public void testMessageEncodeDecode() {
        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.CALL_STD;
        msg.call1 = "HB9IPH";
        msg.call2 = "YO1YO";
        msg.locator = "JN47";
        msg.rstDb = -3;

        byte[] payload = new byte[10];
        assertTrue(LqCodec.encodeMessage(msg, payload));

        LqMessage decoded = new LqMessage();
        assertTrue(LqCodec.decodeMessage(payload, decoded));

        assertEquals(LqMessage.Type.CALL_STD, decoded.type);
        assertEquals("HB9IPH", decoded.call1);
        assertEquals("YO1YO", decoded.call2);
        assertEquals("JN47", decoded.locator);
        assertEquals(-3, decoded.rstDb);
    }

    @Test
    public void test24BitHash() {
        int h1 = LqCodec.hashCallsign24("HB9IPH");
        int h2 = LqCodec.hashCallsign24("hb9iph");
        assertEquals(h1, h2);
        assertTrue(h1 >= 0 && h1 < 16777216);
    }

    @Test
    public void testMultiReply73() {
        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.MULTI_REPLY73;
        msg.call1 = "HB9IPH";
        msg.hash1 = LqCodec.hashCallsign24("HB9IPH") & 0xFFF;
        msg.multiTargets.add(new LqMessage.MultiTarget("YO1YO", LqCodec.hashCallsign24("YO1YO"), 5));
        msg.multiTargets.add(new LqMessage.MultiTarget("TU2TU", LqCodec.hashCallsign24("TU2TU"), -3));

        byte[] payload = new byte[10];
        assertTrue(LqCodec.encodeMessage(msg, payload));

        LqMessage decoded = new LqMessage();
        assertTrue(LqCodec.decodeMessage(payload, decoded));

        assertEquals(LqMessage.Type.MULTI_REPLY73, decoded.type);
        assertEquals(msg.hash1, decoded.hash1);
        assertEquals(2, decoded.multiTargets.size());
        assertEquals(msg.multiTargets.get(0).hash, decoded.multiTargets.get(0).hash);
        assertEquals(5, decoded.multiTargets.get(0).rstDb);
        assertEquals(msg.multiTargets.get(1).hash, decoded.multiTargets.get(1).hash);
        assertEquals(-3, decoded.multiTargets.get(1).rstDb);
    }
}
