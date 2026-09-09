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
#include "lq/c_api.h"
#include <cstring>
#include <vector>
#include <cmath>

TEST(CApiTest, VersionInformation) {
    EXPECT_STREQ(lq_c_version_string(), "1.0.0");
    EXPECT_EQ(lq_c_version_major(), 1);
    EXPECT_EQ(lq_c_version_minor(), 0);
    EXPECT_EQ(lq_c_version_patch(), 0);
}

TEST(CApiTest, ModeParameters) {
    lq_c_mode_params_t p;
    EXPECT_EQ(lq_c_get_mode_params(LQ_MODE_LQ8, &p), 1);
    EXPECT_STREQ(p.name, "LQ8");
    EXPECT_EQ(p.total_symbols, 79);
    EXPECT_EQ(p.num_tones, 8);
    EXPECT_FLOAT_EQ(p.bandwidth_hz, 50.0f);
    EXPECT_FLOAT_EQ(p.slot_duration_sec, 15.0f);

    EXPECT_EQ(lq_c_get_mode_params(LQ_MODE_LQ4, &p), 1);
    EXPECT_STREQ(p.name, "LQ4");
    EXPECT_EQ(p.total_symbols, 105);
    EXPECT_EQ(p.num_tones, 4);
    EXPECT_FLOAT_EQ(p.slot_duration_sec, 7.5f);

    EXPECT_EQ(lq_c_get_mode_params(LQ_MODE_LQ2, &p), 1);
    EXPECT_STREQ(p.name, "LQ2");
    EXPECT_EQ(p.total_symbols, 105);
    EXPECT_FLOAT_EQ(p.slot_duration_sec, 3.75f);

    EXPECT_EQ(lq_c_get_mode_params(LQ_MODE_LQ16, &p), 1);
    EXPECT_STREQ(p.name, "LQ16");
    EXPECT_EQ(p.total_symbols, 79);
    EXPECT_FLOAT_EQ(p.slot_duration_sec, 30.0f);

    // Null safety
    EXPECT_EQ(lq_c_get_mode_params(LQ_MODE_LQ8, nullptr), 0);
}

TEST(CApiTest, CallsignHashing) {
    uint32_t h24 = lq_c_hash_callsign_24("HB9IPH");
    EXPECT_NE(h24, 0U);
    EXPECT_EQ(h24, lq_c_hash_callsign_24("  hb9iph  "));

    uint32_t h23 = lq_c_hash_callsign_23("HB9IPH");
    EXPECT_EQ(h23, h24 & 0x7FFFFFU);
    EXPECT_EQ(h23, lq_c_hash_callsign_23("  hb9iph  "));

    uint32_t h20 = lq_c_hash_callsign_20("HB9IPH");
    EXPECT_EQ(h20, h24 >> 4);
    EXPECT_EQ(h20, lq_c_hash_callsign_20("  hb9iph  "));

    uint8_t payload[10];
    uint32_t h22 = lq_c_hash_callsign_22("HB9IPH");
    uint32_t h14 = lq_c_hash_callsign_14("HB9IPH");
    uint32_t h12 = lq_c_hash_callsign_12("HB9IPH");
    uint32_t h10 = lq_c_hash_callsign_10("HB9IPH");
    EXPECT_NE(h22, 0U);
    EXPECT_NE(h14, 0U);
    EXPECT_NE(h12, 0U);
    EXPECT_NE(h10, 0U);

    // Null safety
    EXPECT_EQ(lq_c_hash_callsign_24(nullptr), 0U);
    EXPECT_EQ(lq_c_hash_callsign_20(nullptr), 0U);
    EXPECT_EQ(lq_c_hash_callsign_23(nullptr), 0U);
    EXPECT_EQ(lq_c_hash_callsign_22(nullptr), 0U);
    EXPECT_EQ(lq_c_hash_callsign_14(nullptr), 0U);
    EXPECT_EQ(lq_c_hash_callsign_12(nullptr), 0U);
    EXPECT_EQ(lq_c_hash_callsign_10(nullptr), 0U);
}

