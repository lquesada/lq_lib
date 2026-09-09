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

import java.math.BigInteger;
import java.util.Arrays;
import java.util.Locale;

public class LqCodec {

    public static final int PAYLOAD_BITS = 77;
    public static final int PAYLOAD_BYTES = 10;
    public static final String NONSTD_ALPHABET = " ABCDEFGHIJKLMNOPQRSTUVWXYZ/0123456789";
    public static final String VARICODE_ALPHABET = " \u2301ETAOINSHRDLCUMWFGYPBVK051@/-JXQZ2347896:.?,><=*_+;\"'&$|{}^~";

    // -----------------------------------------------------------------------
    // BitBuffer (MSB-first)
    // -----------------------------------------------------------------------
    public static class BitBuffer {
        private final byte[] buffer;
        private final int capacityBits;
        private int bitPos;

        public BitBuffer(byte[] buffer, int capacityBits) {
            this.buffer = buffer;
            this.capacityBits = capacityBits;
            this.bitPos = 0;
        }

        public int getPos() { return bitPos; }
        public void setPos(int pos) { this.bitPos = Math.min(pos, capacityBits); }

        public void writeBits(long value, int count) {
            for (int i = count - 1; i >= 0; --i) {
                if (bitPos >= capacityBits) return;
                int byteIdx = bitPos / 8;
                int bitOffset = 7 - (bitPos % 8);
                long bit = (value >> i) & 1L;
                if (bit != 0) {
                    buffer[byteIdx] |= (byte) (1 << bitOffset);
                } else {
                    buffer[byteIdx] &= (byte) ~(1 << bitOffset);
                }
                bitPos++;
            }
        }

        public long readBits(int count) {
            long result = 0;
            for (int i = 0; i < count; ++i) {
                if (bitPos >= capacityBits) break;
                int byteIdx = bitPos / 8;
                int bitOffset = 7 - (bitPos % 8);
                long bit = (buffer[byteIdx] >> bitOffset) & 1L;
                result = (result << 1) | bit;
                bitPos++;
            }
            return result;
        }

