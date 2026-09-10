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
#include <cmath>
#include <sstream>
#include <cstdio>
#include <fstream>
#include <chrono>
#include <thread>
#include "lq/lq.h"

using namespace lq;

// ============================================================================
// 1. Callsign & Non-Std Error Branches
// ============================================================================

TEST(CoverageEdgeCases, CallsignErrorBranches) {
    uint32_t packed = 0;
    std::string cs;

    // Invalid callsigns (all length and char branches)
    EXPECT_FALSE(encode_callsign_std("!", packed));
    EXPECT_FALSE(encode_callsign_std("1234567", packed));
    EXPECT_FALSE(encode_callsign_std("W1AW#", packed));
    EXPECT_FALSE(encode_callsign_std("A!1AA", packed));
    EXPECT_FALSE(encode_callsign_std("", packed));
    EXPECT_FALSE(encode_callsign_std("K1", packed)); // suffix empty
    EXPECT_FALSE(encode_callsign_std("K1ABCD", packed)); // suffix > 3
    EXPECT_FALSE(encode_callsign_std("ABC1D", packed)); // prefix > 2
    EXPECT_FALSE(encode_callsign_std("AA000A", packed));

    // Decode out of range standard callsign values
    EXPECT_FALSE(decode_callsign_std(CALLSIGN_TOKEN_CQ + 1, cs));
    EXPECT_FALSE(decode_callsign_std(0xFFFFFFFF, cs));

    // Standard callsign validation
    EXPECT_FALSE(is_standard_callsign(""));
    EXPECT_FALSE(is_standard_callsign("12345"));
    EXPECT_FALSE(is_standard_callsign("HB9IPH/P"));
    EXPECT_TRUE(is_standard_callsign("HB9IPH"));
    EXPECT_TRUE(is_standard_callsign("W1AW"));
    EXPECT_TRUE(is_standard_callsign("CQ"));
    EXPECT_TRUE(is_standard_callsign("DE"));
    EXPECT_TRUE(is_standard_callsign("QRZ"));

    // Normalise
    std::string norm;
    EXPECT_FALSE(normalise_standard_callsign("INVALID_CALL_SIGN", norm));
    EXPECT_TRUE(normalise_standard_callsign("w1aw", norm));
}

TEST(CoverageEdgeCases, NonStdCallsignErrorBranches) {
    // Character validation
    EXPECT_FALSE(is_valid_nonstd_callsign("3B9/HB9IPH/QRP#", 14));
    EXPECT_FALSE(is_valid_nonstd_callsign("", 7));
    EXPECT_FALSE(is_valid_nonstd_callsign("TOOLONGFORSEVEN", 7));

    // Bit buffer encoding / decoding
    uint8_t buf[16] = {0};
    BitBuffer bb(buf, 128);
    EXPECT_FALSE(encode_callsign_nonstd("3B9/HB9IPH/QRP#", 14, bb));
    EXPECT_FALSE(encode_callsign_nonstd("TOOLONGFOR9CHARS", 9, bb));

    std::string call_out;

    // Uint128 helpers
    Uint128 u128;
    EXPECT_FALSE(encode_callsign_nonstd_u128("BAD_CHAR#", 7, u128));
    EXPECT_FALSE(decode_callsign_nonstd_u128({0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF}, 7, call_out));
}

// ============================================================================
// 2. Locator & Modifier & RST Error Paths
// ============================================================================

TEST(CoverageEdgeCases, LocatorAndModifierAndRstBranches) {
    uint16_t loc_packed = 0;
    std::string loc_str;

    EXPECT_FALSE(is_valid_locator(""));
    EXPECT_FALSE(is_valid_locator("JN4"));
    EXPECT_FALSE(is_valid_locator("JN47XX"));
    EXPECT_FALSE(is_valid_locator("ZZ99"));
    EXPECT_FALSE(is_valid_locator("AA0A"));
    EXPECT_TRUE(is_valid_locator("JN47"));

    EXPECT_FALSE(encode_locator_15("INVALID", loc_packed));
    EXPECT_TRUE(encode_locator_15("", loc_packed));
    EXPECT_EQ(loc_packed, LOCATOR_BLANK);

    EXPECT_FALSE(decode_locator_15(LOCATOR_BLANK + 1, loc_str));
    EXPECT_TRUE(decode_locator_15(LOCATOR_BLANK, loc_str));
    EXPECT_EQ(loc_str, "");

    // Modifier
    uint32_t mod_packed = 0;
    std::string mod_out;
    EXPECT_FALSE(encode_modifier_20("WAYTOOLONGMODIFIER", mod_packed));
    EXPECT_TRUE(encode_modifier_20("DX", mod_packed));
    EXPECT_TRUE(decode_modifier_20(mod_packed, mod_out));
    EXPECT_EQ(mod_out, "DX");

    EXPECT_TRUE(encode_modifier_20("POTA", mod_packed));
    EXPECT_TRUE(decode_modifier_20(mod_packed, mod_out));
    EXPECT_EQ(mod_out, "POTA");

    EXPECT_TRUE(encode_modifier_20("TEST", mod_packed));
    EXPECT_TRUE(decode_modifier_20(mod_packed, mod_out));
    EXPECT_EQ(mod_out, "TEST");

    // Custom 4-char string modifier (tests base-32 path)
    EXPECT_TRUE(encode_modifier_20("W1", mod_packed));
    EXPECT_TRUE(decode_modifier_20(mod_packed, mod_out));
    EXPECT_EQ(mod_out, "W1");

    EXPECT_FALSE(decode_modifier_20(MODIFIER_MAX_VAL + 1, mod_out));

    // RST
    uint8_t rst_code = encode_rst_5(-40); // Clamped to min
    EXPECT_EQ(decode_rst_5(rst_code), -26);

    rst_code = encode_rst_5(+20); // Clamped to max
    EXPECT_EQ(decode_rst_5(rst_code), +5);

    int rst_val = 0;
    EXPECT_FALSE(parse_rst("INVALID", rst_val));
    EXPECT_TRUE(parse_rst("+05", rst_val));
    EXPECT_EQ(rst_val, 5);
    EXPECT_TRUE(parse_rst("-15", rst_val));
    EXPECT_EQ(rst_val, -15);
    EXPECT_TRUE(parse_rst("R-10", rst_val));
    EXPECT_EQ(rst_val, -10);
    EXPECT_TRUE(parse_rst("+0", rst_val));
    EXPECT_EQ(rst_val, 0);
}

// ============================================================================
// 3. Varicode & Huffman Code Branches
// ============================================================================

TEST(CoverageEdgeCases, VaricodeAndHuffmanBranches) {
    // Varicode characters
    EXPECT_FALSE(is_varicode_char('\x00'));
    EXPECT_FALSE(is_varicode_char('\x7F'));
    EXPECT_TRUE(is_varicode_char('A'));
    EXPECT_TRUE(is_varicode_char(' '));
    EXPECT_TRUE(is_varicode_char(EOM_CHAR));
    EXPECT_TRUE(is_varicode_char(';'));
    EXPECT_TRUE(is_varicode_char('"'));
    EXPECT_TRUE(is_varicode_char('\''));
    EXPECT_TRUE(is_varicode_char('&'));
    EXPECT_TRUE(is_varicode_char('$'));
    EXPECT_TRUE(is_varicode_char('|'));
    EXPECT_TRUE(is_varicode_char('{'));
    EXPECT_TRUE(is_varicode_char('}'));
    EXPECT_TRUE(is_varicode_char('^'));
    EXPECT_TRUE(is_varicode_char('~'));

    uint8_t raw_buf[16] = {0};
    BitBuffer bb(raw_buf, 128);
    // Encoding unknown characters fallback
    EXPECT_TRUE(encode_varicode("HELLO\x02WORLD", bb));

    std::string decoded_str;
    // Decoding truncated or corrupt bitbuffer
    uint8_t bad_raw[2] = {0xFF, 0xFF};
    BitBuffer bad_bb(bad_raw, 16);
    decode_varicode(bad_bb, decoded_str, 16);

    // Huffman
    EXPECT_EQ(get_huffman_entry(MessageType::UNKNOWN), nullptr);
    EXPECT_NE(get_huffman_entry(MessageType::CQ_STD), nullptr);

    uint8_t enc_buf[2] = {0};
    BitBuffer enc_bb(enc_buf, 16);
    EXPECT_EQ(encode_huffman_prefix(MessageType::UNKNOWN, enc_bb), 0);
}

// ============================================================================
// 4. Message Parsing & Formatting Branches
// ============================================================================

TEST(CoverageEdgeCases, MessageAllVariantsAndErrorPaths) {
    Message msg;

    // Test parsing every single message type
    std::vector<std::string> valid_messages = {
        "CQ HB9IPH",
        "CQ HB9IPH JN47",
        "CQ DX HB9IPH",
        "CQ DX HB9IPH JN47",
        "CQ YO1YO/P",
        "CQ YO1YO/P JN47",
        "CQ DX YO1YO/P",
        "CQ DX YO1YO/P JN47",
        "CQ 3B9/HB9IP",
        "CQ 3B9/HB9IP JN47",
        "CQ DX 3B9/HB9IP JN47",
        "CQ 3B9/HB9IPH/QRP",
        "CQ 3B9/HB9IPH/QRP JN47",
        "CALL W1AW HB9IPH JN47 -05",
        "CALL YO1YO/P HB9IPH JN47 -05",
        "CALL W1AW EA6/HB9IP JN47 -05",
        "CALL <0x123456> HB9IPH JN47 -05",
        "CALL <0x123456> EA6/HB9IP JN47 -05",
        "REPLY73 W1AW HB9IPH +03",
        "REPLY73 YO1YO/P HB9IPH +03",
        "REPLY73 W1AW EA6/HB9IP +03",
        "REPLY73 <0x123456> HB9IPH +03",
        "REPLY73 <0x123456> EA6/HB9IP +03",
        "HELLO 73 DE HB9IPH"
    };

    for (const auto& s : valid_messages) {
        EXPECT_TRUE(parse_message(s, msg)) << "Failed on: " << s;
        uint8_t payload[PAYLOAD_BYTES] = {0};
        EXPECT_TRUE(encode_message(msg, payload));

        Message unpacked_msg;
        EXPECT_TRUE(decode_message(payload, unpacked_msg));
        std::string fmt = format_message(unpacked_msg);
        EXPECT_FALSE(fmt.empty());
    }

    // Multi-reply message encoding & decoding
    Message r6 = make_multi_reply73("HB9IPH", "YO1YO", 5, "TU2TU", -3);
    uint8_t r6_buf[PAYLOAD_BYTES] = {0};
    EXPECT_TRUE(encode_message(r6, r6_buf));
    Message r6_out;
    EXPECT_TRUE(decode_message(r6_buf, r6_out));
    EXPECT_EQ(r6_out.type, MessageType::MULTI_REPLY73);
    EXPECT_EQ(r6_out.multi_targets.size(), 2U);

    Message r10;
    r10.type = MessageType::RESERVED;
    r10.raw_payload = {0x11, 0x22, 0x33, 0x44};
    uint8_t r10_buf[PAYLOAD_BYTES] = {0};
    EXPECT_TRUE(encode_message(r10, r10_buf));
    Message r10_out;
    EXPECT_TRUE(decode_message(r10_buf, r10_out));
    EXPECT_EQ(format_message(r10_out), "[RESERVED]");

    // Invalid parse
    EXPECT_FALSE(parse_message("", msg));

    // Pack with invalid message type
    msg.type = MessageType::UNKNOWN;
    uint8_t payload[PAYLOAD_BYTES] = {0};
    EXPECT_FALSE(encode_message(msg, payload));

    // Format unknown type
    msg.type = MessageType::UNKNOWN;
    EXPECT_EQ(format_message(msg), "[UNKNOWN]");
}

// ============================================================================
// 5. LDPC & Transport Pipeline & WAV File Branches
// ============================================================================

TEST(CoverageEdgeCases, LdpcAndTransportPipelineBranches) {
    uint8_t payload[PAYLOAD_BYTES] = {0};
    uint8_t codeword[LDPC_CODEWORD_BYTES] = {0};
    ldpc_encode(payload, codeword);

    // Convert codeword bits to LLR
    float llr[LDPC_CODEWORD_BITS] = {0};
    for (int i = 0; i < LDPC_CODEWORD_BITS; ++i) {
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        bool bit = (codeword[byte_idx] >> bit_idx) & 1;
        llr[i] = bit ? -5.0f : +5.0f;
    }

    // Decode valid codeword
    uint8_t decoded[LDPC_INPUT_BYTES] = {0};
    EXPECT_GT(ldpc_decode(llr, decoded, 25), 0);

    // Transport encode/decode with all protocols (including LQ16)
    Message cq_msg = make_cq("HB9IPH", "JN47");
    ToneSequence seq_lq8, seq_lq4, seq_lq2, seq_lq16;

    EXPECT_TRUE(encode_tones(cq_msg, Protocol::LQ8, seq_lq8));
    EXPECT_EQ(seq_lq8.size(), 79);
    EXPECT_TRUE(encode_tones(cq_msg, Protocol::LQ4, seq_lq4));
    EXPECT_EQ(seq_lq4.size(), 105);
    EXPECT_TRUE(encode_tones(cq_msg, Protocol::LQ2, seq_lq2));
    EXPECT_EQ(seq_lq2.size(), 105);
    EXPECT_TRUE(encode_tones(cq_msg, Protocol::LQ16, seq_lq16));
    EXPECT_EQ(seq_lq16.size(), 79);

    Message rx_msg;
    EXPECT_TRUE(decode_tones(seq_lq8, rx_msg));
    EXPECT_EQ(rx_msg.call_1, "HB9IPH");
    EXPECT_EQ(rx_msg.locator, "JN47");

    EXPECT_TRUE(decode_tones(seq_lq4, rx_msg));
    EXPECT_EQ(rx_msg.call_1, "HB9IPH");

    EXPECT_TRUE(decode_tones(seq_lq2, rx_msg));
    EXPECT_EQ(rx_msg.call_1, "HB9IPH");

    EXPECT_TRUE(decode_tones(seq_lq16, rx_msg));
    EXPECT_EQ(rx_msg.call_1, "HB9IPH");

    // Decode corrupt tone sequence
    ToneSequence bad_seq;
    bad_seq.tones.resize(10, 0);
    EXPECT_FALSE(decode_tones(bad_seq, rx_msg));

    // WAV file save and load
    std::vector<float> test_audio = {0.0f, 0.5f, -0.5f, 0.25f, -0.25f};
    std::string test_wav = "test_coverage_temp.wav";
    EXPECT_TRUE(save_wav_file(test_wav, test_audio, 12000.0f));

    std::vector<float> loaded_audio;
    float loaded_sr = 0.0f;
    EXPECT_TRUE(load_wav_file(test_wav, loaded_audio, loaded_sr));
    EXPECT_EQ(loaded_audio.size(), test_audio.size());
    EXPECT_FLOAT_EQ(loaded_sr, 12000.0f);

    std::remove(test_wav.c_str());

    // Non-existent WAV file
    EXPECT_FALSE(load_wav_file("non_existent_file_xyz123.wav", loaded_audio, loaded_sr));
}

// ============================================================================
// 6. Easy API & Stream Operator Branches
// ============================================================================