TEST(CApiTest, ParseAndFormatMessages) {
    lq_c_message_t msg;

    // CQ message
    EXPECT_EQ(lq_c_parse_message("CQ DX HB9IPH JN47", &msg), 1);
    EXPECT_EQ(msg.is_valid, 1);
    EXPECT_EQ(msg.is_cq, 1);
    EXPECT_EQ(msg.is_call, 0);
    EXPECT_EQ(msg.is_reply73, 0);
    EXPECT_STREQ(msg.call_from, "HB9IPH");
    EXPECT_STREQ(msg.modifier, "DX");
    EXPECT_STREQ(msg.grid, "JN47");

    char formatted[128];
    EXPECT_EQ(lq_c_format_message(&msg, formatted, sizeof(formatted)), 1);
    EXPECT_STREQ(formatted, "CQ DX HB9IPH JN47");

    // CALL message
    EXPECT_EQ(lq_c_parse_message("CALL YO1YO TU2TU KL22 -03", &msg), 1);
    EXPECT_EQ(msg.is_valid, 1);
    EXPECT_EQ(msg.is_cq, 0);
    EXPECT_EQ(msg.is_call, 1);
    EXPECT_EQ(msg.is_reply73, 0);
    EXPECT_STREQ(msg.call_to, "YO1YO");
    EXPECT_STREQ(msg.call_from, "TU2TU");
    EXPECT_STREQ(msg.grid, "KL22");
    EXPECT_EQ(msg.rst_db, -3);

    EXPECT_EQ(lq_c_format_message(&msg, formatted, sizeof(formatted)), 1);
    EXPECT_STREQ(formatted, "YO1YO TU2TU KL22 -03");

    // MULTI-REPORT+73 message
    EXPECT_EQ(lq_c_parse_message("MULTI-REPORT+73 HB9IPH YO1YO +05 TU2TU -03", &msg), 1);
    EXPECT_EQ(msg.is_valid, 1);
    EXPECT_EQ(msg.is_multi_report73, 1);
    EXPECT_STREQ(msg.call_from, "HB9IPH");
    EXPECT_EQ(msg.num_multi_targets, 2);
    EXPECT_STREQ(msg.multi_targets[0].call, "YO1YO");
    EXPECT_EQ(msg.multi_targets[0].rst_db, 5);
    EXPECT_STREQ(msg.multi_targets[1].call, "TU2TU");
    EXPECT_EQ(msg.multi_targets[1].rst_db, -3);

    EXPECT_EQ(lq_c_format_message(&msg, formatted, sizeof(formatted)), 1);
    EXPECT_STREQ(formatted, "<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>");

    // Compact QSO format
    EXPECT_EQ(lq_c_parse_message("YO1YO TU2TU KL22 -03", &msg), 1);
    EXPECT_EQ(msg.is_call, 1);
    EXPECT_STREQ(msg.call_to, "YO1YO");
    EXPECT_STREQ(msg.call_from, "TU2TU");

    // Compact 73 format
    EXPECT_EQ(lq_c_parse_message("YO1YO TU2TU 73", &msg), 1);
    EXPECT_EQ(msg.is_73, 1);
    EXPECT_STREQ(msg.call_to, "YO1YO");
    EXPECT_STREQ(msg.call_from, "TU2TU");

    // Null safety & unparseable empty string
    EXPECT_EQ(lq_c_parse_message(nullptr, &msg), 0);
    EXPECT_EQ(lq_c_parse_message("", &msg), 0);
    EXPECT_EQ(lq_c_parse_message("CQ", nullptr), 0);
    EXPECT_EQ(lq_c_format_message(nullptr, formatted, sizeof(formatted)), 0);
    EXPECT_EQ(lq_c_format_message(&msg, nullptr, 0), 0);
}

TEST(CApiTest, MessagePayloadEncodeDecode) {
    lq_c_message_t orig_msg;
    EXPECT_EQ(lq_c_parse_message("CALL YO1YO HB9IPH JN47 -03", &orig_msg), 1);

    uint8_t payload[10];
    EXPECT_EQ(lq_c_encode_message(&orig_msg, payload), 1);

    lq_c_message_t dec_msg;
    EXPECT_EQ(lq_c_decode_message(payload, &dec_msg), 1);
    EXPECT_EQ(dec_msg.is_valid, 1);
    EXPECT_EQ(dec_msg.is_call, 1);
    EXPECT_STREQ(dec_msg.call_to, "YO1YO");
    EXPECT_STREQ(dec_msg.call_from, "HB9IPH");
    EXPECT_STREQ(dec_msg.grid, "JN47");
    EXPECT_EQ(dec_msg.rst_db, -3);

    // Null safety
    EXPECT_EQ(lq_c_encode_message(nullptr, payload), 0);
    EXPECT_EQ(lq_c_encode_message(&orig_msg, nullptr), 0);
    EXPECT_EQ(lq_c_decode_message(nullptr, &dec_msg), 0);
    EXPECT_EQ(lq_c_decode_message(payload, nullptr), 0);
}

