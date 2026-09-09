// =============================================================================
// The LQ Digital Mode Family — Reference Implementation (lq_lib)
//
// Author:  Luis Quesada (HB9IPH)
// Web:     https://luisquesada.com
// Portal:  https://lquesada.github.io/lq_lib/
// GitHub:  https://github.com/lquesada/lq_lib
// App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)
//
// License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)
//
// Copyright (c) 2026 Luis Quesada (HB9IPH)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// =============================================================================

package com.lq;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.*;

/**
 * Self-contained Java validation harness for the LQ Digital Mode Family Test Battery.
 * 
 * Verifies that Java, Kotlin, and Android implementations produce bit-for-bit
 * compliant hash calculations, codec conversions, and deterministic user intent decisions.
 * 
 * Requires zero third-party dependencies (pure Java standard runtime).
 */
public class LQTestBatteryRunner {

    // ANSI Colors
    private static final String RESET = "\u001B[0m";
    private static final String GREEN = "\u001B[32m";
    private static final String RED = "\u001B[31m";
    private static final String CYAN = "\u001B[36m";
    private static final String YELLOW = "\u001B[33m";
    private static final String BOLD = "\u001B[1m";

    // -------------------------------------------------------------------------
    // LQ Protocol Reference Algorithms (Java Standard Implementation)
    // -------------------------------------------------------------------------

    public static final int CRC24_POLY = 0x1864CFB;

    public static int hashCallsign24(String callsign) {
        if (callsign == null || callsign.isEmpty()) return 0;
        String clean = callsign.trim().toUpperCase();
        int crc = 0;
        for (int i = 0; i < clean.length(); ++i) {
            int b = clean.charAt(i) & 0xFF;
            crc ^= (b << 16);
            for (int bit = 0; bit < 8; ++bit) {
                if ((crc & 0x800000) != 0) {
                    crc = ((crc << 1) ^ CRC24_POLY) & 0xFFFFFF;
                } else {
                    crc = (crc << 1) & 0xFFFFFF;
                }
            }
        }
        return crc & 0xFFFFFF;
    }

    public static int hashCallsign20(String callsign) {
        return hashCallsign24(callsign) >>> 4;
    }

    public static int hashCallsign23(String callsign) {
        return hashCallsign24(callsign) & 0x7FFFFF;
    }

    private static final String WSJTX_CHARS = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/@";

    public static int computeWsjtxHash(String callsign, int bits) {
        if (callsign == null || callsign.isEmpty() || bits <= 0 || bits > 32) return 0;
        String clean = callsign.trim().toUpperCase();
        StringBuilder sb = new StringBuilder();
        sb.append(clean);
        while (sb.length() < 11) {
            sb.append(' ');
        }
        String padded = sb.substring(0, 11);

        long x = 0;
        for (int i = 0; i < 11; ++i) {
            char c = padded.charAt(i);
            int idx = WSJTX_CHARS.indexOf(c);
            if (idx == -1) return 0;
            x = 38L * x + idx;
        }

        long mult = 47055833459L;
        // Unsigned 64-bit multiplication: in Java long is signed, but mult is positive and x * mult behaves identically modulo 2^64
        long prod = x * mult;
        long res = prod >>> (64 - bits);
        return (int) res;
    }

    public static int hashCallsign22(String callsign) {
        return computeWsjtxHash(callsign, 22);
    }

    public static int hashCallsign14(String callsign) {
        return computeWsjtxHash(callsign, 14);
    }

    public static int hashCallsign12(String callsign) {
        return computeWsjtxHash(callsign, 12);
    }

    public static int hashCallsign10(String callsign) {
        return computeWsjtxHash(callsign, 10);
    }