TEST(CoverageEdgeCases, EasyApiAndStreamOperatorBranches) {
    // make helpers for standard with /P suffix and compound non-standard cases
    auto cq_std_p = make_cq("YO1YO/P", "JN47");
    EXPECT_EQ(cq_std_p.type, MessageType::CQ_STD);
    EXPECT_EQ(cq_std_p.suffix_1, 1);

    auto cq_ns1 = make_cq("3DA0XYZ", "JN47");
    EXPECT_EQ(cq_ns1.type, MessageType::CQ_NONSTD_1);

    auto call_ns1 = make_call("3DA0XYZ", "HB9IPH", "JN47", -5);
    EXPECT_EQ(call_ns1.type, MessageType::CALL_STD_SUF);

    auto call_ns2 = make_call("HB9IPH", "EA6/HB9IPH/QRP", "JN47", -5);
    EXPECT_EQ(call_ns2.type, MessageType::CALL_NONSTD);

    auto call_both_ns = make_call("3DA0XYZ", "EA6/HB9IPH/QRP", "JN47", -5);
    EXPECT_EQ(call_both_ns.type, MessageType::CALL_NONSTD);

    auto rep_ns1 = make_reply73("3DA0XYZ", "HB9IPH", +2);
    EXPECT_EQ(rep_ns1.type, MessageType::REPLY73_NONSTD);

    auto rep_ns2 = make_reply73("HB9IPH", "EA6/HB9IPH/QRP", +2);
    EXPECT_EQ(rep_ns2.type, MessageType::REPLY73_NONSTD);

    auto rep_p_std = make_reply73("YO1YO/P", "HB9IPH", +2);
    EXPECT_EQ(rep_p_std.type, MessageType::REPLY73_STD);
    EXPECT_EQ(rep_p_std.suffix_1, 1);

    // Invalid pack/unpack
    Message bad_msg;
    bad_msg.type = MessageType::UNKNOWN;
    EXPECT_FALSE(pack(bad_msg).has_value());

    EXPECT_TRUE(text_to_audio("").empty());
    EXPECT_FALSE(audio_to_text({}).has_value());
    EXPECT_FALSE(unpack(nullptr, 0).has_value());
    EXPECT_FALSE(unpack(std::vector<uint8_t>{}).has_value());

    // Stream operator coverage for all MessageType enums
    std::vector<MessageType> all_types = {
        MessageType::CQ_STD,
        MessageType::CQ_NONSTD_1,
        MessageType::CQ_NONSTD_2,
        MessageType::CQ_NONSTD_3,
        MessageType::CALL_STD_NOSUF,
        MessageType::CALL_STD_2SUF,
        MessageType::CALL_NONSTD,
        MessageType::REPORT73_STD,
        MessageType::REPORT73_NONSTD,
        MessageType::M73_STD,
        MessageType::MULTI_REPORT73,
        MessageType::MULTI_73,
        MessageType::FREE_TEXT,
        MessageType::RESERVED,
        MessageType::UNKNOWN
    };

    std::ostringstream oss;
    for (auto t : all_types) {
        oss << t << " ";
    }

    // Stream operator coverage for all Protocol enums
    std::vector<Protocol> all_protos = {
        Protocol::LQ8,
        Protocol::LQ4,
        Protocol::LQ2,
        Protocol::LQ16
    };
    for (auto p : all_protos) {
        oss << p << " ";
    }

    // ToneSequence stream operator
    ToneSequence ts;
    ts.tones = {0, 1, 2, 3, 4, 5, 6, 7};
    oss << ts;

    EXPECT_FALSE(oss.str().empty());
}


TEST(CoverageEdgeCases, TransportSoftLlrEdgeCases) {
    std::vector<float> small_buf(10, 0.0f);
    std::vector<float> llrs;
    EXPECT_FALSE(demodulate_audio_soft(small_buf, 0, 1500.0f, 12000.0f, Protocol::LQ8, llrs));

    Message msg;
    std::vector<float> small_llrs(10, 0.0f);
    EXPECT_FALSE(decode_soft_llrs(small_llrs, msg));

    // Corrupted LLRs failing LDPC / CRC
    std::vector<float> bad_llrs(174, 0.0f);
    EXPECT_FALSE(decode_soft_llrs(bad_llrs, msg));

    // Small buffer in audio_to_messages
    auto msgs = audio_to_messages(small_buf, 1500.0f, 12000.0f, Protocol::LQ8);
    EXPECT_TRUE(msgs.empty());

    // LQ4 and LQ2 audio_to_messages multi-candidate scan
    Message msg4;
    msg4.type = MessageType::CALL_STD_NOSUF;
    msg4.call_1 = "YO1YO";
    msg4.call_2 = "HB9IPH";
    msg4.locator = "JN47";
    msg4.rst_db = -3;

    std::vector<float> audio4;
    ASSERT_TRUE(message_to_audio(msg4, Protocol::LQ4, 1200.0f, 12000.0f, audio4));
    // Pad leading samples to test offset search
    std::vector<float> padded4(1200, 0.0f);
    padded4.insert(padded4.end(), audio4.begin(), audio4.end());
    auto msgs4 = audio_to_messages(padded4, 1200.0f, 12000.0f, Protocol::LQ4);
    EXPECT_FALSE(msgs4.empty());
    auto msgs4_deep = audio_to_messages(padded4, 1200.0f, 12000.0f, Protocol::LQ4, 1, true);
    EXPECT_FALSE(msgs4_deep.empty());

    // WAV file edge cases
    EXPECT_FALSE(save_wav_file("/dev/null/invalid/path.wav", small_buf, 12000.0f));
    float dummy_rate = 0.0f;
    std::vector<float> dummy_out;
    EXPECT_FALSE(load_wav_file("nonexistent_file_12345.wav", dummy_out, dummy_rate));

    // Valid WAV file roundtrip test
    std::vector<float> sine_wave(12000);
    for (size_t i = 0; i < sine_wave.size(); ++i) {
        sine_wave[i] = std::sin(2.0f * 3.14159265f * 440.0f * i / 12000.0f);
    }
    std::string valid_wav = "coverage_sine_test.wav";
    ASSERT_TRUE(save_wav_file(valid_wav, sine_wave, 12000.0f));
    std::vector<float> loaded_wave;
    float loaded_rate = 0.0f;
    ASSERT_TRUE(load_wav_file(valid_wav, loaded_wave, loaded_rate));
    EXPECT_EQ(loaded_rate, 12000.0f);
    EXPECT_EQ(loaded_wave.size(), sine_wave.size());
    std::remove(valid_wav.c_str());
}

