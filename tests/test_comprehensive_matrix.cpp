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

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <random>
#include <cmath>
#include "lq/lq.h"

using namespace lq;

// ============================================================================
// 1. Standard Callsigns Matrix (100+ Global ITU Prefix Patterns & Tokens)
// ============================================================================

TEST(ComprehensiveMatrixTest, StandardCallsignsWorldwideMatrix) {
    std::vector<std::string> test_callsigns = {
        // Single letter prefixes
        "K1A", "K1AB", "K1ABC", "W1AW", "W1A", "W2ABC", "N1MM", "N0A", "F1AAA", "F6KOP",
        "G4ABC", "G0XYZ", "I1AAA", "I0JAY", "M0XYZ", "R1AAA", "R9WXT", "U1AA",
        // Two letter prefixes
        "DL1ABC", "DJ2YA", "DK0MN", "EA1ABC", "EA4URA", "EB5XYZ", "EC7ABC", "HB9IPH", "HB9BZA", "HB3XYZ",
        "JA1ABC", "JH1XYZ", "JR2XXX", "JE1JKL", "VK1AA", "VK2ABC", "VK3XYZ", "VK7XX", "ZL1AA", "ZL2ABC",
        "VE1AA", "VE3ABC", "VA7XYZ", "VY2AA", "PY1AA", "PY2ABC", "LU1AA", "LU4XYZ", "ZS1AA", "ZS6XYZ",
        "BY1AA", "BG7XYZ", "BA4ABC", "VU2AA", "VU3XYZ", "SM0AAA", "SM5XYZ", "OH2AA", "OH6XYZ", "SP1AAA",
        "OK1AA", "OK2XYZ", "OM3AA", "ON4AA", "ON7XYZ", "PA0AAA", "PA3XYZ", "SV1AA", "SV8XYZ", "OE1AAA",
        "YO1YO", "YO3ABC", "LZ1AA", "LZ2XYZ", "ER1AA", "UN7AA", "EY8AA", "EX8AA", "EK6AA", "4X4AA", "4X6TT",
        "9A1AA", "9A5XYZ", "S51AA", "S57XYZ", "9M2AA", "9M6XYZ", "9V1AA", "9V1XYZ", "YB1AA", "YB0XYZ",
        "HS0AAA", "HS1XYZ", "VR2AA", "XX9AA", "BV2AA", "DU1AA", "DU7XYZ", "A22AA", "5B4AA", "9H1AA",
        "CT1AA", "CT7XYZ", "CU2AA", "TF3AA", "LA1AA", "OZ1AA", "LY1AA", "YL2AA", "ES1AA",
        // Special tokens & digits in prefix
        "3D2AA", "8P6AA", "9Y4AA", "3B8AA", "6Y5AA", "VP5AA", "VP9AA",
        "ZF1AA", "PJ2AA", "C6AAA", "V31AA", "TI2AA", "HP1AA", "HK3AA", "YV5AA", "HC1AA", "OA4AA",
        "CP4AA", "ZP5AA", "CX2AA", "CE3AA", "CQ", "DE", "QRZ"
    };

    for (const auto& call : test_callsigns) {
        uint32_t packed = 0;
        bool pack_ok = encode_callsign_std(call, packed);
        EXPECT_TRUE(pack_ok) << "Failed to encode standard callsign: " << call;

        std::string unpacked;
        bool unpack_ok = decode_callsign_std(packed, unpacked);
        EXPECT_TRUE(unpack_ok) << "Failed to decode standard callsign: " << call;
        EXPECT_EQ(unpacked, call) << "Mismatch for standard callsign: " << call;
    }
}

// ============================================================================
// 2. Non-Standard & Compound Callsigns Matrix (1 to 14 Characters)
// ============================================================================