        public void padZeros() {
            while (bitPos < capacityBits) {
                writeBits(0, 1);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Standard Callsign (28 bits)
    // -----------------------------------------------------------------------
    public static final long CALLSIGN_TOKEN_DE  = 262177560L;
    public static final long CALLSIGN_TOKEN_QRZ = 262177561L;
    public static final long CALLSIGN_TOKEN_CQ  = 262177562L;
    public static final long CALLSIGN_MAX_STD   = 262177559L;

    private static final long RADIX_S3 = 1L;
    private static final long RADIX_S2 = 27L;
    private static final long RADIX_S1 = 27L * 27L;
    private static final long RADIX_D  = 27L * 27L * 27L;
    private static final long RADIX_C2 = 10L * RADIX_D;
    private static final long RADIX_C1 = 36L * RADIX_C2;

    public static boolean isStandardCallsign(String call) {
        return encodeStandardCallsign(call) > 0;
    }

    public static long encodeStandardCallsign(String call) {
        if (call == null) return 0;
        String c = call.trim().toUpperCase(Locale.ROOT);
        if (c.equals("DE"))  return CALLSIGN_TOKEN_DE;
        if (c.equals("QRZ")) return CALLSIGN_TOKEN_QRZ;
        if (c.equals("CQ"))  return CALLSIGN_TOKEN_CQ;

        // Find separating digit: 1-3 letters after it
        int digitIdx = -1;
        for (int i = 1; i < c.length(); ++i) {
            if (Character.isDigit(c.charAt(i))) {
                int lettersAfter = c.length() - 1 - i;
                if (lettersAfter >= 1 && lettersAfter <= 3) {
                    boolean allLetters = true;
                    for (int j = i + 1; j < c.length(); ++j) {
                        if (!Character.isLetter(c.charAt(j))) {
                            allLetters = false;
                            break;
                        }
                    }
                    if (allLetters) {
                        digitIdx = i;
                        break;
                    }
                }
            }
        }
        if (digitIdx == -1) return 0;

        String prefix = c.substring(0, digitIdx);
        char digit = c.charAt(digitIdx);
        String suffix = c.substring(digitIdx + 1);

        if (prefix.length() < 1 || prefix.length() > 2) return 0;
        if (suffix.length() < 1 || suffix.length() > 3) return 0;

        char c1 = (prefix.length() == 1) ? ' ' : prefix.charAt(0);
        char c2 = (prefix.length() == 1) ? prefix.charAt(0) : prefix.charAt(1);
        char d = digit;
        char s1 = suffix.charAt(0);
        char s2 = (suffix.length() >= 2) ? suffix.charAt(1) : ' ';
        char s3 = (suffix.length() >= 3) ? suffix.charAt(2) : ' ';

        int nc1 = charToRadix37(c1);
        int nc2 = charToRadix36(c2);
        int nd  = d - '0';
        int ns1 = charToRadix27(s1);
        int ns2 = charToRadix27(s2);
        int ns3 = charToRadix27(s3);

        return nc1 * RADIX_C1
             + nc2 * RADIX_C2
             + nd  * RADIX_D
             + ns1 * RADIX_S1
             + ns2 * RADIX_S2
             + ns3 * RADIX_S3;
    }

    public static String decodeStandardCallsign(long packed) {
        if (packed == CALLSIGN_TOKEN_DE)  return "DE";
        if (packed == CALLSIGN_TOKEN_QRZ) return "QRZ";
        if (packed == CALLSIGN_TOKEN_CQ)  return "CQ";
        if (packed > CALLSIGN_MAX_STD)    return "";

        long rem = packed;
        int nc1 = (int) (rem / RADIX_C1); rem %= RADIX_C1;
        int nc2 = (int) (rem / RADIX_C2); rem %= RADIX_C2;
        int nd  = (int) (rem / RADIX_D);  rem %= RADIX_D;
        int ns1 = (int) (rem / RADIX_S1); rem %= RADIX_S1;
        // Standard amateur callsigns must have at least 1 suffix letter (ns1 >= 1)
        if (ns1 == 0) return "";

        int ns2 = (int) (rem / RADIX_S2); rem %= RADIX_S2;
        int ns3 = (int) (rem / RADIX_S3);

        StringBuilder sb = new StringBuilder();
        char c1 = radix37ToChar(nc1);
        char c2 = radix36ToChar(nc2);
        char s1 = radix27ToChar(ns1);
        char s2 = radix27ToChar(ns2);
        char s3 = radix27ToChar(ns3);

        if (c1 != ' ') sb.append(c1);
        if (c2 != ' ') sb.append(c2);
        sb.append((char) ('0' + nd));
        if (s1 != ' ') sb.append(s1);
        if (s2 != ' ') sb.append(s2);
        if (s3 != ' ') sb.append(s3);

        return sb.toString().trim();
    }

    private static int charToRadix37(char c) {
        if (c == ' ') return 0;
        if (c >= '0' && c <= '9') return c - '0' + 1;
        if (c >= 'A' && c <= 'Z') return c - 'A' + 11;
        return 0;
    }
    private static char radix37ToChar(int n) {
        if (n == 0) return ' ';
        if (n >= 1 && n <= 10) return (char) ('0' + n - 1);
        return (char) ('A' + n - 11);
    }
    private static int charToRadix36(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
        return 0;
    }
    private static char radix36ToChar(int n) {
        if (n <= 9) return (char) ('0' + n);
        return (char) ('A' + n - 10);
    }
    private static int charToRadix10(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        return 0;
    }
    private static char radix10ToChar(int n) { return (char) ('0' + (n % 10)); }
    private static int charToRadix27(char c) {
        if (c == ' ') return 0;
        if (c >= 'A' && c <= 'Z') return c - 'A' + 1;
        return 0;
    }
    private static char radix27ToChar(int n) {
        if (n == 0) return ' ';
        return (char) ('A' + n - 1);
    }

    // -----------------------------------------------------------------------
    // Non-Standard Callsign (Base-38 BigInteger)
    // -----------------------------------------------------------------------
    public static BigInteger encodeNonStdCallsign(String call, int maxChars) {
        if (call == null) return BigInteger.ZERO;
        String c = call.trim().toUpperCase(Locale.ROOT);
        BigInteger result = BigInteger.ZERO;
        BigInteger base = BigInteger.valueOf(38);

        for (int i = 0; i < maxChars; ++i) {
            char ch = (i < c.length()) ? c.charAt(i) : ' ';
            int idx = NONSTD_ALPHABET.indexOf(ch);
            if (idx < 0) idx = 0;
            result = result.multiply(base).add(BigInteger.valueOf(idx));
        }
        return result;
    }

    public static String decodeNonStdCallsign(BigInteger value, int maxChars) {
        char[] chars = new char[maxChars];
        BigInteger v = value;
        BigInteger base = BigInteger.valueOf(38);

        for (int i = maxChars - 1; i >= 0; --i) {
            BigInteger[] dr = v.divideAndRemainder(base);
            int idx = dr[1].intValue();
            chars[i] = (idx >= 0 && idx < NONSTD_ALPHABET.length()) ? NONSTD_ALPHABET.charAt(idx) : ' ';
            v = dr[0];
        }
        return new String(chars).trim();
    }

    // -----------------------------------------------------------------------
    // Callsign Hash (CRC-24/Q, poly 0x864CFB)
    // -----------------------------------------------------------------------
    public static int hashCallsign(String call) {
        if (call == null) return 0;
        String c = call.trim().toUpperCase(Locale.ROOT);
        int crc = 0;
        int poly = 0x864CFB;

        for (int i = 0; i < c.length(); ++i) {
            byte b = (byte) c.charAt(i);
            crc ^= (b & 0xFF) << 16;
            for (int bit = 0; bit < 8; ++bit) {
                if ((crc & 0x800000) != 0) {
                    crc = ((crc << 1) ^ poly) & 0xFFFFFF;
                } else {
                    crc = (crc << 1) & 0xFFFFFF;
                }
            }
        }
        return crc & 0xFFFFFF;
    }

    public static int hashCallsign24(String call) {
        return hashCallsign(call);
    }

    public static int hashCallsign20(String call) {
        return hashCallsign(call) >>> 4;
    }

    // -----------------------------------------------------------------------
    // 14-bit Callsign Hash (WSJT-X multiplicative hash)
    // -----------------------------------------------------------------------
    public static int hashCallsign14(String call) {
        if (call == null) return 0;
        String trimmed = call.trim().toUpperCase(Locale.ROOT);
        if (trimmed.isEmpty()) return 0;

        String chars = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/@";
        char[] callsign = new char[11];
        Arrays.fill(callsign, ' ');
        int len = Math.min(trimmed.length(), 11);
        for (int i = 0; i < len; ++i) {
            callsign[i] = trimmed.charAt(i);
        }

        long x = 0;
        for (int i = 0; i < 11; ++i) {
            char c = callsign[i];
            int idx = chars.indexOf(c);
            if (idx < 0) return 0;
            x = 38L * x + idx;
        }

        x = x * 47055833459L;
        x = x >>> (64 - 14);
        return (int) (x & 0x3FFF);
    }

    // -----------------------------------------------------------------------
    // Maidenhead Grid Locator (15 bits)
    // -----------------------------------------------------------------------
    public static int encodeLocator(String loc) {
        if (loc == null || loc.trim().isEmpty()) return 32400; // Blank
        String s = loc.trim().toUpperCase(Locale.ROOT);
        if (s.length() < 4) return 32400;

        char fLon = s.charAt(0);
        char fLat = s.charAt(1);
        char sLon = s.charAt(2);
        char sLat = s.charAt(3);

        if (fLon < 'A' || fLon > 'R' || fLat < 'A' || fLat > 'R' ||
            sLon < '0' || sLon > '9' || sLat < '0' || sLat > '9') {
            return 32400;
        }

        int ilon1 = fLon - 'A';
        int ilat1 = fLat - 'A';
        int ilon2 = sLon - '0';
        int ilat2 = sLat - '0';

        return ilon1 * 18 * 10 * 10 + ilat1 * 10 * 10 + ilon2 * 10 + ilat2;
    }

    public static String decodeLocator(int n15) {
        if (n15 < 0 || n15 >= 32400) return "";
        int ilat2 = n15 % 10; n15 /= 10;
        int ilon2 = n15 % 10; n15 /= 10;
        int ilat1 = n15 % 18; n15 /= 18;
        int ilon1 = n15 % 18;

        return "" + (char)('A' + ilon1) + (char)('A' + ilat1)
                  + (char)('0' + ilon2) + (char)('0' + ilat2);
    }

    // -----------------------------------------------------------------------
    // CQ Modifier (20 bits)
    // -----------------------------------------------------------------------
    private static final String[] CQ_MOD_DICT = {
        "", "DX", "FD", "QRP", "TEST", "POTA", "SOTA", "WWFF", "NA", "EU",
        "AF", "SA", "AS", "OC", "AN", "RTTY"
    };

    public static int encodeModifier(String mod) {
        if (mod == null || mod.trim().isEmpty()) return 0;
        String m = mod.trim().toUpperCase(Locale.ROOT);
        for (int i = 1; i < CQ_MOD_DICT.length; ++i) {
            if (m.equals(CQ_MOD_DICT[i])) return i;
        }
        // Base-32 string (up to 4 chars)
        int val = 0;
        for (int i = 0; i < 4; ++i) {
            char ch = (i < m.length()) ? m.charAt(i) : ' ';
            int code = 0;
            if (ch >= 'A' && ch <= 'Z') code = ch - 'A' + 1;
            else if (ch >= '0' && ch <= '4') code = ch - '0' + 27;
            val = (val * 32) + code;
        }
        return 1000 + (val & 0xFFFFF);
    }

    public static String decodeModifier(int n20) {
        if (n20 == 0) return "";
        if (n20 > 0 && n20 < CQ_MOD_DICT.length) return CQ_MOD_DICT[n20];
        if (n20 >= 1000) {
            int val = n20 - 1000;
            char[] chars = new char[4];
            for (int i = 3; i >= 0; --i) {
                int code = val % 32;
                val /= 32;
                if (code == 0) chars[i] = ' ';
                else if (code >= 1 && code <= 26) chars[i] = (char) ('A' + code - 1);
                else chars[i] = (char) ('0' + code - 27);
            }
            return new String(chars).trim();
        }
        return "";
    }

    // -----------------------------------------------------------------------
    // RST Signal Report (-26 to +5 dB -> 5 bits)
    // -----------------------------------------------------------------------
    public static int encodeRst(int rstDb) {
        int clamped = Math.max(-26, Math.min(5, rstDb));
        return clamped + 26;
    }

    public static int decodeRst(int n5) {
        int clamped = Math.max(0, Math.min(31, n5));
        return clamped - 26;
    }

    // -----------------------------------------------------------------------
    // Varicode Free Text (61-char Huffman with EOM & Invalid '1's Filler)
    // -----------------------------------------------------------------------
    private static class VaricodeEntry {
        final char ch;
        final int val;
        final int len;
        VaricodeEntry(char ch, int val, int len) {
            this.ch = ch; this.val = val; this.len = len;
        }
    }

    private static final VaricodeEntry[] VARICODE_TABLE = {
        new VaricodeEntry(' ',    0x05, 3),  // 101
        new VaricodeEntry('\u0001', 0x03, 3), // 011 (EOM)
        new VaricodeEntry('E',    0x0F, 4),  // 1111
        new VaricodeEntry('T',    0x0C, 4),  // 1100
        new VaricodeEntry('A',    0x08, 4),  // 1000
        new VaricodeEntry('O',    0x04, 4),  // 0100
        new VaricodeEntry('I',    0x02, 4),  // 0010
        new VaricodeEntry('N',    0x01, 4),  // 0001
        new VaricodeEntry('S',    0x00, 4),  // 0000
        new VaricodeEntry('H',    0x1D, 5),  // 11101
        new VaricodeEntry('R',    0x1C, 5),  // 11100
        new VaricodeEntry('D',    0x12, 5),  // 10010
        new VaricodeEntry('L',    0x0A, 5),  // 01010
        new VaricodeEntry('C',    0x37, 6),  // 110111
        new VaricodeEntry('U',    0x36, 6),  // 110110
        new VaricodeEntry('M',    0x34, 6),  // 110100
        new VaricodeEntry('W',    0x27, 6),  // 100111
        new VaricodeEntry('F',    0x26, 6),  // 100110
        new VaricodeEntry('G',    0x16, 6),  // 010110
        new VaricodeEntry('Y',    0x0F, 6),  // 001111
        new VaricodeEntry('P',    0x0E, 6),  // 001110
        new VaricodeEntry('B',    0x6B, 7),  // 1101011
        new VaricodeEntry('V',    0x2E, 7),  // 0101110
        new VaricodeEntry('K',    0x19, 7),  // 0011001
        new VaricodeEntry('0',    0xD4, 8),  // 11010100
        new VaricodeEntry('5',    0x5E, 8),  // 01011110
        new VaricodeEntry('1',    0x37, 8),  // 00110111
        new VaricodeEntry('@',    0x36, 8),  // 00110110
        new VaricodeEntry('/',    0x34, 8),  // 00110100
        new VaricodeEntry('-',    0x30, 8),  // 00110000
        new VaricodeEntry('J',    0x1AA, 9), // 110101010
        new VaricodeEntry('X',    0x63, 9),  // 001100011
        new VaricodeEntry('Q',    0x17F, 10),// 0101111111
        new VaricodeEntry('Z',    0x17C, 10),// 0101111100
        new VaricodeEntry('2',    0xD7, 10), // 0011010111
        new VaricodeEntry('3',    0xD5, 10), // 0011010101
        new VaricodeEntry('4',    0xD4, 10), // 0011010100
        new VaricodeEntry('7',    0xC4, 10), // 0011000100
        new VaricodeEntry('8',    0x6AF, 11),// 11010101111
        new VaricodeEntry('9',    0x6AE, 11),// 11010101110
        new VaricodeEntry('6',    0x6AD, 11),// 11010101101
        new VaricodeEntry(':',    0x2FC, 11),// 01011111100
        new VaricodeEntry('.',    0x2FA, 11),// 01011111010
        new VaricodeEntry('?',    0x1AD, 11),// 00110101101
        new VaricodeEntry(',',    0x18B, 11),// 00110001011
        new VaricodeEntry('>',    0xD59, 12),// 110101011001
        new VaricodeEntry('<',    0x5FB, 12),// 010111111011
        new VaricodeEntry('=',    0x5FA, 12),// 010111111010
        new VaricodeEntry('*',    0x5F6, 12),// 010111110110
        new VaricodeEntry('_',    0x359, 12),// 001101011001
        new VaricodeEntry('+',    0x358, 12),// 001101011000
        new VaricodeEntry(';',    0x314, 12),// 001100010100
        new VaricodeEntry('"',    0x1AB1, 13),// 1101010110001
        new VaricodeEntry('\'',   0xBEF, 13), // 0101111101111
        new VaricodeEntry('&',    0xBEE, 13), // 0101111101110
        new VaricodeEntry('$',    0x62A, 13), // 0011000101010
        new VaricodeEntry('|',    0x3561, 14),// 11010101100001
        new VaricodeEntry('{',    0x3560, 14),// 11010101100000
        new VaricodeEntry('}',    0xC56, 14), // 00110001010110
        new VaricodeEntry('^',    0x18AF, 15),// 001100010101111
        new VaricodeEntry('~',    0x315C, 16),// 0011000101011100
        new VaricodeEntry('\u0004', 0x315D, 16)// 0011000101011101 (EOF/FILL sentinel)
    };

    public static final int VARICODE_FILL_PATTERN = 0x315D;
    public static final int VARICODE_FILL_LEN = 16;

    private static VaricodeEntry findVaricode(char c) {
        char u = Character.toUpperCase(c);
        for (VaricodeEntry e : VARICODE_TABLE) {
            if (e.ch == u || e.ch == c) return e;
        }
        return null;
    }

    public static void encodeVaricode(String text, BitBuffer bb, int maxBits) {
        int startPos = bb.getPos();
        String s = (text != null) ? text : "";

        for (int i = 0; i < s.length(); ++i) {
            char c = s.charAt(i);
            VaricodeEntry e = findVaricode(c);
            if (e == null) e = findVaricode(' ');

            if ((bb.getPos() - startPos) + e.len + 3 > maxBits) {
                break; // Stop when character + EOM won't fit
            }
            bb.writeBits(e.val, e.len);
        }

        // Append EOM sentinel (011)
        VaricodeEntry eom = findVaricode('\u0001');
        if ((bb.getPos() - startPos) + eom.len <= maxBits) {
            bb.writeBits(eom.val, eom.len);
        }

        // Fill trailing bits with repeating 16-bit EOF/fill pattern (0011000101011101)
        int fillBitIdx = 0;
        while ((bb.getPos() - startPos) < maxBits) {
            int bit = (VARICODE_FILL_PATTERN >> (VARICODE_FILL_LEN - 1 - (fillBitIdx % VARICODE_FILL_LEN))) & 1;
            bb.writeBits(bit, 1);
            fillBitIdx++;
        }
    }

    public static String decodeVaricode(BitBuffer bb, int maxBits) {
        StringBuilder sb = new StringBuilder();
        int startPos = bb.getPos();
        int currentVal = 0;
        int currentLen = 0;

        while ((bb.getPos() - startPos) < maxBits) {
            long bit = bb.readBits(1);
            currentVal = (currentVal << 1) | (int) bit;
            currentLen++;

            boolean matched = false;
            for (VaricodeEntry e : VARICODE_TABLE) {
                if (e.len == currentLen && e.val == currentVal) {
                    if (e.ch == '\u0001' || e.ch == '\u0004') {
                        // Reached EOM or EOF/FILL sentinel -> stop immediately
                        bb.setPos(startPos + maxBits);
                        return sb.toString();
                    }
                    sb.append(e.ch);
                    currentVal = 0;
                    currentLen = 0;
                    matched = true;
                    break;
                }
            }

            if (!matched && currentLen >= 16) {
                // Invalid sequence / padding bits -> reset
                currentVal = 0;
                currentLen = 0;
            }
        }

        bb.setPos(startPos + maxBits);
        return sb.toString();
    }

    // -----------------------------------------------------------------------
    // Unified Message Encoder & Decoder
    // -----------------------------------------------------------------------
    public static boolean encodeMessage(LqMessage msg, byte[] payload) {
        if (msg == null || payload == null) return false;
        if (LqNative.isAvailable()) {
            if (LqNative.encodeMessage(msg, payload)) {
                return true;
            }
        }
        Arrays.fill(payload, (byte) 0);
        BitBuffer bb = new BitBuffer(payload, PAYLOAD_BITS);

        switch (msg.type) {
            case CALL_STD_NOSUF: // Type 5: 1 (1 bit) + 28 + 28 + 15 + 5 = 77
                bb.writeBits(0b1L, 1);
                bb.writeBits(encodeStandardCallsign(msg.call1), 28);
                bb.writeBits(encodeStandardCallsign(msg.call2), 28);
                bb.writeBits(encodeLocator(msg.locator), 15);
                bb.writeBits(encodeRst(msg.rstDb), 5);
                break;
            case CALL_STD_SUF: // Type 6: 0100 (4 bits) + 28 + 1 + 28 + 1 + 15 = 77
                bb.writeBits(0b0100L, 4);
                bb.writeBits(encodeStandardCallsign(msg.call1), 28);
                bb.writeBits(msg.suffix1 & 1, 1);
                bb.writeBits(encodeStandardCallsign(msg.call2), 28);
                bb.writeBits(msg.suffix2 & 1, 1);
                bb.writeBits(encodeLocator(msg.locator), 15);
                break;
            case CALL_NONSTD: // Type 7: 001 (3 bits) + 20 + 48 + 1 + 5 = 77
                bb.writeBits(0b001L, 3);
                int h1_20 = msg.hash1 != 0 ? (msg.hash1 > 0xFFFFF ? (msg.hash1 >>> 4) & 0xFFFFF : msg.hash1 & 0xFFFFF) : hashCallsign20(msg.call1);
                bb.writeBits(h1_20, 20);
                bb.writeBits(encodeNonStdCallsign(msg.call2, 9).longValue(), 48);
                bb.writeBits(msg.suffix2 & 1, 1);
                bb.writeBits(encodeRst(msg.rstDb), 5);
                break;
            case FREE_TEXT: // Type 13: 0101 (4 bits) + 73 = 77
                bb.writeBits(0b0101L, 4);
                encodeVaricode(msg.text, bb, 73);
                break;
            case MULTI_REPORT73: // Type 11: 011 (3 bits) + 16 + 24 + 5 + 24 + 5 = 77
                bb.writeBits(0b011L, 3);
                bb.writeBits(msg.hash1 & 0xFFFF, 16);
                {
                    LqMessage.MultiTarget t0 = !msg.multiTargets.isEmpty() ? msg.multiTargets.get(0) : new LqMessage.MultiTarget();
                    LqMessage.MultiTarget t1 = msg.multiTargets.size() > 1 ? msg.multiTargets.get(1) : t0;
                    LqMessage.MultiTarget[] targets = new LqMessage.MultiTarget[]{t0, t1};
                    for (LqMessage.MultiTarget t : targets) {
                        int th = t.hash != 0 ? (t.hash & 0xFFFFFF) : (t.call != null && !t.call.isEmpty() ? hashCallsign24(t.call) : 0);
                        bb.writeBits(th, 24);
                        bb.writeBits(encodeRst(t.rstDb), 5);
                    }
                }
                break;
            case M73_NONSTD: // Type 10: 0001 (4 bits) + 24 + 48 + 1 = 77
                bb.writeBits(0b0001L, 4);
                bb.writeBits(msg.hash1 != 0 ? msg.hash1 & 0xFFFFFF : hashCallsign24(msg.call1), 24);
                bb.writeBits(encodeNonStdCallsign(msg.call2, 9).longValue(), 48);
                bb.writeBits(msg.suffix2 & 1, 1);
                break;
            case CQ_NONSTD_3: // Type 4: 0000001 (7 bits) + 69 + 1 = 77
                bb.writeBits(0b0000001L, 7);
                bb.writeBits(encodeNonStdCallsign(msg.call1, 13).longValue(), 69);
                bb.writeBits(msg.suffix1 & 1, 1);
                break;
            case CQ_NONSTD_2: // Type 3: 00000001 (8 bits) + 48 + 1 + 20 = 77
                bb.writeBits(0b00000001L, 8);
                bb.writeBits(encodeNonStdCallsign(msg.call1, 9).longValue(), 48);
                bb.writeBits(msg.suffix1 & 1, 1);
                bb.writeBits(encodeModifier(msg.modifier), 20);
                break;
            case MULTI_73: // Type 12: 00000111 (8 bits) + 16 + 24 + 24 + 5 = 77
                bb.writeBits(0b00000111L, 8);
                bb.writeBits(msg.hash1 & 0xFFFF, 16);
                {
                    LqMessage.MultiTarget t0 = !msg.multiTargets.isEmpty() ? msg.multiTargets.get(0) : new LqMessage.MultiTarget();
                    LqMessage.MultiTarget t1 = msg.multiTargets.size() > 1 ? msg.multiTargets.get(1) : t0;
                    bb.writeBits(t0.hash != 0 ? (t0.hash & 0xFFFFFF) : hashCallsign24(t0.call), 24);
                    bb.writeBits(t1.hash != 0 ? (t1.hash & 0xFFFFFF) : hashCallsign24(t1.call), 24);
                    bb.writeBits(0, 5); // 5-bit pad
                }
                break;
            case CQ_STD: // Type 1: 0000000000 (10 bits) + 28 + 1 + 20 + 15 + 3 = 77
                bb.writeBits(0b0000000000L, 10);
                bb.writeBits(encodeStandardCallsign(msg.call1), 28);
                bb.writeBits(msg.suffix1 & 1, 1);
                bb.writeBits(encodeModifier(msg.modifier), 20);
                bb.writeBits(encodeLocator(msg.locator), 15);
                bb.writeBits(0, 3); // 3-bit pad
                break;
            case CQ_NONSTD_1: // Type 2: 0000000001 (10 bits) + 48 + 15 + 1 + 3 = 77
                bb.writeBits(0b0000000001L, 10);
                bb.writeBits(encodeNonStdCallsign(msg.call1, 9).longValue(), 48);
                bb.writeBits(encodeLocator(msg.locator), 15);
                bb.writeBits(msg.suffix1 & 1, 1);
                bb.writeBits(0, 3); // 3-bit pad
                break;
            case REPORT73_STD: // Type 8: 0000000010 (10 bits) + 28 + 1 + 28 + 1 + 5 + 4 = 77
                bb.writeBits(0b0000000010L, 10);
                bb.writeBits(encodeStandardCallsign(msg.call1), 28);
                bb.writeBits(msg.suffix1 & 1, 1);
                bb.writeBits(encodeStandardCallsign(msg.call2), 28);
                bb.writeBits(msg.suffix2 & 1, 1);
                bb.writeBits(encodeRst(msg.rstDb), 5);
                bb.writeBits(0, 4); // 4-bit pad
                break;
            case M73_STD: // Type 9: 0000000011 (10 bits) + 28 + 1 + 28 + 1 + 9 = 77
                bb.writeBits(0b0000000011L, 10);
                bb.writeBits(encodeStandardCallsign(msg.call1), 28);
                bb.writeBits(msg.suffix1 & 1, 1);
                bb.writeBits(encodeStandardCallsign(msg.call2), 28);
                bb.writeBits(msg.suffix2 & 1, 1);
                bb.writeBits(0, 9); // 9-bit pad
                break;
            case RESERVED_A: // Type 14: 0101 (4 bits)
                bb.writeBits(0b0101L, 4);
                break;
            case RESERVED_B: // Type 15: 00001 (5 bits)
                bb.writeBits(0b00001L, 5);
                break;
            case RESERVED_C: // Type 16: 0000010 (7 bits)
                bb.writeBits(0b0000010L, 7);
                break;
        }

        bb.padZeros();
        return true;
    }

    public static boolean decodeMessage(byte[] payload, LqMessage msg) {
        if (payload == null || msg == null) return false;
        if (LqNative.isAvailable()) {
            if (LqNative.decodeMessage(payload, msg)) {
                return true;
            }
        }
        BitBuffer bb = new BitBuffer(payload, PAYLOAD_BITS);

        long b1 = bb.readBits(1);
        if (b1 == 1) { // 1 -> CALL_STD_NOSUF (Type 5)
            msg.type = LqMessage.Type.CALL_STD_NOSUF;
            msg.call1 = decodeStandardCallsign(bb.readBits(28));
            msg.call2 = decodeStandardCallsign(bb.readBits(28));
            msg.locator = decodeLocator((int) bb.readBits(15));
            msg.rstDb = decodeRst((int) bb.readBits(5));
            return true;
        }

        // Starts with 0
        long b2 = bb.readBits(1);
        if (b2 == 1) { // 01
            long b3 = bb.readBits(1);
            if (b3 == 1) { // 011 -> MULTI_REPORT73 (Type 11)
                msg.type = LqMessage.Type.MULTI_REPORT73;
                msg.hash1 = (int) bb.readBits(16);
                for (int i = 0; i < 2; ++i) {
                    int th = (int) bb.readBits(24);
                    int r = decodeRst((int) bb.readBits(5));
                    msg.multiTargets.add(new LqMessage.MultiTarget("", th, r));
                }
                return true;
            } else { // 010
                long b4 = bb.readBits(1);
                if (b4 == 0) { // 0100 -> CALL_STD_SUF (Type 6)
                    msg.type = LqMessage.Type.CALL_STD_SUF;
                    msg.call1 = decodeStandardCallsign(bb.readBits(28));
                    msg.suffix1 = (int) bb.readBits(1);
                    msg.call2 = decodeStandardCallsign(bb.readBits(28));
                    msg.suffix2 = (int) bb.readBits(1);
                    msg.locator = decodeLocator((int) bb.readBits(15));
                    return true;
                } else { // 0101 -> FREE_TEXT (Type 13)
                    msg.type = LqMessage.Type.FREE_TEXT;
                    msg.text = decodeVaricode(bb, 73);
                    return true;
                }
            }
        }

        // Starts with 00
        long b3 = bb.readBits(1);
        if (b3 == 1) { // 001 -> CALL_NONSTD (Type 7)
            msg.type = LqMessage.Type.CALL_NONSTD;
            msg.hash1 = (int) bb.readBits(20);
            BigInteger b48 = BigInteger.valueOf(bb.readBits(48));
            msg.call2 = decodeNonStdCallsign(b48, 9);
            msg.suffix2 = (int) bb.readBits(1);
            msg.rstDb = decodeRst((int) bb.readBits(5));
            return true;
        }

        // Starts with 000
        long b4 = bb.readBits(1);
        if (b4 == 1) { // 0001 -> M73_NONSTD (Type 10)
            msg.type = LqMessage.Type.M73_NONSTD;
            msg.hash1 = (int) bb.readBits(24);
            BigInteger b48 = BigInteger.valueOf(bb.readBits(48));
            msg.call2 = decodeNonStdCallsign(b48, 9);
            msg.suffix2 = (int) bb.readBits(1);
            return true;
        }

        // Starts with 0000
        long b5 = bb.readBits(1);
        if (b5 == 1) { // 00001 -> RESERVED_A (Type 14)
            msg.type = LqMessage.Type.RESERVED_A;
            return true;
        }

        // Starts with 00000
        long b6 = bb.readBits(1);
        if (b6 == 1) { // 000001
            long b7 = bb.readBits(1);
            if (b7 == 0) { // 0000010 -> RESERVED_B (Type 15)
                msg.type = LqMessage.Type.RESERVED_B;
                return true;
            } else { // 0000011
                long b8 = bb.readBits(1);
                if (b8 == 0) { // 00000110 -> RESERVED_C (Type 16)
                    msg.type = LqMessage.Type.RESERVED_C;
                    return true;
                } else { // 00000111 -> MULTI_73 (Type 12)
                    msg.type = LqMessage.Type.MULTI_73;
                    msg.hash1 = (int) bb.readBits(16);
                    int th0 = (int) bb.readBits(24);
                    int th1 = (int) bb.readBits(24);
                    msg.multiTargets.add(new LqMessage.MultiTarget("", th0, 0));
                    msg.multiTargets.add(new LqMessage.MultiTarget("", th1, 0));
                    return true;
                }
            }
        }

        // Starts with 000000
        long b7 = bb.readBits(1);
        if (b7 == 1) { // 0000001 -> CQ_NONSTD_3 (Type 4)
            msg.type = LqMessage.Type.CQ_NONSTD_3;
            long hi = bb.readBits(5);
            long lo = bb.readBits(64);
            BigInteger b69 = BigInteger.valueOf(hi).shiftLeft(64).or(new BigInteger(Long.toUnsignedString(lo)));
            msg.call1 = decodeNonStdCallsign(b69, 13);
            msg.suffix1 = (int) bb.readBits(1);
            return true;
        }

        // Starts with 0000000
        long b8 = bb.readBits(1);
        if (b8 == 1) { // 00000001 -> CQ_NONSTD_2 (Type 3)
            msg.type = LqMessage.Type.CQ_NONSTD_2;
            long n48 = bb.readBits(48);
            msg.call1 = decodeNonStdCallsign(BigInteger.valueOf(n48), 9);
            msg.suffix1 = (int) bb.readBits(1);
            msg.modifier = decodeModifier((int) bb.readBits(20));
            return true;
        }

        // Starts with 00000000
        long b9 = bb.readBits(1);
        long b10 = bb.readBits(1);
        long code10 = (b9 << 1) | b10;
        if (code10 == 0b00) { // 0000000000 -> CQ_STD (Type 1)
            msg.type = LqMessage.Type.CQ_STD;
            msg.call1 = decodeStandardCallsign(bb.readBits(28));
            msg.suffix1 = (int) bb.readBits(1);
            msg.modifier = decodeModifier((int) bb.readBits(20));
            msg.locator = decodeLocator((int) bb.readBits(15));
            return true;
        } else if (code10 == 0b01) { // 0000000001 -> CQ_NONSTD_1 (Type 2)
            msg.type = LqMessage.Type.CQ_NONSTD_1;
            long n48 = bb.readBits(48);
            msg.call1 = decodeNonStdCallsign(BigInteger.valueOf(n48), 9);
            msg.locator = decodeLocator((int) bb.readBits(15));
            msg.suffix1 = (int) bb.readBits(1);
            return true;
        } else if (code10 == 0b10) { // 0000000010 -> REPORT73_STD (Type 8)
            msg.type = LqMessage.Type.REPORT73_STD;
            msg.call1 = decodeStandardCallsign(bb.readBits(28));
            msg.suffix1 = (int) bb.readBits(1);
            msg.call2 = decodeStandardCallsign(bb.readBits(28));
            msg.suffix2 = (int) bb.readBits(1);
            msg.rstDb = decodeRst((int) bb.readBits(5));
            return true;
        } else { // 0000000011 -> M73_STD (Type 9)
            msg.type = LqMessage.Type.M73_STD;
            msg.call1 = decodeStandardCallsign(bb.readBits(28));
            msg.suffix1 = (int) bb.readBits(1);
            msg.call2 = decodeStandardCallsign(bb.readBits(28));
            msg.suffix2 = (int) bb.readBits(1);
            return true;
        }
    }
}
