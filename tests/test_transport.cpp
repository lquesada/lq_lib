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
#include <vector>
#include <cmath>
#include <random>

using namespace lq;

TEST(TransportTest, LQ8FrameStructure) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "YO1YO";
    msg.locator = "JN47";

    ToneSequence seq;
    ASSERT_TRUE(encode_tones(msg, Protocol::LQ8, seq));
    EXPECT_EQ(seq.protocol, Protocol::LQ8);
    EXPECT_EQ(seq.size(), 79U);

    // Verify Costas array sync blocks
    // Sync 1 at 0..6
    for (size_t i = 0; i < 7; ++i) {
        EXPECT_EQ(seq[i], COSTAS_ARRAY_8[i]);
    }
    // Sync 2 at 36..42
    for (size_t i = 0; i < 7; ++i) {
        EXPECT_EQ(seq[36 + i], COSTAS_ARRAY_8[i]);
    }
    // Sync 3 at 72..78
    for (size_t i = 0; i < 7; ++i) {
        EXPECT_EQ(seq[72 + i], COSTAS_ARRAY_8[i]);
    }

    // Verify data tones are within range 0..7
    for (size_t i = 0; i < seq.size(); ++i) {
        EXPECT_LE(seq[i], 7U);
    }
}

TEST(TransportTest, LQ4FrameStructure) {
    Message msg;
    msg.type = MessageType::CALL_STD;
    msg.call_1 = "YO1YO";
    msg.call_2 = "TU2TU";
    msg.locator = "KL22";
    msg.rst_db = -3;

    ToneSequence seq;
    ASSERT_TRUE(encode_tones(msg, Protocol::LQ4, seq));
    EXPECT_EQ(seq.protocol, Protocol::LQ4);
    EXPECT_EQ(seq.size(), 105U);

    // Verify ramps
    EXPECT_EQ(seq[0], 0U);
    EXPECT_EQ(seq[104], 0U);

    // Verify 4 Costas sync blocks
    for (size_t i = 0; i < 4; ++i) EXPECT_EQ(seq[1 + i], COSTAS_SYNC1_4[i]);
    for (size_t i = 0; i < 4; ++i) EXPECT_EQ(seq[34 + i], COSTAS_SYNC2_4[i]);
    for (size_t i = 0; i < 4; ++i) EXPECT_EQ(seq[67 + i], COSTAS_SYNC3_4[i]);
    for (size_t i = 0; i < 4; ++i) EXPECT_EQ(seq[100 + i], COSTAS_SYNC4_4[i]);

    // Verify 4-FSK tone range (0..3)
    for (size_t i = 0; i < seq.size(); ++i) {
        EXPECT_LE(seq[i], 3U);
    }
}

TEST(TransportTest, LQ2FrameStructure) {
    Message msg;
    msg.type = MessageType::REPLY73_STD;
    msg.call_1 = "YO1YO";
    msg.call_2 = "TU2TU";
    msg.rst_db = 5;

    ToneSequence seq;
    ASSERT_TRUE(encode_tones(msg, Protocol::LQ2, seq));
    EXPECT_EQ(seq.protocol, Protocol::LQ2);
    EXPECT_EQ(seq.size(), 105U);
    EXPECT_FLOAT_EQ(seq.symbol_period, 0.024f);
    EXPECT_FLOAT_EQ(seq.tx_duration, 2.52f);
}

TEST(TransportTest, LQ16FrameStructure) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    ToneSequence seq;
    ASSERT_TRUE(encode_tones(msg, Protocol::LQ16, seq));
    EXPECT_EQ(seq.protocol, Protocol::LQ16);
    EXPECT_EQ(seq.size(), 79U);
    EXPECT_FLOAT_EQ(seq.symbol_period, 0.320f);
    EXPECT_FLOAT_EQ(seq.tone_spacing, 3.125f);
    EXPECT_FLOAT_EQ(seq.tx_duration, 25.28f);

    // Verify Costas array sync blocks at 0..6, 36..42, 72..78
    for (size_t i = 0; i < 7; ++i) {
        EXPECT_EQ(seq[i], COSTAS_ARRAY_8[i]);
        EXPECT_EQ(seq[36 + i], COSTAS_ARRAY_8[i]);
        EXPECT_EQ(seq[72 + i], COSTAS_ARRAY_8[i]);
    }
}

TEST(TransportTest, FullPipelineRoundTripAllProtocols) {
    const std::vector<Protocol> protocols = {Protocol::LQ8, Protocol::LQ4, Protocol::LQ2, Protocol::LQ16};

    // Test messages
    Message msg_cq;
    msg_cq.type = MessageType::CQ_STD;
    msg_cq.modifier = "DX";
    msg_cq.call_1 = "W1AW";
    msg_cq.locator = "FN31";

    Message msg_call;
    msg_call.type = MessageType::CALL_STD;
    msg_call.call_1 = "YO1YO";
    msg_call.call_2 = "TU2TU";
    msg_call.locator = "KL22";
    msg_call.rst_db = -3;

    Message msg_reply;
    msg_reply.type = MessageType::REPLY73_STD;
    msg_reply.call_1 = "YO1YO";
    msg_reply.call_2 = "TU2TU";
    msg_reply.rst_db = 5;

    const std::vector<Message> test_messages = {msg_cq, msg_call, msg_reply};

    for (auto proto : protocols) {
        for (const auto& original : test_messages) {
            ToneSequence seq;
            ASSERT_TRUE(encode_tones(original, proto, seq));

            Message decoded;
            ASSERT_TRUE(decode_tones(seq, decoded));

            EXPECT_EQ(decoded.type, original.type);
            EXPECT_EQ(decoded.call_1, original.call_1);
            EXPECT_EQ(decoded.call_2, original.call_2);
            EXPECT_EQ(decoded.locator, original.locator);
            EXPECT_EQ(decoded.modifier, original.modifier);
            EXPECT_EQ(decoded.rst_db, original.rst_db);
        }
    }
}

TEST(TransportTest, AudioGeneration) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "YO1YO";
    msg.locator = "JN47";

    ToneSequence seq;
    ASSERT_TRUE(encode_tones(msg, Protocol::LQ8, seq));

    std::vector<float> audio;
    float sample_rate = 12000.0f;
    float base_freq = 1500.0f;
    generate_audio(seq, base_freq, sample_rate, audio);

    EXPECT_FALSE(audio.empty());
    size_t expected_samples = static_cast<size_t>(std::round(seq.symbol_period * sample_rate)) * seq.size();
    EXPECT_EQ(audio.size(), expected_samples);

    // Verify all audio samples are in valid normalised float range [-1.0, 1.0]
    for (float s : audio) {
        EXPECT_GE(s, -1.0f);
        EXPECT_LE(s, 1.0f);
    }
}

