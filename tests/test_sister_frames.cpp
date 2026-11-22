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

// Sister frames: LQ4 / LQ2 are the FT4 / FT2 frame (same whitening, Gray map, LDPC and CRC-14) with their own four
// sync ramps, and 8 of the 16 ramp symbols are the same as FT4's. An FT4 / FT2 frame therefore used to pass the
// LQ4 / LQ2 sync check (>= 5 of 16) and decode with a valid CRC: its FT message read through the LQ message layer
// as a made-up station ("CQ 1SGX 00BVM/P IO91") or a reserved type. The LQ4 / LQ2 arrays changed on 2026-11-22 (constants.h) share no ramp
// symbol with FT4's at that alignment. These tests check that the tone and audio decoders refuse a frame whose ramps are
// FT4's, that the waterfall sync search scores it 0, and that LQ's own frames are still read.

#include <gtest/gtest.h>
#include "lq/transport.h"
#include "lq/message.h"
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using namespace lq;

namespace {

const int kRampStarts[4] = {1, 34, 67, 100};
const uint8_t kFt4Ramps[4][4] = {{0, 1, 3, 2}, {1, 0, 2, 3}, {2, 3, 1, 0}, {3, 2, 0, 1}};

// 77-bit FT-protocol payloads (the FT family's own packing): "CQ G0CHM IO91", "CQ SP9XYZ KO00",
// "G0CHM M0ABC IO91", "G0CHM M0ABC RR73".
const char* kFtPayloads[] = {"0000002047CA1C0F84C8", "00000026768B7F12F208", "08F9438050C9870F84C8",
                             "08F9438050C9871F9D48"};

void hex_to_payload(const char* hex, uint8_t p[PAYLOAD_BYTES]) {
    for (int i = 0; i < PAYLOAD_BYTES; ++i) {
        unsigned v = 0;
        std::sscanf(hex + 2 * i, "%2x", &v);
        p[i] = static_cast<uint8_t>(v);
    }
}

// The FT4 / FT2 frame of these 77 bits: LQ's frame with FT4's ramps.
ToneSequence sister_frame(const uint8_t payload[PAYLOAD_BYTES], Protocol proto) {
    ToneSequence seq;
    EXPECT_TRUE(encode_payload(payload, proto, seq));
    for (int r = 0; r < 4; ++r)
        for (int k = 0; k < 4; ++k) seq.tones[kRampStarts[r] + k] = kFt4Ramps[r][k];
    return seq;
}

std::vector<Message> lq_messages() {
    std::vector<Message> out;
    const char* texts[] = {"CQ HB9IPH JN47", "CQ POTA G0CHM IO91", "HB9IPH G0CHM IO91 -10", "HB9IPH G0CHM R-10",
                           "HB9IPH G0CHM 73", "CQ 2E0KLM IO92"};
    for (const char* t : texts) {
        Message m;
        EXPECT_TRUE(parse_message(t, m)) << t;
        out.push_back(m);
    }
    return out;
}

// A slot of audio: 0.5 s of quiet noise, the frame, 0.5 s of quiet noise.
std::vector<float> slot_audio(const ToneSequence& seq, float freq, uint32_t seed) {
    std::vector<float> frame;
    generate_audio(seq, freq, 12000.0f, frame);
    std::vector<float> audio(frame.size() + 12000, 0.0f);
    std::mt19937 rng(seed);
    std::normal_distribution<float> n(0.0f, 0.001f);
    for (auto& s : audio) s = n(rng);
    for (size_t i = 0; i < frame.size(); ++i) audio[6000 + i] += frame[i];
    return audio;
}

// A clean magnitude waterfall of a tone sequence: one row per symbol, 4 bins, the sent tone strong.
std::vector<uint8_t> waterfall(const ToneSequence& seq) {
    std::vector<uint8_t> mag(seq.size() * 4, 20);
    for (size_t t = 0; t < seq.size(); ++t) mag[t * 4 + seq[t]] = 220;
    return mag;
}

}  // namespace