TEST(CApiTest, ToneEncodeDecodeAcrossAllModes) {
    lq_c_message_t orig_msg;
    EXPECT_EQ(lq_c_parse_message("CALL YO1YO HB9IPH JN47 -03", &orig_msg), 1);

    // LQ8
    uint8_t tones_lq8[79];
    EXPECT_EQ(lq_c_encode_tones(&orig_msg, tones_lq8, LQ_MODE_LQ8), 1);
    lq_c_message_t dec_lq8;
    EXPECT_EQ(lq_c_decode_tones(tones_lq8, &dec_lq8, LQ_MODE_LQ8), 1);
    EXPECT_STREQ(dec_lq8.call_to, "YO1YO");
    EXPECT_STREQ(dec_lq8.call_from, "HB9IPH");

    // LQ4
    uint8_t tones_lq4[105];
    EXPECT_EQ(lq_c_encode_tones(&orig_msg, tones_lq4, LQ_MODE_LQ4), 1);
    lq_c_message_t dec_lq4;
    EXPECT_EQ(lq_c_decode_tones(tones_lq4, &dec_lq4, LQ_MODE_LQ4), 1);
    EXPECT_STREQ(dec_lq4.call_to, "YO1YO");
    EXPECT_STREQ(dec_lq4.call_from, "HB9IPH");

    // LQ2
    uint8_t tones_lq2[105];
    EXPECT_EQ(lq_c_encode_tones(&orig_msg, tones_lq2, LQ_MODE_LQ2), 1);
    lq_c_message_t dec_lq2;
    EXPECT_EQ(lq_c_decode_tones(tones_lq2, &dec_lq2, LQ_MODE_LQ2), 1);
    EXPECT_STREQ(dec_lq2.call_to, "YO1YO");
    EXPECT_STREQ(dec_lq2.call_from, "HB9IPH");

    // LQ16
    uint8_t tones_lq16[79];
    EXPECT_EQ(lq_c_encode_tones(&orig_msg, tones_lq16, LQ_MODE_LQ16), 1);
    lq_c_message_t dec_lq16;
    EXPECT_EQ(lq_c_decode_tones(tones_lq16, &dec_lq16, LQ_MODE_LQ16), 1);
    EXPECT_STREQ(dec_lq16.call_to, "YO1YO");
    EXPECT_STREQ(dec_lq16.call_from, "HB9IPH");

    // Direct payload encode/decode
    uint8_t payload[10];
    lq_c_encode_message(&orig_msg, payload);
    uint8_t tones_buf[105];
    EXPECT_EQ(lq_c_encode_payload(payload, tones_buf, LQ_MODE_LQ4), 1);
    uint8_t dec_payload[10];
    EXPECT_EQ(lq_c_decode_payload(tones_buf, dec_payload, LQ_MODE_LQ4), 1);
    EXPECT_EQ(std::memcmp(payload, dec_payload, 10), 0);
}

TEST(CApiTest, AudioAndGfskSynthesis) {
    uint8_t symbols[4] = {0, 1, 3, 2};
    int n_spsym = static_cast<int>(std::round(0.048f * 12000.0f));
    std::vector<float> signal(4 * n_spsym, 0.0f);
    lq_c_synth_gfsk(symbols, 4, 1000.0f, 1.0f, 0.048f, 12000.0f, signal.data());

    float max_val = 0.0f;
    for (float s : signal) {
        max_val = std::max(max_val, std::abs(s));
    }
    EXPECT_GT(max_val, 0.1f);
    EXPECT_LE(max_val, 1.0f);

    // lq_c_generate_audio
    lq_c_message_t msg;
    lq_c_parse_message("CQ HB9IPH JN47", &msg);
    uint8_t tones[79];
    lq_c_encode_tones(&msg, tones, LQ_MODE_LQ8);

    std::vector<float> audio_buf(79 * 1920);
    int count = lq_c_generate_audio(tones, 79, 1000.0f, 12000.0f, LQ_MODE_LQ8, audio_buf.data(), static_cast<int>(audio_buf.size()));
    EXPECT_EQ(count, 79 * 1920);

    // Test lq_c_audio_to_message and lq_c_audio_to_messages across thread counts
    for (int t : {1, 2, 4, 16}) {
        lq_c_message_t dec_msg;
        EXPECT_EQ(lq_c_audio_to_message(audio_buf.data(), audio_buf.size(), 1000.0f, 12000.0f, LQ_MODE_LQ8, &dec_msg, t), 1);
        EXPECT_STREQ(dec_msg.call_from, "HB9IPH");
        EXPECT_STREQ(dec_msg.grid, "JN47");

        lq_c_message_t dec_list[4];
        int n_dec = lq_c_audio_to_messages(audio_buf.data(), audio_buf.size(), 1000.0f, 12000.0f, LQ_MODE_LQ8, dec_list, 4, t);
        EXPECT_EQ(n_dec, 1);
        EXPECT_STREQ(dec_list[0].call_from, "HB9IPH");
    }
}