TEST(TransportTest, SilenceRejection) {
    std::vector<float> silence(12000 * 13, 0.0f);
    Message decoded;
    EXPECT_FALSE(audio_to_message(silence, 1500.0f, 12000.0f, Protocol::LQ8, decoded));

    ToneSequence all_zeros;
    all_zeros.protocol = Protocol::LQ8;
    all_zeros.tones.resize(79, 0);
    EXPECT_FALSE(decode_tones(all_zeros, decoded));
}

TEST(TransportTest, SoftLlrDemodulationLQ8) {
    Message msg;
    msg.type = MessageType::CALL_STD;
    msg.call_1 = "HB9IPH";
    msg.call_2 = "YO1YO";
    msg.locator = "JN47";
    msg.rst_db = -3;

    std::vector<float> audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, 1500.0f, 12000.0f, audio));

    std::vector<float> llrs;
    ASSERT_TRUE(demodulate_audio_soft(audio, 0, 1500.0f, 12000.0f, Protocol::LQ8, llrs));
    EXPECT_EQ(llrs.size(), 174U);

    Message decoded;
    ASSERT_TRUE(decode_soft_llrs(llrs, decoded));
    EXPECT_EQ(decoded.type, MessageType::CALL_STD);
    EXPECT_EQ(decoded.call_1, "HB9IPH");
    EXPECT_EQ(decoded.call_2, "YO1YO");
    EXPECT_EQ(decoded.locator, "JN47");
    EXPECT_EQ(decoded.rst_db, -3);
}

TEST(TransportTest, SoftLlrDemodulationLQ4) {
    Message msg;
    msg.type = MessageType::REPLY73_STD;
    msg.call_1 = "TU2TU";
    msg.call_2 = "HB9IPH";
    msg.rst_db = 5;

    std::vector<float> audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ4, 1200.0f, 12000.0f, audio));

    std::vector<float> llrs;
    ASSERT_TRUE(demodulate_audio_soft(audio, 0, 1200.0f, 12000.0f, Protocol::LQ4, llrs));
    EXPECT_EQ(llrs.size(), 174U);

    Message decoded;
    ASSERT_TRUE(decode_soft_llrs(llrs, decoded, Protocol::LQ4));
    EXPECT_EQ(decoded.type, MessageType::REPLY73_STD);
    EXPECT_EQ(decoded.call_1, "TU2TU");
    EXPECT_EQ(decoded.call_2, "HB9IPH");
    EXPECT_EQ(decoded.rst_db, 5);
}

TEST(TransportTest, FrequencyOffsetTolerance) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    // Transmitted on 1508.0 Hz (+8 Hz offset from nominal 1500.0 Hz)
    std::vector<float> audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, 1508.0f, 12000.0f, audio));

    // Receiver searches around nominal 1500.0 Hz
    Message decoded;
    EXPECT_TRUE(audio_to_message(audio, 1500.0f, 12000.0f, Protocol::LQ8, decoded));
    EXPECT_EQ(decoded.call_1, "HB9IPH");
    EXPECT_EQ(decoded.locator, "JN47");
}

TEST(TransportTest, WidebandPassbandDecoding) {
    Message msg;
    msg.type = MessageType::CALL_STD;
    msg.call_1 = "W1AW";
    msg.call_2 = "HB9IPH";
    msg.locator = "FN31";
    msg.rst_db = 0;

    // Transmitted on 650.0 Hz (low in the audio passband)
    std::vector<float> audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, 650.0f, 12000.0f, audio));

    // Wideband receiver (base_freq = 0.0f scans full 250 Hz - 2600 Hz)
    Message decoded;
    EXPECT_TRUE(audio_to_message(audio, 0.0f, 12000.0f, Protocol::LQ8, decoded));
    EXPECT_EQ(decoded.call_1, "W1AW");
    EXPECT_EQ(decoded.call_2, "HB9IPH");
    EXPECT_EQ(decoded.locator, "FN31");
}

TEST(TransportTest, MultiSignalWaterfallDecoding) {
    Message msg1;
    msg1.type = MessageType::CQ_STD;
    msg1.call_1 = "W1AW";
    msg1.locator = "FN31";

    Message msg2;
    msg2.type = MessageType::CQ_STD;
    msg2.call_1 = "YO1YO";
    msg2.locator = "JN47";

    // Synthesize two simultaneous signals at different frequencies
    std::vector<float> audio1;
    std::vector<float> audio2;
    ASSERT_TRUE(message_to_audio(msg1, Protocol::LQ8, 700.0f, 12000.0f, audio1));
    ASSERT_TRUE(message_to_audio(msg2, Protocol::LQ8, 1600.0f, 12000.0f, audio2));

    // Superimpose waveforms
    size_t len = std::min(audio1.size(), audio2.size());
    std::vector<float> combined(len, 0.0f);
    for (size_t i = 0; i < len; ++i) {
        combined[i] = 0.5f * audio1[i] + 0.5f * audio2[i];
    }

    auto decoded_list = audio_to_messages(combined, 0.0f, 12000.0f, Protocol::LQ8);
    EXPECT_GE(decoded_list.size(), 1U);
}

TEST(TransportTest, SixSimultaneousSignalsSpreadAcrossPassbandLQ8) {
    std::vector<Message> tx_msgs;
    std::vector<float> freqs = {400.0f, 750.0f, 1100.0f, 1450.0f, 1800.0f, 2150.0f};

    // 6 distinct messages across the audio passband
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "W1AW"; m.locator = "FN31"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "HB9IPH"; m.locator = "JN47"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "YO1YO"; m.locator = "KN34"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "JA1ABC"; m.locator = "PM95"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "VK2BKL"; m.locator = "QF56"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CALL_STD; m.call_1 = "K1ABC"; m.call_2 = "DL1ABC"; m.locator = "JO62"; m.rst_db = -10; tx_msgs.push_back(m); }

    std::vector<std::vector<float>> audios(6);
    size_t min_len = 99999999;
    for (size_t i = 0; i < 6; ++i) {
        ASSERT_TRUE(message_to_audio(tx_msgs[i], Protocol::LQ8, freqs[i], 12000.0f, audios[i]));
        min_len = std::min(min_len, audios[i].size());
    }

    // Superimpose all 6 waveforms
    std::vector<float> combined(min_len, 0.0f);
    for (size_t i = 0; i < min_len; ++i) {
        for (size_t s = 0; s < 6; ++s) {
            combined[i] += (1.0f / 6.0f) * audios[s][i];
        }
    }

    // Wideband decoding (base_freq = 0.0f scans whole band)
    auto decoded_list = audio_to_messages(combined, 0.0f, 12000.0f, Protocol::LQ8);

    // Verify all 6 messages are found
    for (const auto& tx : tx_msgs) {
        bool found = false;
        for (const auto& rx : decoded_list) {
            if (rx.call_1 == tx.call_1 && rx.type == tx.type) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Failed to decode simultaneous signal: " << tx.call_1;
    }
}

