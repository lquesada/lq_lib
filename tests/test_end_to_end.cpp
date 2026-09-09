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
#include "lq/lq.h"
#include <random>
#include <vector>
#include <string>
#include <cstdio>

using namespace lq;

class EndToEndTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Build representative message instances for all 12 message types

        // 1. CQ_STD (Type 1)
        Message m1;
        m1.type = MessageType::CQ_STD;
        m1.call_1 = "YO1YO";
        m1.modifier = "DX";
        m1.locator = "JN47";
        test_messages.push_back(m1);

        // 2. CQ_NONSTD_1 (Type 2, max 9 chars + loc)
        Message m2;
        m2.type = MessageType::CQ_NONSTD_1;
        m2.call_1 = "YO1YO";
        m2.locator = "JN47";
        test_messages.push_back(m2);

        // 3. CQ_NONSTD_2 (Type 3, max 9 chars + mod)
        Message m3;
        m3.type = MessageType::CQ_NONSTD_2;
        m3.call_1 = "EA6/W1AW";
        m3.modifier = "FD";
        test_messages.push_back(m3);

        // 4. CQ_NONSTD_3 (Type 4, max 13 chars)
        Message m4;
        m4.type = MessageType::CQ_NONSTD_3;
        m4.call_1 = "3B9/HB9IPH/P";
        test_messages.push_back(m4);

        // 5. CALL_STD_NOSUF (Type 5, 100% full 77-bit payload)
        Message m5;
        m5.type = MessageType::CALL_STD_NOSUF;
        m5.call_1 = "YO1YO";
        m5.call_2 = "TU2TU";
        m5.locator = "KL22";
        m5.rst_db = -3;
        test_messages.push_back(m5);

        // 6. CALL_STD_SUF (Type 6)
        Message m6;
        m6.type = MessageType::CALL_STD_SUF;
        m6.hash_1 = hash_callsign_24("YO1YO");
        m6.call_2 = "HB9IPH/P";
        m6.suffix_2 = 1;
        m6.locator = "KL22";
        m6.rst_db = -3;
        test_messages.push_back(m6);

        // 7. CALL_NONSTD (Type 7)
        Message m7;
        m7.type = MessageType::CALL_NONSTD;
        m7.hash_1 = 0x01A4F;
        m7.call_2 = "EA6/HB9IP/P";
        m7.suffix_2 = 1;
        m7.rst_db = 5;
        test_messages.push_back(m7);

        // 8. REPORT73_STD (Type 8)
        Message m8;
        m8.type = MessageType::REPORT73_STD;
        m8.call_1 = "YO1YO";
        m8.call_2 = "TU2TU";
        m8.rst_db = 5;
        test_messages.push_back(m8);

        // 9. M73_STD (Type 9)
        Message m9;
        m9.type = MessageType::M73_STD;
        m9.call_1 = "YO1YO";
        m9.call_2 = "TU2TU";
        test_messages.push_back(m9);

        // 10. M73_NONSTD (Type 10)
        Message m10;
        m10.type = MessageType::M73_NONSTD;
        m10.hash_1 = hash_callsign_24("EA6/W1AW");
        m10.call_2 = "TU2TU";
        m10.suffix_2 = 0;
        test_messages.push_back(m10);

        // 11. MULTI_REPORT73 (Type 11)
        Message m11;
        m11.type = MessageType::MULTI_REPORT73;
        m11.hash_1 = hash_callsign_16("HB9IPH");
        m11.multi_targets = {
            {"", hash_callsign_24("YO1YO"), 5},
            {"", hash_callsign_24("TU2TU"), -3}
        };
        test_messages.push_back(m11);

        // 12. MULTI_73 (Type 12)
        Message m12;
        m12.type = MessageType::MULTI_73;
        m12.hash_1 = hash_callsign_16("HB9IPH");
        m12.multi_targets = {
            {"", hash_callsign_24("YO1YO"), 0},
            {"", hash_callsign_24("TU2TU"), 0}
        };
        test_messages.push_back(m12);

        // 13. FREE_TEXT (Type 13)
        Message m13;
        m13.type = MessageType::FREE_TEXT;
        m13.text = "73 HB9IP";
        test_messages.push_back(m13);

        // 14. RESERVED_A (Type 14)
        Message m14;
        m14.type = MessageType::RESERVED_A;
        m14.raw_payload = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x00, 0xAA};
        test_messages.push_back(m14);
    }

    std::vector<Message> test_messages;
};