TEST(CApiTest, ComprehensiveCoverageAndEdgeCases) {
    // Mode conversions
    lq_c_mode_params_t p;
    EXPECT_EQ(lq_c_get_mode_params(LQ_MODE_LQ8, &p), 1);
    EXPECT_EQ(lq_c_get_mode_params(LQ_MODE_LQ4, &p), 1);
    EXPECT_EQ(lq_c_get_mode_params(LQ_MODE_LQ2, &p), 1);
    EXPECT_EQ(lq_c_get_mode_params(LQ_MODE_LQ16, &p), 1);
    EXPECT_EQ(lq_c_get_mode_params(static_cast<lq_mode_t>(999), &p), 1);

    // Null and error branches
    EXPECT_EQ(lq_c_format_message(nullptr, nullptr, 0), 0);
    lq_c_message_t msg;
    EXPECT_EQ(lq_c_format_message(&msg, nullptr, 0), 0);

    // Corrupted payload decode
    uint8_t bad_payload[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    EXPECT_EQ(lq_c_decode_message(bad_payload, &msg), 0);

    // Invalid tones decode
    uint8_t bad_tones[79] = {99};
    EXPECT_EQ(lq_c_decode_tones(bad_tones, &msg, LQ_MODE_LQ8), 0);

    // Audio synthesis null safety
    EXPECT_EQ(lq_c_generate_audio(nullptr, 0, 0, 0, LQ_MODE_LQ8, nullptr, 0), 0);

    // Invalid message parse error path (null arguments)
    EXPECT_EQ(lq_c_parse_message(nullptr, &msg), 0);
    EXPECT_EQ(lq_c_parse_message("CQ HB9IPH JN47", nullptr), 0);
}

TEST(CApiTest, WaterfallFunctions) {
    const int num_blocks = 160;
    const int block_stride = 128;
    std::vector<uint8_t> mag(num_blocks * block_stride, 10);
    std::vector<float> mag2(num_blocks * block_stride, 100.0f);

    // Sync score
    int score = lq_c_waterfall_sync_score(mag.data(), num_blocks, block_stride, 0, 20, LQ_MODE_LQ8);
    EXPECT_GE(score, 0);

    // Refine frequency
    int out_score = 0;
    float refined_freq = lq_c_waterfall_refine_frequency(mag.data(), num_blocks, block_stride, 0, 20, 0, 2, 0.160f, LQ_MODE_LQ8, &out_score);
    EXPECT_GE(refined_freq, 0.0f);

    // Guess SNR
    float snr = lq_c_waterfall_guess_snr(mag2.data(), num_blocks, block_stride, 0, 20, LQ_MODE_LQ8);
    EXPECT_TRUE(std::isfinite(snr));

    // Extract LLRs
    float llrs[174] = {0.0f};
    int extracted = lq_c_waterfall_extract_llrs(mag.data(), num_blocks, block_stride, 0, 20, LQ_MODE_LQ8, llrs);
    EXPECT_EQ(extracted, 1);

    // Subtract signal
    uint8_t payload[10] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34};
    lq_c_waterfall_subtract_signal(mag.data(), static_cast<int>(mag.size()), 128, block_stride, payload, 1000.0f, 0.0f, 12000.0f, LQ_MODE_LQ8);
}