TEST(TransportTest, SixSimultaneousSignalsSpreadAcrossPassbandLQ4) {
    std::vector<Message> tx_msgs;
    std::vector<float> freqs = {450.0f, 800.0f, 1150.0f, 1500.0f, 1850.0f, 2200.0f};

    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "W1AW"; m.locator = "FN31"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "HB9IPH"; m.locator = "JN47"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "YO1YO"; m.locator = "KN34"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "JA1ABC"; m.locator = "PM95"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "VK2BKL"; m.locator = "QF56"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CALL_STD; m.call_1 = "K1ABC"; m.call_2 = "DL1ABC"; m.locator = "JO62"; m.rst_db = -10; tx_msgs.push_back(m); }

    std::vector<std::vector<float>> audios(6);
    size_t min_len = 99999999;
    for (size_t i = 0; i < 6; ++i) {
        ASSERT_TRUE(message_to_audio(tx_msgs[i], Protocol::LQ4, freqs[i], 12000.0f, audios[i]));
        min_len = std::min(min_len, audios[i].size());
    }

    std::vector<float> combined(min_len, 0.0f);
    for (size_t i = 0; i < min_len; ++i) {
        for (size_t s = 0; s < 6; ++s) {
            combined[i] += (1.0f / 6.0f) * audios[s][i];
        }
    }

    auto decoded_list = audio_to_messages(combined, 0.0f, 12000.0f, Protocol::LQ4);

    for (const auto& tx : tx_msgs) {
        bool found = false;
        for (const auto& rx : decoded_list) {
            if (rx.call_1 == tx.call_1 && rx.type == tx.type) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "LQ4 failed to decode simultaneous signal: " << tx.call_1;
    }
}

TEST(TransportTest, SixSimultaneousSignalsWithDynamicRangeAndNoiseLQ8) {
    std::vector<Message> tx_msgs;
    std::vector<float> freqs = {420.0f, 780.0f, 1140.0f, 1520.0f, 1880.0f, 2260.0f};
    std::vector<float> gains = {1.0f, 0.7f, 0.5f, 0.9f, 0.6f, 0.8f};

    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "W1AW"; m.locator = "FN31"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "HB9IPH"; m.locator = "JN47"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "YO1YO"; m.locator = "KN34"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "JA1ABC"; m.locator = "PM95"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CQ_STD; m.call_1 = "VK2BKL"; m.locator = "QF56"; tx_msgs.push_back(m); }
    { Message m; m.type = MessageType::CALL_STD; m.call_1 = "K1ABC"; m.call_2 = "DL1ABC"; m.locator = "JO62"; m.rst_db = -10; tx_msgs.push_back(m); }

    std::vector<std::vector<float>> audios(6);
    size_t min_len = 99999999;
    for (size_t i = 0; i < 6; ++i) {
        ASSERT_TRUE(message_to_audio(tx_msgs[i], Protocol::LQ8, freqs[i], 12000.0f, audios[i]));
        min_len = std::min(min_len, audios[i].size());
    }

    std::mt19937 rng(1234);
    std::normal_distribution<float> noise(0.0f, 0.05f);

    std::vector<float> combined(min_len, 0.0f);
    for (size_t i = 0; i < min_len; ++i) {
        float sum = 0.0f;
        for (size_t s = 0; s < 6; ++s) {
            sum += gains[s] * audios[s][i];
        }
        combined[i] = (sum / 4.5f) + noise(rng);
    }

    auto decoded_list = audio_to_messages(combined, 0.0f, 12000.0f, Protocol::LQ8);

    int decoded_count = 0;
    for (const auto& tx : tx_msgs) {
        for (const auto& rx : decoded_list) {
            if (rx.call_1 == tx.call_1 && rx.type == tx.type) {
                decoded_count++;
                break;
            }
        }
    }
    EXPECT_GE(decoded_count, 5); // At least 5 of 6 decoded under dynamic range & noise
}

TEST(TransportTest, DirectPayloadRoundTripLQ8) {
    uint8_t payload[PAYLOAD_BYTES] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x30};
    uint8_t tones[79] = {0};

    EXPECT_TRUE(encode_payload(payload, tones, Protocol::LQ8));

    // Verify sync tones
    for (size_t i = 0; i < 7; ++i) {
        EXPECT_EQ(tones[i], COSTAS_ARRAY_8[i]);
        EXPECT_EQ(tones[36 + i], COSTAS_ARRAY_8[i]);
        EXPECT_EQ(tones[72 + i], COSTAS_ARRAY_8[i]);
    }

    uint8_t recovered[PAYLOAD_BYTES] = {0};
    EXPECT_TRUE(decode_payload(tones, recovered, Protocol::LQ8));

    for (size_t i = 0; i < 9; ++i) {
        EXPECT_EQ(recovered[i], payload[i]);
    }
    EXPECT_EQ(recovered[9] & 0xF8, payload[9] & 0xF8);
}

TEST(TransportTest, DirectPayloadRoundTripLQ4) {
    uint8_t payload[PAYLOAD_BYTES] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x00};
    uint8_t tones[105] = {0};

    EXPECT_TRUE(encode_payload(payload, tones, Protocol::LQ4));
    EXPECT_EQ(tones[0], 0);
    EXPECT_EQ(tones[104], 0);

    uint8_t recovered[PAYLOAD_BYTES] = {0};
    EXPECT_TRUE(decode_payload(tones, recovered, Protocol::LQ4));

    for (size_t i = 0; i < 9; ++i) {
        EXPECT_EQ(recovered[i], payload[i]);
    }
    EXPECT_EQ(recovered[9] & 0xF8, payload[9] & 0xF8);
}