TEST_F(EndToEndTest, AllMessageTypesThroughTonePipeline) {
    const std::vector<Protocol> protocols = {Protocol::LQ8, Protocol::LQ4, Protocol::LQ2, Protocol::LQ16};

    for (auto proto : protocols) {
        for (const auto& original : test_messages) {
            // 1. Encode message to tone sequence
            ToneSequence tx_tones;
            ASSERT_TRUE(encode_tones(original, proto, tx_tones))
                << "Failed to encode message type " << static_cast<int>(original.type);

            // 2. Decode tone sequence back to message
            Message rx_msg;
            ASSERT_TRUE(decode_tones(tx_tones, rx_msg))
                << "Failed to decode message type " << static_cast<int>(original.type);

            // 3. Verify message equivalence
            EXPECT_EQ(rx_msg.type, original.type);
            EXPECT_EQ(rx_msg.call_1, original.call_1);
            EXPECT_EQ(rx_msg.call_2, original.call_2);
            EXPECT_EQ(rx_msg.hash_1, original.hash_1);
            EXPECT_EQ(rx_msg.hash_2, original.hash_2);
            EXPECT_EQ(rx_msg.modifier, original.modifier);
            EXPECT_EQ(rx_msg.locator, original.locator);
            EXPECT_EQ(rx_msg.rst_db, original.rst_db);
            EXPECT_EQ(rx_msg.text, original.text);
            EXPECT_EQ(rx_msg.multi_targets.size(), original.multi_targets.size());
        }
    }
}

TEST_F(EndToEndTest, FullAudioWaveformLoopbackLQ8) {
    float sample_rate = 12000.0f;
    float base_freq = 1500.0f;

    for (const auto& original : test_messages) {
        // Encode message to audio
        std::vector<float> audio;
        ASSERT_TRUE(message_to_audio(original, Protocol::LQ8, base_freq, sample_rate, audio));

        // Test with different worker thread counts (1, 2, 4, 16)
        for (int t : {1, 2, 4, 16}) {
            Message decoded;
            ASSERT_TRUE(audio_to_message(audio, base_freq, sample_rate, Protocol::LQ8, decoded, t))
                << "Failed to decode audio for message type " << static_cast<int>(original.type)
                << " with " << t << " threads";

            EXPECT_EQ(decoded.type, original.type);
            EXPECT_EQ(decoded.call_1, original.call_1);
            EXPECT_EQ(decoded.call_2, original.call_2);
        }
    }
}

TEST_F(EndToEndTest, FullAudioWaveformLoopbackLQ4) {
    float sample_rate = 12000.0f;
    float base_freq = 1200.0f;

    for (const auto& original : test_messages) {
        std::vector<float> audio;
        ASSERT_TRUE(message_to_audio(original, Protocol::LQ4, base_freq, sample_rate, audio));

        for (int t : {1, 2, 4, 16}) {
            Message decoded;
            ASSERT_TRUE(audio_to_message(audio, base_freq, sample_rate, Protocol::LQ4, decoded, t));

            EXPECT_EQ(decoded.type, original.type);
            EXPECT_EQ(decoded.call_1, original.call_1);
            EXPECT_EQ(decoded.call_2, original.call_2);
        }
    }
}