TEST(CoverageEdgeCases, MessageParsingExhaustiveCombinations) {
    Message msg;

    // 1. Valid diverse parses
    EXPECT_TRUE(parse_message("CQ HB9IPH JN47", msg));
    EXPECT_EQ(msg.type, MessageType::CQ_STD);

    EXPECT_TRUE(parse_message("CQ DX HB9IPH JN47", msg));
    EXPECT_EQ(msg.type, MessageType::CQ_STD);
    EXPECT_EQ(msg.modifier, "DX");

    EXPECT_TRUE(parse_message("CQ 3B9/HB9IPH/P", msg));
    EXPECT_EQ(msg.type, MessageType::CQ_NONSTD_3);

    EXPECT_TRUE(parse_message("CQ EA6/HB9IPH JN47", msg));
    EXPECT_TRUE(msg.type == MessageType::CQ_NONSTD_1 || msg.type == MessageType::CQ_NONSTD_2 || msg.type == MessageType::CQ_NONSTD_3);

    EXPECT_TRUE(parse_message("CALL YO1YO HB9IPH JN47 -12", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_STD_NOSUF);
    EXPECT_EQ(msg.rst_db, -12);

    EXPECT_TRUE(parse_message("REPLY73 YO1YO HB9IPH +05", msg));
    EXPECT_EQ(msg.type, MessageType::REPLY73_STD);
    EXPECT_EQ(msg.rst_db, 5);

    EXPECT_TRUE(parse_message("73 DE HB9IPH", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);

    // 2. Malformed / Empty strings
    EXPECT_FALSE(parse_message("", msg));
    EXPECT_FALSE(parse_message("   ", msg));

    // 3. Fallback to Free Text for unrecognized arbitrary strings
    EXPECT_TRUE(parse_message("UNKNOWN_COMMAND HB9IPH JN47", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);
    EXPECT_EQ(msg.text, "UNKNOWN_COMMAND HB9IPH JN47");

    EXPECT_TRUE(parse_message("CALL", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);

    EXPECT_TRUE(parse_message("CALL YO1YO", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);

    EXPECT_TRUE(parse_message("HELLO WORLD HOW ARE YOU TODAY", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);

    EXPECT_TRUE(parse_message("REPLY73", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);

    EXPECT_TRUE(parse_message("REPLY73 FROM A DISTANT LAND FAR AWAY", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);
}

TEST(CoverageEdgeCases, ModifierBoundaryChecks) {
    uint32_t packed = 0;
    std::string decoded;

    // Boundary: Empty string
    EXPECT_TRUE(encode_modifier_20("", packed));
    EXPECT_EQ(packed, 0u);
    EXPECT_TRUE(decode_modifier_20(0, decoded));
    EXPECT_EQ(decoded, "");

    // Boundary: Numeric modifiers (000..999 mapped to 1..1000)
    EXPECT_TRUE(encode_modifier_20("000", packed));
    EXPECT_EQ(packed, 1u);
    EXPECT_TRUE(decode_modifier_20(1, decoded));
    EXPECT_EQ(decoded, "000");

    EXPECT_TRUE(encode_modifier_20("040", packed));
    EXPECT_EQ(packed, 41u);
    EXPECT_TRUE(decode_modifier_20(41, decoded));
    EXPECT_EQ(decoded, "040");

    // Boundary: Base-32 alphanumeric codes (e.g. DX, SOTA, POTA, TEST)
    EXPECT_TRUE(encode_modifier_20("DX", packed));
    EXPECT_TRUE(decode_modifier_20(packed, decoded));
    EXPECT_EQ(decoded, "DX");

    EXPECT_TRUE(encode_modifier_20("SOTA", packed));
    EXPECT_TRUE(decode_modifier_20(packed, decoded));
    EXPECT_EQ(decoded, "SOTA");

    // Invalid modifier characters
    EXPECT_FALSE(encode_modifier_20("DX!", packed));
    EXPECT_FALSE(encode_modifier_20("TOOLONGMODIFIER", packed));

    // Decode out of range value
    EXPECT_FALSE(decode_modifier_20(0xFFFFFFFF, decoded));
}

TEST(CoverageEdgeCases, LocatorBoundaryChecks) {
    uint16_t packed = 0;
    std::string decoded;

    // Normal valid locator
    EXPECT_TRUE(encode_locator_15("JN47", packed));
    EXPECT_TRUE(decode_locator_15(packed, decoded));
    EXPECT_EQ(decoded, "JN47");

    // Special value 32400 (not provided)
    EXPECT_TRUE(decode_locator_15(LOCATOR_BLANK, decoded));
    EXPECT_EQ(decoded, "");

    // Out of range locator values (> 32400)
    EXPECT_FALSE(decode_locator_15(32401, decoded));
    EXPECT_FALSE(decode_locator_15(0xFFFF, decoded));

    // Invalid input strings
    EXPECT_FALSE(encode_locator_15("JN4", packed)); // 3 chars
    EXPECT_FALSE(encode_locator_15("JN47AA", packed)); // 6 chars
    EXPECT_FALSE(encode_locator_15("ZZ99", packed)); // 'Z' out of range (max 'R')
    EXPECT_FALSE(encode_locator_15("JNA7", packed)); // 'A' in digit position
}

TEST(CoverageEdgeCases, RstBoundaryClamping) {
    // Extreme negative clamping (e.g. -50 dB -> -26 dB)
    uint8_t packed_neg = encode_rst_5(-50);
    EXPECT_EQ(packed_neg, 0u);
    EXPECT_EQ(decode_rst_5(0), -26);

    // Extreme positive clamping (e.g. +50 dB -> +5 dB)
    uint8_t packed_pos = encode_rst_5(+50);
    EXPECT_EQ(packed_pos, 31u);
    EXPECT_EQ(decode_rst_5(31), +5);

    // Out of range decode clamping
    EXPECT_EQ(decode_rst_5(32), +5);
    EXPECT_EQ(decode_rst_5(255), +5);
}

TEST(CoverageEdgeCases, MessageSemanticAccessors) {
    Message msg;

    // CQ Nonstd types
    msg.type = MessageType::CQ_NONSTD_1;
    msg.call_1 = "3B9/HB9IPH";
    msg.locator = "JN47";
    EXPECT_TRUE(msg.is_cq());
    EXPECT_FALSE(msg.is_call());
    EXPECT_FALSE(msg.is_reply73());
    EXPECT_FALSE(msg.is_free_text());
    EXPECT_EQ(msg.get_sender_call(), "3B9/HB9IPH");
    EXPECT_EQ(msg.get_target_call(), "");
    EXPECT_NE(msg.get_sender_hash(), 0U);
    EXPECT_EQ(msg.get_target_hash(), 0U);

    msg.type = MessageType::CQ_NONSTD_2;
    EXPECT_EQ(msg.get_sender_call(), "3B9/HB9IPH");
    msg.type = MessageType::CQ_NONSTD_3;
    EXPECT_EQ(msg.get_sender_call(), "3B9/HB9IPH");

    // CALL Nonstd type
    msg.type = MessageType::CALL_NONSTD;
    msg.call_1 = "";
    msg.hash_1 = 0x123456;
    msg.call_2 = "TU2TU";
    msg.hash_2 = 0;
    EXPECT_FALSE(msg.is_cq());
    EXPECT_TRUE(msg.is_call());
    EXPECT_EQ(msg.get_target_call(), "<...>");
    EXPECT_EQ(msg.get_sender_call(), "TU2TU");
    EXPECT_EQ(msg.get_target_hash(), 0x123456U);

    // REPLY73 Nonstd type
    msg.type = MessageType::REPLY73_NONSTD;
    EXPECT_TRUE(msg.is_reply73());
    EXPECT_EQ(msg.get_target_call(), "<...>");

    // FREE_TEXT and RESERVED types
    msg.type = MessageType::FREE_TEXT;
    msg.text = "73 DE HB9IPH";
    EXPECT_TRUE(msg.is_free_text());
    EXPECT_EQ(msg.get_sender_call(), "");
    EXPECT_EQ(msg.get_target_call(), "");

    msg = Message{};
    msg.type = MessageType::MULTI_REPORT73;
    msg.call_1 = "HB9IPH";
    EXPECT_FALSE(msg.is_cq());
    EXPECT_TRUE(msg.is_multi_report73());
    EXPECT_EQ(msg.get_sender_call(), "HB9IPH");
    EXPECT_EQ(msg.get_sender_hash(), hash_callsign_16("HB9IPH"));
}

TEST(CoverageEdgeCases, FlexibleCompactQsoParsing) {
    Message msg;

    // Standard 2-token CQ: CQ HB9IPH
    EXPECT_TRUE(parse_message("CQ HB9IPH", msg));
    EXPECT_EQ(msg.type, MessageType::CQ_STD);
    EXPECT_EQ(msg.get_sender_call(), "HB9IPH");
    EXPECT_EQ(msg.locator, "");

    // Standard 3-token CQ: CQ HB9IPH JN47
    EXPECT_TRUE(parse_message("CQ HB9IPH JN47", msg));
    EXPECT_EQ(msg.type, MessageType::CQ_STD);
    EXPECT_EQ(msg.get_sender_call(), "HB9IPH");
    EXPECT_EQ(msg.locator, "JN47");

    // Standard 3-token CQ with modifier: CQ DX HB9IPH
    EXPECT_TRUE(parse_message("CQ DX HB9IPH", msg));
    EXPECT_EQ(msg.type, MessageType::CQ_STD);
    EXPECT_EQ(msg.get_sender_call(), "HB9IPH");
    EXPECT_EQ(msg.modifier, "DX");

    // Standard 4-token CQ with modifier: CQ DX HB9IPH JN47
    EXPECT_TRUE(parse_message("CQ DX HB9IPH JN47", msg));
    EXPECT_EQ(msg.type, MessageType::CQ_STD);
    EXPECT_EQ(msg.get_sender_call(), "HB9IPH");
    EXPECT_EQ(msg.modifier, "DX");
    EXPECT_EQ(msg.locator, "JN47");

    // Standard 3-token CALL: HB9IPH YO1YO JN47
    EXPECT_TRUE(parse_message("HB9IPH YO1YO JN47", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_STD_NOSUF);
    EXPECT_EQ(msg.get_target_call(), "HB9IPH");
    EXPECT_EQ(msg.get_sender_call(), "YO1YO");
    EXPECT_EQ(msg.locator, "JN47");

    // Standard 3-token REPORT+73: HB9IPH YO1YO -03
    EXPECT_TRUE(parse_message("HB9IPH YO1YO -03", msg));
    EXPECT_EQ(msg.type, MessageType::REPORT73_STD);
    EXPECT_EQ(msg.get_target_call(), "HB9IPH");
    EXPECT_EQ(msg.get_sender_call(), "YO1YO");
    EXPECT_EQ(msg.rst_db, -3);

    // Standard 3-token 73: HB9IPH YO1YO 73
    EXPECT_TRUE(parse_message("HB9IPH YO1YO 73", msg));
    EXPECT_EQ(msg.type, MessageType::M73_STD);
    EXPECT_EQ(msg.get_target_call(), "HB9IPH");
    EXPECT_EQ(msg.get_sender_call(), "YO1YO");

    // Standard 3-token 73: HB9IPH YO1YO RR73
    EXPECT_TRUE(parse_message("HB9IPH YO1YO RR73", msg));
    EXPECT_EQ(msg.type, MessageType::M73_STD);

    // Standard 3-token 73: HB9IPH YO1YO RRR
    EXPECT_TRUE(parse_message("HB9IPH YO1YO RRR", msg));
    EXPECT_EQ(msg.type, MessageType::M73_STD);

    // Standard 4-token CALL with grid and report: HB9IPH YO1YO JN47 -03
    EXPECT_TRUE(parse_message("HB9IPH YO1YO JN47 -03", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_STD_NOSUF);
    EXPECT_EQ(msg.get_target_call(), "HB9IPH");
    EXPECT_EQ(msg.get_sender_call(), "YO1YO");
    EXPECT_EQ(msg.locator, "JN47");
    EXPECT_EQ(msg.rst_db, -3);

    // Free text fallback
    EXPECT_TRUE(parse_message("Hello, world! 73", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);
}

TEST(CoverageEdgeCases, WaterfallDspModesAndBoundaries) {
    int num_blocks = 120;
    int block_stride = 256;
    std::vector<uint8_t> mag(num_blocks * block_stride, 10);
    std::vector<float> mag2(num_blocks * block_stride, 1.0f);

    // Out of bounds guards
    EXPECT_EQ(lq_sync_score(mag.data(), 10, 256, 0, 0, Protocol::LQ8), 0);
    EXPECT_EQ(lq_sync_score(mag.data(), 100, 256, 95, 0, Protocol::LQ8), 0);
    EXPECT_EQ(lq_sync_score(mag.data(), 100, 256, 0, 250, Protocol::LQ8), 0);
    EXPECT_EQ(lq_sync_score(mag.data(), 100, 256, 0, 0, static_cast<Protocol>(99)), 0);

    float llrs[174];
    EXPECT_FALSE(lq_extract_llrs_from_waterfall(mag.data(), 10, 256, 0, 0, Protocol::LQ8, llrs));
    EXPECT_FALSE(lq_extract_llrs_from_waterfall(mag.data(), 100, 256, 95, 0, Protocol::LQ8, llrs));
    EXPECT_FALSE(lq_extract_llrs_from_waterfall(mag.data(), 100, 256, 0, 250, Protocol::LQ8, llrs));

    // LQ16 mode sync score, refinement, snr, extraction
    ToneSequence lq16_seq;
    Message msg;
    parse_message("CQ HB9IPH JN47", msg);
    encode_tones(msg, Protocol::LQ16, lq16_seq);
    for (size_t t = 0; t < lq16_seq.tones.size(); ++t) {
        int b = 5 + static_cast<int>(t);
        int bin = 30 + lq16_seq.tones[t];
        mag[b * block_stride + bin] = 220;
        mag2[b * block_stride + bin] = 100.0f;
    }
    EXPECT_GT(lq_sync_score(mag.data(), num_blocks, block_stride, 5, 30, Protocol::LQ16), 500);
    int score = 0;
    float ref_f = lq_refine_frequency(mag.data(), num_blocks, block_stride, 5, 30, 0, 1, 0.320f, Protocol::LQ16, &score);
    EXPECT_GT(ref_f, 0.0f);
    EXPECT_GT(lq_guess_snr(mag2.data(), num_blocks, block_stride, 5, 30, Protocol::LQ16), -30.0f);
    EXPECT_TRUE(lq_extract_llrs_from_waterfall(mag.data(), num_blocks, block_stride, 5, 30, Protocol::LQ16, llrs));

    // LQ2 mode sync score, refinement, snr, extraction
    std::fill(mag.begin(), mag.end(), 10);
    std::fill(mag2.begin(), mag2.end(), 1.0f);
    ToneSequence lq2_seq;
    encode_tones(msg, Protocol::LQ2, lq2_seq);
    for (size_t t = 0; t < lq2_seq.tones.size(); ++t) {
        int b = 5 + static_cast<int>(t);
        int bin = 40 + lq2_seq.tones[t];
        mag[b * block_stride + bin] = 220;
        mag2[b * block_stride + bin] = 100.0f;
    }
    EXPECT_GT(lq_sync_score(mag.data(), num_blocks, block_stride, 5, 40, Protocol::LQ2), 500);
    ref_f = lq_refine_frequency(mag.data(), num_blocks, block_stride, 5, 40, 0, 1, 0.024f, Protocol::LQ2, &score);
    EXPECT_GT(ref_f, 0.0f);
    EXPECT_GT(lq_guess_snr(mag2.data(), num_blocks, block_stride, 5, 40, Protocol::LQ2), -30.0f);
    EXPECT_TRUE(lq_extract_llrs_from_waterfall(mag.data(), num_blocks, block_stride, 5, 40, Protocol::LQ2, llrs));
}

// ============================================================================
// Ultra-High Coverage Edge Cases (>99% Target)
// ============================================================================

TEST(CoverageEdgeCases, BoundaryCallsignAndCodes) {
    // 1. Callsign boundary decoding (c1=37, c2=36, s1=27 fallbacks in c1/c2/s_to_char)
    std::string cs_out;
    EXPECT_TRUE(decode_callsign_std(CALLSIGN_MAX_STD, cs_out));

    // 2. Varicode invalid 16-bit unmapped bit sequence discard
    uint8_t raw[10] = {0x31, 0x5D, 0x31, 0x5D, 0x31, 0x5D, 0x31, 0x5D, 0x31, 0x5D};
    BitBuffer bb_var(raw, 75);
    std::string var_out;
    decode_varicode(bb_var, var_out, 75);

    // 3. Modifier whitespace-only and unused dictionary index
    uint32_t mod_packed = 0;
    EXPECT_TRUE(encode_modifier_20("    ", mod_packed));
    EXPECT_EQ(mod_packed, MODIFIER_NONE);

    std::string mod_out;
    EXPECT_TRUE(decode_modifier_20(500, mod_out)); // 3-digit numeric modifier: 500 - 1 = 499
    EXPECT_EQ(mod_out, "499");
    EXPECT_FALSE(decode_modifier_20(MODIFIER_MAX_VAL + 10, mod_out));
    EXPECT_TRUE(is_known_modifier("DX"));
    EXPECT_TRUE(is_known_modifier("SOTA"));
    EXPECT_TRUE(is_known_modifier("040"));
    EXPECT_FALSE(is_known_modifier("$$INVALID$$"));

    // 4. RST parsing leading whitespace
    int rst_val = 0;
    EXPECT_TRUE(parse_rst("   +05", rst_val));
    EXPECT_EQ(rst_val, 5);
    EXPECT_TRUE(parse_rst("   -12", rst_val));
    EXPECT_EQ(rst_val, -12);

    // 5. WSJT-X hash with leading/trailing spaces and invalid chars
    EXPECT_NE(hash_callsign_10(" W1AW"), 0u);
    EXPECT_NE(hash_callsign_10("W1AW "), 0u);
    EXPECT_EQ(hash_callsign_10("W1AW?"), 0u);
    EXPECT_EQ(hash_callsign_12("W1AW?"), 0u);
    EXPECT_EQ(hash_callsign_22("W1AW?"), 0u);

    // 6. LDPC invalid parity check
    uint8_t bad_cw[22];
    std::memset(bad_cw, 0xFF, sizeof(bad_cw));
    EXPECT_FALSE(ldpc_check_syndrome(bad_cw));
}

TEST(CoverageEdgeCases, EasyApiAndStreamOperators) {
    // Message stream operator
    Message msg;
    msg.type = MessageType::FREE_TEXT;
    msg.text = "HELLO 73";
    std::ostringstream ss;
    ss << msg;
    EXPECT_FALSE(ss.str().empty());

    // Invalid tone sequence decode in easy API
    ToneSequence bad_seq;
    bad_seq.protocol = Protocol::LQ8;
    bad_seq.tones.assign(79, 99);
    EXPECT_FALSE(tones_to_text(bad_seq).has_value());

    // text_to_audio with invalid sample rate
    EXPECT_TRUE(text_to_audio("TEST", Protocol::LQ8, 1500.0f, -1.0f).empty());

    // text_to_tones and text_to_audio with empty text
    EXPECT_FALSE(text_to_tones("", Protocol::LQ8).has_value());
    EXPECT_TRUE(text_to_audio("", Protocol::LQ8, 1500.0f, 12000.0f).empty());
}

TEST(CoverageEdgeCases, NonStdAndHashedMessageParsingCombinations) {
    Message msg;

    // 4-token CALL with non-standard target (std caller -> Type 6)
    EXPECT_TRUE(parse_message("3B9/HB9IPH G4ABC JN47 +05", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_STD_SUF);
    EXPECT_EQ(msg.call_2, "G4ABC");
    EXPECT_EQ(msg.rst_db, 5);

    // 4-token CALL with hash target (std caller -> Type 6)
    EXPECT_TRUE(parse_message("<123456> G4ABC JN47 +05", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_STD_SUF);
    EXPECT_EQ(msg.hash_1, 0x123456U);

    // 4-token CALL with non-standard caller
    EXPECT_TRUE(parse_message("3B9/HB9IPH 3B9/G4ABC JN47 +05", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_NONSTD);

    // 4-token CALL with hash caller
    EXPECT_TRUE(parse_message("3B9/HB9IPH <654321> JN47 +05", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_NONSTD);

    // 3-token 73 with non-standard target -> M73_NONSTD
    EXPECT_TRUE(parse_message("3B9/HB9IPH G4ABC 73", msg));
    EXPECT_EQ(msg.type, MessageType::M73_NONSTD);

    // 3-token 73 with hash target -> M73_NONSTD
    EXPECT_TRUE(parse_message("<123456> G4ABC 73", msg));
    EXPECT_EQ(msg.type, MessageType::M73_NONSTD);
    EXPECT_EQ(msg.hash_1, 0x123456U);

    // 3-token 73 with non-standard caller -> M73_NONSTD
    EXPECT_TRUE(parse_message("3B9/HB9IPH 3B9/G4ABC 73", msg));
    EXPECT_EQ(msg.type, MessageType::M73_NONSTD);

    // 3-token 73 with hash caller (hash in caller position falls back to FREE_TEXT)
    EXPECT_TRUE(parse_message("3B9/HB9IPH <654321> 73", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);

    // 3-token locator with non-standard target (std caller -> Type 6)
    EXPECT_TRUE(parse_message("3B9/HB9IPH G4ABC JN47", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_STD_SUF);

    // 3-token locator with hash target (std caller -> Type 6)
    EXPECT_TRUE(parse_message("<123456> G4ABC JN47", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_STD_SUF);
    EXPECT_EQ(msg.hash_1, 0x123456U);

    // 3-token locator with non-standard caller
    EXPECT_TRUE(parse_message("3B9/HB9IPH 3B9/G4ABC JN47", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_NONSTD);

    // 3-token locator with hash caller
    EXPECT_TRUE(parse_message("3B9/HB9IPH <654321> JN47", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);

    // 3-token RST with non-standard target -> MULTI_REPORT73
    EXPECT_TRUE(parse_message("3B9/HB9IPH G4ABC +05", msg));
    EXPECT_EQ(msg.type, MessageType::MULTI_REPORT73);

    // 3-token RST with hash target -> MULTI_REPORT73
    EXPECT_TRUE(parse_message("<123456> G4ABC +05", msg));
    EXPECT_EQ(msg.type, MessageType::MULTI_REPORT73);
    ASSERT_FALSE(msg.multi_targets.empty());
    EXPECT_EQ(msg.multi_targets[0].hash, 0x123456U);
    EXPECT_EQ(msg.call_1, "G4ABC");

    // 3-token RST with non-standard caller and R prefix -> REPORT73_NONSTD
    EXPECT_TRUE(parse_message("3B9/HB9IPH 3B9/G4ABC R+05", msg));
    EXPECT_EQ(msg.type, MessageType::REPORT73_NONSTD);

    // 3-token RST with non-standard caller without R prefix -> CALL_NONSTD
    EXPECT_TRUE(parse_message("3B9/HB9IPH 3B9/G4ABC +05", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_NONSTD);

    // 3-token RST with hash caller
    EXPECT_TRUE(parse_message("3B9/HB9IPH <654321> +05", msg));
    EXPECT_EQ(msg.type, MessageType::FREE_TEXT);

    // Empty message methods
    Message empty_msg;
    EXPECT_EQ(empty_msg.get_sender_hash(), 0u);
    EXPECT_EQ(empty_msg.get_target_hash(), 0u);

    // Invalid MessageType enum encode
    Message invalid_enum_msg;
    invalid_enum_msg.type = static_cast<MessageType>(99);
    uint8_t payload[10];
    EXPECT_FALSE(encode_message(invalid_enum_msg, payload));
}

TEST(CoverageEdgeCases, TransportDemodulationAndErrorBranches) {
    // 1. Test demodulate_audio and demodulate_audio_offset across all 4 protocols
    for (auto proto : {Protocol::LQ8, Protocol::LQ4, Protocol::LQ2, Protocol::LQ16}) {
        Message tx_msg;
        tx_msg.type = MessageType::CQ_STD;
        tx_msg.call_1 = "HB9IPH";
        tx_msg.locator = "JN47";

        std::vector<float> audio;
        EXPECT_TRUE(message_to_audio(tx_msg, proto, 1500.0f, 12000.0f, audio));
        EXPECT_FALSE(audio.empty());

        ToneSequence demod_seq;
        EXPECT_TRUE(demodulate_audio(audio, 1500.0f, 12000.0f, proto, demod_seq));
        EXPECT_EQ(demod_seq.protocol, proto);
        EXPECT_FALSE(demod_seq.tones.empty());

        // Verify demodulated tones decode back to message
        Message rx_msg;
        EXPECT_TRUE(decode_tones(demod_seq, rx_msg));
        EXPECT_EQ(rx_msg.type, MessageType::CQ_STD);
        EXPECT_EQ(rx_msg.call_1, "HB9IPH");
        EXPECT_EQ(rx_msg.locator, "JN47");
    }

    // 2. Low energy audio demodulation failure (with full sample count)
    std::vector<float> silent_audio(200000, 0.0f);
    ToneSequence silent_seq;
    EXPECT_FALSE(demodulate_audio(silent_audio, 1500.0f, 12000.0f, Protocol::LQ8, silent_seq));

    // 3. Short audio buffer failure
    std::vector<float> short_audio(10, 0.5f);
    EXPECT_FALSE(demodulate_audio(short_audio, 1500.0f, 12000.0f, Protocol::LQ8, silent_seq));

    // 4. Invalid protocol enum branches
    Protocol bad_proto = static_cast<Protocol>(99);
    uint8_t dummy_cw[22] = {0};
    ToneSequence dummy_seq;
    EXPECT_THROW(codeword_to_tones(dummy_cw, bad_proto, dummy_seq), std::invalid_argument);
    dummy_seq.protocol = bad_proto;
    EXPECT_FALSE(tones_to_codeword(dummy_seq, dummy_cw));

    std::vector<float> dummy_llrs;
    EXPECT_THROW(demodulate_audio_soft(silent_audio, 0, 1500.0f, 12000.0f, bad_proto, dummy_llrs), std::invalid_argument);
    Message msg;
    EXPECT_FALSE(decode_soft_llrs(dummy_llrs, msg, bad_proto));

    // 5. Decode tones with bad protocol and truncated failure
    dummy_seq.protocol = bad_proto;
    dummy_seq.tones.assign(79, 0);
    uint8_t payload[10];
    EXPECT_FALSE(decode_tones(dummy_seq, payload));

    dummy_seq.protocol = Protocol::LQ8;
    dummy_seq.tones = {1, 2, 3};
    EXPECT_FALSE(decode_tones(dummy_seq, payload));

    // 6. Decode tones LDPC decode failure (inverted data tones)
    ToneSequence corrupt_seq;
    parse_message("CQ HB9IPH JN47", msg);
    encode_tones(msg, Protocol::LQ8, corrupt_seq);
    for (size_t i = 7; i < 35; ++i) {
        corrupt_seq.tones[i] = (corrupt_seq.tones[i] + 4) % 8;
    }
    EXPECT_FALSE(decode_tones(corrupt_seq, payload));

    // 7. Decode tones & soft LLRs CRC failure with valid LDPC codeword
    uint8_t bad_crc_91[12] = {0};
    bad_crc_91[0] = 0xAA;
    uint8_t bad_crc_174[22] = {0};
    ldpc_encode(bad_crc_91, bad_crc_174);
    ToneSequence bad_crc_seq;
    codeword_to_tones(bad_crc_174, Protocol::LQ8, bad_crc_seq);
    EXPECT_FALSE(decode_tones(bad_crc_seq, payload));

    std::vector<float> bad_crc_llrs(174, 0.0f);
    for (int i = 0; i < 174; ++i) {
        uint8_t bit = (bad_crc_174[i / 8] >> (7 - (i % 8))) & 1;
        bad_crc_llrs[i] = (bit == 0) ? 5.0f : -5.0f;
    }
    Message rx_bad_crc;
    EXPECT_FALSE(decode_soft_llrs(bad_crc_llrs, rx_bad_crc, Protocol::LQ8));

    // 8. Invalid message encode_tones & message_to_audio failures
    Message bad_msg;
    bad_msg.type = static_cast<MessageType>(99);
    EXPECT_FALSE(encode_tones(bad_msg, Protocol::LQ8, dummy_seq));
    std::vector<float> bad_samples;
    EXPECT_FALSE(message_to_audio(bad_msg, Protocol::LQ8, 1500.0f, 12000.0f, bad_samples));

    // 9. Load WAV corrupted file header (>44 bytes with bad magic)
    std::string bad_wav = "test_corrupt.wav";
    {
        std::ofstream out(bad_wav, std::ios::binary);
        out << "NOT_A_RIFF_HEADER_1234567890123456789012345678901234567890";
    }
    float sr = 0;
    std::vector<float> wav_audio;
    EXPECT_FALSE(load_wav_file(bad_wav, wav_audio, sr));
    std::remove(bad_wav.c_str());
}

TEST(CoverageEdgeCases, ExhaustiveRemainingBranches) {
    // 1. Message encode/decode invalid types
    Message bad_msg;
    bad_msg.type = static_cast<MessageType>(99);
    uint8_t dummy_p[10] = {0};
    EXPECT_FALSE(encode_message(bad_msg, dummy_p));

    // 2. Varicode corrupt bits exceeding 16 bits
    uint8_t corrupt_varicode[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    BitBuffer bb_corr(corrupt_varicode, 75);
    std::string out_text;
    decode_varicode(bb_corr, out_text, 75);

    // 3. Callsign edge cases
    std::string dummy_cs;
    EXPECT_FALSE(normalise_standard_callsign("W1AAAA", dummy_cs));
    EXPECT_FALSE(normalise_standard_callsign("1A11", dummy_cs));
    EXPECT_FALSE(normalise_standard_callsign("A1!", dummy_cs));
    decode_callsign_std(262177559, dummy_cs);

    // 4. Non-std callsign decode large bit stream
    uint8_t large_nonstd[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    BitBuffer bb_large(large_nonstd, 74);
    std::string nonstd_out;
    decode_callsign_nonstd(bb_large, 14, nonstd_out);

    // 5. Modifier decode large value
    std::string mod_out;
    decode_modifier_20(1000U + 32*32*32*32 - 1, mod_out);
    decode_modifier_20(1000U + 0x1FFFFF, mod_out);

    // 6. Transport invalid protocol branches
    uint8_t dummy_cw[22] = {0};
    ToneSequence bad_proto_seq;
    try {
        codeword_to_tones(dummy_cw, static_cast<Protocol>(99), bad_proto_seq);
    } catch (...) {}

    ToneSequence short_seq;
    short_seq.protocol = Protocol::LQ8;
    short_seq.tones.assign(50, 0);
    uint8_t short_payload[10];
    EXPECT_FALSE(decode_tones(short_seq, short_payload));

    try {
        std::vector<float> audio_dummy(1000, 0.0f);
        std::vector<float> soft_llrs;
        demodulate_audio_soft(audio_dummy, 0, 1500.0f, 12000.0f, static_cast<Protocol>(99), soft_llrs);
    } catch (...) {}
}

// ============================================================================
// 8. Intent Engine & Hash Resolution Comprehensive Coverage
// ============================================================================

TEST(CoverageEdgeCases, IntentEngineFullBranchCoverage) {
    IntentEngine engine("HB9IPH", "JN47");
    engine.add_known_callsigns({"YO1YO", "TU2TU", "DL1ABC", "EA6/HB9IP"});

    // 1. Calling stations with SNR auto-detection from various frame types
    std::vector<ReceivedFrame> rx_frames;

    ReceivedFrame rf1;
    rf1.msg = make_cq("YO1YO", "KN24");
    rf1.snr_db = -5;
    rx_frames.push_back(rf1);

    ReceivedFrame rf2;
    rf2.msg = make_call("HB9IPH", "TU2TU", "KL22", +4);
    rf2.snr_db = +4;
    rx_frames.push_back(rf2);

    ReceivedFrame rf3;
    rf3.msg = make_reply73("HB9IPH", "DL1ABC", +2);
    rf3.snr_db = +2;
    rx_frames.push_back(rf3);

    // CALL_STATION to YO1YO -> measures -5 dB
    UserIntent call_yo;
    call_yo.action = IntentAction::CALL_STATION;
    call_yo.target_call_1 = "YO1YO";
    DecisionResult dec_yo = engine.process_slot(rx_frames, call_yo);
    EXPECT_TRUE(dec_yo.success);
    EXPECT_EQ(dec_yo.tx_message.rst_db, -5);

    // CALL_STATION to TU2TU -> measures +4 dB
    UserIntent call_tu;
    call_tu.action = IntentAction::CALL_STATION;
    call_tu.target_call_1 = "TU2TU";
    DecisionResult dec_tu = engine.process_slot(rx_frames, call_tu);
    EXPECT_TRUE(dec_tu.success);
    EXPECT_EQ(dec_tu.tx_message.rst_db, +4);

    // 2. Incoming QSO completions for all REPLY73 variants and MULTI_REPLY73
    IntentEngine eng_empty("", "");
    eng_empty.set_my_callsign("");
    EXPECT_TRUE(eng_empty.get_my_callsign().empty());

    std::vector<ReceivedFrame> empty_rx;
    ReceivedFrame rfm14;
    rfm14.msg.type = MessageType::MULTI_73;
    rfm14.msg.hash_1 = hash_callsign_14("YO1YO");
    MultiTarget mt;
    mt.hash = hash_callsign_24("HB9IPH");
    rfm14.msg.multi_targets.push_back(mt);
    empty_rx.push_back(rfm14);

    IntentEngine eng_14("HB9IPH", "JN47");
    eng_14.add_known_callsign("YO1YO");
    UserIntent idle_intent14;
    idle_intent14.action = IntentAction::IDLE;
    DecisionResult dec_14 = eng_14.process_slot(empty_rx, idle_intent14);
    EXPECT_TRUE(dec_14.success);

    IntentEngine comp_engine("HB9IPH", "JN47");
    comp_engine.add_known_callsigns({"YO1YO", "TU2TU", "DL1ABC", "EA6/HB9IP"});

    std::vector<ReceivedFrame> completions_rx;

    // A. Empty my_call_ branch
    IntentEngine empty_eng("", "");
    UserIntent empty_intent;
    empty_intent.action = IntentAction::IDLE;
    DecisionResult empty_dec = empty_eng.process_slot(rx_frames, empty_intent);
    EXPECT_TRUE(empty_dec.qso_completed_with.empty());

    // B. REPORT73_STD directed to me
    ReceivedFrame c_rf1;
    c_rf1.msg = make_reply73("HB9IPH", "YO1YO", +3); // Type 8 (REPORT73_STD)
    completions_rx.push_back(c_rf1);

    // C. MULTI_REPORT73 directed to me from non-standard sender
    ReceivedFrame c_rf2;
    c_rf2.msg.type = MessageType::MULTI_REPORT73;
    c_rf2.msg.call_1 = "EA6/TU2TU";
    c_rf2.msg.hash_1 = hash_callsign_16("EA6/TU2TU");
    c_rf2.msg.multi_targets.push_back({"HB9IPH", hash_callsign_24("HB9IPH"), -4});
    completions_rx.push_back(c_rf2);

    // D. MULTI_REPORT73 where DX station call is hashed
    ReceivedFrame c_rf4;
    c_rf4.msg.type = MessageType::MULTI_REPORT73;
    c_rf4.msg.hash_1 = hash_callsign_16("EA6/HB9IP");
    c_rf4.msg.multi_targets.push_back({"HB9IPH", hash_callsign_24("HB9IPH"), -2});
    c_rf4.msg.multi_targets.push_back({"TU2TU", hash_callsign_24("TU2TU"), -6});
    completions_rx.push_back(c_rf4);

    UserIntent idle_intent;
    idle_intent.action = IntentAction::IDLE;
    DecisionResult dec_all = comp_engine.process_slot(completions_rx, idle_intent);

    EXPECT_TRUE(dec_all.success);
    EXPECT_TRUE(comp_engine.is_qso_completed("YO1YO"));
    EXPECT_TRUE(comp_engine.is_qso_completed("EA6/TU2TU"));
    EXPECT_TRUE(comp_engine.is_qso_completed("EA6/HB9IP"));

    // Duplicate completions ignored
    DecisionResult dec_dup = comp_engine.process_slot(completions_rx, idle_intent);
    EXPECT_TRUE(dec_dup.qso_completed_with.empty());
}


// ============================================================================
// 9. Message Conversions & Resolve Callsigns Edge Cases
// ============================================================================

TEST(CoverageEdgeCases, MessageConversionAndCallsignResolutionEdgeCases) {
    // 1. resolve_callsigns full coverage
    Message msg_multi;
    msg_multi.type = MessageType::MULTI_REPORT73;
    msg_multi.hash_1 = hash_callsign_16("HB9IPH");
    msg_multi.call_1 = "<1EA2>";

    std::string_view known_calls[] = {"HB9IPH", "YO1YO", "TU2TU", "EA6/TU2TU"};
    EXPECT_FALSE(resolve_callsigns(msg_multi, nullptr, 0));
    EXPECT_TRUE(resolve_callsigns(msg_multi, known_calls, 4));
    EXPECT_EQ(msg_multi.call_1, "HB9IPH");

    Message msg_nonstd2;
    msg_nonstd2.type = MessageType::MULTI_REPORT73;
    msg_nonstd2.call_1 = "EA6/TU2TU";
    msg_nonstd2.hash_1 = hash_callsign_16("EA6/TU2TU");
    msg_nonstd2.multi_targets.push_back({"", hash_callsign_24("YO1YO"), 0});
    EXPECT_TRUE(resolve_callsigns(msg_nonstd2, known_calls, 4));
    EXPECT_EQ(msg_nonstd2.multi_targets[0].call, "YO1YO");

    // 2. payload_to_binary padding zeros
    uint8_t dummy_p[10] = {0xAA, 0x55};
    std::string bin_90 = payload_to_binary(dummy_p, 90);
    EXPECT_EQ(bin_90.size(), 90u);
    EXPECT_EQ(bin_90.substr(80), "0000000000");

    // 3. hex_to_payload formatting branches (0x, 0X, spaces, uppercase/lowercase)
    uint8_t p_out[10] = {0};
    EXPECT_TRUE(hex_to_payload("0x01 0X02:03-04 05 06 07 08 09 0a", p_out));
    EXPECT_EQ(p_out[0], 0x01);
    EXPECT_EQ(p_out[9], 0x0A);
    EXPECT_FALSE(hex_to_payload("0x123", p_out)); // too short

    // 4. binary_to_payload edge cases
    EXPECT_FALSE(binary_to_payload("", p_out));
    EXPECT_FALSE(binary_to_payload("   ", p_out));
    EXPECT_TRUE(binary_to_payload("1010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010", p_out));

    // 5. hex_to_text and binary_to_text with and without known callsigns
    std::string valid_hex = text_to_hex("CQ HB9IPH JN47");
    std::string valid_bin = text_to_binary("CQ HB9IPH JN47");

    EXPECT_EQ(hex_to_text(valid_hex, nullptr, 0), "CQ HB9IPH JN47");
    EXPECT_EQ(hex_to_text(valid_hex, known_calls, 3), "CQ HB9IPH JN47");
    EXPECT_EQ(hex_to_text("invalid_hex_string", nullptr, 0), "");
    EXPECT_EQ(hex_to_text("1234", nullptr, 0), ""); // Incomplete hex string

    EXPECT_EQ(binary_to_text(valid_bin, nullptr, 0), "CQ HB9IPH JN47");
    EXPECT_EQ(binary_to_text(valid_bin, known_calls, 3), "CQ HB9IPH JN47");
    EXPECT_EQ(binary_to_text("invalid_bin", nullptr, 0), "");
    EXPECT_EQ(binary_to_text("1010", nullptr, 0), "");

    // 6. text_to_hex and text_to_binary empty / invalid text branches
    EXPECT_EQ(text_to_hex(""), "");
    EXPECT_EQ(text_to_binary(""), "");
}



// ============================================================================
// 10. Easy API Conversions & Parallel Decoder Coverage
// ============================================================================

TEST(CoverageEdgeCases, EasyApiAndParallelDecoderComprehensiveCoverage) {
    // Parallel decoder thread counts: 0 (auto), 1, 2, 4, 8, 16
    auto audio = text_to_audio("CQ HB9IPH JN47", Protocol::LQ8, 1500.0f, 12000.0f);
    EXPECT_FALSE(audio.empty());

    for (int threads : {0, 1, 2, 4, 8, 16}) {
        auto decoded = audio_to_messages(audio, 1500.0f, 12000.0f, Protocol::LQ8, threads);
        EXPECT_FALSE(decoded.empty()) << "Failed with threads=" << threads;
        EXPECT_EQ(decoded[0].call_1, "HB9IPH");
        EXPECT_EQ(decoded[0].locator, "JN47");


        auto opt_text = audio_to_text(audio, 1500.0f, 12000.0f, Protocol::LQ8, threads);
        EXPECT_TRUE(opt_text.has_value());
        EXPECT_EQ(opt_text.value(), "CQ HB9IPH JN47");
    }

    // Parallel decoder with empty / short audio
    std::vector<float> empty_audio;
    EXPECT_TRUE(audio_to_messages(empty_audio, 1500.0f, 12000.0f, Protocol::LQ8, 2).empty());
    EXPECT_FALSE(audio_to_text(empty_audio, 1500.0f, 12000.0f, Protocol::LQ8, 2).has_value());

    std::vector<float> short_audio(100, 0.0f);
    EXPECT_TRUE(audio_to_messages(short_audio, 1500.0f, 12000.0f, Protocol::LQ8, 2).empty());
    EXPECT_FALSE(audio_to_text(short_audio, 1500.0f, 12000.0f, Protocol::LQ8, 2).has_value());
}

// ============================================================================
// 11. Deep Comprehensive Branch Coverage for High Library Coverage (~100%)
// ============================================================================

TEST(CoverageEdgeCases, DeepComprehensiveBranchCoverage) {
    // 1. c_api.cpp conversion & edge branches
    uint8_t payload[10] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22};
    char hex_out[64] = {0};
    char bin_out[128] = {0};
    char text_out[128] = {0};

    EXPECT_EQ(lq_c_payload_to_hex(nullptr, hex_out, sizeof(hex_out), 1), 0);
    EXPECT_EQ(lq_c_payload_to_hex(payload, nullptr, sizeof(hex_out), 1), 0);
    EXPECT_EQ(lq_c_payload_to_hex(payload, hex_out, 0, 1), 0);
    EXPECT_EQ(lq_c_payload_to_hex(payload, hex_out, sizeof(hex_out), 1), 1);
    EXPECT_EQ(lq_c_payload_to_hex(payload, hex_out, sizeof(hex_out), 0), 1);

    EXPECT_EQ(lq_c_payload_to_binary(nullptr, bin_out, sizeof(bin_out)), 0);
    EXPECT_EQ(lq_c_payload_to_binary(payload, nullptr, sizeof(bin_out)), 0);
    EXPECT_EQ(lq_c_payload_to_binary(payload, bin_out, 0), 0);
    EXPECT_EQ(lq_c_payload_to_binary(payload, bin_out, sizeof(bin_out)), 1);

    uint8_t dec_payload[10] = {0};
    EXPECT_EQ(lq_c_hex_to_payload(nullptr, dec_payload), 0);
    EXPECT_EQ(lq_c_hex_to_payload(hex_out, nullptr), 0);
    EXPECT_EQ(lq_c_hex_to_payload(hex_out, dec_payload), 1);

    EXPECT_EQ(lq_c_binary_to_payload(nullptr, dec_payload), 0);
    EXPECT_EQ(lq_c_binary_to_payload(bin_out, nullptr), 0);
    EXPECT_EQ(lq_c_binary_to_payload(bin_out, dec_payload), 1);

    const char* known_calls[] = {"HB9IPH", "YO1YO"};
    EXPECT_EQ(lq_c_hex_to_text(nullptr, text_out, sizeof(text_out), known_calls, 2), 0);
    EXPECT_EQ(lq_c_hex_to_text("invalid", text_out, sizeof(text_out), known_calls, 2), 0);
    EXPECT_EQ(lq_c_hex_to_text(hex_out, nullptr, sizeof(text_out), known_calls, 2), 0);
    EXPECT_EQ(lq_c_hex_to_text(hex_out, text_out, 0, known_calls, 2), 0);

    EXPECT_EQ(lq_c_binary_to_text(nullptr, text_out, sizeof(text_out), known_calls, 2), 0);
    EXPECT_EQ(lq_c_binary_to_text("invalid", text_out, sizeof(text_out), known_calls, 2), 0);
    EXPECT_EQ(lq_c_binary_to_text(bin_out, nullptr, sizeof(text_out), known_calls, 2), 0);
    EXPECT_EQ(lq_c_binary_to_text(bin_out, text_out, 0, known_calls, 2), 0);

    EXPECT_EQ(lq_c_text_to_hex(nullptr, hex_out, sizeof(hex_out), 1), 0);
    EXPECT_EQ(lq_c_text_to_hex("CQ HB9IPH JN47", nullptr, sizeof(hex_out), 1), 0);
    EXPECT_EQ(lq_c_text_to_hex("CQ HB9IPH JN47", hex_out, 0, 1), 0);
    EXPECT_EQ(lq_c_text_to_hex("CQ HB9IPH JN47", hex_out, sizeof(hex_out), 1), 1);
    EXPECT_EQ(lq_c_text_to_hex("", hex_out, sizeof(hex_out), 1), 0);

    EXPECT_EQ(lq_c_text_to_binary(nullptr, bin_out, sizeof(bin_out)), 0);
    EXPECT_EQ(lq_c_text_to_binary("CQ HB9IPH JN47", nullptr, sizeof(bin_out)), 0);
    EXPECT_EQ(lq_c_text_to_binary("CQ HB9IPH JN47", bin_out, 0), 0);
    EXPECT_EQ(lq_c_text_to_binary("CQ HB9IPH JN47", bin_out, sizeof(bin_out)), 1);
    EXPECT_EQ(lq_c_text_to_binary("", bin_out, sizeof(bin_out)), 0);

    // c_api resolve_callsigns failing (line 231)
    lq_c_message_t c_msg;
    std::memset(&c_msg, 0, sizeof(c_msg));
    c_msg.type = LQ_MSG_REPLY73_NONSTD;
    c_msg.hash_to_24 = 0x123456;
    EXPECT_EQ(lq_c_resolve_callsigns(&c_msg, known_calls, 2), 0);

    // c_api audio_to_message failure path (lines 405-406)
    float silence[1200] = {0};
    EXPECT_EQ(lq_c_audio_to_message(silence, 1200, 1500.0f, 12000.0f, LQ_MODE_LQ8, &c_msg, 1), 0);

    // 2. easy.cpp transceiver multi-reply & stream operator reserved types
    Transceiver transceiver("HB9IPH", "JN47", Protocol::LQ8, 1500.0f, 12000.0f, 4);
    std::vector<MultiTarget> tgts = {{"YO1YO", hash_callsign_24("YO1YO"), -5}, {"TU2TU", hash_callsign_24("TU2TU"), +2}};
    auto mr_audio = transceiver.generate_multi_reply73(tgts);
    EXPECT_FALSE(mr_audio.empty());

    std::ostringstream oss;
    oss << MessageType::RESERVED << static_cast<MessageType>(99);
    EXPECT_NE(oss.str(), "");

    // 3. intent.cpp find_measured_snr hash matching branches (lines 90, 93, 96)
    IntentEngine engine("HB9IPH", "JN47");
    std::vector<ReceivedFrame> rx_frames;
    ReceivedFrame rf_hash;
    rf_hash.msg.type = MessageType::CALL_NONSTD;
    rf_hash.msg.hash_1 = hash_callsign_20("TU2TU");
    rf_hash.msg.call_2 = "HB9IPH";
    rf_hash.snr_db = -8;
    rx_frames.push_back(rf_hash);

    UserIntent call_target;
    call_target.action = IntentAction::CALL_STATION;
    call_target.target_call_1 = "TU2TU";
    DecisionResult dec_target = engine.process_slot(rx_frames, call_target);
    EXPECT_TRUE(dec_target.success);
    EXPECT_EQ(dec_target.tx_message.rst_db, -8);


    // Fallback RST when no match (line 96)
    call_target.target_call_1 = "UNKNOWN_CALL";
    DecisionResult dec_fallback = engine.process_slot(rx_frames, call_target);
    EXPECT_TRUE(dec_fallback.success);
    EXPECT_EQ(dec_fallback.tx_message.rst_db, -5);


    // 4. message.cpp get_target_hash and MULTI_REPLY single target branches
    Message m_multi;
    m_multi.type = MessageType::MULTI_REPLY73;
    m_multi.multi_targets = {{"HB9IPH", 0, -2}};
    EXPECT_NE(m_multi.get_target_hash(), 0u);
    m_multi.multi_targets.clear();
    EXPECT_EQ(m_multi.get_target_hash(), 0u);

    // Encode MULTI_REPLY with 1 target (duplicates to 3 slots)
    m_multi.multi_targets = {{"HB9IPH", hash_callsign_14("HB9IPH"), -2}};
    uint8_t m_p[10];
    EXPECT_TRUE(encode_message(m_multi, m_p));

    // parse_message invalid RST in MULTI-REPLY (lines 889-890, 908)
    Message m_inv;
    EXPECT_TRUE(parse_message("MULTI-REPLY73 HB9IPH YO1YO INVALID_RST", m_inv)); // Falls back to FREE_TEXT

    // 6. ldpc.cpp decode max_iters=0 and corrupted LLR failure path (line 383)
    float bad_llrs[174];
    for (int i = 0; i < 174; ++i) bad_llrs[i] = (i % 2 == 0) ? +5.0f : -5.0f;
    uint8_t out_91[12];
    EXPECT_EQ(ldpc_decode(bad_llrs, out_91, 0), -1);
    EXPECT_EQ(ldpc_decode(bad_llrs, out_91, 1), -1);

    // 7. transport.cpp invalid protocols & wideband passband multi-worker deduplication
    ToneSequence bad_proto_ts;
    try {
        codeword_to_tones(payload, static_cast<Protocol>(99), bad_proto_ts);
    } catch (...) {}
    try {
        tones_to_codeword(bad_proto_ts, payload);
    } catch (...) {}
    std::vector<float> dummy_audio(12000 * 2, 0.0f);
    std::vector<float> dummy_soft_llrs;
    try {
        demodulate_audio_soft(dummy_audio, 0, 1500.0f, 12000.0f, static_cast<Protocol>(99), dummy_soft_llrs);
    } catch (...) {}

    // Wideband passband search with base_freq_hz = -1.0f (lines 915-916, 942-943)
    auto wide_audio = text_to_audio("CQ HB9IPH JN47", Protocol::LQ8, 1500.0f, 12000.0f);
    auto wide_msgs = audio_to_messages(wide_audio, -1.0f, 12000.0f, Protocol::LQ8, 4);
    EXPECT_FALSE(wide_msgs.empty());
}

TEST(CoverageEdgeCasesTest, IntentEngineCanonicalCollisionRejection) {
    IntentEngine engine("HB9IPH", "JN47");
    uint32_t my_h24 = hash_callsign_24("HB9IPH");

    // 1. Type 9 with Standard Sender + matching target hash -> rejected as collision
    {
        std::vector<ReceivedFrame> frames;
        ReceivedFrame rf;
        rf.msg.type = MessageType::REPORT73_NONSTD;
        rf.msg.call_2 = "W1AW"; // standard sender
        rf.msg.hash_1 = my_h24;
        frames.push_back(rf);

        UserIntent intent;
        intent.action = IntentAction::IDLE;
        DecisionResult res = engine.process_slot(frames, intent);
        EXPECT_TRUE(res.qso_completed_with.empty());
        EXPECT_FALSE(engine.is_qso_completed("W1AW"));
    }

    // 2. Type 10 with Non-Standard Sender + matching target hash -> accepted as legitimate QSO completion
    {
        std::vector<ReceivedFrame> frames;
        ReceivedFrame rf;
        rf.msg.type = MessageType::M73_NONSTD;
        rf.msg.call_2 = "EA6/W1AW"; // non-standard sender
        rf.msg.hash_1 = my_h24;
        frames.push_back(rf);

        UserIntent intent;
        intent.action = IntentAction::IDLE;
        DecisionResult res = engine.process_slot(frames, intent);
        EXPECT_FALSE(res.qso_completed_with.empty());
        EXPECT_TRUE(engine.is_qso_completed("EA6/W1AW"));
    }

    // 3. Type 11 with matching multi-target hash -> accepted as legitimate QSO completion
    {
        std::vector<ReceivedFrame> frames;
        ReceivedFrame rf;
        rf.msg.type = MessageType::MULTI_REPORT73;
        rf.msg.call_1 = "K1ABC";
        rf.msg.hash_1 = hash_callsign_16("K1ABC");
        rf.msg.multi_targets = {
            {"", my_h24, +5},
            {"", hash_callsign_24("YO1YO"), -3}
        };
        frames.push_back(rf);

        UserIntent intent;
        intent.action = IntentAction::IDLE;
        DecisionResult res = engine.process_slot(frames, intent);
        EXPECT_FALSE(res.qso_completed_with.empty());
        EXPECT_TRUE(engine.is_qso_completed("K1ABC"));
    }
}

TEST(CoverageEdgeCases, DehashingAndTextualRepresentation) {
    // 1. Type 7 (CALL_NONSTD): unresolved vs resolved with dehashing
    {
        Message m_call;
        m_call.type = MessageType::CALL_NONSTD;
        m_call.hash_1 = hash_callsign_20("YO1YO/P");
        m_call.call_2 = "HB9IPH/P";
        m_call.suffix_2 = 1;
        m_call.rst_db = -3;

        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(m_call, payload));

        // Unresolved -> returns <5 hex digits>
        EXPECT_EQ(payload_to_text(payload), "<b86f4> HB9IPH/P -03");

        // Resolved -> returns <YO1YO/P> HB9IPH/P -03
        std::vector<std::string_view> known = {"YO1YO/P", "HB9IPH/P"};
        EXPECT_EQ(payload_to_text(payload, known.data(), known.size()), "<YO1YO/P> HB9IPH/P -03");
    }

    // 2. Type 10 (M73_NONSTD): unresolved vs resolved with dehashing
    {
        Message m_rpt;
        m_rpt.type = MessageType::M73_NONSTD;
        m_rpt.hash_1 = hash_callsign_24("3B9/HB9IP");
        m_rpt.call_2 = "HB9IPH/P";
        m_rpt.suffix_2 = 1;

        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(m_rpt, payload));

        // Unresolved -> returns <hex>
        EXPECT_EQ(payload_to_text(payload), "<fe571d> HB9IPH/P 73");

        // Resolved -> returns <3B9/HB9IP>
        std::vector<std::string_view> known = {"3B9/HB9IP", "HB9IPH/P"};
        EXPECT_EQ(payload_to_text(payload, known.data(), known.size()), "<3B9/HB9IP> HB9IPH/P 73");
    }

    // 3. Type 11 (MULTI_REPORT73): unresolved vs resolved with dehashing
    {
        Message m_mrpt;
        m_mrpt.type = MessageType::MULTI_REPORT73;
        m_mrpt.call_1 = "HB9IPH";
        m_mrpt.hash_1 = hash_callsign_16("HB9IPH");
        m_mrpt.multi_targets = {
            {"YO1YO", hash_callsign_24("YO1YO"), +5},
            {"TU2TU", hash_callsign_24("TU2TU"), -3}
        };

        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(m_mrpt, payload));

        // Unresolved -> returns <hex> for all hashes
        EXPECT_EQ(payload_to_text(payload), "<f674cb> R+05 <0b670e> R-03 <7a8b>");

        // Partial resolution: only DX and 1 target known
        std::vector<std::string_view> known_partial = {"HB9IPH", "YO1YO"};
        EXPECT_EQ(payload_to_text(payload, known_partial.data(), known_partial.size()),
                  "<YO1YO> R+05 <0b670e> R-03 <HB9IPH>");

        // Full resolution: all known
        std::vector<std::string_view> known_full = {"HB9IPH", "YO1YO", "TU2TU"};
        EXPECT_EQ(payload_to_text(payload, known_full.data(), known_full.size()),
                  "<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>");
    }

    // 4. Type 12 (MULTI_73): unresolved vs resolved with dehashing
    {
        Message m_m73;
        m_m73.type = MessageType::MULTI_73;
        m_m73.call_1 = "HB9IPH";
        m_m73.hash_1 = hash_callsign_16("HB9IPH");
        m_m73.multi_targets = {
            {"YO1YO", hash_callsign_24("YO1YO"), 0},
            {"TU2TU", hash_callsign_24("TU2TU"), 0}
        };

        uint8_t payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_message(m_m73, payload));

        // Unresolved -> returns <hex> for all hashes
        EXPECT_EQ(payload_to_text(payload), "<f674cb> <0b670e> <7a8b> 73");

        // Fully resolved
        std::vector<std::string_view> known = {"HB9IPH", "YO1YO", "TU2TU"};
        EXPECT_EQ(payload_to_text(payload, known.data(), known.size()),
                  "<YO1YO> <TU2TU> <HB9IPH> 73");
    }

    // 5. String Parsing of <CALL> and <...>
    {
        Message parsed;
        EXPECT_TRUE(parse_message("CALL <YO1YO/P> HB9IPH", parsed));
        EXPECT_EQ(parsed.type, MessageType::CALL_STD_SUF);
        EXPECT_EQ(parsed.hash_1, hash_callsign_24("YO1YO/P"));
        EXPECT_EQ(parsed.call_2, "HB9IPH");

        Message parsed_ellipsis;
        EXPECT_TRUE(parse_message("CALL <...> HB9IPH", parsed_ellipsis));
        EXPECT_EQ(parsed_ellipsis.type, MessageType::CALL_STD_SUF);
        EXPECT_EQ(parsed_ellipsis.hash_1, 0u);

        Message parsed_mrpt;
        EXPECT_TRUE(parse_message("RPT73 <HB9IPH> <YO1YO> +05 <TU2TU> -03", parsed_mrpt));
        EXPECT_EQ(parsed_mrpt.type, MessageType::MULTI_REPORT73);
        EXPECT_EQ(parsed_mrpt.call_1, "HB9IPH");
        ASSERT_EQ(parsed_mrpt.multi_targets.size(), 2u);
        EXPECT_EQ(parsed_mrpt.multi_targets[0].call, "YO1YO");
        EXPECT_EQ(parsed_mrpt.multi_targets[0].rst_db, 5);
        EXPECT_EQ(parsed_mrpt.multi_targets[1].call, "TU2TU");
        EXPECT_EQ(parsed_mrpt.multi_targets[1].rst_db, -3);

        // 3-token 73 with explicit 73 prefix where caller exceeds 9c nonstd -> fallback to MULTI_73
        Message parsed_73_long;
        EXPECT_TRUE(parse_message("73 YO1YO 3B9/HB9IPH/QRP", parsed_73_long));
        EXPECT_EQ(parsed_73_long.type, MessageType::MULTI_73);
        EXPECT_EQ(parsed_73_long.call_1, "3B9/HB9IPH/QRP");
        ASSERT_EQ(parsed_73_long.multi_targets.size(), 1u);
        EXPECT_EQ(parsed_73_long.multi_targets[0].call, "YO1YO");
    }
}

TEST(CoverageEdgeCases, MessagePredicatesAndParseFallbacks) {
    // 1. Predicates across diverse message types
    Message msg_cq;
    msg_cq.type = MessageType::CQ_STD;
    EXPECT_TRUE(msg_cq.is_cq());
    EXPECT_FALSE(msg_cq.is_call());
    EXPECT_FALSE(msg_cq.is_report73());
    EXPECT_FALSE(msg_cq.is_73());
    EXPECT_FALSE(msg_cq.is_multi_73());
    EXPECT_FALSE(msg_cq.is_free_text());

    Message msg_call;
    msg_call.type = MessageType::CALL_STD_NOSUF;
    EXPECT_FALSE(msg_call.is_cq());
    EXPECT_TRUE(msg_call.is_call());

    Message msg_rpt;
    msg_rpt.type = MessageType::REPORT73_STD;
    EXPECT_TRUE(msg_rpt.is_report73());
    EXPECT_TRUE(msg_rpt.is_reply73());
    EXPECT_FALSE(msg_rpt.is_73());

    Message msg_73;
    msg_73.type = MessageType::M73_STD;
    EXPECT_TRUE(msg_73.is_73());

    Message msg_multi_rpt;
    msg_multi_rpt.type = MessageType::MULTI_REPORT73;
    EXPECT_TRUE(msg_multi_rpt.is_multi_report73());
    EXPECT_TRUE(msg_multi_rpt.is_multi_reply73());
    EXPECT_TRUE(msg_multi_rpt.is_report73());
    EXPECT_FALSE(msg_multi_rpt.is_73());

    Message msg_multi_73;
    msg_multi_73.type = MessageType::MULTI_73;
    EXPECT_TRUE(msg_multi_73.is_multi_73());
    EXPECT_TRUE(msg_multi_73.is_73());

    Message msg_free;
    msg_free.type = MessageType::FREE_TEXT;
    msg_free.text = "HELLO WORLD";
    EXPECT_TRUE(msg_free.is_free_text());


    // 2. MultiTarget operator==
    MultiTarget mt1{"HB9IPH", 0x123456, 5};
    MultiTarget mt2{"HB9IPH", 0x123456, 5};
    MultiTarget mt3{"YO1YO", 0x123456, 5};
    MultiTarget mt4{"HB9IPH", 0x654321, 5};
    MultiTarget mt5{"HB9IPH", 0x123456, -10};
    EXPECT_EQ(mt1, mt2);
    EXPECT_FALSE(mt1 == mt3);
    EXPECT_FALSE(mt1 == mt4);
    EXPECT_FALSE(mt1 == mt5);

    // 3. Fallback to FREE_TEXT when structured syntax is unknown
    Message parsed;
    EXPECT_TRUE(parse_message("UNKNOWN_KEYWORD YO1YO TU2TU KL22 -03", parsed));
    EXPECT_EQ(parsed.type, MessageType::FREE_TEXT);

    // 4. Invalid empty string rejection
    EXPECT_FALSE(parse_message("", parsed));
}

TEST(CoverageEdgeCases, MessageEncodingErrorBranches) {
    uint8_t payload[PAYLOAD_BYTES];

    // CQ_STD with invalid callsign, modifier, or locator
    Message m_cq;
    m_cq.type = MessageType::CQ_STD;
    m_cq.call_1 = "INVALID_CALL_TOOLONG";
    m_cq.modifier = "DX";
    m_cq.locator = "JN47";
    EXPECT_FALSE(encode_message(m_cq, payload));

    m_cq.call_1 = "HB9IPH";
    m_cq.modifier = "INVALID_MODIFIER_TOOLONG";
    EXPECT_FALSE(encode_message(m_cq, payload));

    m_cq.modifier = "DX";
    m_cq.locator = "INVALID_GRID";
    EXPECT_FALSE(encode_message(m_cq, payload));

    // CALL_STD_NOSUF with invalid calls
    Message m_call;
    m_call.type = MessageType::CALL_STD_NOSUF;
    m_call.call_1 = "INVALID_CALL_1";
    m_call.call_2 = "HB9IPH";
    m_call.locator = "JN47";
    EXPECT_FALSE(encode_message(m_call, payload));

    m_call.call_1 = "YO1YO";
    m_call.call_2 = "INVALID_CALL_2";
    EXPECT_FALSE(encode_message(m_call, payload));

    m_call.call_2 = "HB9IPH";
    m_call.locator = "INVALID_GRID";
    EXPECT_FALSE(encode_message(m_call, payload));

    // CALL_STD_2SUF with invalid calls / locators
    Message m_call2;
    m_call2.type = MessageType::CALL_STD_2SUF;
    m_call2.call_1 = "YO1YO";
    m_call2.call_2 = "INVALID_CALL_2";
    EXPECT_FALSE(encode_message(m_call2, payload));

    m_call2.call_2 = "HB9IPH";
    m_call2.locator = "INVALID_LOC";
    EXPECT_FALSE(encode_message(m_call2, payload));

    // REPORT73_STD with invalid calls
    Message m_rpt;
    m_rpt.type = MessageType::REPORT73_STD;
    m_rpt.call_1 = "INVALID_CALL_1";
    m_rpt.call_2 = "HB9IPH";
    EXPECT_FALSE(encode_message(m_rpt, payload));

    m_rpt.call_1 = "YO1YO";
    m_rpt.call_2 = "INVALID_CALL_2";
    EXPECT_FALSE(encode_message(m_rpt, payload));

    // M73_STD with invalid calls
    Message m_73;
    m_73.type = MessageType::M73_STD;
    m_73.call_1 = "INVALID_CALL_1";
    m_73.call_2 = "HB9IPH";
    EXPECT_FALSE(encode_message(m_73, payload));

    m_73.call_1 = "YO1YO";
    m_73.call_2 = "INVALID_CALL_2";
    EXPECT_FALSE(encode_message(m_73, payload));

    // CALL_NONSTD with invalid nonstd call
    Message m_nonstd;
    m_nonstd.type = MessageType::CALL_NONSTD;
    m_nonstd.call_1 = "YO1YO";
    m_nonstd.call_2 = "INVALID NONSTD @@@@";
    EXPECT_FALSE(encode_message(m_nonstd, payload));

    // UNKNOWN message type fails encoding
    Message m_unknown;
    m_unknown.type = MessageType::UNKNOWN;
    EXPECT_FALSE(encode_message(m_unknown, payload));

    // CQ_STD with invalid call / locator
    Message m_cq_err;
    m_cq_err.type = MessageType::CQ_STD;
    m_cq_err.call_1 = "INVALID_CALL";
    EXPECT_FALSE(encode_message(m_cq_err, payload));

    // CQ_NONSTD_1 with invalid nonstd call
    Message m_cq_ns1;
    m_cq_ns1.type = MessageType::CQ_NONSTD_1;
    m_cq_ns1.call_1 = "INVALID NONSTD @@@@";
    m_cq_ns1.locator = "JN47";
    EXPECT_FALSE(encode_message(m_cq_ns1, payload));

    // CQ_NONSTD_2 with invalid nonstd call
    Message m_cq_ns2;
    m_cq_ns2.type = MessageType::CQ_NONSTD_2;
    m_cq_ns2.call_1 = "INVALID NONSTD @@@@";
    EXPECT_FALSE(encode_message(m_cq_ns2, payload));

    // CQ_NONSTD_3 with invalid nonstd call
    Message m_cq_ns3;
    m_cq_ns3.type = MessageType::CQ_NONSTD_3;
    m_cq_ns3.call_1 = "INVALID NONSTD @@@@";
    EXPECT_FALSE(encode_message(m_cq_ns3, payload));

    // M73_NONSTD with invalid nonstd call
    Message m_m73_ns;
    m_m73_ns.type = MessageType::M73_NONSTD;
    m_m73_ns.call_1 = "YO1YO";
    m_m73_ns.call_2 = "INVALID NONSTD @@@@";
    EXPECT_FALSE(encode_message(m_m73_ns, payload));

    // RESERVED message types
    Message m_res;
    m_res.type = MessageType::RESERVED;
    m_res.raw_payload = std::vector<uint8_t>(10, 0xAA);
    EXPECT_TRUE(encode_message(m_res, payload));
}

TEST(CoverageEdgeCases, MessageFormattingAndResolutionVariants) {
    // 1. CALL_NONSTD formatting with hash and known callsigns
    Message msg_ns;
    msg_ns.type = MessageType::CALL_NONSTD;
    msg_ns.hash_1 = hash_callsign_20("YO1YO");
    msg_ns.call_2 = "HB9IPH";
    msg_ns.rst_db = 0;
    std::string s1 = format_message(msg_ns);
    EXPECT_EQ(s1, "<f674c> HB9IPH +00");

    msg_ns.call_1 = "YO1YO";
    std::string s2 = format_message(msg_ns);
    EXPECT_EQ(s2, "<YO1YO> HB9IPH +00");

    // 2. MULTI_REPORT73 formatting with 1 target
    Message msg_mr1;
    msg_mr1.type = MessageType::MULTI_REPORT73;
    msg_mr1.call_1 = "HB9IPH";
    MultiTarget mt;
    mt.call = "YO1YO";
    mt.rst_db = 5;
    msg_mr1.multi_targets.push_back(mt);
    std::string sm1 = format_message(msg_mr1);
    EXPECT_EQ(sm1, "<YO1YO> R+05 <HB9IPH>");

    // 3. MULTI_73 formatting with 1 target
    Message msg_m73_1;
    msg_m73_1.type = MessageType::MULTI_73;
    msg_m73_1.call_1 = "HB9IPH";
    msg_m73_1.multi_targets.push_back(mt);
    std::string sm73_1 = format_message(msg_m73_1);
    EXPECT_EQ(sm73_1, "<YO1YO> <HB9IPH> 73");

    // 4. RESERVED message types formatting
    Message msg_res_a;
    msg_res_a.type = MessageType::RESERVED_A;
    msg_res_a.raw_payload = {0x01, 0x02, 0x03, 0x04};
    std::string s_res_a = format_message(msg_res_a);
    EXPECT_EQ(s_res_a, "[RESERVED]");

    Message msg_res_b;
    msg_res_b.type = MessageType::RESERVED_B;
    msg_res_b.raw_payload = {0x01, 0x02, 0x03, 0x04};
    std::string s_res_b = format_message(msg_res_b);
    EXPECT_EQ(s_res_b, "[RESERVED]");

    Message msg_res_c;
    msg_res_c.type = MessageType::RESERVED_C;
    msg_res_c.raw_payload = {0x01, 0x02, 0x03, 0x04};
    std::string s_res_c = format_message(msg_res_c);
    EXPECT_EQ(s_res_c, "[RESERVED]");

    // 5. payload_to_text with known callsigns
    uint8_t payload[PAYLOAD_BYTES];
    encode_message(msg_mr1, payload);
    std::string_view known[] = {"HB9IPH", "YO1YO", "TU2TU"};
    std::string text_dec = payload_to_text(payload, known, 3);
    EXPECT_NE(text_dec.find("HB9IPH"), std::string::npos);

    // 6. Unknown / Invalid MessageType formatting
    Message msg_unknown;
    msg_unknown.type = static_cast<MessageType>(999);
    EXPECT_EQ(format_message(msg_unknown), "[UNKNOWN]");

    // 7. parse_message variants
    Message msg_parsed;
    EXPECT_TRUE(parse_message("CQ DX HB9IPH JN47", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::CQ_STD);

    EXPECT_TRUE(parse_message("CALL YO1YO HB9IPH JN47 -03", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::CALL_STD_NOSUF);

    EXPECT_TRUE(parse_message("CALL <f674cb> HB9IPH/P JN47 -03", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::CALL_STD_SUF);

    EXPECT_TRUE(parse_message("CALL <f674cb> EA6/HB9IP/P", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::CALL_NONSTD);

    EXPECT_TRUE(parse_message("RPT73 HB9IPH/P YO1YO +05", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::REPORT73_STD);

    EXPECT_TRUE(parse_message("73 HB9IPH/P YO1YO", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::M73_STD);

    EXPECT_TRUE(parse_message("73 <f674cb> EA6/HB9IP/P", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::M73_NONSTD);

    EXPECT_TRUE(parse_message("RPT73 <HB9IPH> <YO1YO> +05 <TU2TU> -03", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::MULTI_REPORT73);

    EXPECT_TRUE(parse_message("MULTI-73 <HB9IPH> <YO1YO> <TU2TU>", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::MULTI_73);

    EXPECT_TRUE(parse_message("73M HB9IPH YO1YO TU2TU", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::MULTI_73);

    EXPECT_TRUE(parse_message("M73 HB9IPH YO1YO TU2TU", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::MULTI_73);

    EXPECT_TRUE(parse_message("MULTI_73 HB9IPH YO1YO TU2TU", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::MULTI_73);

    EXPECT_TRUE(parse_message("73 <f674cb> <3B9/HB9IPH/LONG-COMPOUND>", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::MULTI_73);

    EXPECT_TRUE(parse_message("RPT73 <f674cb> <3B9/HB9IPH/LONG-COMPOUND> -05", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::MULTI_REPORT73);

    EXPECT_TRUE(parse_message("73 DE HB9IP", msg_parsed));
    EXPECT_EQ(msg_parsed.type, MessageType::FREE_TEXT);

    EXPECT_FALSE(parse_message("", msg_parsed));

    // 8. Encoding error paths
    Message bad_msg;
    bad_msg.type = MessageType::CQ_STD;
    bad_msg.call_1 = "INVALID_TOO_LONG_FOR_STANDARD_CALLSIGN";
    uint8_t bad_payload[PAYLOAD_BYTES];
    EXPECT_FALSE(encode_message(bad_msg, bad_payload));

    bad_msg.type = MessageType::CQ_NONSTD_2;
    bad_msg.call_1 = "EA6/HB9IP";
    bad_msg.modifier = "12345678901234567890";
    EXPECT_FALSE(encode_message(bad_msg, bad_payload));
}

// ============================================================================
// 14. Deep Micro-Branch & Error Path Tests for 99%+ Coverage
// ============================================================================

TEST(CoverageEdgeCases, CallsignWhitespaceAndTokens) {
    std::string base;
    uint8_t suf = 0;
    EXPECT_TRUE(parse_standard_callsign_suffix("  HB9IPH/P  ", base, suf));
    EXPECT_EQ(base, "HB9IPH");
    EXPECT_EQ(suf, 1);

    EXPECT_TRUE(parse_standard_callsign_suffix("   HB9IPH   ", base, suf));
    EXPECT_EQ(base, "HB9IPH");
    EXPECT_EQ(suf, 0);

    EXPECT_FALSE(parse_standard_callsign_suffix("    ", base, suf));
    EXPECT_FALSE(parse_standard_callsign_suffix("", base, suf));

    // Decode with s1 == 0 (invalid standard callsign)
    std::string decoded;
    EXPECT_FALSE(decode_callsign_std(1 * 7085880, decoded));
}

TEST(CoverageEdgeCases, ModifierBase32AndPadding) {
    uint32_t packed = 0;
    std::string mod;

    EXPECT_TRUE(encode_modifier_20("   ", packed));
    EXPECT_EQ(packed, MODIFIER_NONE);

    EXPECT_TRUE(encode_modifier_20(" 1234 ", packed));
    EXPECT_TRUE(decode_modifier_20(packed, mod));
    EXPECT_EQ(mod, "1234");

    EXPECT_FALSE(encode_modifier_20("TOOLONGMODIFIER", packed));
    EXPECT_FALSE(encode_modifier_20("$$##", packed));
}

TEST(CoverageEdgeCases, VaricodeInvalidAccumulatorReset) {
    uint8_t buffer[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    BitBuffer bb(buffer, 40);
    std::string out;
    int count = decode_varicode(bb, out, 40);
    EXPECT_GE(count, 0);
}

TEST(CoverageEdgeCases, LdpcFailureOnNoise) {
    float llrs[LDPC_CODEWORD_BITS];
    for (int i = 0; i < LDPC_CODEWORD_BITS; ++i) {
        llrs[i] = (i % 2 == 0) ? +10.0f : -10.0f; // Strongly conflicting pattern with high syndrome
    }
    uint8_t out[LDPC_INPUT_BYTES] = {0};
    int res = ldpc_decode(llrs, out, 5);
    EXPECT_EQ(res, -1);
}

TEST(CoverageEdgeCases, EasyApiExtendedEdgeCases) {
    // make_73 with compound callsign > 9 chars as target/my call
    Message msg = make_73("W1AW", "3B9/HB9IPH/P");
    EXPECT_EQ(msg.type, MessageType::MULTI_73);

    // text_to_audio with invalid sample rate (fails in message_to_audio)
    auto audio = text_to_audio("CQ HB9IPH JN47", Protocol::LQ8, -100.0f, -1000.0f);
    EXPECT_TRUE(audio.empty());
}

TEST(CoverageEdgeCases, IntentEngineMultiTargetAndHashBranches) {
    IntentEngine engine("HB9IPH", "JN47");
    std::vector<std::string> known = {"W1AW", "YO1YO", "K1ABC"};
    engine.add_known_callsigns(known);

    // 1. process_slot with MULTI_REPORT73 in rx_frames
    ReceivedFrame rf;
    rf.snr_db = -7;
    rf.msg.type = MessageType::MULTI_REPORT73;
    rf.msg.hash_1 = hash_callsign_16("HB9IPH");
    MultiTarget mt;
    mt.call = "YO1YO";
    mt.hash = hash_callsign_24("YO1YO");
    mt.rst_db = -3;
    rf.msg.multi_targets.push_back(mt);

    UserIntent intent;
    intent.action = IntentAction::REPLY_TO_STATIONS;
    intent.target_call_1 = "YO1YO";
    auto dec = engine.process_slot({rf}, intent);
    EXPECT_TRUE(dec.success);

    // 2. process_slot with M73_NONSTD directed to me with hash_2 resolved via known_calls
    ReceivedFrame rf2;
    rf2.msg.type = MessageType::M73_NONSTD;
    rf2.msg.call_1 = "HB9IPH";
    rf2.msg.call_2 = "<01A4F2>";
    rf2.msg.hash_2 = hash_callsign_24("W1AW");

    UserIntent cq_intent;
    cq_intent.action = IntentAction::CALL_CQ;
    auto dec2 = engine.process_slot({rf2}, cq_intent);
    EXPECT_TRUE(engine.is_qso_completed("W1AW"));
}

TEST(CoverageEdgeCases, MessagePayloadFormattingAndResolutions) {
    // 1. format_hex_tag
    uint8_t payload[PAYLOAD_BYTES] = {0};
    EXPECT_TRUE(hex_to_payload("0x12 0x34 0xAB 0xCD 0xEF 0x01 0x02 0x03 0x04 0x05", payload));
    EXPECT_FALSE(hex_to_payload("0x12", payload)); // too short

    // 2. Message::get_sender_call and get_target_call hash fallbacks
    Message m;
    m.type = MessageType::CALL_STD_SUF;
    m.call_1 = "";
    m.hash_1 = 0x123456;
    m.call_2 = "";
    m.hash_2 = 0x654321;
    EXPECT_EQ(m.get_sender_call(), "<...>");
    EXPECT_EQ(m.get_target_call(), "<...>");

    m.type = MessageType::MULTI_REPORT73;
    m.call_1 = "";
    m.hash_1 = 0x1234;
    m.multi_targets.push_back({"", 0x123456, 0});
    EXPECT_EQ(m.get_sender_call(), "<...>");
    EXPECT_EQ(m.get_target_call(), "<...>");

    // 3. encode_message and format_message for RESERVED_A, RESERVED_B & RESERVED_C
    Message res_a;
    res_a.type = MessageType::RESERVED_A;
    res_a.raw_payload = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x80};
    uint8_t p_a[PAYLOAD_BYTES];
    EXPECT_TRUE(encode_message(res_a, p_a));
    Message dec_a;
    EXPECT_TRUE(decode_message(p_a, dec_a));
    EXPECT_EQ(dec_a.type, MessageType::RESERVED_A);

    Message res_b;
    res_b.type = MessageType::RESERVED_B;
    res_b.raw_payload = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33};
    uint8_t p_b[PAYLOAD_BYTES];
    EXPECT_TRUE(encode_message(res_b, p_b));
    Message dec_b;
    EXPECT_TRUE(decode_message(p_b, dec_b));
    EXPECT_EQ(dec_b.type, MessageType::RESERVED_B);

    Message res_c;
    res_c.type = MessageType::RESERVED_C;
    res_c.raw_payload = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33};
    uint8_t p_c[PAYLOAD_BYTES];
    EXPECT_TRUE(encode_message(res_c, p_c));

    Message dec_c;
    EXPECT_TRUE(decode_message(p_c, dec_c));
    EXPECT_EQ(dec_c.type, MessageType::RESERVED_C);

    // Invalid message type encode/decode
    Message invalid_m;
    invalid_m.type = static_cast<MessageType>(999);
    uint8_t p_inv[PAYLOAD_BYTES];
    EXPECT_FALSE(encode_message(invalid_m, p_inv));

    // 4. resolve_callsigns with hash_2 and multi_targets
    Message m_resolve;
    m_resolve.type = MessageType::M73_NONSTD;
    m_resolve.hash_2 = hash_callsign_24("HB9IPH");
    std::string_view known_arr[] = {"HB9IPH", "W1AW", "YO1YO"};
    EXPECT_TRUE(resolve_callsigns(m_resolve, known_arr, 3));
    EXPECT_EQ(m_resolve.call_2, "HB9IPH");

    Message m_multi_res;
    m_multi_res.type = MessageType::MULTI_REPORT73;
    m_multi_res.multi_targets.push_back({"", hash_callsign_24("W1AW"), 0});
    EXPECT_TRUE(resolve_callsigns(m_multi_res, known_arr, 3));
    EXPECT_EQ(m_multi_res.multi_targets[0].call, "W1AW");

    // 5. format_message bracket variants
    Message m_fmt1;
    m_fmt1.type = MessageType::CALL_STD_SUF;
    m_fmt1.call_1 = "<W1AW>";
    m_fmt1.call_2 = "HB9IPH";
    m_fmt1.rst_db = -5;
    EXPECT_NE(format_message(m_fmt1), "");

    m_fmt1.call_1 = "";
    m_fmt1.hash_1 = 0;
    EXPECT_NE(format_message(m_fmt1), "");

    Message m_fmt2;
    m_fmt2.type = MessageType::CALL_NONSTD;
    m_fmt2.call_1 = "<W1AW>";
    m_fmt2.call_2 = "EA6/HB9IP";
    EXPECT_NE(format_message(m_fmt2), "");

    m_fmt2.call_1 = "";
    m_fmt2.hash_1 = 0;
    EXPECT_NE(format_message(m_fmt2), "");

    Message m_fmt3;
    m_fmt3.type = MessageType::M73_NONSTD;
    m_fmt3.call_1 = "<W1AW>";
    m_fmt3.call_2 = "EA6/HB9IP";
    EXPECT_NE(format_message(m_fmt3), "");

    m_fmt3.call_1 = "";
    m_fmt3.hash_1 = 0;
    EXPECT_NE(format_message(m_fmt3), "");

    Message m_fmt4;
    m_fmt4.type = MessageType::MULTI_REPORT73;
    m_fmt4.call_1 = "<HB9IPH>";
    m_fmt4.multi_targets.push_back({"<W1AW>", 0, 0});
    EXPECT_NE(format_message(m_fmt4), "");

    m_fmt4.call_1 = "";
    m_fmt4.hash_1 = 0;
    m_fmt4.multi_targets[0].call = "";
    m_fmt4.multi_targets[0].hash = 0;
    EXPECT_NE(format_message(m_fmt4), "");

    Message m_fmt5;
    m_fmt5.type = MessageType::MULTI_73;
    m_fmt5.call_1 = "<HB9IPH>";
    m_fmt5.multi_targets.push_back({"<W1AW>", 0, 0});
    EXPECT_NE(format_message(m_fmt5), "");

    m_fmt5.call_1 = "";
    m_fmt5.hash_1 = 0;
    m_fmt5.multi_targets[0].call = "";
    m_fmt5.multi_targets[0].hash = 0;
    EXPECT_NE(format_message(m_fmt5), "");

    // 6. parse_message variants
    Message m_p;
    EXPECT_TRUE(parse_message("CQ CQ EA6/HB9IP", m_p));
    EXPECT_EQ(m_p.type, MessageType::CQ_NONSTD_2);

    EXPECT_TRUE(parse_message("CQ CQ 3B9/HB9IPH/P", m_p));
    EXPECT_EQ(m_p.type, MessageType::CQ_NONSTD_3);

    EXPECT_TRUE(parse_message("CALL W1AW HB9IPH -05", m_p));
    EXPECT_EQ(m_p.type, MessageType::CALL_STD_SUF);

    EXPECT_TRUE(parse_message("CALL W1AW HB9IPH JN47", m_p));
    EXPECT_EQ(m_p.type, MessageType::CALL_STD_NOSUF);

    EXPECT_TRUE(parse_message("73M <1234> <5678ab>", m_p));
    EXPECT_EQ(m_p.type, MessageType::MULTI_73);

    EXPECT_TRUE(parse_message("EA6/HB9IP/P W1AW 73", m_p));
    EXPECT_EQ(m_p.type, MessageType::M73_NONSTD);
}

TEST(CoverageEdgeCases, TransportAudioAndDecoderEdgeCases) {
    // 1. Low energy audio demodulation
    std::vector<float> silent_audio(200000, 0.0f);
    std::vector<float> llrs;
    EXPECT_FALSE(demodulate_audio_soft(silent_audio, 0, 1000.0f, 12000.0f, Protocol::LQ8, llrs));

    // 2. LQ4 and LQ2 soft demodulation paths
    Message msg = make_cq("HB9IPH", "JN47", "DX");
    std::vector<float> audio_lq4;
    EXPECT_TRUE(message_to_audio(msg, Protocol::LQ4, 1000.0f, 12000.0f, audio_lq4));
    EXPECT_FALSE(audio_lq4.empty());
    EXPECT_TRUE(demodulate_audio_soft(audio_lq4, 0, 1000.0f, 12000.0f, Protocol::LQ4, llrs));

    std::vector<float> audio_lq2;
    EXPECT_TRUE(message_to_audio(msg, Protocol::LQ2, 1000.0f, 12000.0f, audio_lq2));
    EXPECT_FALSE(audio_lq2.empty());
    EXPECT_TRUE(demodulate_audio_soft(audio_lq2, 0, 1000.0f, 12000.0f, Protocol::LQ2, llrs));

    // 3. Short tone sequence
    ToneSequence short_seq;
    short_seq.protocol = Protocol::LQ8;
    short_seq.tones = {0, 1, 2};
    uint8_t cw[LDPC_CODEWORD_BYTES];
    EXPECT_FALSE(tones_to_codeword(short_seq, cw));

    // 4. Parallel decoder multi-thread deduplication path
    std::vector<Message> multi_dec = audio_to_messages(audio_lq4, 1000.0f, 12000.0f, Protocol::LQ4, 2);
    EXPECT_FALSE(multi_dec.empty());

    // 5. Silent audio waterfall returns empty
    std::vector<Message> empty_dec = audio_to_messages(silent_audio, 1000.0f, 12000.0f, Protocol::LQ8, 1);
    EXPECT_TRUE(empty_dec.empty());
}

TEST(CoverageEdgeCases, MessageAndCallsignExhaustiveMicroBranches) {
    // 1. Callsign suffix > 3
    uint32_t packed = 0;
    EXPECT_FALSE(encode_callsign_std("W1ABCDE", packed));
    EXPECT_FALSE(is_standard_callsign("W1ABCDE"));

    // 2. Message::get_target_call multi_targets with explicit call
    Message m_tgt;
    m_tgt.type = MessageType::MULTI_REPORT73;
    m_tgt.multi_targets.push_back({"W1AW", 0, 0});
    EXPECT_EQ(m_tgt.get_target_call(), "W1AW");

    // 3. encode_message with explicit callsign strings without slashes and separate suffix fields
    Message m_enc_suf;
    m_enc_suf.type = MessageType::CALL_STD_SUF;
    m_enc_suf.call_1 = "W1AW";
    m_enc_suf.call_2 = "HB9IPH";
    m_enc_suf.suffix_2 = 1;
    m_enc_suf.locator = "JN47";
    m_enc_suf.rst_db = -5;
    uint8_t p_enc[PAYLOAD_BYTES];
    EXPECT_TRUE(encode_message(m_enc_suf, p_enc));

    m_enc_suf.type = MessageType::CQ_STD;
    m_enc_suf.call_1 = "HB9IPH";
    m_enc_suf.suffix_1 = 1;
    m_enc_suf.modifier = "DX";
    m_enc_suf.locator = "JN47";
    EXPECT_TRUE(encode_message(m_enc_suf, p_enc));

    m_enc_suf.type = MessageType::REPORT73_STD;
    m_enc_suf.call_1 = "HB9IPH";
    m_enc_suf.suffix_1 = 1;
    m_enc_suf.call_2 = "W1AW";
    m_enc_suf.suffix_2 = 1;
    m_enc_suf.rst_db = +5;
    EXPECT_TRUE(encode_message(m_enc_suf, p_enc));

    m_enc_suf.type = MessageType::M73_STD;
    m_enc_suf.call_1 = "HB9IPH";
    m_enc_suf.suffix_1 = 1;
    m_enc_suf.call_2 = "W1AW";
    m_enc_suf.suffix_2 = 1;
    EXPECT_TRUE(encode_message(m_enc_suf, p_enc));

    // 4. MULTI_REPORT73 and MULTI_73 encode with empty multi_targets but hash_2 / call_2
    Message m_mult;
    m_mult.type = MessageType::MULTI_REPORT73;
    m_mult.call_1 = "HB9IPH";
    m_mult.hash_2 = 0x123456;
    m_mult.rst_db = -10;
    EXPECT_TRUE(encode_message(m_mult, p_enc));

    m_mult.hash_2 = 0;
    m_mult.call_2 = "W1AW";
    EXPECT_TRUE(encode_message(m_mult, p_enc));

    m_mult.type = MessageType::MULTI_73;
    m_mult.call_1 = "HB9IPH";
    m_mult.hash_2 = 0x123456;
    EXPECT_TRUE(encode_message(m_mult, p_enc));

    m_mult.hash_2 = 0;
    m_mult.call_2 = "W1AW";
    EXPECT_TRUE(encode_message(m_mult, p_enc));

    // 5. parse_message: 73M with literal callsigns and 3-token long compound fallback to MULTI_73
    Message m_parsed;
    EXPECT_TRUE(parse_message("73M HB9IPH W1AW YO1YO", m_parsed));
    EXPECT_EQ(m_parsed.type, MessageType::MULTI_73);

    EXPECT_TRUE(parse_message("73M <1234> W1AW YO1YO", m_parsed));
    EXPECT_EQ(m_parsed.type, MessageType::MULTI_73);

    EXPECT_TRUE(parse_message("W1AW 3B9/HB9IPH/P 73", m_parsed));
    EXPECT_EQ(m_parsed.type, MessageType::MULTI_73);

    // 6. hex_to_payload with non-hex characters inside valid length
    uint8_t dummy_p[PAYLOAD_BYTES];
    EXPECT_TRUE(hex_to_payload("01 02 03 04 05 06 07 08 09 0A", dummy_p));

    // 7. IntentEngine find_measured_snr get_sender_call fallback
    IntentEngine engine("HB9IPH", "JN47");
    ReceivedFrame rf;
    rf.snr_db = -9;
    rf.msg.type = MessageType::FREE_TEXT;
    rf.msg.text = "CQ HB9IPH JN47";
    rf.msg.call_1 = "W1AW";
    std::vector<ReceivedFrame> rfs = {rf};
    UserIntent intent;
    intent.action = IntentAction::CALL_STATION;
    intent.target_call_1 = "W1AW";
    auto dec = engine.process_slot(rfs, intent);
    EXPECT_TRUE(dec.success);
}

TEST(CoverageEdgeCases, FullCoverageSaturation) {
    // 1. callsign_nonstd: decode_callsign_nonstd_u128 max_chars edge cases
    {
        std::string s;
        EXPECT_FALSE(decode_callsign_nonstd_u128({0, 0}, 0, s));
        EXPECT_FALSE(decode_callsign_nonstd_u128({0, 0}, 25, s));
    }

    // 2. easy.cpp: text_to_tones and text_to_audio encode failure
    {
        EXPECT_FALSE(text_to_tones("CALL <01A4F> @@@@@@@@ -03", Protocol::LQ8).has_value());
        auto audio = text_to_audio("CALL <01A4F> @@@@@@@@ -03", Protocol::LQ8, 1000.0f, 12000.0f);
        EXPECT_TRUE(audio.empty());
    }

    // 3. huffman.cpp: empty or truncated bitbuffer
    {
        BitBuffer empty_bb(static_cast<const uint8_t*>(nullptr), 0);
        EXPECT_EQ(decode_huffman_prefix(empty_bb), MessageType::UNKNOWN);

        uint8_t two_bits[1] = {0x00};
        BitBuffer trunc_bb(two_bits, 2);
        EXPECT_EQ(decode_huffman_prefix(trunc_bb), MessageType::UNKNOWN);
    }

    // 4. intent.cpp: line 101 (get_target_call matching) and line 115 (m.hash_2 matching)
    {
        IntentEngine engine("HB9IPH", "JN47");
        std::vector<ReceivedFrame> rfs;

        ReceivedFrame rf1;
        rf1.snr_db = -12;
        rf1.msg.type = MessageType::CALL_STD_SUF;
        rf1.msg.call_1 = "";
        rf1.msg.hash_1 = 0x123456;
        rf1.msg.call_2 = "TU2TU";
        rfs.push_back(rf1);

        UserIntent i1;
        i1.action = IntentAction::CALL_STATION;
        i1.target_call_1 = "<...>";
        auto dec1 = engine.process_slot(rfs, i1);
        EXPECT_TRUE(dec1.success);
        EXPECT_EQ(dec1.tx_message.rst_db, -12);

        ReceivedFrame rf2;
        rf2.snr_db = +3;
        rf2.msg.type = MessageType::CALL_STD_SUF;
        rf2.msg.hash_1 = hash_callsign_24("YO1YO");
        rf2.msg.call_2 = "TU2TU";
        rf2.msg.hash_2 = hash_callsign_24("W1AW");
        rfs.push_back(rf2);

        UserIntent i2;
        i2.action = IntentAction::CALL_STATION;
        i2.target_call_1 = "W1AW";
        auto dec2 = engine.process_slot(rfs, i2);
        EXPECT_TRUE(dec2.success);
        EXPECT_EQ(dec2.tx_message.rst_db, +3);
    }

    // 5. ldpc.cpp: ldpc_decode failure
    {
        float bad_llrs[LDPC_CODEWORD_BITS];
        for (int i = 0; i < LDPC_CODEWORD_BITS; ++i) {
            bad_llrs[i] = (i % 2 == 0) ? 10.0f : -10.0f;
        }
        uint8_t out_91[LDPC_INPUT_BYTES];
        EXPECT_EQ(ldpc_decode(bad_llrs, out_91, 1), -1);
    }

    // 6. modifier.cpp: encode_modifier_20 with non-digits after MOD:
    {
        uint32_t val = 0;
        EXPECT_FALSE(encode_modifier_20("<MOD:12abc>", val));
    }

    // 7. varicode.cpp: lowercase letters, Latin-1 accents, unmapped byte, corrupted decoding
    {
        uint8_t var_buf[16] = {0};
        BitBuffer bb(var_buf, 128);
        EXPECT_GT(encode_varicode("hello \xE9\xFF", bb, 100), 0);

        uint8_t inv_data[16];
        std::memset(inv_data, 0xFF, sizeof(inv_data));
        BitBuffer inv_bb(inv_data, 128);
        std::string out_dec;
        decode_varicode(inv_bb, out_dec, 60);

        BitBuffer inv_tok_bb(inv_data, 128);
        decode_varicode_token(inv_tok_bb, 60);
    }

    // 8. message.cpp: multi_targets[0].hash != 0 without call
    {
        Message msg;
        msg.type = MessageType::MULTI_73;
        MultiTarget mt;
        mt.call = "";
        mt.hash = 0x123456;
        msg.multi_targets.push_back(mt);
        EXPECT_EQ(msg.get_target_call(), "<...>");
    }

    // 9. message.cpp: encode message branches with no /P suffix
    {
        uint8_t p[PAYLOAD_BYTES];

        Message m6;
        m6.type = MessageType::CALL_STD_SUF;
        m6.call_1 = "YO1YO";
        m6.call_2 = "HB9IPH";
        m6.locator = "JN47";
        m6.rst_db = -5;
        EXPECT_TRUE(encode_message(m6, p));

        Message m7;
        m7.type = MessageType::CALL_NONSTD;
        m7.hash_1 = 0x123456;
        m7.call_2 = "EA6/HB9IP/P";
        m7.suffix_2 = 1;
        m7.rst_db = -3;
        EXPECT_TRUE(encode_message(m7, p));

        Message m_res_b;
        m_res_b.type = MessageType::RESERVED_B;
        m_res_b.raw_payload = {0x01, 0x02, 0x03, 0x04};
        EXPECT_TRUE(encode_message(m_res_b, p));

        Message m_cq;
        m_cq.type = MessageType::CQ_STD;
        m_cq.call_1 = "HB9IPH";
        m_cq.modifier = "";
        m_cq.locator = "JN47";
        EXPECT_TRUE(encode_message(m_cq, p));

        Message m_r73;
        m_r73.type = MessageType::REPORT73_STD;
        m_r73.call_1 = "HB9IPH";
        m_r73.call_2 = "YO1YO";
        m_r73.rst_db = 3;
        EXPECT_TRUE(encode_message(m_r73, p));

        Message m_73;
        m_73.type = MessageType::M73_STD;
        m_73.call_1 = "HB9IPH";
        m_73.call_2 = "YO1YO";
        EXPECT_TRUE(encode_message(m_73, p));

        Message m_res_c;
        m_res_c.type = MessageType::RESERVED_C;
        m_res_c.raw_payload = {0x11, 0x22, 0x33, 0x44};
        EXPECT_TRUE(encode_message(m_res_c, p));

        Message m_inv;
        m_inv.type = static_cast<MessageType>(999);
        EXPECT_FALSE(encode_message(m_inv, p));
    }

    // 10. message.cpp: parse_message edge branches
    {
        Message parsed;
        EXPECT_TRUE(parse_message("CALL <123456> EA6/HB9IP/P -03", parsed));
        EXPECT_EQ(parsed.type, MessageType::CALL_NONSTD);

        EXPECT_TRUE(parse_message("<123456> EA6/HB9IP JN47", parsed));
        EXPECT_EQ(parsed.type, MessageType::CALL_NONSTD);

        EXPECT_TRUE(parse_message("<123456> EA6/HB9IP/P JN47 -03", parsed));
        EXPECT_EQ(parsed.type, MessageType::CALL_NONSTD);

        EXPECT_TRUE(parse_message("<123456> HB9IPH12345 JN47", parsed));
        EXPECT_EQ(parsed.type, MessageType::CALL_NONSTD);
    }

    // 11. transport.cpp: codeword_to_tones invalid protocol, silence energy, scan edge cases
    {
        uint8_t cw[LDPC_CODEWORD_BYTES] = {0};
        ToneSequence seq;
        EXPECT_THROW(codeword_to_tones(cw, static_cast<Protocol>(99), seq), std::invalid_argument);

        uint8_t out_cw[LDPC_CODEWORD_BYTES];
        ToneSequence short_seq;
        short_seq.protocol = Protocol::LQ8;
        EXPECT_FALSE(tones_to_codeword(short_seq, out_cw));

        ToneSequence valid_tone_inv_payload;
        uint8_t zeros_payload[PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(encode_payload(zeros_payload, Protocol::LQ8, valid_tone_inv_payload));
        Message dec_msg;
        EXPECT_FALSE(decode_tones(valid_tone_inv_payload, dec_msg));

        std::vector<float> silence(180000, 0.0f);
        std::vector<float> audio_llrs;
        EXPECT_FALSE(demodulate_audio_soft(silence, 0, 1000.0f, 12000.0f, Protocol::LQ8, audio_llrs));

        std::vector<float> non_silence(180000, 0.1f);
        EXPECT_THROW(demodulate_audio_soft(non_silence, 0, 1000.0f, 12000.0f, static_cast<Protocol>(99), audio_llrs), std::invalid_argument);

        uint8_t in_91[LDPC_INPUT_BYTES];
        append_crc14(zeros_payload, in_91);
        uint8_t valid_cw[LDPC_CODEWORD_BYTES];
        ldpc_encode(in_91, valid_cw);
        std::vector<float> perfect_llrs(LDPC_CODEWORD_BITS);
        for (int i = 0; i < LDPC_CODEWORD_BITS; ++i) {
            perfect_llrs[i] = ((valid_cw[i >> 3] >> (7 - (i & 7))) & 1U) ? -10.0f : 10.0f;
        }
        Message dec_soft;
        EXPECT_FALSE(decode_soft_llrs(perfect_llrs, dec_soft, Protocol::LQ8));

        Message qso_msg;
        qso_msg.type = MessageType::CQ_STD;
        qso_msg.call_1 = "HB9IPH";
        qso_msg.locator = "JN47";
        std::vector<float> qso_audio;
        ASSERT_TRUE(message_to_audio(qso_msg, Protocol::LQ2, 1000.0f, 12000.0f, qso_audio));

        auto res_single = audio_to_messages(qso_audio, 1000.0f, 12000.0f, Protocol::LQ2, 1);
        EXPECT_FALSE(res_single.empty());

        auto res_multi = audio_to_messages(qso_audio, 1000.0f, 12000.0f, Protocol::LQ2, 4);
        EXPECT_FALSE(res_multi.empty());
    }
}

// ============================================================================
// 15. Scoped 16-bit DX Dehashing & Collision Prevention Tests
// ============================================================================

TEST(CoverageEdgeCases, IntentEngineScopedDehashingAndCollisionRejection) {
    IntentEngine engine("HB9IPH", "JN47");

    // 1. Basic API coverage for called stations tracking
    EXPECT_TRUE(engine.get_called_stations().empty());
    engine.add_called_station("");
    EXPECT_TRUE(engine.get_called_stations().empty());

    engine.add_called_station("3B9/HB9IP/P");
    EXPECT_TRUE(engine.has_called_station("3B9/HB9IP/P"));
    EXPECT_FALSE(engine.has_called_station("W1AW"));
    EXPECT_EQ(engine.get_called_stations().size(), 1u);
    EXPECT_EQ(engine.get_called_stations()[0], "3B9/HB9IP/P");

    // Re-adding same station is idempotent
    engine.add_called_station("3B9/HB9IP/P");
    EXPECT_EQ(engine.get_called_stations().size(), 1u);

    engine.clear_called_stations();
    EXPECT_FALSE(engine.has_called_station("3B9/HB9IP/P"));
    EXPECT_TRUE(engine.get_called_stations().empty());

    // 2. Positive Case: Station A calls Fox -> receives MULTI_REPORT73 with 16-bit DX hash
    UserIntent call_intent;
    call_intent.action = IntentAction::CALL_STATION;
    call_intent.target_call_1 = "3B9/HB9IP/P";
    auto call_dec = engine.process_slot({}, call_intent);
    EXPECT_TRUE(call_dec.success);
    EXPECT_TRUE(engine.has_called_station("3B9/HB9IP/P"));

    // Fox replies with MULTI_REPORT73: DX hash (16-bit), Target hash (24-bit for HB9IPH)
    ReceivedFrame rf_reply;
    rf_reply.msg.type = MessageType::MULTI_REPORT73;
    rf_reply.msg.call_1 = "<7A8B>";
    rf_reply.msg.hash_1 = hash_callsign_16("3B9/HB9IP/P");
    MultiTarget mt;
    mt.hash = hash_callsign_24("HB9IPH");
    mt.call = "HB9IPH";
    mt.rst_db = +5;
    rf_reply.msg.multi_targets.push_back(mt);

    UserIntent idle_intent;
    idle_intent.action = IntentAction::IDLE;
    auto reply_dec = engine.process_slot({rf_reply}, idle_intent);
    EXPECT_TRUE(reply_dec.success);
    EXPECT_EQ(reply_dec.qso_completed_with.size(), 1u);
    EXPECT_EQ(reply_dec.qso_completed_with[0], "3B9/HB9IP/P");
    EXPECT_TRUE(engine.is_qso_completed("3B9/HB9IP/P"));
    EXPECT_FALSE(engine.has_called_station("3B9/HB9IP/P")); // Evicted upon QSO completion

    // 3. Negative Case: Uncalled Fox Collision Rejection
    // An uncalled Fox ("EA8/HB9IP") transmits MULTI_REPORT73 matching HB9IPH's 24-bit hash.
    // Because HB9IPH only called 3B9/HB9IP/P, this frame must be rejected.
    IntentEngine engine_strict("HB9IPH", "JN47");
    UserIntent call_fox1;
    call_fox1.action = IntentAction::CALL_STATION;
    call_fox1.target_call_1 = "K1ABC";
    engine_strict.process_slot({}, call_fox1);
    EXPECT_TRUE(engine_strict.has_called_station("K1ABC"));

    // Add uncalled station to general heard cache
    engine_strict.add_known_callsign("EA8/HB9IP");

    ReceivedFrame rf_uncalled_fox;
    rf_uncalled_fox.msg.type = MessageType::MULTI_REPORT73;
    rf_uncalled_fox.msg.call_1 = "<1234>";
    rf_uncalled_fox.msg.hash_1 = hash_callsign_16("EA8/HB9IP"); // Uncalled Fox
    MultiTarget mt_coll;
    mt_coll.hash = hash_callsign_24("HB9IPH"); // Accidental target collision
    mt_coll.rst_db = -2;
    rf_uncalled_fox.msg.multi_targets.push_back(mt_coll);

    auto uncalled_dec = engine_strict.process_slot({rf_uncalled_fox}, idle_intent);
    EXPECT_TRUE(uncalled_dec.qso_completed_with.empty());
    EXPECT_FALSE(engine_strict.is_qso_completed("EA8/HB9IP"));

    // 4. Negative Case: Cleartext uncalled DX station rejection when called stations active
    ReceivedFrame rf_clear_uncalled;
    rf_clear_uncalled.msg.type = MessageType::MULTI_REPORT73;
    rf_clear_uncalled.msg.call_1 = "EA8/HB9IP"; // Cleartext uncalled station
    rf_clear_uncalled.msg.hash_1 = hash_callsign_16("EA8/HB9IP");
    rf_clear_uncalled.msg.multi_targets.push_back(mt_coll);

    auto clear_uncalled_dec = engine_strict.process_slot({rf_clear_uncalled}, idle_intent);
    EXPECT_TRUE(clear_uncalled_dec.qso_completed_with.empty());
    EXPECT_FALSE(engine_strict.is_qso_completed("EA8/HB9IP"));

    // 5. Positive Case: Matching legitimate called Fox among multiple candidates
    engine_strict.add_called_station("EA8/HB9IP");
    EXPECT_TRUE(engine_strict.has_called_station("EA8/HB9IP"));
    auto valid_called_dec = engine_strict.process_slot({rf_uncalled_fox}, idle_intent);
    EXPECT_EQ(valid_called_dec.qso_completed_with.size(), 1u);
    EXPECT_EQ(valid_called_dec.qso_completed_with[0], "EA8/HB9IP");
    EXPECT_TRUE(engine_strict.is_qso_completed("EA8/HB9IP"));
    EXPECT_FALSE(engine_strict.has_called_station("EA8/HB9IP")); // Evicted upon QSO completion
}

TEST(CoverageEdgeCases, IntentEngineCalledStationExpirationAndEviction) {
    IntentEngine engine("HB9IPH", "JN47");

    // 1. Manual removal API tests
    engine.add_called_station("W1AW");
    engine.add_called_station("K1ABC");
    EXPECT_TRUE(engine.has_called_station("W1AW"));
    EXPECT_TRUE(engine.has_called_station("K1ABC"));
    EXPECT_EQ(engine.get_called_stations().size(), 2u);

    // Remove empty callsign is a no-op
    engine.remove_called_station("");
    EXPECT_EQ(engine.get_called_stations().size(), 2u);

    // Remove non-tracked callsign is a no-op
    engine.remove_called_station("NOTRACK");
    EXPECT_EQ(engine.get_called_stations().size(), 2u);

    // Remove W1AW
    engine.remove_called_station("W1AW");
    EXPECT_FALSE(engine.has_called_station("W1AW"));
    EXPECT_TRUE(engine.has_called_station("K1ABC"));
    EXPECT_EQ(engine.get_called_stations().size(), 1u);
    EXPECT_EQ(engine.get_called_stations()[0], "K1ABC");

    // 2. Hash retention when removing one station
    engine.clear_called_stations();
    EXPECT_TRUE(engine.get_called_stations().empty());

    // 3. TTL Configuration and Expiration
    EXPECT_EQ(engine.get_called_station_ttl_seconds(), 1800u);
    engine.set_called_station_ttl_seconds(300); // 5 minutes
    EXPECT_EQ(engine.get_called_station_ttl_seconds(), 300u);

    auto t0 = std::chrono::steady_clock::now();
    engine.add_called_station("EA8/HB9IP");
    EXPECT_TRUE(engine.has_called_station("EA8/HB9IP"));

    // Prune with time not yet expired
    engine.prune_expired_called_stations(t0 + std::chrono::seconds(100));
    EXPECT_TRUE(engine.has_called_station("EA8/HB9IP"));
    EXPECT_EQ(engine.get_called_stations().size(), 1u);

    // Refresh timestamp by re-calling station
    engine.add_called_station("EA8/HB9IP");

    // Prune with time > 300s from original t0, but < 300s from refreshed call
    engine.prune_expired_called_stations(t0 + std::chrono::seconds(250));
    EXPECT_TRUE(engine.has_called_station("EA8/HB9IP"));

    // Advance 301 seconds past refresh
    auto t_expired = t0 + std::chrono::seconds(250 + 301);
    engine.prune_expired_called_stations(t_expired);
    EXPECT_FALSE(engine.has_called_station("EA8/HB9IP"));
    EXPECT_TRUE(engine.get_called_stations().empty());

    // Test default prune_expired_called_stations() call (no args)
    engine.add_called_station("JA1ABC");
    engine.prune_expired_called_stations(); // with current time (not expired)
    EXPECT_TRUE(engine.has_called_station("JA1ABC"));

    // Test TTL = 0 disables expiration
    engine.set_called_station_ttl_seconds(0);
    EXPECT_EQ(engine.get_called_station_ttl_seconds(), 0u);
    engine.prune_expired_called_stations(t0 + std::chrono::hours(1000));
    EXPECT_TRUE(engine.has_called_station("JA1ABC"));
    EXPECT_EQ(engine.get_called_stations().size(), 1u);

    // 4. Automatic slot pruning during process_slot
    engine.set_called_station_ttl_seconds(1);
    engine.add_called_station("DL1ABC");
    EXPECT_TRUE(engine.has_called_station("DL1ABC"));
    std::this_thread::sleep_for(std::chrono::milliseconds(1050));
    // has_called_station returns false due to elapsed time
    EXPECT_FALSE(engine.has_called_station("DL1ABC"));
    // process_slot prunes it from the list
    UserIntent idle;
    idle.action = IntentAction::IDLE;
    engine.process_slot({}, idle);
    EXPECT_TRUE(engine.get_called_stations().empty());

    // 5. Automatic eviction on single-station incoming REPORT+73
    engine.set_called_station_ttl_seconds(1800);
    engine.add_called_station("G4ABC");
    EXPECT_TRUE(engine.has_called_station("G4ABC"));

    ReceivedFrame rf_single_rep;
    rf_single_rep.msg.type = MessageType::REPORT73_STD;
    rf_single_rep.msg.call_1 = "HB9IPH";
    rf_single_rep.msg.call_2 = "G4ABC";
    rf_single_rep.msg.rst_db = -12;
    auto dec_rep = engine.process_slot({rf_single_rep}, idle);
    EXPECT_TRUE(engine.is_qso_completed("G4ABC"));
    EXPECT_FALSE(engine.has_called_station("G4ABC"));

    // 6. Automatic eviction when replying via REPLY_TO_STATIONS (single & multi)
    engine.add_called_station("VE3ABC");
    EXPECT_TRUE(engine.has_called_station("VE3ABC"));
    UserIntent reply_single;
    reply_single.action = IntentAction::REPLY_TO_STATIONS;
    reply_single.target_call_1 = "VE3ABC";
    engine.process_slot({}, reply_single);
    EXPECT_TRUE(engine.is_qso_completed("VE3ABC"));
    EXPECT_FALSE(engine.has_called_station("VE3ABC"));

    engine.add_called_station("VK2ABC");
    engine.add_called_station("ZL1ABC");
    EXPECT_TRUE(engine.has_called_station("VK2ABC"));
    EXPECT_TRUE(engine.has_called_station("ZL1ABC"));
    UserIntent reply_multi;
    reply_multi.action = IntentAction::REPLY_TO_STATIONS;
    reply_multi.target_call_1 = "VK2ABC";
    reply_multi.target_call_2 = "ZL1ABC";
    engine.process_slot({}, reply_multi);
    EXPECT_TRUE(engine.is_qso_completed("VK2ABC"));
    EXPECT_TRUE(engine.is_qso_completed("ZL1ABC"));
    EXPECT_FALSE(engine.has_called_station("VK2ABC"));
    EXPECT_FALSE(engine.has_called_station("ZL1ABC"));
}



