TEST(TransportTest, DirectPayloadRoundTripLQ2) {
    uint8_t payload[PAYLOAD_BYTES] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x20};
    ToneSequence seq;
    EXPECT_TRUE(encode_payload(payload, Protocol::LQ2, seq));
    EXPECT_EQ(seq.size(), 105U);

    uint8_t recovered[PAYLOAD_BYTES] = {0};
    EXPECT_TRUE(decode_tones(seq, recovered));

    for (size_t i = 0; i < 9; ++i) {
        EXPECT_EQ(recovered[i], payload[i]);
    }
    EXPECT_EQ(recovered[9] & 0xF8, payload[9] & 0xF8);
}

TEST(TransportTest, DirectPayloadRoundTripLQ16) {
    uint8_t payload[PAYLOAD_BYTES] = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xA0};
    uint8_t tones[79] = {0};

    EXPECT_TRUE(encode_payload(payload, tones, Protocol::LQ16));

    // Verify sync tones
    for (size_t i = 0; i < 7; ++i) {
        EXPECT_EQ(tones[i], COSTAS_ARRAY_8[i]);
        EXPECT_EQ(tones[36 + i], COSTAS_ARRAY_8[i]);
        EXPECT_EQ(tones[72 + i], COSTAS_ARRAY_8[i]);
    }

    uint8_t recovered[PAYLOAD_BYTES] = {0};
    EXPECT_TRUE(decode_payload(tones, recovered, Protocol::LQ16));

    for (size_t i = 0; i < 9; ++i) {
        EXPECT_EQ(recovered[i], payload[i]);
    }
    EXPECT_EQ(recovered[9] & 0xF8, payload[9] & 0xF8);
}

TEST(TransportTest, DirectPayloadNullAndEdgeCases) {
    uint8_t payload[PAYLOAD_BYTES] = {0};
    uint8_t tones[105] = {0};

    EXPECT_FALSE(encode_payload(nullptr, tones, Protocol::LQ8));
    EXPECT_FALSE(encode_payload(payload, nullptr, Protocol::LQ8));
    EXPECT_FALSE(decode_payload(nullptr, payload, Protocol::LQ8));
    EXPECT_FALSE(decode_payload(tones, nullptr, Protocol::LQ8));

    ToneSequence corrupted_seq;
    corrupted_seq.protocol = Protocol::LQ8;
    corrupted_seq.tones.assign(79, 0); // All 0 tones -> corrupt sync
    EXPECT_FALSE(decode_tones(corrupted_seq, payload));
}

TEST(TransportTest, NullCodewordRejectInDecodeSoftLlrs) {
    // All-positive LLRs represent bit 0 in LDPC, converging to the all-zero codeword (nullspace)
    std::vector<float> bit0_llrs(174, 5.0f);
    Message msg;

    EXPECT_FALSE(decode_soft_llrs(bit0_llrs, msg, Protocol::LQ4));
    EXPECT_FALSE(decode_soft_llrs(bit0_llrs, msg, Protocol::LQ2));
    EXPECT_FALSE(decode_soft_llrs(bit0_llrs, msg, Protocol::LQ8));
    EXPECT_FALSE(decode_soft_llrs(bit0_llrs, msg, Protocol::LQ16));
}

TEST(TransportTest, NoiseDoesNotProducePhantomMessageLQ4) {
    // Generate AWGN noise audio for the duration of an LQ4 transmission
    auto params = get_protocol_params(Protocol::LQ4);
    float sample_rate = 12000.0f;
    size_t num_samples = static_cast<size_t>(std::round(params.symbol_period * sample_rate)) * params.total_symbols;
    std::vector<float> noise(num_samples);

    std::mt19937 rng(42);
    std::normal_distribution<float> dist(0.0f, 0.5f);
    for (size_t i = 0; i < num_samples; ++i) {
        noise[i] = dist(rng);
    }

    auto messages = audio_to_messages(noise, 1000.0f, sample_rate, Protocol::LQ4, 1);
    for (const auto& msg : messages) {
        // Ensure the phantom call "A37GVA" / "CALL <a5e89b>" is never decoded from noise
        EXPECT_NE(msg.call_1, "A37GVA");
        EXPECT_NE(msg.locator, "MD50");
    }
}

TEST(TransportTest, FastDecodeBaselinePerformance) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";
    std::vector<float> audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, 1200.0f, 12000.0f, audio));

    auto res = audio_to_messages(audio, 1200.0f, 12000.0f, Protocol::LQ8, 1, false);
    ASSERT_EQ(res.size(), 1U);
    EXPECT_EQ(res[0].call_1, "HB9IPH");
    EXPECT_EQ(res[0].locator, "JN47");
}

TEST(TransportTest, DecodeSoftLlrsCustomIterations) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";
    std::vector<float> audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, 1500.0f, 12000.0f, audio));

    std::vector<float> llrs;
    ASSERT_TRUE(demodulate_audio_soft(audio, 0, 1500.0f, 12000.0f, Protocol::LQ8, llrs));

    Message dec_msg;
    EXPECT_TRUE(decode_soft_llrs(llrs, dec_msg, Protocol::LQ8, 25));
    EXPECT_TRUE(decode_soft_llrs(llrs, dec_msg, Protocol::LQ8, 100));

    std::vector<float> noisy_llrs = llrs;
    for (size_t i = 0; i < 6; ++i) {
        noisy_llrs[i * 25] = -noisy_llrs[i * 25];
    }
    EXPECT_FALSE(decode_soft_llrs(noisy_llrs, dec_msg, Protocol::LQ8, 1));
    EXPECT_TRUE(decode_soft_llrs(noisy_llrs, dec_msg, Protocol::LQ8, 100));
}

TEST(TransportTest, DeepDecodeMultiPassSignalSubtractionAndThreadEquivalence) {
    Message msg1;
    msg1.type = MessageType::CQ_STD;
    msg1.call_1 = "HB9IPH";
    msg1.locator = "JN47";

    Message msg2;
    msg2.type = MessageType::CQ_STD;
    msg2.call_1 = "YO1YO";
    msg2.locator = "KN34";

    std::vector<float> audio1, audio2;
    ASSERT_TRUE(message_to_audio(msg1, Protocol::LQ8, 1000.0f, 12000.0f, audio1));
    ASSERT_TRUE(message_to_audio(msg2, Protocol::LQ8, 1400.0f, 12000.0f, audio2));

    size_t len = std::min(audio1.size(), audio2.size());
    std::vector<float> combined(len, 0.0f);
    for (size_t i = 0; i < len; ++i) {
        combined[i] = 0.8f * audio1[i] + 0.3f * audio2[i];
    }

    // Deep decode single-threaded
    auto res_deep_1 = audio_to_messages(combined, 0.0f, 12000.0f, Protocol::LQ8, 1, true);
    ASSERT_GE(res_deep_1.size(), 2U);

    bool found1 = false, found2 = false;
    for (const auto& m : res_deep_1) {
        if (m.call_1 == "HB9IPH") found1 = true;
        if (m.call_1 == "YO1YO") found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);

    // Deep decode multi-threaded (2 threads and 4 threads)
    auto res_deep_2 = audio_to_messages(combined, 0.0f, 12000.0f, Protocol::LQ8, 2, true);
    auto res_deep_4 = audio_to_messages(combined, 0.0f, 12000.0f, Protocol::LQ8, 4, true);

    EXPECT_EQ(res_deep_1.size(), res_deep_2.size());
    EXPECT_EQ(res_deep_1.size(), res_deep_4.size());
    EXPECT_EQ(res_deep_1, res_deep_2);
    EXPECT_EQ(res_deep_1, res_deep_4);
}

