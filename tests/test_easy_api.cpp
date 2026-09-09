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
#include <sstream>
#include "lq/lq.h"

using namespace lq;

TEST(EasyApiTest, FactoryFunctions) {
    // 1. make_cq standard
    Message msg1 = make_cq("HB9IPH", "JN47", "DX");
    EXPECT_EQ(msg1.type, MessageType::CQ_STD);
    EXPECT_EQ(msg1.call_1, "HB9IPH");
    EXPECT_EQ(msg1.locator, "JN47");
    EXPECT_EQ(msg1.modifier, "DX");

    // 2. make_cq non-standard
    Message msg2 = make_cq("3B9/HB9IPH/P", "IH23");
    EXPECT_EQ(msg2.type, MessageType::CQ_NONSTD_3);
    EXPECT_EQ(msg2.call_1, "3B9/HB9IPH/P");

    // 3. make_call standard & non-standard
    Message msg3 = make_call("HB9IPH", "W1AW", "FN31", -15);
    EXPECT_EQ(msg3.type, MessageType::CALL_STD_NOSUF);
    EXPECT_EQ(msg3.call_1, "HB9IPH");
    EXPECT_EQ(msg3.call_2, "W1AW");
    EXPECT_EQ(msg3.locator, "FN31");
    EXPECT_EQ(msg3.rst_db, -15);

    // Target non-standard, caller standard -> CALL_STD_SUF (delivers locator + SNR)
    Message msg3b = make_call("3B9/HB9IPH/P", "W1AW", "FN31", -10);
    EXPECT_EQ(msg3b.type, MessageType::CALL_STD_SUF);
    EXPECT_EQ(msg3b.hash_1, hash_callsign_24("3B9/HB9IPH/P"));
    EXPECT_EQ(msg3b.call_2, "W1AW");
    EXPECT_EQ(msg3b.locator, "FN31");
    EXPECT_EQ(msg3b.rst_db, -10);

    // Target standard, caller standard with suffix -> CALL_STD_SUF
    Message msg3c = make_call("W1AW", "HB9IPH/P", "FN31", -10);
    EXPECT_EQ(msg3c.type, MessageType::CALL_STD_SUF);
    EXPECT_EQ(msg3c.call_1, "W1AW");
    EXPECT_EQ(msg3c.call_2, "HB9IPH/P");
    EXPECT_EQ(msg3c.suffix_2, 1);
    EXPECT_EQ(msg3c.locator, "FN31");
    EXPECT_EQ(msg3c.rst_db, -10);

    // Both non-standard -> CALL_NONSTD
    Message msg3d = make_call("EA8/YO1YO", "3B9/HB9IPH/P", "FN31", -10);
    EXPECT_EQ(msg3d.type, MessageType::CALL_NONSTD);

    // 4. make_reply73 / make_report73 standard & non-standard
    Message msg4 = make_reply73("HB9IPH", "W1AW", 5);
    EXPECT_EQ(msg4.type, MessageType::REPORT73_STD);
    EXPECT_EQ(msg4.call_1, "HB9IPH");
    EXPECT_EQ(msg4.call_2, "W1AW");
    EXPECT_EQ(msg4.rst_db, 5);

    Message msg4b = make_reply73("3B9/HB9IPH/P", "W1AW", 5);
    EXPECT_EQ(msg4b.type, MessageType::MULTI_REPORT73);
    EXPECT_EQ(msg4b.call_1, "W1AW");
    ASSERT_FALSE(msg4b.multi_targets.empty());
    EXPECT_EQ(msg4b.multi_targets[0].hash, hash_callsign_24("3B9/HB9IPH/P"));
    EXPECT_EQ(msg4b.multi_targets[0].rst_db, 5);

    Message msg4c = make_reply73("EA8/YO1YO", "3B9/HB9IPH/P", 5);
    EXPECT_EQ(msg4c.type, MessageType::MULTI_REPORT73);
    EXPECT_EQ(msg4c.call_1, "3B9/HB9IPH/P");
    ASSERT_FALSE(msg4c.multi_targets.empty());
    EXPECT_EQ(msg4c.multi_targets[0].hash, hash_callsign_24("EA8/YO1YO"));
    EXPECT_EQ(msg4c.multi_targets[0].rst_db, 5);

    // 5. make_73 standard & non-standard
    Message msg4d = make_73("HB9IPH", "W1AW");
    EXPECT_EQ(msg4d.type, MessageType::M73_STD);
    EXPECT_EQ(msg4d.call_1, "HB9IPH");
    EXPECT_EQ(msg4d.call_2, "W1AW");

    Message msg4e = make_73("3B9/HB9IP", "W1AW");
    EXPECT_EQ(msg4e.type, MessageType::M73_NONSTD);
    EXPECT_EQ(msg4e.hash_1, hash_callsign_24("3B9/HB9IP"));
    EXPECT_EQ(msg4e.call_2, "W1AW");

    // Multi report and multi 73 helpers
    Message msg_mr = make_multi_report73("HB9IPH", "YO1YO", 5, "TU2TU", -3);
    EXPECT_EQ(msg_mr.type, MessageType::MULTI_REPORT73);
    EXPECT_EQ(msg_mr.multi_targets.size(), 2u);

    std::vector<MultiTarget> mtargets = {
        {"YO1YO", hash_callsign_24("YO1YO"), 0},
        {"TU2TU", hash_callsign_24("TU2TU"), 0}
    };
    Message msg_m73_v = make_multi_73("HB9IPH", mtargets);
    EXPECT_EQ(msg_m73_v.type, MessageType::MULTI_73);
    EXPECT_EQ(msg_m73_v.multi_targets.size(), 2u);

    Message msg_m73_2 = make_multi_73("HB9IPH", "YO1YO", "TU2TU");
    EXPECT_EQ(msg_m73_2.type, MessageType::MULTI_73);
    EXPECT_EQ(msg_m73_2.multi_targets.size(), 2u);

    Message msg_mr_single = make_multi_report73("HB9IPH", "YO1YO", 5, "", 0);
    EXPECT_EQ(msg_mr_single.type, MessageType::MULTI_REPORT73);
    EXPECT_EQ(msg_mr_single.multi_targets.size(), 1u);

    Message msg_m73_single = make_multi_73("HB9IPH", "YO1YO", "");
    EXPECT_EQ(msg_m73_single.type, MessageType::MULTI_73);
    EXPECT_EQ(msg_m73_single.multi_targets.size(), 1u);

    // 6. make_cq variations
    Message msg_cq_mod = make_cq("EA6/HB9IP", "", "DX");
    EXPECT_EQ(msg_cq_mod.type, MessageType::CQ_NONSTD_2);

    Message msg_cq_free = make_cq("VERYLONGINVALIDCALLSIGN123456789", "", "");
    EXPECT_EQ(msg_cq_free.type, MessageType::FREE_TEXT);

    // 7. make_free_text
    Message msg5 = make_free_text("73 DE HB9IPH");
    EXPECT_EQ(msg5.type, MessageType::FREE_TEXT);
    EXPECT_EQ(msg5.text, "73 DE HB9IPH");
}

