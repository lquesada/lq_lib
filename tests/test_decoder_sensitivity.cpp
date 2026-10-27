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

// Weak-signal sensitivity of the multi-signal decoder on AWGN, one slot per frame: a signal at a random audio
// frequency and start time near the slot start, white noise scaled to the SNR in 2500 Hz. Deterministic seeds.

#include <gtest/gtest.h>
#include "lq/transport.h"
#include "lq/easy.h"
#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace {

using lq::Protocol;

const char* const MESSAGES[] = {
    "CQ HB9IPH JN47", "CQ YO1YO KN24", "W1AW HB9IPH FN31 +05", "TU2TU HB9IPH IJ85 +00",
    "K1JT HB9IPH FN20 -10", "CQ POTA HB9IPH JN47", "CQ DX HB9IPH JN47", "YO1YO HB9IPH JN47 -26",
};

struct Slot {
    std::vector<float> audio;
    std::string text;
};

Slot make_slot(Protocol proto, const char* text, float snr_db, unsigned seed, bool with_signal) {
    const float sr = 12000.0f;
    const auto params = lq::get_protocol_params(proto);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> uf(600.0f, 2200.0f), ut(0.1f, 0.6f);
    const float f0 = uf(rng);
    const float start = ut(rng) * params.symbol_period * 10.0f;
    lq::Message msg;
    EXPECT_TRUE(lq::parse_message(text, msg));
    std::vector<float> sig;
    EXPECT_TRUE(lq::message_to_audio(msg, proto, f0, sr, sig));
    Slot slot;
    slot.text = lq::format_message(msg);
    slot.audio.assign(static_cast<size_t>(params.slot_duration * sr), 0.0f);
    const size_t off = static_cast<size_t>(start * sr);
    double ps = 0.0;
    for (float x : sig) ps += static_cast<double>(x) * x;
    ps /= static_cast<double>(sig.size());
    if (with_signal) {
        for (size_t i = 0; i < sig.size() && off + i < slot.audio.size(); ++i) slot.audio[off + i] = sig[i];
    }
    const double noise_var = ps / std::pow(10.0, snr_db / 10.0) * (sr / 2.0) / 2500.0;
    std::normal_distribution<double> n(0.0, std::sqrt(noise_var));
    for (float& x : slot.audio) x += static_cast<float>(n(rng));
    return slot;
}

// Frames decoded (and wrong messages seen) out of `frames` slots at `snr_db`.
std::pair<int, int> run(Protocol proto, float snr_db, int frames, bool with_signal) {
    int ok = 0, wrong = 0;
    for (int i = 0; i < frames; ++i) {
        Slot slot = make_slot(proto, MESSAGES[i % 8], snr_db, 1000u + static_cast<unsigned>(i) * 7919u, with_signal);
        auto decoded = lq::audio_to_messages(slot.audio, 0.0f, 12000.0f, proto, 2, true);
        for (const auto& m : decoded) {
            if (with_signal && lq::format_message(m) == slot.text) ++ok; else ++wrong;
        }
    }
    return {ok, wrong};
}

}  // namespace

// One dB above each protocol's threshold (1.5 dB for LQ4, the 4-tone mode closest to its limit) the deep decoder
// recovers most frames; at the threshold itself it decodes more than half of them (AWGN, 196 frames per protocol).
TEST(DecoderSensitivity, LQ8OneDbAboveThreshold) {
    auto r = run(Protocol::LQ8, -20.0f, 8, true);
    EXPECT_GE(r.first, 6);
    EXPECT_EQ(r.second, 0);
}

TEST(DecoderSensitivity, LQ16OneDbAboveThreshold) {
    auto r = run(Protocol::LQ16, -23.0f, 8, true);
    EXPECT_GE(r.first, 6);
    EXPECT_EQ(r.second, 0);
}

TEST(DecoderSensitivity, LQ4OneAndAHalfDbAboveThreshold) {
    auto r = run(Protocol::LQ4, -16.0f, 16, true);
    EXPECT_GE(r.first, 12);
    EXPECT_EQ(r.second, 0);
}

TEST(DecoderSensitivity, LQ2OneDbAboveThreshold) {
    auto r = run(Protocol::LQ2, -13.0f, 8, true);
    EXPECT_GE(r.first, 6);
    EXPECT_EQ(r.second, 0);
}

TEST(DecoderSensitivity, NoiseOnlyGivesNoDecodes) {
    for (Protocol proto : {Protocol::LQ8, Protocol::LQ4, Protocol::LQ2}) {
        auto r = run(proto, lq::get_protocol_params(proto).threshold_snr, 4, false);
        EXPECT_EQ(r.second, 0) << lq::get_protocol_params(proto).name;
    }
}