TEST(TransportTest, DeepDecodeWeakOverlappingSignalSubtraction) {
    Message msg_strong;
    msg_strong.type = MessageType::CQ_STD;
    msg_strong.call_1 = "W1AW";
    msg_strong.locator = "FN31";

    Message msg_weak;
    msg_weak.type = MessageType::CQ_STD;
    msg_weak.call_1 = "HB9IPH";
    msg_weak.locator = "JN47";

    std::vector<float> audio_strong, audio_weak;
    ASSERT_TRUE(message_to_audio(msg_strong, Protocol::LQ8, 1200.0f, 12000.0f, audio_strong));
    ASSERT_TRUE(message_to_audio(msg_weak, Protocol::LQ8, 1280.0f, 12000.0f, audio_weak));

    size_t len = std::min(audio_strong.size(), audio_weak.size());
    std::vector<float> combined(len, 0.0f);
    for (size_t i = 0; i < len; ++i) {
        combined[i] = 1.0f * audio_strong[i] + 0.35f * audio_weak[i];
    }

    auto res_deep = audio_to_messages(combined, 0.0f, 12000.0f, Protocol::LQ8, 1, true);
    bool found_strong = false;
    bool found_weak = false;
    for (const auto& m : res_deep) {
        if (m.call_1 == "W1AW") found_strong = true;
        if (m.call_1 == "HB9IPH") found_weak = true;
    }
    EXPECT_TRUE(found_strong);
    EXPECT_TRUE(found_weak);
}

namespace {
void apply_test_fading(std::vector<float>& audio, size_t start_sym, size_t num_syms, float factor, size_t samples_per_sym) {
    size_t start_idx = start_sym * samples_per_sym;
    size_t end_idx = std::min(audio.size(), (start_sym + num_syms) * samples_per_sym);
    for (size_t i = start_idx; i < end_idx; ++i) {
        audio[i] *= factor;
    }
}
} // namespace

TEST(TransportTest, CostasSlicedFadingBeginningMidEndAndCombinations) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    float base_f = 1200.0f;
    float fs = 12000.0f;
    auto params = get_protocol_params(Protocol::LQ8);
    size_t sps = static_cast<size_t>(std::round(params.symbol_period * fs));

    std::vector<float> clean_audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_f, fs, clean_audio));

    struct SliceScenario {
        std::string name;
        std::vector<std::pair<size_t, size_t>> slices;
    };

    std::vector<SliceScenario> scenarios = {
        {"Beg Costas Fade", {{0, 7}}},
        {"Mid Costas Fade", {{36, 7}}},
        {"End Costas Fade", {{72, 7}}},
        {"Beg + Mid Costas Fade", {{0, 7}, {36, 7}}},
        {"Mid + End Costas Fade", {{36, 7}, {72, 7}}},
        {"Beg + End Costas Fade", {{0, 7}, {72, 7}}},
    };

    for (const auto& sc : scenarios) {
        std::vector<float> audio = clean_audio;
        for (const auto& sl : sc.slices) {
            apply_test_fading(audio, sl.first, sl.second, 0.01f, sps);
        }

        // Fast mode with known base freq
        auto res_fast = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
        ASSERT_FALSE(res_fast.empty()) << "Failed fast decode on " << sc.name;
        EXPECT_EQ(res_fast[0].call_1, "HB9IPH") << "Wrong call on " << sc.name;

        // Wideband fast mode
        auto res_wb = audio_to_messages(audio, 0.0f, fs, Protocol::LQ8, 1, false);
        bool found_wb = false;
        for (const auto& m : res_wb) {
            if (m.call_1 == "HB9IPH") { found_wb = true; break; }
        }
        EXPECT_TRUE(found_wb) << "Failed wideband decode on " << sc.name;
    }
}

TEST(TransportTest, PayloadSlicedFadingBurstErasures) {
    Message msg;
    msg.type = MessageType::CALL_STD;
    msg.call_1 = "YO1YO";
    msg.call_2 = "TU2TU";
    msg.locator = "KL22";
    msg.rst_db = -3;

    float base_f = 1400.0f;
    float fs = 12000.0f;
    auto params = get_protocol_params(Protocol::LQ8);
    size_t sps = static_cast<size_t>(std::round(params.symbol_period * fs));

    std::vector<float> clean_audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_f, fs, clean_audio));

    // Test 1: Payload 1 burst erasure (10 symbols = 30 bits)
    {
        std::vector<float> audio = clean_audio;
        apply_test_fading(audio, 7, 10, 0.0f, sps);
        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
        ASSERT_FALSE(res.empty());
        EXPECT_EQ(res[0].call_1, "YO1YO");
        EXPECT_EQ(res[0].call_2, "TU2TU");
    }

    // Test 2: Payload 2 burst erasure (12 symbols = 36 bits)
    {
        std::vector<float> audio = clean_audio;
        apply_test_fading(audio, 43, 12, 0.0f, sps);
        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
        ASSERT_FALSE(res.empty());
        EXPECT_EQ(res[0].call_1, "YO1YO");
        EXPECT_EQ(res[0].call_2, "TU2TU");
    }

    // Test 3: Mixed Beg Costas + 5 symbols Payload 1 fading
    {
        std::vector<float> audio = clean_audio;
        apply_test_fading(audio, 0, 12, 0.05f, sps);
        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
        ASSERT_FALSE(res.empty());
        EXPECT_EQ(res[0].call_1, "YO1YO");
        EXPECT_EQ(res[0].call_2, "TU2TU");
    }

    // Test 4: Severe 18 symbols payload erasure (54 bits = 31% of payload) decoded in deep mode
    {
        std::vector<float> audio = clean_audio;
        apply_test_fading(audio, 7, 18, 0.0f, sps);
        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, true);
        bool found = false;
        for (const auto& m : res) {
            if (m.call_1 == "YO1YO" && m.call_2 == "TU2TU") { found = true; break; }
        }
        EXPECT_TRUE(found);
    }
}