TEST(ComprehensiveMatrixTest, NonStandardCallsignsLengthMatrix) {
    std::vector<std::pair<std::string, int>> nonstd_callsigns = {
        // <= 9 chars
        {"HB9IPH/P", 9}, {"EA8/HB9A", 9}, {"W1AW/QRP", 9}, {"DL1ABC/M", 9},
        {"3B9/HB9A", 9}, {"GB100BBC", 9}, {"HG2024OL", 9}, {"PA600XYZ", 9},
        // <= 13 chars
        {"3B9/HB9IPH/P", 13}, {"EA8/HB9IP/QRP", 13}, {"VP8/G4ABC/MM", 13},
        {"VK9W/HB9IPH", 13}, {"DP0GVN/QRP", 13}, {"KH8/N6PSE", 13},
        {"4W/HB9IPH/P", 13}, {"Z6/HB9IPH/P", 13}, {"VK0/HB9IPH/P", 13}
    };

    for (const auto& [call, max_chars] : nonstd_callsigns) {
        uint8_t buffer[16] = {0};
        BitBuffer bb_enc(buffer, sizeof(buffer) * 8);
        bool pack_ok = encode_callsign_nonstd(call, max_chars, bb_enc);
        EXPECT_TRUE(pack_ok) << "Failed to encode non-standard callsign: " << call;

        BitBuffer bb_dec(buffer, sizeof(buffer) * 8);
        std::string unpacked;
        bool unpack_ok = decode_callsign_nonstd(bb_dec, max_chars, unpacked);
        EXPECT_TRUE(unpack_ok) << "Failed to decode non-standard callsign: " << call;
        EXPECT_EQ(unpacked, call) << "Mismatch for non-standard callsign: " << call;
    }
}

// ============================================================================
// 3. Maidenhead Grid Locators Matrix (Hemispheres, Corners & Empty)
// ============================================================================

TEST(ComprehensiveMatrixTest, MaidenheadGridMatrix) {
    std::vector<std::string> test_grids = {
        // Corners of global coordinate space
        "AA00", "AA99", "AR00", "AR99",
        "RA00", "RA99", "RR00", "RR99",
        // Center equator / meridian
        "JJ00", "JJ99", "II55", "JK00",
        // Well-known population centres & DX zones
        "FN31", "JN47", "IO91", "PM95", "QF22", "RE78", "BL11", "CM87",
        "DM04", "EM10", "FM18", "JM19", "KP00", "LL34", "MM55", "NL21",
        "OK49", "PL30", "QL82", "RL99", "IK20", "JO62", "KO29", "LO02",
        // Empty sentinel
        ""
    };

    for (const auto& grid : test_grids) {
        uint16_t packed = 0;
        bool pack_ok = encode_locator_15(grid, packed);
        EXPECT_TRUE(pack_ok) << "Failed to encode locator: '" << grid << "'";

        std::string unpacked;
        bool unpack_ok = decode_locator_15(packed, unpacked);
        EXPECT_TRUE(unpack_ok) << "Failed to decode locator: '" << grid << "'";
        EXPECT_EQ(unpacked, grid) << "Mismatch for locator: '" << grid << "'";
    }
}

// ============================================================================
// 4. RST Signal Reports Full Sweep (-26 dB to +5 dB for 5-bit RST)
// ============================================================================

TEST(ComprehensiveMatrixTest, RstReportFullSweep) {
    for (int rst = -26; rst <= 5; ++rst) {
        uint8_t packed = encode_rst_5(rst);
        int unpacked = decode_rst_5(packed);
        EXPECT_EQ(unpacked, rst) << "RST 5-bit mismatch at " << rst << " dB";
    }
}

// ============================================================================
// 5. CQ Modifiers Matrix (Dictionary & Custom)
// ============================================================================