TEST(SisterFramesTest, HardTonesFtRampsAreRefused) {
    for (Protocol proto : {Protocol::LQ4, Protocol::LQ2}) {
        for (const char* hex : kFtPayloads) {
            uint8_t p[PAYLOAD_BYTES];
            hex_to_payload(hex, p);
            Message m;
            EXPECT_FALSE(decode_tones(sister_frame(p, proto), m)) << hex << " read as " << format_message(m);
        }
        for (const Message& lq : lq_messages()) {
            uint8_t p[PAYLOAD_BYTES];
            ASSERT_TRUE(encode_message(lq, p));
            Message m;
            EXPECT_FALSE(decode_tones(sister_frame(p, proto), m)) << "an LQ message with FT4's ramps";
            ToneSequence own;
            ASSERT_TRUE(encode_payload(p, proto, own));
            Message back;
            ASSERT_TRUE(decode_tones(own, back));
            EXPECT_EQ(format_message(back), format_message(lq));
        }
    }
}

TEST(SisterFramesTest, AudioFtRampsAreRefused) {
    for (Protocol proto : {Protocol::LQ4, Protocol::LQ2}) {
        uint32_t seed = 1;
        for (const char* hex : kFtPayloads) {
            uint8_t p[PAYLOAD_BYTES];
            hex_to_payload(hex, p);
            auto got = audio_to_messages(slot_audio(sister_frame(p, proto), 1000.0f, seed++), 0.0f, 12000.0f, proto, 1, true);
            std::string texts;
            for (const auto& m : got) texts += format_message(m) + "; ";
            EXPECT_TRUE(got.empty()) << hex << " read as " << texts;
        }
        for (const Message& lq : lq_messages()) {
            ToneSequence own;
            ASSERT_TRUE(encode_tones(lq, proto, own));
            auto got = audio_to_messages(slot_audio(own, 1000.0f, seed++), 0.0f, 12000.0f, proto, 1, true);
            bool found = false;
            for (const auto& m : got) found = found || format_message(m) == format_message(lq);
            EXPECT_TRUE(found) << "LQ's own " << format_message(lq);
        }
    }
}

TEST(SisterFramesTest, WaterfallFtRampsScoreNothing) {
    for (Protocol proto : {Protocol::LQ4, Protocol::LQ2}) {
        for (const char* hex : kFtPayloads) {
            uint8_t p[PAYLOAD_BYTES];
            hex_to_payload(hex, p);
            auto mag = waterfall(sister_frame(p, proto));
            // the sync search's score of an FT4 / FT2 frame at its own place: none of its ramp symbols is LQ's (the earlier arrays: 8 of 16)
            EXPECT_EQ(lq_sync_score(mag.data(), 105, 4, 0, 0, proto), 0) << hex;
        }
        for (const Message& lq : lq_messages()) {
            ToneSequence own;
            ASSERT_TRUE(encode_tones(lq, proto, own));
            auto mag = waterfall(own);
            EXPECT_GT(lq_sync_score(mag.data(), 105, 4, 0, 0, proto), 0);
            float llrs[LDPC_CODEWORD_BITS];
            EXPECT_TRUE(lq_extract_llrs_from_waterfall(mag.data(), 105, 4, 0, 0, proto, llrs));
            std::vector<float> v(llrs, llrs + LDPC_CODEWORD_BITS);
            Message back;
            ASSERT_TRUE(decode_soft_llrs(v, back, proto, 50));
            EXPECT_EQ(format_message(back), format_message(lq));
        }
    }
}

TEST(SisterFramesTest, Lq8IsUnchanged) {
    for (const Message& lq : lq_messages()) {
        ToneSequence own;
        ASSERT_TRUE(encode_tones(lq, Protocol::LQ8, own));
        Message back;
        ASSERT_TRUE(decode_tones(own, back));
        EXPECT_EQ(format_message(back), format_message(lq));
    }
}