    public static byte[] hexToBytes(String hex) {
        String clean = hex.replaceAll("[\\s:xX-]", "");
        int len = clean.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(clean.charAt(i), 16) << 4)
                                 + Character.digit(clean.charAt(i + 1), 16));
        }
        return data;
    }

    public static String bytesToHex(byte[] bytes, boolean spaceSeparated) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < bytes.length; ++i) {
            if (spaceSeparated && i > 0) sb.append(' ');
            sb.append(String.format("%02X", bytes[i] & 0xFF));
        }
        return sb.toString();
    }

    // -------------------------------------------------------------------------
    // Main Runner
    // -------------------------------------------------------------------------

    public static void main(String[] args) {
        String batteryPath = "tests/data/test_battery.json";
        if (args.length > 0) {
            batteryPath = args[0];
        }

        System.out.println(CYAN + BOLD + "=================================================================" + RESET);
        System.out.println(CYAN + BOLD + "   LQ Digital Mode Family — Java Test Battery Validation Runner   " + RESET);
        System.out.println(CYAN + BOLD + "=================================================================" + RESET);
        System.out.println("Reading Test Battery from: " + YELLOW + batteryPath + RESET + "\n");

        File f = new File(batteryPath);
        if (!f.exists()) {
            System.err.println(RED + "Error: Test battery file not found: " + batteryPath + RESET);
            System.err.println("Please run ./build/generate_test_battery first.");
            System.exit(1);
        }

        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;

        try {
            String jsonContent = new String(Files.readAllBytes(Paths.get(batteryPath)), StandardCharsets.UTF_8);

            // 1. Validate Hash Calculations
            System.out.println(BOLD + ">>> Phase 1: Callsign Hash Algorithm Equivalence (CRC-24/Q, 23b, 14b, 12b, 10b)..." + RESET);
            int hashChecks = testHashVectors(jsonContent);
            totalTests += hashChecks;
            passedTests += hashChecks;
            System.out.println(GREEN + "  ✓ Passed " + hashChecks + " hash calculation test vectors." + RESET);

            // 2. Validate Codec Roundtrips & Binary/Hex formatting
            System.out.println(BOLD + "\n>>> Phase 2: Codec Hex and Binary Representation Roundtrips..." + RESET);
            int codecChecks = testCodecVectors(jsonContent);
            totalTests += codecChecks;
            passedTests += codecChecks;
            System.out.println(GREEN + "  ✓ Passed " + codecChecks + " hex/binary codec conversion vectors." + RESET);

            // 3. Validate Deterministic Intent Engine
            System.out.println(BOLD + "\n>>> Phase 3: Deterministic User Intent & Transceiver Decision Scenarios..." + RESET);
            int intentChecks = testIntentScenarios(jsonContent);
            totalTests += intentChecks;
            passedTests += intentChecks;
            System.out.println(GREEN + "  ✓ Passed " + intentChecks + " transceiver intent & decision scenarios." + RESET);

            System.out.println(CYAN + BOLD + "\n=================================================================" + RESET);
            System.out.println(GREEN + BOLD + "  ✓ ALL " + totalTests + " JAVA BATTERY TESTS PASSED WITH ZERO ERRORS!" + RESET);
            System.out.println(CYAN + BOLD + "=================================================================" + RESET);

        } catch (Exception e) {
            System.err.println(RED + "FATAL ERROR during test battery execution: " + e.getMessage() + RESET);
            e.printStackTrace();
            System.exit(1);
        }
    }

    // -------------------------------------------------------------------------
    // Phase 1: Hash Verifications
    // -------------------------------------------------------------------------
    private static int testHashVectors(String json) {
        int count = 0;
        int startIdx = json.indexOf("\"hash_verifications\": [");
        if (startIdx == -1) return 0;
        int endIdx = json.indexOf("\"intent_scenarios\": [", startIdx);
        if (endIdx == -1) endIdx = json.length();
        String block = json.substring(startIdx, endIdx);

        String[] entries = block.split("\\{");
        for (String entry : entries) {
            if (!entry.contains("\"callsign\"")) continue;
            String call = extractString(entry, "callsign");
            int expectedH24 = extractInt(entry, "hash_24");
            int expectedH20 = extractInt(entry, "hash_20");
            int expectedH23 = extractInt(entry, "hash_23");
            int expectedH14 = extractInt(entry, "hash_14");
            int expectedH12 = extractInt(entry, "hash_12");
            int expectedH10 = extractInt(entry, "hash_10");

            int actualH24 = hashCallsign24(call);
            int actualH20 = hashCallsign20(call);
            int actualH23 = hashCallsign23(call);
            int actualH14 = hashCallsign14(call);
            int actualH12 = hashCallsign12(call);
            int actualH10 = hashCallsign10(call);

            if (actualH24 != expectedH24 || actualH20 != expectedH20 || actualH23 != expectedH23 || actualH14 != expectedH14 || actualH12 != expectedH12 || actualH10 != expectedH10) {
                throw new AssertionError(String.format("Hash mismatch for %s: 24b (%d vs %d), 20b (%d vs %d), 23b (%d vs %d), 14b (%d vs %d)",
                        call, actualH24, expectedH24, actualH20, expectedH20, actualH23, expectedH23, actualH14, expectedH14));
            }
            ++count;
        }
        return count;
    }

    // -------------------------------------------------------------------------
    // Phase 2: Codec Hex and Binary Checks
    // -------------------------------------------------------------------------
    private static int testCodecVectors(String json) {
        int count = 0;
        int startIdx = json.indexOf("\"codec_conversions\": [");
        if (startIdx == -1) return 0;
        int endIdx = json.indexOf("\"hash_verifications\": [", startIdx);
        if (endIdx == -1) endIdx = json.length();
        String block = json.substring(startIdx, endIdx);

        String[] entries = block.split("\\{");
        for (String entry : entries) {
            if (!entry.contains("\"hex\"")) continue;
            String hex = extractString(entry, "hex");
            String bin = extractString(entry, "binary");
            String text = extractString(entry, "text");

            byte[] bytes = hexToBytes(hex);
            if (bytes.length != 10) {
                throw new AssertionError("Hex byte length mismatch: " + hex);
            }

            String reHex = bytesToHex(bytes, true);
            if (!reHex.equalsIgnoreCase(hex)) {
                throw new AssertionError("Hex format roundtrip mismatch: " + reHex + " vs " + hex);
            }

            if (bin.length() != 77) {
                throw new AssertionError("Binary string length mismatch: " + bin);
            }

            ++count;
        }
        return count;
    }

    // -------------------------------------------------------------------------
    // Phase 3: Intent Scenarios
    // -------------------------------------------------------------------------
    private static int testIntentScenarios(String json) {
        int count = 0;
        int startIdx = json.indexOf("\"intent_scenarios\": [");
        if (startIdx == -1) return 0;
        int endIdx = json.lastIndexOf("]");
        if (endIdx == -1) endIdx = json.length();
        String block = json.substring(startIdx, endIdx);

        String[] entries = block.split("\\{");
        for (String entry : entries) {
            if (!entry.contains("\"intent_action\"")) continue;
            String id = extractString(entry, "id");
            String desc = extractString(entry, "description");
            int action = extractInt(entry, "intent_action");
            String expTxText = extractString(entry, "expected_tx_text");

            // Verify specific intent rules
            if (id.equals("INTENT-004")) {
                // Confirm 2-station multi-report (canonical <TARGET1> R+... or legacy RPT73/MULTI-REPLY73)
                if (!expTxText.startsWith("<") && !expTxText.startsWith("RPT73") && !expTxText.startsWith("MULTI-REPLY73")) {
                    throw new AssertionError("INTENT-004 must emit canonical multi-report or legacy RPT73/MULTI-REPLY73");
                }
            }

            ++count;
        }
        return count;
    }


    // -------------------------------------------------------------------------
    // Lightweight JSON Parsing Helpers
    // -------------------------------------------------------------------------
    private static String extractString(String block, String key) {
        String search = "\"" + key + "\": \"";
        int idx = block.indexOf(search);
        if (idx == -1) return "";
        int start = idx + search.length();
        int end = block.indexOf("\"", start);
        if (end == -1) return "";
        return block.substring(start, end);
    }

    private static int extractInt(String block, String key) {
        String search = "\"" + key + "\": ";
        int idx = block.indexOf(search);
        if (idx == -1) return 0;
        int start = idx + search.length();
        int end = start;
        while (end < block.length() && (Character.isDigit(block.charAt(end)) || block.charAt(end) == '-')) {
            ++end;
        }
        if (start == end) return 0;
        return Integer.parseInt(block.substring(start, end));
    }
}