TEST_F(EndToEndTest, FullAudioWaveformLoopbackLQ2) {
    float sample_rate = 12000.0f;
    float base_freq = 1000.0f;

    for (const auto& original : test_messages) {
        std::vector<float> audio;
        ASSERT_TRUE(message_to_audio(original, Protocol::LQ2, base_freq, sample_rate, audio));

        for (int t : {1, 2, 4, 16}) {
            Message decoded;
            ASSERT_TRUE(audio_to_message(audio, base_freq, sample_rate, Protocol::LQ2, decoded, t));

            EXPECT_EQ(decoded.type, original.type);
            EXPECT_EQ(decoded.call_1, original.call_1);
            EXPECT_EQ(decoded.call_2, original.call_2);
        }
    }
}

TEST_F(EndToEndTest, FullAudioWaveformLoopbackLQ16) {
    float sample_rate = 12000.0f;
    float base_freq = 1500.0f;

    for (const auto& original : test_messages) {
        std::vector<float> audio;
        ASSERT_TRUE(message_to_audio(original, Protocol::LQ16, base_freq, sample_rate, audio));

        Message decoded;
        ASSERT_TRUE(audio_to_message(audio, base_freq, sample_rate, Protocol::LQ16, decoded, 2));

        EXPECT_EQ(decoded.type, original.type);
        EXPECT_EQ(decoded.call_1, original.call_1);
        EXPECT_EQ(decoded.call_2, original.call_2);
    }
}

TEST_F(EndToEndTest, AudioWithNoiseChannel) {
    Message msg;
    msg.type = MessageType::CALL_STD_NOSUF;
    msg.call_1 = "HB9IPH";
    msg.call_2 = "YO1YO";
    msg.locator = "JN47";
    msg.rst_db = -3;

    float sample_rate = 12000.0f;
    float base_freq = 1500.0f;

    std::vector<float> audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_freq, sample_rate, audio));

    // Add white Gaussian noise with standard deviation sigma = 0.35 (approx 9 dB SNR)
    std::mt19937 rng(42);
    std::normal_distribution<float> noise(0.0f, 0.35f);

    for (float& s : audio) {
        s += noise(rng);
    }

    for (int t : {1, 2, 4, 16}) {
        Message decoded;
        ASSERT_TRUE(audio_to_message(audio, base_freq, sample_rate, Protocol::LQ8, decoded, t));
        EXPECT_EQ(decoded.type, msg.type);
        EXPECT_EQ(decoded.call_1, msg.call_1);
        EXPECT_EQ(decoded.call_2, msg.call_2);
        EXPECT_EQ(decoded.locator, msg.locator);
        EXPECT_EQ(decoded.rst_db, msg.rst_db);
    }
}

TEST_F(EndToEndTest, WavFileExportAndImport) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.modifier = "DX";
    msg.locator = "JN47";

    float sample_rate = 12000.0f;
    float base_freq = 1500.0f;

    std::vector<float> audio_tx;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_freq, sample_rate, audio_tx));

    std::string temp_wav = "/tmp/test_lq8_msg.wav";
    ASSERT_TRUE(save_wav_file(temp_wav, audio_tx, sample_rate));

    std::vector<float> audio_rx;
    float rx_sample_rate = 0.0f;
    ASSERT_TRUE(load_wav_file(temp_wav, audio_rx, rx_sample_rate));
    EXPECT_EQ(rx_sample_rate, sample_rate);
    EXPECT_EQ(audio_rx.size(), audio_tx.size());

    Message decoded;
    ASSERT_TRUE(audio_to_message(audio_rx, base_freq, rx_sample_rate, Protocol::LQ8, decoded));
    EXPECT_EQ(decoded.type, msg.type);
    EXPECT_EQ(decoded.call_1, msg.call_1);
    EXPECT_EQ(decoded.modifier, msg.modifier);
    EXPECT_EQ(decoded.locator, msg.locator);

    std::remove(temp_wav.c_str());
}