TEST(TransportTest, CarrierFrequencyOffsetHalfBinWaterfallStress) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    float fs = 12000.0f;

    // Test at half-bin boundary (1336.0 Hz has ~3.125 Hz offset from nearest 6.25 Hz grid)
    for (float carrier_f : {1336.0f, 1339.125f, 1342.25f}) {
        std::vector<float> audio;
        ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, carrier_f, fs, audio));

        // Add moderate AWGN noise (sigma = 0.25)
        std::mt19937 rng(42);
        std::normal_distribution<float> dist(0.0f, 0.25f);
        for (float& s : audio) {
            s += dist(rng);
        }

        // Fast mode with known nominal frequency (within 10 Hz)
        auto res_fast = audio_to_messages(audio, carrier_f, fs, Protocol::LQ8, 1, false);
        ASSERT_FALSE(res_fast.empty()) << "Failed fast decode at carrier " << carrier_f;
        EXPECT_EQ(res_fast[0].call_1, "HB9IPH");

        // Fast mode wideband
        auto res_wb = audio_to_messages(audio, 0.0f, fs, Protocol::LQ8, 1, false);
        bool found_wb = false;
        for (const auto& m : res_wb) {
            if (m.call_1 == "HB9IPH") { found_wb = true; break; }
        }
        EXPECT_TRUE(found_wb) << "Failed wideband fast decode at carrier " << carrier_f;
    }
}

TEST(TransportTest, CorruptedSyncAndDataTonesRecovery) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    ToneSequence clean_seq;
    ASSERT_TRUE(encode_tones(msg, Protocol::LQ8, clean_seq));

    // Test 1: Corrupt 12 sync tones (more than half of all 21 Costas tones)
    {
        ToneSequence corrupted = clean_seq;
        for (int i = 0; i < 7; ++i) {
            corrupted[i] = (corrupted[i] + 3) % 8; // Corrupt all 7 tones of Beg Costas
        }
        for (int i = 0; i < 5; ++i) {
            corrupted[36 + i] = (corrupted[36 + i] + 2) % 8; // Corrupt 5 tones of Mid Costas
        }

        Message dec_msg;
        EXPECT_TRUE(decode_tones(corrupted, dec_msg));
        EXPECT_EQ(dec_msg.call_1, "HB9IPH");
    }

    // Test 2: Corrupt 3 data tones in payload (exercising LDPC deep fallback)
    {
        ToneSequence corrupted = clean_seq;
        const int data_indices[] = {8, 25, 52};
        for (int idx : data_indices) {
            corrupted[idx] = (corrupted[idx] + 1) % 8;
        }

        Message dec_msg;
        EXPECT_TRUE(decode_tones(corrupted, dec_msg));
        EXPECT_EQ(dec_msg.call_1, "HB9IPH");
    }
}

TEST(TransportTest, MultiProtocolSlicedFadingResilience) {
    for (Protocol proto : {Protocol::LQ4, Protocol::LQ2, Protocol::LQ16}) {
        Message msg;
        msg.type = MessageType::CQ_STD;
        msg.call_1 = "HB9IPH";
        msg.locator = "JN47";

        float base_f = 1200.0f;
        float fs = 12000.0f;
        auto params = get_protocol_params(proto);
        size_t sps = static_cast<size_t>(std::round(params.symbol_period * fs));

        std::vector<float> clean_audio;
        ASSERT_TRUE(message_to_audio(msg, proto, base_f, fs, clean_audio));

        // Test Beginning Sync Fading
        {
            std::vector<float> audio = clean_audio;
            size_t sync1_sym = (proto == Protocol::LQ4 || proto == Protocol::LQ2) ? 1 : 0;
            size_t sync1_len = (proto == Protocol::LQ4 || proto == Protocol::LQ2) ? 4 : 7;
            apply_test_fading(audio, sync1_sym, sync1_len, 0.0f, sps);

            auto res = audio_to_messages(audio, base_f, fs, proto, 1, false);
            ASSERT_FALSE(res.empty()) << "Failed beg sync fade for " << params.name;
            EXPECT_EQ(res[0].call_1, "HB9IPH");
        }

        // Test Mid Sync Fading
        {
            std::vector<float> audio = clean_audio;
            size_t sync2_sym = (proto == Protocol::LQ4 || proto == Protocol::LQ2) ? 34 : 36;
            size_t sync2_len = (proto == Protocol::LQ4 || proto == Protocol::LQ2) ? 4 : 7;
            apply_test_fading(audio, sync2_sym, sync2_len, 0.0f, sps);

            auto res = audio_to_messages(audio, base_f, fs, proto, 1, false);
            ASSERT_FALSE(res.empty()) << "Failed mid sync fade for " << params.name;
            EXPECT_EQ(res[0].call_1, "HB9IPH");
        }

        // Test Payload Fading (10 symbols erased)
        {
            std::vector<float> audio = clean_audio;
            size_t pay_sym = (proto == Protocol::LQ4 || proto == Protocol::LQ2) ? 5 : 7;
            apply_test_fading(audio, pay_sym, 10, 0.0f, sps);

            auto res = audio_to_messages(audio, base_f, fs, proto, 1, false);
            ASSERT_FALSE(res.empty()) << "Failed payload fade for " << params.name;
            EXPECT_EQ(res[0].call_1, "HB9IPH");
        }
    }
}