TEST(EasyApiTest, PackAndUnpack) {
    Message msg = make_cq("K1ABC", "FN20", "POTA");
    auto payload_opt = pack(msg);
    ASSERT_TRUE(payload_opt.has_value());
    EXPECT_EQ(payload_opt->size(), 10u);

    auto decoded_opt = unpack(*payload_opt);
    ASSERT_TRUE(decoded_opt.has_value());
    EXPECT_EQ(decoded_opt->type, MessageType::CQ_STD);
    EXPECT_EQ(decoded_opt->call_1, "K1ABC");
    EXPECT_EQ(decoded_opt->locator, "FN20");
    EXPECT_EQ(decoded_opt->modifier, "POTA");
}

TEST(EasyApiTest, DirectTextToAudioAndBack) {
    std::string original_text = "CQ HB9IPH JN47";
    auto audio = text_to_audio(original_text, Protocol::LQ8, 1500.0f, 12000.0f);
    ASSERT_FALSE(audio.empty());

    for (int t : {1, 2, 4, 16}) {
        auto decoded_text = audio_to_text(audio, 1500.0f, 12000.0f, Protocol::LQ8, t);
        ASSERT_TRUE(decoded_text.has_value());
        EXPECT_EQ(*decoded_text, original_text);
    }
}

TEST(EasyApiTest, TransceiverHelperClass) {
    Transceiver txrx("HB9IPH", "JN47", Protocol::LQ8, 1500.0f, 12000.0f, 1);
    EXPECT_EQ(txrx.get_num_threads(), 1);
    txrx.set_num_threads(4);
    EXPECT_EQ(txrx.get_num_threads(), 4);

    // 1. Generate CQ audio
    auto cq_audio = txrx.generate_cq("SOTA");
    EXPECT_FALSE(cq_audio.empty());
    for (int t : {1, 2, 4, 16}) {
        auto rx_cq = txrx.decode(cq_audio, t);
        ASSERT_TRUE(rx_cq.has_value());
        EXPECT_EQ(rx_cq->type, MessageType::CQ_STD);
        EXPECT_EQ(rx_cq->call_1, "HB9IPH");
        EXPECT_EQ(rx_cq->locator, "JN47");
        EXPECT_EQ(rx_cq->modifier, "SOTA");
    }

    // 2. Generate Call audio
    auto call_audio = txrx.generate_call("W1AW", -8);
    EXPECT_FALSE(call_audio.empty());
    for (int t : {1, 2, 4, 16}) {
        auto rx_call = txrx.decode(call_audio, t);
        ASSERT_TRUE(rx_call.has_value());
        EXPECT_EQ(rx_call->type, MessageType::CALL_STD_NOSUF);
        EXPECT_EQ(rx_call->call_1, "W1AW");
        EXPECT_EQ(rx_call->call_2, "HB9IPH");
        EXPECT_EQ(rx_call->locator, "JN47");
        EXPECT_EQ(rx_call->rst_db, -8);
    }

    // 3. Generate Reply73 audio
    auto rep_audio = txrx.generate_reply73("W1AW", +3);
    EXPECT_FALSE(rep_audio.empty());
    for (int t : {1, 2, 4, 16}) {
        auto rx_rep = txrx.decode(rep_audio, t);
        ASSERT_TRUE(rx_rep.has_value());
        EXPECT_EQ(rx_rep->type, MessageType::REPORT73_STD);
        EXPECT_EQ(rx_rep->rst_db, +3);
    }

    // 4. Generate Free text audio
    auto ft_audio = txrx.generate_free_text("TNX 73 GL");
    EXPECT_FALSE(ft_audio.empty());
    for (int t : {1, 2, 4, 16}) {
        auto rx_ft = txrx.decode_text(ft_audio, t);
        ASSERT_TRUE(rx_ft.has_value());
        EXPECT_EQ(*rx_ft, "TNX 73 GL");
    }

    // 5. Dynamic reconfigurations
    txrx.set_callsign("TU2TU");
    EXPECT_EQ(txrx.get_callsign(), "TU2TU");
    txrx.set_grid("KL22");
    EXPECT_EQ(txrx.get_grid(), "KL22");
    txrx.set_frequency(1800.0f);
    EXPECT_EQ(txrx.get_frequency(), 1800.0f);
    txrx.set_sample_rate(24000.0f);
    EXPECT_EQ(txrx.get_sample_rate(), 24000.0f);
    txrx.set_protocol(Protocol::LQ4);
    EXPECT_EQ(txrx.get_protocol(), Protocol::LQ4);

    auto lq4_cq = txrx.generate_cq();
    for (int t : {1, 2, 4, 16}) {
        auto rx_lq4 = txrx.decode(lq4_cq, t);
        ASSERT_TRUE(rx_lq4.has_value());
        EXPECT_EQ(rx_lq4->call_1, "TU2TU");
        EXPECT_EQ(rx_lq4->locator, "KL22");
    }

    // Test on silence / empty audio
    std::vector<float> silence(24000, 0.0f);
    EXPECT_FALSE(txrx.decode(silence).has_value());
    EXPECT_FALSE(txrx.decode_text(silence).has_value());
}

