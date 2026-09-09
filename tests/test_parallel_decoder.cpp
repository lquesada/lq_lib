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
#include "lq/transport.h"
#include "lq/easy.h"
#include "lq/c_api.h"
#include <vector>
#include <string>
#include <thread>
#include <future>

namespace {

// Helper to synthesize multi-signal audio buffer
std::vector<float> create_multi_signal_audio(const std::vector<std::pair<lq::Message, float>>& signals,
                                             lq::Protocol proto,
                                             float sample_rate = 12000.0f) {
    std::vector<float> composite;
    for (const auto& [msg, freq] : signals) {
        std::vector<float> sig;
        EXPECT_TRUE(lq::message_to_audio(msg, proto, freq, sample_rate, sig));
        if (composite.empty()) {
            composite.resize(sig.size(), 0.0f);
        }
        for (size_t i = 0; i < sig.size(); ++i) {
            composite[i] += sig[i] * 0.5f;
        }
    }
    return composite;
}

} // anonymous namespace

TEST(ParallelDecoder, ThreadCountParameterClampingAndEdgeCases) {
    lq::Message orig = lq::make_cq("HB9IPH", "JN47", "DX");
    std::vector<float> audio;
    ASSERT_TRUE(lq::message_to_audio(orig, lq::Protocol::LQ8, 1200.0f, 12000.0f, audio));

    // Test defensive handling of 0, negative, 1, 2, 4, 16, 128 threads
    const int thread_counts[] = {-10, -1, 0, 1, 2, 3, 4, 8, 16, 32, 64, 128};

    for (int t : thread_counts) {
        lq::Message decoded;
        bool ok = lq::audio_to_message(audio, 1200.0f, 12000.0f, lq::Protocol::LQ8, decoded, t);
        EXPECT_TRUE(ok) << "Failed decoding with thread count: " << t;
        EXPECT_EQ(decoded.call_1, "HB9IPH");
        EXPECT_EQ(decoded.locator, "JN47");
        EXPECT_EQ(decoded.modifier, "DX");

        auto list = lq::audio_to_messages(audio, 1200.0f, 12000.0f, lq::Protocol::LQ8, t);
        EXPECT_EQ(list.size(), 1U);
        if (!list.empty()) {
            EXPECT_EQ(list[0].call_1, "HB9IPH");
        }
    }
}

TEST(ParallelDecoder, BitForBitDeterminismAcrossThreads) {
    // 3 distinct signals at 700 Hz, 1350 Hz, and 2100 Hz
    std::vector<std::pair<lq::Message, float>> signals = {
        {lq::make_cq("HB9IPH", "JN47", "DX"), 700.0f},
        {lq::make_call("HB9IPH", "YO1YO", "KN34", -5), 1350.0f},
        {lq::make_reply73("YO1YO", "HB9IPH", 12), 2100.0f}
    };

    auto audio = create_multi_signal_audio(signals, lq::Protocol::LQ8);
    ASSERT_FALSE(audio.empty());

    // Decode with 1 thread as reference baseline
    auto baseline = lq::audio_to_messages(audio, 0.0f, 12000.0f, lq::Protocol::LQ8, 1);
    ASSERT_EQ(baseline.size(), 3U);

    // Compare results with 2, 4, 8, and 16 threads
    const int test_threads[] = {2, 4, 8, 16};
    for (int t : test_threads) {
        auto result = lq::audio_to_messages(audio, 0.0f, 12000.0f, lq::Protocol::LQ8, t);
        ASSERT_EQ(result.size(), baseline.size()) << "Thread count " << t << " produced different number of messages";
        for (size_t i = 0; i < baseline.size(); ++i) {
            EXPECT_EQ(result[i].type, baseline[i].type) << "Mismatch at index " << i << " with " << t << " threads";
            EXPECT_EQ(result[i].call_1, baseline[i].call_1);
            EXPECT_EQ(result[i].call_2, baseline[i].call_2);
            EXPECT_EQ(result[i].locator, baseline[i].locator);
            EXPECT_EQ(result[i].rst_db, baseline[i].rst_db);
        }
    }
}

TEST(ParallelDecoder, ZeroBoundarySeamsAcrossAdjacentSubBands) {
    // Test signals placed in adjacent channels across partition boundaries
    std::vector<std::pair<lq::Message, float>> signals = {
        {lq::make_cq("HB9IPH", "JN47"), 500.0f},
        {lq::make_cq("YO1YO", "KN34"), 560.0f},
        {lq::make_cq("TU2TU", "IJ88"), 620.0f}
    };

    auto audio = create_multi_signal_audio(signals, lq::Protocol::LQ8);

    for (int t : {1, 2, 4, 16}) {
        auto result = lq::audio_to_messages(audio, 0.0f, 12000.0f, lq::Protocol::LQ8, t);
        EXPECT_EQ(result.size(), 3U) << "Failed detecting all boundary signals with " << t << " threads";
    }
}

TEST(ParallelDecoder, TransceiverAndEasyApiMultiThreading) {
    lq::Transceiver tx("HB9IPH", "JN47", lq::Protocol::LQ8, 1400.0f, 12000.0f, 1);
    EXPECT_EQ(tx.get_num_threads(), 1);

    tx.set_num_threads(4);
    EXPECT_EQ(tx.get_num_threads(), 4);

    auto audio = tx.generate_cq("POTA");
    ASSERT_FALSE(audio.empty());

    // Test default instance threads
    auto dec1 = tx.decode(audio);
    ASSERT_TRUE(dec1.has_value());
    EXPECT_EQ(dec1->call_1, "HB9IPH");
    EXPECT_EQ(dec1->modifier, "POTA");

    // Test explicit thread overrides across 1, 2, 4, 16
    for (int t : {1, 2, 4, 16}) {
        auto dec = tx.decode(audio, t);
        ASSERT_TRUE(dec.has_value());
        EXPECT_EQ(dec->call_1, "HB9IPH");

        auto text = tx.decode_text(audio, t);
        ASSERT_TRUE(text.has_value());
        EXPECT_NE(text->find("HB9IPH"), std::string::npos);

        auto direct_text = lq::audio_to_text(audio, 1400.0f, 12000.0f, lq::Protocol::LQ8, t);
        ASSERT_TRUE(direct_text.has_value());
        EXPECT_EQ(*direct_text, *text);
    }
}