TEST(TransportTest, ImpossibleSevereFadingRejectionAndZeroFalsePositives) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    float base_f = 1200.0f;
    float fs = 12000.0f;
    auto params = get_protocol_params(Protocol::LQ8);
    size_t sps = static_cast<size_t>(std::round(params.symbol_period * fs));

    std::vector<float> clean_audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_f, fs, clean_audio));

    // Test 1: 26 payload symbols erased (44% of total codeword, exceeds channel capacity)
    {
        std::vector<float> audio = clean_audio;
        apply_test_fading(audio, 7, 26, 0.0f, sps);
        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
        EXPECT_TRUE(res.empty()) << "Should cleanly fail on 26 erased symbols";
    }

    // Test 2: 29 payload symbols erased (entire Payload Block 1 completely zeroed)
    {
        std::vector<float> audio = clean_audio;
        apply_test_fading(audio, 7, 29, 0.0f, sps);
        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, true); // Deep mode
        EXPECT_TRUE(res.empty()) << "Should cleanly fail in deep mode when Block 1 is completely wiped";
    }

    // Test 3: Complete erasure of all 3 Costas blocks (pure silence on sync)
    {
        std::vector<float> audio = clean_audio;
        apply_test_fading(audio, 0, 7, 0.0f, sps);
        apply_test_fading(audio, 36, 7, 0.0f, sps);
        apply_test_fading(audio, 72, 7, 0.0f, sps);
        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, true);
        EXPECT_TRUE(res.empty()) << "Should cleanly fail when all sync is wiped";
    }

    // Test 4: Heavy AWGN noise (sigma = 2.0) with 20 erased payload symbols
    {
        std::vector<float> audio = clean_audio;
        apply_test_fading(audio, 7, 20, 0.0f, sps);
        std::mt19937 rng(9999);
        std::normal_distribution<float> dist(0.0f, 2.0f);
        for (float& s : audio) s += dist(rng);

        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
        // If it fails, must be empty; if it decodes, must NEVER be a false positive
        for (const auto& m : res) {
            EXPECT_EQ(m.call_1, "HB9IPH") << "Phantom message produced under heavy noise!";
        }
    }
}

TEST(TransportTest, IntenseFadingNoiseTradeoffBreakdownCurve) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    float base_f = 1336.0f;
    float fs = 12000.0f;
    auto params = get_protocol_params(Protocol::LQ8);
    size_t sps = static_cast<size_t>(std::round(params.symbol_period * fs));

    std::vector<float> clean_audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_f, fs, clean_audio));

    std::mt19937 rng(4321);

    // 1. Moderate fading (15 erased syms, sigma 0.4): high success rate expected
    {
        int decoded = 0;
        constexpr int TRIALS = 10;
        for (int t = 0; t < TRIALS; ++t) {
            std::vector<float> audio = clean_audio;
            apply_test_fading(audio, 7, 15, 0.0f, sps);
            std::normal_distribution<float> dist(0.0f, 0.4f);
            for (float& s : audio) s += dist(rng);

            auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
            for (const auto& m : res) {
                EXPECT_EQ(m.call_1, "HB9IPH") << "Wrong message decoded!";
                if (m.call_1 == "HB9IPH") ++decoded;
            }
        }
        EXPECT_GE(decoded, 8) << "Pass rate too low on moderate fading";
    }

    // 2. Near-capacity fading (22 erased syms, sigma 0.5): graceful degradation
    {
        int decoded = 0;
        int false_positives = 0;
        constexpr int TRIALS = 10;
        for (int t = 0; t < TRIALS; ++t) {
            std::vector<float> audio = clean_audio;
            apply_test_fading(audio, 7, 22, 0.0f, sps);
            std::normal_distribution<float> dist(0.0f, 0.5f);
            for (float& s : audio) s += dist(rng);

            auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
            for (const auto& m : res) {
                if (m.call_1 == "HB9IPH") ++decoded;
                else ++false_positives;
            }
        }
        EXPECT_GT(decoded, 0) << "Should recover some near-capacity frames";
        EXPECT_EQ(false_positives, 0) << "Zero false positives strictly required!";
    }

    // 3. Beyond capacity (26 erased syms, sigma 0.8): strictly 0 decodes and 0 false positives
    {
        int false_positives = 0;
        int decoded = 0;
        constexpr int TRIALS = 10;
        for (int t = 0; t < TRIALS; ++t) {
            std::vector<float> audio = clean_audio;
            apply_test_fading(audio, 7, 26, 0.0f, sps);
            std::normal_distribution<float> dist(0.0f, 0.8f);
            for (float& s : audio) s += dist(rng);

            auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
            for (const auto& m : res) {
                if (m.call_1 == "HB9IPH") ++decoded;
                else ++false_positives;
            }
        }
        EXPECT_EQ(decoded, 0) << "Should be impossible beyond capacity";
        EXPECT_EQ(false_positives, 0) << "No false positives allowed!";
    }
}

TEST(TransportTest, PureNoiseMultiTrialZeroPhantomGuarantee) {
    float fs = 12000.0f;
    auto params = get_protocol_params(Protocol::LQ8);
    size_t sps = static_cast<size_t>(std::round(params.symbol_period * fs));
    size_t total_samples = sps * params.total_symbols;

    std::mt19937 rng(777);
    int total_noise_trials = 50;

    for (int t = 0; t < total_noise_trials; ++t) {
        float sigma = 0.2f + static_cast<float>(t % 8) * 0.25f; // sigma: 0.2 to 2.0
        std::normal_distribution<float> dist(0.0f, sigma);
        std::vector<float> noise(total_samples);
        for (size_t i = 0; i < total_samples; ++i) noise[i] = dist(rng);

        // Fast mode wideband across passband
        auto res_fast = audio_to_messages(noise, 0.0f, fs, Protocol::LQ8, 1, false);
        EXPECT_TRUE(res_fast.empty()) << "Phantom message detected in Fast mode on pure noise!";

        // Deep mode wideband across passband
        auto res_deep = audio_to_messages(noise, 0.0f, fs, Protocol::LQ8, 1, true);
        EXPECT_TRUE(res_deep.empty()) << "Phantom message detected in Deep mode on pure noise!";
    }
}

TEST(TransportTest, DeepCostasDestructionWithNoisyPayload) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    float base_f = 1200.0f;
    float fs = 12000.0f;
    auto params = get_protocol_params(Protocol::LQ8);
    size_t sps = static_cast<size_t>(std::round(params.symbol_period * fs));

    std::vector<float> clean_audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_f, fs, clean_audio));

    // Destroy 2 out of 3 Costas blocks completely (Beginning and Mid wiped)
    std::vector<float> audio = clean_audio;
    apply_test_fading(audio, 0, 7, 0.0f, sps);
    apply_test_fading(audio, 36, 7, 0.0f, sps);

    // Add noise to payload
    std::mt19937 rng(5555);
    std::normal_distribution<float> dist(0.0f, 0.3f);
    for (float& s : audio) s += dist(rng);

    // End Costas (72..78) is the sole survivor (7 tones match)
    auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
    ASSERT_FALSE(res.empty()) << "Should synchronize on single intact End Costas block";
    EXPECT_EQ(res[0].call_1, "HB9IPH");
}