TEST(CApiTest, KnownCallsignsHashResolution) {
    lq_c_message_t orig_msg;
    std::memset(&orig_msg, 0, sizeof(orig_msg));
    orig_msg.type = LQ_MSG_MULTI_REPLY73;
    orig_msg.hash_from_16 = lq_c_hash_callsign_16("HB9IPH");
    orig_msg.num_multi_targets = 2;
    orig_msg.multi_targets[0].hash_24 = lq_c_hash_callsign_24("YO1YO");
    orig_msg.multi_targets[0].rst_db = 5;
    orig_msg.multi_targets[1].hash_24 = lq_c_hash_callsign_24("TU2TU");
    orig_msg.multi_targets[1].rst_db = -3;

    uint8_t payload[10];
    EXPECT_EQ(lq_c_encode_message(&orig_msg, payload), 1);

    const char* known_calls[] = { "W1AW", "HB9IPH", "YO1YO", "TU2TU", "DL1ABC" };
    lq_c_message_t dec_msg;
    EXPECT_EQ(lq_c_decode_message_with_known_calls(payload, &dec_msg, known_calls, 5), 1);
    EXPECT_EQ(dec_msg.type, LQ_MSG_MULTI_REPLY73);
    EXPECT_STREQ(dec_msg.call_from, "HB9IPH");
    EXPECT_EQ(dec_msg.num_multi_targets, 2);
    EXPECT_STREQ(dec_msg.multi_targets[0].call, "YO1YO");
    EXPECT_EQ(dec_msg.multi_targets[0].rst_db, 5);
    EXPECT_STREQ(dec_msg.multi_targets[1].call, "TU2TU");
    EXPECT_EQ(dec_msg.multi_targets[1].rst_db, -3);

    // Also test null safety & resolve callsigns directly
    EXPECT_EQ(lq_c_resolve_callsigns(nullptr, known_calls, 5), 0);
    EXPECT_EQ(lq_c_resolve_callsigns(&dec_msg, nullptr, 0), 0);
}

TEST(CApiTest, MessageToTextAndPayloadToText) {
    lq_c_message_t msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.type = LQ_MSG_CQ_STD;
    std::strncpy(msg.call_from, "HB9IPH", sizeof(msg.call_from) - 1);
    std::strncpy(msg.grid, "JN47", sizeof(msg.grid) - 1);
    std::strncpy(msg.modifier, "DX", sizeof(msg.modifier) - 1);

    char out_str[128] = {0};
    EXPECT_EQ(lq_c_message_to_text(&msg, out_str, sizeof(out_str)), 1);
    EXPECT_STREQ(out_str, "CQ DX HB9IPH JN47");

    uint8_t payload[10] = {0};
    EXPECT_EQ(lq_c_encode_message(&msg, payload), 1);

    char payload_str[128] = {0};
    EXPECT_EQ(lq_c_payload_to_text(payload, payload_str, sizeof(payload_str), nullptr, 0), 1);
    // Known callsigns resolution test
    const char* known[] = { "HB9IPH", "YO1YO", nullptr };
    EXPECT_EQ(lq_c_payload_to_text(payload, payload_str, sizeof(payload_str), known, 3), 1);
    EXPECT_STREQ(payload_str, "CQ DX HB9IPH JN47");

    // Invalid payload test returning 0
    uint8_t invalid_payload[10];
    std::memset(invalid_payload, 0xFF, sizeof(invalid_payload));
    char invalid_out[128] = "dummy";
    EXPECT_EQ(lq_c_payload_to_text(invalid_payload, invalid_out, sizeof(invalid_out), nullptr, 0), 0);
    EXPECT_STREQ(invalid_out, "");

    // Null and boundary checks
    EXPECT_EQ(lq_c_message_to_text(nullptr, out_str, sizeof(out_str)), 0);
    EXPECT_EQ(lq_c_message_to_text(&msg, nullptr, sizeof(out_str)), 0);
    EXPECT_EQ(lq_c_message_to_text(&msg, out_str, 0), 0);
    EXPECT_EQ(lq_c_payload_to_text(nullptr, payload_str, sizeof(payload_str), nullptr, 0), 0);
    EXPECT_EQ(lq_c_payload_to_text(payload, nullptr, sizeof(payload_str), nullptr, 0), 0);
    EXPECT_EQ(lq_c_payload_to_text(payload, payload_str, 0, nullptr, 0), 0);
}

TEST(CApiTest, HashFieldsAndFallbacks) {
    lq_c_message_t msg;
    std::memset(&msg, 0, sizeof(msg));
    EXPECT_EQ(lq_c_parse_message("CALL YO1YO HB9IPH JN47 -03", &msg), 1);
    EXPECT_NE(msg.hash_from_24, 0U);
    EXPECT_NE(msg.hash_to_24, 0U);
    EXPECT_EQ(msg.hash_from_20, msg.hash_from_24 >> 4);
    EXPECT_EQ(msg.hash_to_20, msg.hash_to_24 >> 4);

    // Test encoding using 20-bit target hash fallback when 24-bit hash is 0
    lq_c_message_t nonstd_call;
    std::memset(&nonstd_call, 0, sizeof(nonstd_call));
    nonstd_call.type = LQ_MSG_CALL_NONSTD;
    std::strncpy(nonstd_call.call_from, "EA6/HB9IP", sizeof(nonstd_call.call_from));
    nonstd_call.hash_to_20 = 0x12345U;
    nonstd_call.rst_db = -10;
    uint8_t payload[10] = {0};
    EXPECT_EQ(lq_c_encode_message(&nonstd_call, payload), 1);

    lq_c_message_t dec_msg;
    EXPECT_EQ(lq_c_decode_message(payload, &dec_msg), 1);
    EXPECT_EQ(dec_msg.type, LQ_MSG_CALL_NONSTD);
}