TEST(EasyApiTest, TextToTonesAndTonesToTextAllModes) {
    std::vector<std::string> test_strings = {
        "CQ HB9IPH JN47",
        "CQ DX HB9IPH JN47",
        "YO1YO HB9IPH JN47 -03",
        "YO1YO HB9IPH R+05",
        "YO1YO HB9IPH 73",
        "73 HB9IP"
    };

    std::vector<Protocol> protocols = {
        Protocol::LQ8,
        Protocol::LQ4,
        Protocol::LQ2,
        Protocol::LQ16
    };

    for (auto proto : protocols) {
        for (const auto& str : test_strings) {
            auto tones_opt = text_to_tones(str, proto);
            ASSERT_TRUE(tones_opt.has_value()) << "Failed text_to_tones for: " << str;
            EXPECT_GT(tones_opt->size(), 0u);

            auto text_opt = tones_to_text(*tones_opt);
            ASSERT_TRUE(text_opt.has_value()) << "Failed tones_to_text for: " << str;
            EXPECT_EQ(*text_opt, str);
        }
    }

    // Invalid text (empty string)
    EXPECT_FALSE(text_to_tones("", Protocol::LQ8).has_value());
    EXPECT_FALSE(text_to_tones("   ", Protocol::LQ8).has_value());
    ToneSequence empty_seq;
    EXPECT_FALSE(tones_to_text(empty_seq).has_value());
}