TEST(TransportTest, ContinuousStreamEnvelopeQsbFadingBreakdown) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    float base_f = 1200.0f;
    float fs = 12000.0f;

    std::vector<float> clean_audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_f, fs, clean_audio));
    size_t N = clean_audio.size();

    std::mt19937 rng(4321);

    // Test continuous sinusoidal QSB envelope fading across the 12.64s audio stream
    // Fading depth 0.85 dips signal power by -16.5 dB periodically
    float depth = 0.85f;
    float f_fade = 0.25f; // One fade cycle every 4 seconds

    // 1. Moderate SNR (+5 dB): 100% decode expected in both modes
    {
        float sigma = std::sqrt(0.5f / std::pow(10.0f, 5.0f / 10.0f));
        std::vector<float> audio(N);
        for (size_t i = 0; i < N; ++i) {
            float t = static_cast<float>(i) / fs;
            float envelope = (1.0f - depth) + depth * 0.5f * (1.0f + std::cos(2.0f * 3.14159265f * f_fade * t));
            audio[i] = clean_audio[i] * envelope;
        }
        std::normal_distribution<float> dist(0.0f, sigma);
        for (float& s : audio) s += dist(rng);

        auto res_fast = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
        ASSERT_FALSE(res_fast.empty());
        EXPECT_EQ(res_fast[0].call_1, "HB9IPH");

        auto res_deep = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, true);
        ASSERT_FALSE(res_deep.empty());
        EXPECT_EQ(res_deep[0].call_1, "HB9IPH");
    }

    // 2. Severe SNR (-15 dB) under deep QSB: Deep mode recovers signal; zero false positives
    {
        float sigma = std::sqrt(0.5f / std::pow(10.0f, -15.0f / 10.0f));
        int deep_decoded = 0;
        int false_positives = 0;
        constexpr int TRIALS = 5;

        for (int t = 0; t < TRIALS; ++t) {
            float phi = static_cast<float>(t) * 1.25f;
            std::vector<float> audio(N);
            for (size_t i = 0; i < N; ++i) {
                float time = static_cast<float>(i) / fs;
                float envelope = (1.0f - depth) + depth * 0.5f * (1.0f + std::cos(2.0f * 3.14159265f * f_fade * time + phi));
                audio[i] = clean_audio[i] * envelope;
            }
            std::normal_distribution<float> dist(0.0f, sigma);
            for (float& s : audio) s += dist(rng);

            auto res_deep = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, true);
            for (const auto& m : res_deep) {
                if (m.call_1 == "HB9IPH") ++deep_decoded;
                else ++false_positives;
            }
        }
        EXPECT_GT(deep_decoded, 0) << "Deep mode should recover deep QSB signals at -15 dB SNR";
        EXPECT_EQ(false_positives, 0) << "No false positives allowed under deep QSB + noise";
    }

    // 3. Impossible SNR (-25 dB) under deep QSB: clean rejection, zero false positives
    {
        float sigma = std::sqrt(0.5f / std::pow(10.0f, -25.0f / 10.0f));
        int false_positives = 0;
        constexpr int TRIALS = 5;

        for (int t = 0; t < TRIALS; ++t) {
            std::vector<float> audio(N);
            for (size_t i = 0; i < N; ++i) {
                float time = static_cast<float>(i) / fs;
                float envelope = (1.0f - depth) + depth * 0.5f * (1.0f + std::cos(2.0f * 3.14159265f * f_fade * time));
                audio[i] = clean_audio[i] * envelope;
            }
            std::normal_distribution<float> dist(0.0f, sigma);
            for (float& s : audio) s += dist(rng);

            auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
            for (const auto& m : res) {
                if (m.call_1 != "HB9IPH") ++false_positives;
            }
        }
        EXPECT_EQ(false_positives, 0) << "No false positives allowed at -25 dB SNR";
    }
}

TEST(TransportTest, ContinuousStreamMultipathRayleighFading) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    float base_f = 1200.0f;
    float fs = 12000.0f;

    std::vector<float> clean_audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_f, fs, clean_audio));
    size_t N = clean_audio.size();

    // Two-ray multipath with 2.0 ms delay spread and equal amplitude (creates frequency-selective nulls)
    size_t delay_samples = static_cast<size_t>(std::round(0.002f * fs));
    for (float dphase : {0.0f, 0.785f, 1.57f, 3.14159f}) {
        std::vector<float> audio(N, 0.0f);
        for (size_t i = 0; i < N; ++i) {
            float s1 = clean_audio[i];
            float s2 = (i >= delay_samples) ? clean_audio[i - delay_samples] : 0.0f;
            audio[i] = s1 + std::cos(dphase) * s2;
        }

        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
        ASSERT_FALSE(res.empty()) << "Failed to decode multipath stream with dphase " << dphase;
        EXPECT_EQ(res[0].call_1, "HB9IPH");
    }
}

TEST(TransportTest, ContinuousStreamExtremeNoiseThreshold) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.call_1 = "HB9IPH";
    msg.locator = "JN47";

    float base_f = 1200.0f;
    float fs = 12000.0f;

    std::vector<float> clean_audio;
    ASSERT_TRUE(message_to_audio(msg, Protocol::LQ8, base_f, fs, clean_audio));
    size_t N = clean_audio.size();

    std::mt19937 rng(8888);

    // 1. Near threshold (-18 dB SNR): reliable decode without false positives
    {
        float sigma = std::sqrt(0.5f / std::pow(10.0f, -18.0f / 10.0f));
        std::vector<float> audio = clean_audio;
        std::normal_distribution<float> dist(0.0f, sigma);
        for (float& s : audio) s += dist(rng);

        auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, false);
        ASSERT_FALSE(res.empty());
        EXPECT_EQ(res[0].call_1, "HB9IPH");
    }

    // 2. Impossible SNR (-30 dB, buried 30 dB below noise): strictly empty, zero false positives
    {
        float sigma = std::sqrt(0.5f / std::pow(10.0f, -30.0f / 10.0f));
        int false_positives = 0;
        constexpr int TRIALS = 5;

        for (int t = 0; t < TRIALS; ++t) {
            std::vector<float> audio = clean_audio;
            std::normal_distribution<float> dist(0.0f, sigma);
            for (float& s : audio) s += dist(rng);

            auto res = audio_to_messages(audio, base_f, fs, Protocol::LQ8, 1, true); // Deep mode
            for (const auto& m : res) {
                if (m.call_1 != "HB9IPH") ++false_positives;
            }
        }
        EXPECT_EQ(false_positives, 0) << "Zero false positives strictly required at -30 dB SNR";
    }
}