TEST(ComprehensiveMatrixTest, CqModifierMatrix) {
    std::vector<std::string> test_modifiers = {
        // Standard dictionary keywords
        "DX", "POTA", "SOTA", "WWFF", "FD", "NA", "EU", "AS", "AF", "OC", "SA",
        "VHF", "UHF", "QRP", "TEST", "IARU",
        // Custom base-32 alphanumeric 1-4 char modifiers (A-Z, 0-4)
        "10M", "20M", "40M", "2M", "EME", "SAT", "SUM", "PARK",
        "ISL", "LH", "BOTA", "COTA", "W1AW", "CQWW", "ARRL", "GRID", "CON"
    };

    for (const auto& mod : test_modifiers) {
        uint32_t packed = 0;
        bool ok = encode_modifier_20(mod, packed);
        EXPECT_TRUE(ok) << "Failed to encode modifier: " << mod;

        std::string unpacked;
        bool un_ok = decode_modifier_20(packed, unpacked);
        EXPECT_TRUE(un_ok) << "Failed to decode modifier: " << mod;
        EXPECT_EQ(unpacked, mod) << "Mismatch for modifier: " << mod;
    }
}

// ============================================================================
// 6. Varicode Free-Text Comprehensive Alphabet & Length Tests
// ============================================================================

TEST(ComprehensiveMatrixTest, VaricodeAlphabetAndStringMatrix) {
    std::vector<std::string> test_strings = {
        "73",
        "TNX FOR QSO",
        "FB 599",
        "WX SUNNY",
        "RIG KX3 5W",
        "HELLO WORLD",
        "POTA K-12",
        "QSL VIA BURO",
        "OP IS LUIS",
        "QRG 14M",
        "CALL @HB9I",
        "FREQ 14",
        "RPRT -12DB",
        "BEST 73 ALL",
        "SOS SOS CQ",
        "ABC DEF GHI"
    };

    for (const auto& str : test_strings) {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, VARICODE_PAYLOAD_BITS);
        int chars_enc = encode_varicode(str, bb_enc, VARICODE_PAYLOAD_BITS);
        EXPECT_GT(chars_enc, 0) << "Failed to encode Varicode text: " << str;

        BitBuffer bb_dec(buffer, VARICODE_PAYLOAD_BITS);
        std::string unpacked;
        decode_varicode(bb_dec, unpacked, VARICODE_PAYLOAD_BITS);
        EXPECT_EQ(unpacked, str) << "Mismatch for Varicode string: " << str;
    }
}

// ============================================================================
// 7. Exhaustive Message Type Roundtrip Tests (All 12 Protocol Types)
// ============================================================================