TEST(CApiTest, AudioToMessagesExtFastAndDeep) {
    uint8_t payload[10];
    lq_c_message_t msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.type = LQ_MSG_CQ_STD;
    std::strncpy(msg.call_from, "HB9IPH", sizeof(msg.call_from));
    std::strncpy(msg.grid, "JN47", sizeof(msg.grid));
    ASSERT_EQ(lq_c_encode_message(&msg, payload), 1);

    uint8_t tones[79];
    ASSERT_EQ(lq_c_encode_payload(payload, tones, LQ_MODE_LQ8), 1);

    std::vector<float> audio(79 * 1920);
    int gen_samples = lq_c_generate_audio(tones, 79, 1200.0f, 12000.0f, LQ_MODE_LQ8, audio.data(), static_cast<int>(audio.size()));
    ASSERT_EQ(gen_samples, 79 * 1920);

    // Fast mode via ext API (is_deep = 0)
    lq_c_message_t msgs_fast[4];
    int n_fast = lq_c_audio_to_messages_ext(audio.data(), static_cast<int>(audio.size()), 1200.0f, 12000.0f, LQ_MODE_LQ8, msgs_fast, 4, 1, 0);
    ASSERT_EQ(n_fast, 1);
    EXPECT_STREQ(msgs_fast[0].call_from, "HB9IPH");
    EXPECT_STREQ(msgs_fast[0].grid, "JN47");

    // Deep mode via ext API (is_deep = 1)
    lq_c_message_t msgs_deep[4];
    int n_deep = lq_c_audio_to_messages_ext(audio.data(), static_cast<int>(audio.size()), 1200.0f, 12000.0f, LQ_MODE_LQ8, msgs_deep, 4, 2, 1);
    ASSERT_EQ(n_deep, 1);
    EXPECT_STREQ(msgs_deep[0].call_from, "HB9IPH");
    EXPECT_STREQ(msgs_deep[0].grid, "JN47");

    // Equivalence with legacy wrapper lq_c_audio_to_messages
    lq_c_message_t msgs_legacy[4];
    int n_legacy = lq_c_audio_to_messages(audio.data(), audio.size(), 1200.0f, 12000.0f, LQ_MODE_LQ8, msgs_legacy, 4, 1);
    EXPECT_EQ(n_fast, n_legacy);
    EXPECT_STREQ(msgs_fast[0].call_from, msgs_legacy[0].call_from);

    // Defensive boundary validation for lq_c_audio_to_messages_ext
    EXPECT_EQ(lq_c_audio_to_messages_ext(nullptr, static_cast<int>(audio.size()), 1200.0f, 12000.0f, LQ_MODE_LQ8, msgs_fast, 4, 1, 0), 0);
    EXPECT_EQ(lq_c_audio_to_messages_ext(audio.data(), 0, 1200.0f, 12000.0f, LQ_MODE_LQ8, msgs_fast, 4, 1, 0), 0);
    EXPECT_EQ(lq_c_audio_to_messages_ext(audio.data(), -1, 1200.0f, 12000.0f, LQ_MODE_LQ8, msgs_fast, 4, 1, 0), 0);
    EXPECT_EQ(lq_c_audio_to_messages_ext(audio.data(), static_cast<int>(audio.size()), 1200.0f, 12000.0f, LQ_MODE_LQ8, nullptr, 4, 1, 0), 0);
    EXPECT_EQ(lq_c_audio_to_messages_ext(audio.data(), static_cast<int>(audio.size()), 1200.0f, 12000.0f, LQ_MODE_LQ8, msgs_fast, 0, 1, 0), 0);
    EXPECT_EQ(lq_c_audio_to_messages_ext(audio.data(), static_cast<int>(audio.size()), 1200.0f, 12000.0f, LQ_MODE_LQ8, msgs_fast, -1, 1, 0), 0);
}