TEST(EasyApiTest, TextToAudioMultiSampleRates) {
    std::vector<float> sample_rates = { 8000.0f, 11025.0f, 12000.0f, 16000.0f, 24000.0f, 48000.0f };
    std::string test_msg = "CQ HB9IPH JN47";

    for (float sr : sample_rates) {
        auto audio = text_to_audio(test_msg, Protocol::LQ8, 1500.0f, sr);
        ASSERT_FALSE(audio.empty()) << "Failed text_to_audio at rate " << sr;

        auto decoded = audio_to_text(audio, 1500.0f, sr, Protocol::LQ8);
        ASSERT_TRUE(decoded.has_value()) << "Failed audio_to_text at rate " << sr;
        EXPECT_EQ(*decoded, test_msg);
    }
}

TEST(EasyApiTest, PackUnpackBoundaryAndErrors) {
    // 1. Invalid size vector in unpack
    std::vector<uint8_t> short_payload(8, 0xAA);
    EXPECT_FALSE(unpack(short_payload).has_value());
    std::vector<uint8_t> empty_payload;
    EXPECT_FALSE(unpack(empty_payload).has_value());

    // 2. Corrupted prefix unpack
    std::vector<uint8_t> corrupt_payload(10, 0xFF);
    // Unpack may or may not succeed with unknown/invalid prefix depending on bit pattern
    auto corrupt_res = unpack(corrupt_payload);
    (void)corrupt_res;

    // 3. Roundtrip unpack std::vector<uint8_t> overload
    Message msg = make_call("YO1YO", "HB9IPH", "JN47", -5);
    auto packed = pack(msg);
    ASSERT_TRUE(packed.has_value());
    auto unpacked = unpack(*packed);
    ASSERT_TRUE(unpacked.has_value());
    EXPECT_EQ(unpacked->type, MessageType::CALL_STD_NOSUF);
    EXPECT_EQ(unpacked->call_1, "YO1YO");
    EXPECT_EQ(unpacked->call_2, "HB9IPH");
    EXPECT_EQ(unpacked->locator, "JN47");
    EXPECT_EQ(unpacked->rst_db, -5);

    // 4. Multi-reply easy helper
    Message multi_msg = make_multi_reply73("HB9IPH", "YO1YO", 5, "TU2TU", -3);
    EXPECT_EQ(multi_msg.type, MessageType::MULTI_REPLY73);
    EXPECT_EQ(multi_msg.call_1, "HB9IPH");
    EXPECT_EQ(multi_msg.multi_targets.size(), 2U);
    Transceiver trx("HB9IPH", "JN47", Protocol::LQ8, 1500.0f, 12000.0f);
    auto audio_multi = trx.generate_multi_reply73("YO1YO", 5, "TU2TU", -3);
    EXPECT_FALSE(audio_multi.empty());
}

TEST(EasyApiTest, StreamOperatorsExhaustive) {
    std::vector<MessageType> types = {
        MessageType::CQ_STD,
        MessageType::CQ_NONSTD_1,
        MessageType::CQ_NONSTD_2,
        MessageType::CQ_NONSTD_3,
        MessageType::CALL_STD_NOSUF,
        MessageType::CALL_STD_SUF,
        MessageType::CALL_NONSTD,
        MessageType::REPORT73_STD,
        MessageType::M73_STD,
        MessageType::M73_NONSTD,
        MessageType::MULTI_REPORT73,
        MessageType::MULTI_73,
        MessageType::FREE_TEXT,
        MessageType::RESERVED_A,
        MessageType::RESERVED_B,
        MessageType::RESERVED_C
    };

    for (auto t : types) {
        std::ostringstream ss;
        ss << t;
        EXPECT_FALSE(ss.str().empty());
    }

    std::vector<Protocol> protos = {
        Protocol::LQ8,
        Protocol::LQ4,
        Protocol::LQ2,
        Protocol::LQ16
    };

    for (auto p : protos) {
        std::ostringstream ss;
        ss << p;
        EXPECT_FALSE(ss.str().empty());
    }

    // Message stream operator and message_to_text equivalence
    Message m = make_cq("HB9IPH", "JN47", "DX");
    EXPECT_EQ(message_to_text(m), "CQ DX HB9IPH JN47");
    std::ostringstream ss_msg;
    ss_msg << m;
    EXPECT_EQ(ss_msg.str(), "CQ DX HB9IPH JN47");
}