TEST(ComprehensiveMatrixTest, All12MessageTypesRoundtrip) {
    // Type 1: CQ_STD
    {
        Message msg = make_cq("HB9IPH", "JN47", "DX");
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CQ_STD);
        EXPECT_EQ(dec.call_1, "HB9IPH");
        EXPECT_EQ(dec.locator, "JN47");
        EXPECT_EQ(dec.modifier, "DX");
    }

    // Type 2: CQ_NONSTD_1 (<= 9 chars + loc)
    {
        Message msg;
        msg.type = MessageType::CQ_NONSTD_1;
        msg.call_1 = "YO1YO";
        msg.locator = "FN31";
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CQ_NONSTD_1);
        EXPECT_EQ(dec.call_1, "YO1YO");
        EXPECT_EQ(dec.locator, "FN31");
    }

    // Type 3: CQ_NONSTD_2 (<= 9 chars + mod)
    {
        Message msg;
        msg.type = MessageType::CQ_NONSTD_2;
        msg.call_1 = "EA8/HB9IP";
        msg.modifier = "SOTA";
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CQ_NONSTD_2);
        EXPECT_EQ(dec.call_1, "EA8/HB9IP");
        EXPECT_EQ(dec.modifier, "SOTA");
    }

    // Type 4: CQ_NONSTD_3 (<= 13 chars)
    {
        Message msg;
        msg.type = MessageType::CQ_NONSTD_3;
        msg.call_1 = "3B9/HB9IPH/P";
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CQ_NONSTD_3);
        EXPECT_EQ(dec.call_1, "3B9/HB9IPH/P");
    }

    // Type 5: CALL_STD_NOSUF
    {
        Message msg = make_call("HB9IPH", "W1AW", "FN31", -18);
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CALL_STD_NOSUF);
        EXPECT_EQ(dec.call_1, "HB9IPH");
        EXPECT_EQ(dec.call_2, "W1AW");
        EXPECT_EQ(dec.locator, "FN31");
        EXPECT_EQ(dec.rst_db, -18);
    }

    // Type 6: CALL_STD_SUF
    {
        Message msg;
        msg.type = MessageType::CALL_STD_SUF;
        msg.call_1 = "YO1YO";
        msg.hash_1 = hash_callsign_24("YO1YO");
        msg.call_2 = "HB9IPH/P";
        msg.suffix_2 = 1;
        msg.locator = "JN47";
        msg.rst_db = -3;
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CALL_STD_SUF);
        EXPECT_EQ(dec.hash_1, msg.hash_1);
        EXPECT_EQ(dec.call_2, "HB9IPH/P");
        EXPECT_EQ(dec.locator, "JN47");
        EXPECT_EQ(dec.rst_db, -3);
    }

    // Type 7: CALL_NONSTD (Target Hash 20b + Caller 48b + 1b Suf + 5b SNR)
    {
        Message msg;
        msg.type = MessageType::CALL_NONSTD;
        msg.hash_1 = hash_callsign_20("EA8/HB9IPH");
        msg.call_2 = "EA6/HB9IP/P";
        msg.suffix_2 = 1;
        msg.rst_db = 5;
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        std::vector<std::string_view> known = {"EA8/HB9IPH", "EA6/HB9IP/P"};
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec, known.data(), known.size()));
        EXPECT_EQ(dec.type, MessageType::CALL_NONSTD);
        EXPECT_EQ(dec.hash_1, msg.hash_1);
        EXPECT_EQ(dec.call_2, "EA6/HB9IP/P");
        EXPECT_EQ(dec.suffix_2, 1);
        EXPECT_EQ(dec.rst_db, 5);
    }

    // Type 8: REPORT73_STD
    {
        Message msg = make_report73("HB9IPH", "W1AW", 0);
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::REPORT73_STD);
        EXPECT_EQ(dec.call_1, "HB9IPH");
        EXPECT_EQ(dec.call_2, "W1AW");
        EXPECT_EQ(dec.rst_db, 0);
    }

    // Type 9: M73_STD
    {
        Message msg = make_73("HB9IPH", "W1AW");
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::M73_STD);
        EXPECT_EQ(dec.call_1, "HB9IPH");
        EXPECT_EQ(dec.call_2, "W1AW");
    }

    // Type 10: M73_NONSTD
    {
        Message msg;
        msg.type = MessageType::M73_NONSTD;
        msg.hash_1 = hash_callsign_24("EA8/HB9IPH");
        msg.call_2 = "W1AW";
        msg.suffix_2 = 0;
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::M73_NONSTD);
        EXPECT_EQ(dec.hash_1, msg.hash_1);
        EXPECT_EQ(dec.call_2, "W1AW");
    }

    // Type 11: MULTI_REPORT73 (16-bit DX hash)
    {
        Message msg = make_multi_report73("HB9IPH", "YO1YO", 5, "TU2TU", -3);
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::MULTI_REPORT73);
        EXPECT_EQ(dec.hash_1, hash_callsign_16("HB9IPH"));
        ASSERT_EQ(dec.multi_targets.size(), 2U);
        EXPECT_EQ(dec.multi_targets[0].hash, hash_callsign_24("YO1YO"));
        EXPECT_EQ(dec.multi_targets[0].rst_db, 5);
        EXPECT_EQ(dec.multi_targets[1].hash, hash_callsign_24("TU2TU"));
        EXPECT_EQ(dec.multi_targets[1].rst_db, -3);
    }

    // Type 12: MULTI_73 (16-bit DX hash)
    {
        Message msg;
        msg.type = MessageType::MULTI_73;
        msg.call_1 = "HB9IPH";
        msg.hash_1 = hash_callsign_16("HB9IPH");
        msg.multi_targets = {
            {"YO1YO", hash_callsign_24("YO1YO"), 0},
            {"TU2TU", hash_callsign_24("TU2TU"), 0}
        };
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::MULTI_73);
        EXPECT_EQ(dec.hash_1, hash_callsign_16("HB9IPH"));
        ASSERT_EQ(dec.multi_targets.size(), 2U);
        EXPECT_EQ(dec.multi_targets[0].hash, hash_callsign_24("YO1YO"));
        EXPECT_EQ(dec.multi_targets[1].hash, hash_callsign_24("TU2TU"));
    }

    // Type 13: FREE_TEXT
    {
        Message msg = make_free_text("HELLO WORLD");
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::FREE_TEXT);
        EXPECT_EQ(dec.text, "HELLO WORLD");
    }

    // Type 14: RESERVED_A
    {
        Message msg;
        msg.type = MessageType::RESERVED_A;
        msg.raw_payload = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA};
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::RESERVED_A);
    }
}