TEST(ParallelDecoder, PureCApiMultiThreading) {
    lq::Message msg = lq::make_call("HB9IPH", "YO1YO", "KN34", 5);
    std::vector<float> audio;
    ASSERT_TRUE(lq::message_to_audio(msg, lq::Protocol::LQ8, 1600.0f, 12000.0f, audio));

    for (int t : {1, 2, 4, 16}) {
        lq_c_message_t c_msg;
        int ok = lq_c_audio_to_message(audio.data(), audio.size(), 1600.0f, 12000.0f, LQ_MODE_LQ8, &c_msg, t);
        EXPECT_EQ(ok, 1);
        EXPECT_STREQ(c_msg.call_to, "HB9IPH");
        EXPECT_STREQ(c_msg.call_from, "YO1YO");
        EXPECT_EQ(c_msg.rst_db, 5);

        lq_c_message_t c_msgs[4];
        int count = lq_c_audio_to_messages(audio.data(), audio.size(), 1600.0f, 12000.0f, LQ_MODE_LQ8, c_msgs, 4, t);
        EXPECT_EQ(count, 1);
        EXPECT_STREQ(c_msgs[0].call_to, "HB9IPH");
        EXPECT_STREQ(c_msgs[0].call_from, "YO1YO");
    }

    // Invalid parameters
    lq_c_message_t c_msg;
    EXPECT_EQ(lq_c_audio_to_message(nullptr, audio.size(), 1600.0f, 12000.0f, LQ_MODE_LQ8, &c_msg, 2), 0);
    EXPECT_EQ(lq_c_audio_to_message(audio.data(), 0, 1600.0f, 12000.0f, LQ_MODE_LQ8, &c_msg, 2), 0);
    EXPECT_EQ(lq_c_audio_to_message(audio.data(), audio.size(), 1600.0f, 12000.0f, LQ_MODE_LQ8, nullptr, 2), 0);
    EXPECT_EQ(lq_c_audio_to_messages(nullptr, audio.size(), 1600.0f, 12000.0f, LQ_MODE_LQ8, &c_msg, 1, 2), 0);
    EXPECT_EQ(lq_c_audio_to_messages(audio.data(), 0, 1600.0f, 12000.0f, LQ_MODE_LQ8, &c_msg, 1, 2), 0);
    EXPECT_EQ(lq_c_audio_to_messages(audio.data(), audio.size(), 1600.0f, 12000.0f, LQ_MODE_LQ8, nullptr, 1, 2), 0);
    EXPECT_EQ(lq_c_audio_to_messages(audio.data(), audio.size(), 1600.0f, 12000.0f, LQ_MODE_LQ8, &c_msg, 0, 2), 0);
}

TEST(ParallelDecoder, MultiProtocolParallelDecoding) {
    const lq::Protocol protocols[] = {lq::Protocol::LQ8, lq::Protocol::LQ4, lq::Protocol::LQ2, lq::Protocol::LQ16};

    for (auto proto : protocols) {
        lq::Message msg = lq::make_cq("HB9IPH", "JN47");
        std::vector<float> audio;
        ASSERT_TRUE(lq::message_to_audio(msg, proto, 1000.0f, 12000.0f, audio));

        for (int t : {1, 2, 4, 16}) {
            lq::Message decoded;
            bool ok = lq::audio_to_message(audio, 1000.0f, 12000.0f, proto, decoded, t);
            EXPECT_TRUE(ok) << "Failed on proto " << static_cast<int>(proto) << " with " << t << " threads";
            EXPECT_EQ(decoded.call_1, "HB9IPH");
            EXPECT_EQ(decoded.locator, "JN47");
        }
    }
}

TEST(ParallelDecoder, ConcurrencyStressTest) {
    lq::Message msg1 = lq::make_cq("HB9IPH", "JN47");
    lq::Message msg2 = lq::make_call("HB9IPH", "YO1YO", "KN34", -3);

    std::vector<float> audio1, audio2;
    ASSERT_TRUE(lq::message_to_audio(msg1, lq::Protocol::LQ8, 900.0f, 12000.0f, audio1));
    ASSERT_TRUE(lq::message_to_audio(msg2, lq::Protocol::LQ8, 1700.0f, 12000.0f, audio2));

    // Run 10 parallel decoding tasks concurrently
    std::vector<std::future<bool>> futures;
    for (int i = 0; i < 10; ++i) {
        const auto& audio = (i % 2 == 0) ? audio1 : audio2;
        const std::string expected_call = (i % 2 == 0) ? "HB9IPH" : "YO1YO";
        float freq = (i % 2 == 0) ? 900.0f : 1700.0f;

        futures.push_back(std::async(std::launch::async, [&audio, expected_call, freq]() {
            for (int round = 0; round < 3; ++round) {
                for (int t : {1, 2, 4, 16}) {
                    lq::Message dec;
                    if (!lq::audio_to_message(audio, freq, 12000.0f, lq::Protocol::LQ8, dec, t)) {
                        return false;
                    }
                    if (dec.call_1 != "HB9IPH") {
                        return false;
                    }
                }
            }
            return true;
        }));
    }

    for (auto& f : futures) {
        EXPECT_TRUE(f.get());
    }
}