// ============================================================================
// 8. Multi-Protocol Audio Demodulation Matrix (LQ8, LQ4, LQ2 across Frequencies & Rates)
// ============================================================================

TEST(ComprehensiveMatrixTest, AudioPhysicalLayerMultiRateMatrix) {
    std::vector<Protocol> protocols = {Protocol::LQ8, Protocol::LQ4, Protocol::LQ2, Protocol::LQ16};
    std::vector<float> sample_rates = {8000.0f, 12000.0f, 16000.0f, 48000.0f};
    std::vector<float> carrier_freqs = {800.0f, 1500.0f, 2200.0f};

    Message test_msg = make_cq("HB9IPH", "JN47", "DX");

    for (auto proto : protocols) {
        for (float sr : sample_rates) {
            for (float f0 : carrier_freqs) {
                std::vector<float> audio;
                bool tx_ok = message_to_audio(test_msg, proto, f0, sr, audio);
                ASSERT_TRUE(tx_ok) << "TX failed for proto=" << static_cast<int>(proto)
                                   << " sr=" << sr << " f0=" << f0;

                Message rx_msg;
                bool rx_ok = audio_to_message(audio, f0, sr, proto, rx_msg);
                ASSERT_TRUE(rx_ok) << "RX failed for proto=" << static_cast<int>(proto)
                                   << " sr=" << sr << " f0=" << f0;
                EXPECT_EQ(rx_msg.type, MessageType::CQ_STD);
                EXPECT_EQ(rx_msg.call_1, "HB9IPH");
                EXPECT_EQ(rx_msg.locator, "JN47");
                EXPECT_EQ(rx_msg.modifier, "DX");
            }
        }
    }
}

// ============================================================================
// 9. 500+ Randomized Property-Based Roundtrip Contacts (Full Pipeline)
// ============================================================================

TEST(ComprehensiveMatrixTest, Randomized500ContactExchanges) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> prefix_char_dist('A', 'Z');
    std::uniform_int_distribution<int> digit_dist(0, 9);
    std::uniform_int_distribution<int> rst_dist(-26, 5);
    std::uniform_int_distribution<int> grid_field_dist('A', 'R');
    std::uniform_int_distribution<int> grid_square_dist(0, 9);
    std::uniform_int_distribution<int> mod_select(0, 5);

    std::vector<std::string> sample_mods = {"", "DX", "POTA", "SOTA", "QRP", "TEST"};

    auto gen_call = [&]() -> std::string {
        std::string call;
        call += static_cast<char>(prefix_char_dist(rng));
        call += static_cast<char>(prefix_char_dist(rng));
        call += std::to_string(digit_dist(rng));
        call += static_cast<char>(prefix_char_dist(rng));
        call += static_cast<char>(prefix_char_dist(rng));
        call += static_cast<char>(prefix_char_dist(rng));
        return call;
    };

    auto gen_grid = [&]() -> std::string {
        std::string grid;
        grid += static_cast<char>(grid_field_dist(rng));
        grid += static_cast<char>(grid_field_dist(rng));
        grid += std::to_string(grid_square_dist(rng));
        grid += std::to_string(grid_square_dist(rng));
        return grid;
    };

    for (int i = 0; i < 500; ++i) {
        std::string station = gen_call();
        std::string caller = gen_call();
        std::string grid = gen_grid();
        int rst = rst_dist(rng);
        std::string mod = sample_mods[mod_select(rng)];

        // Test 1: CQ
        Message cq_msg = make_cq(station, grid, mod);
        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(cq_msg, payload));
        Message cq_dec;
        ASSERT_TRUE(decode_message(payload, cq_dec));
        EXPECT_EQ(cq_dec.type, MessageType::CQ_STD);
        EXPECT_EQ(cq_dec.call_1, station);
        EXPECT_EQ(cq_dec.locator, grid);
        EXPECT_EQ(cq_dec.modifier, mod);

        // Test 2: CALL
        Message call_msg = make_call(station, caller, grid, rst);
        memset(payload, 0, sizeof(payload));
        ASSERT_TRUE(encode_message(call_msg, payload));
        Message call_dec;
        ASSERT_TRUE(decode_message(payload, call_dec));
        EXPECT_EQ(call_dec.type, MessageType::CALL_STD_NOSUF);
        EXPECT_EQ(call_dec.call_1, station);
        EXPECT_EQ(call_dec.call_2, caller);
        EXPECT_EQ(call_dec.locator, grid);
        EXPECT_EQ(call_dec.rst_db, rst);

        // Test 3: REPLY73
        Message rep_msg = make_reply73(station, caller, rst);
        memset(payload, 0, sizeof(payload));
        ASSERT_TRUE(encode_message(rep_msg, payload));
        Message rep_dec;
        ASSERT_TRUE(decode_message(payload, rep_dec));
        EXPECT_EQ(rep_dec.type, MessageType::REPLY73_STD);
        EXPECT_EQ(rep_dec.call_1, station);
        EXPECT_EQ(rep_dec.call_2, caller);
        EXPECT_EQ(rep_dec.rst_db, rst);
    }
}

// ============================================================================
// 10. Hash Distribution Uniformity (1,000 Synthetic Callsigns)
// ============================================================================

TEST(ComprehensiveMatrixTest, HashDistribution1000Callsigns) {
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> char_dist('A', 'Z');
    std::uniform_int_distribution<int> num_dist('0', '9');

    std::vector<uint32_t> hashes;
    hashes.reserve(1000);

    for (int i = 0; i < 1000; ++i) {
        std::string call;
        call += static_cast<char>(char_dist(rng));
        call += static_cast<char>(char_dist(rng));
        call += static_cast<char>(num_dist(rng));
        call += static_cast<char>(char_dist(rng));
        call += "/P";

        uint32_t h = hash_callsign_24(call);
        EXPECT_LT(h, (1U << 24));
        hashes.push_back(h);
    }

    // Check for collisions across 1,000 random hashes
    std::sort(hashes.begin(), hashes.end());
    size_t unique_count = std::unique(hashes.begin(), hashes.end()) - hashes.begin();
    EXPECT_GE(unique_count, 995u) << "Unexpectedly high collision rate in 24-bit hash";
}

// ============================================================================
// 11. Corrupted Bitstream Fuzzing (CRC Rejection & Robustness)
// ============================================================================

TEST(ComprehensiveMatrixTest, FuzzCorruptedPayloads) {
    std::mt19937 rng(999);
    std::uniform_int_distribution<int> byte_dist(0, 255);

    for (int i = 0; i < 200; ++i) {
        uint8_t corrupt_payload[PAYLOAD_BYTES];
        for (int b = 0; b < PAYLOAD_BYTES; ++b) {
            corrupt_payload[b] = static_cast<uint8_t>(byte_dist(rng));
        }

        // Must not crash or trigger undefined behaviour
        Message dec;
        decode_message(corrupt_payload, dec);
    }
}
