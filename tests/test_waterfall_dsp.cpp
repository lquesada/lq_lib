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
#include "lq/constants.h"
#include "lq/message.h"
#include "lq/c_api.h"
#include <vector>
#include <cmath>
#include <cstring>

using namespace lq;

TEST(WaterfallDspTest, SyncScoreCostasCorrelationLQ8) {
    int num_blocks = 100;
    int block_stride = 256;
    std::vector<uint8_t> mag(num_blocks * block_stride, 10); // Background noise = 10

    // Inject clean LQ8 signal at time_offset = 10, freq_offset = 50
    ToneSequence seq;
    Message msg;
    parse_message("CALL YO1YO HB9IPH JN47 -03", msg);
    encode_tones(msg, Protocol::LQ8, seq);

    for (size_t t = 0; t < seq.tones.size(); ++t) {
        int block = 10 + static_cast<int>(t);
        int bin = 50 + seq.tones[t];
        mag[block * block_stride + bin] = 200; // Strong tone magnitude
    }

    int on_target_score = lq_sync_score(mag.data(), num_blocks, block_stride, 10, 50, Protocol::LQ8);
    int off_target_time = lq_sync_score(mag.data(), num_blocks, block_stride, 15, 50, Protocol::LQ8);
    int off_target_freq = lq_sync_score(mag.data(), num_blocks, block_stride, 10, 60, Protocol::LQ8);

    EXPECT_GT(on_target_score, 1000);
    EXPECT_GT(on_target_score, off_target_time * 2);
    EXPECT_GT(on_target_score, off_target_freq * 2);
}

TEST(WaterfallDspTest, SyncScoreCostasCorrelationLQ4) {
    int num_blocks = 120;
    int block_stride = 256;
    std::vector<uint8_t> mag(num_blocks * block_stride, 10);

    ToneSequence seq;
    Message msg;
    parse_message("CALL YO1YO HB9IPH JN47 -03", msg);
    encode_tones(msg, Protocol::LQ4, seq);

    for (size_t t = 0; t < seq.tones.size(); ++t) {
        int block = 5 + static_cast<int>(t);
        int bin = 80 + seq.tones[t];
        mag[block * block_stride + bin] = 220;
    }

    int on_target_score = lq_sync_score(mag.data(), num_blocks, block_stride, 5, 80, Protocol::LQ4);
    int off_target = lq_sync_score(mag.data(), num_blocks, block_stride, 20, 80, Protocol::LQ4);

    EXPECT_GT(on_target_score, 800);
    EXPECT_GT(on_target_score, off_target * 2);
}

TEST(WaterfallDspTest, SyncScoreCostasCorrelationLQ2) {
    int num_blocks = 120;
    int block_stride = 256;
    std::vector<uint8_t> mag(num_blocks * block_stride, 10);

    ToneSequence seq;
    Message msg;
    parse_message("CALL YO1YO HB9IPH JN47 -03", msg);
    encode_tones(msg, Protocol::LQ2, seq);

    for (size_t t = 0; t < seq.tones.size(); ++t) {
        int block = 5 + static_cast<int>(t);
        int bin = 80 + seq.tones[t];
        mag[block * block_stride + bin] = 220;
    }

    int on_target_score = lq_sync_score(mag.data(), num_blocks, block_stride, 5, 80, Protocol::LQ2);
    int off_target = lq_sync_score(mag.data(), num_blocks, block_stride, 20, 80, Protocol::LQ2);

    EXPECT_GT(on_target_score, 800);
    EXPECT_GT(on_target_score, off_target * 2);
}

TEST(WaterfallDspTest, SyncScoreCostasCorrelationLQ16) {
    int num_blocks = 100;
    int block_stride = 256;
    std::vector<uint8_t> mag(num_blocks * block_stride, 10);

    ToneSequence seq;
    Message msg;
    parse_message("CALL YO1YO HB9IPH JN47 -03", msg);
    encode_tones(msg, Protocol::LQ16, seq);

    for (size_t t = 0; t < seq.tones.size(); ++t) {
        int block = 10 + static_cast<int>(t);
        int bin = 50 + seq.tones[t];
        mag[block * block_stride + bin] = 200;
    }

    int on_target_score = lq_sync_score(mag.data(), num_blocks, block_stride, 10, 50, Protocol::LQ16);
    int off_target_time = lq_sync_score(mag.data(), num_blocks, block_stride, 15, 50, Protocol::LQ16);
    int off_target_freq = lq_sync_score(mag.data(), num_blocks, block_stride, 10, 60, Protocol::LQ16);

    EXPECT_GT(on_target_score, 1000);
    EXPECT_GT(on_target_score, off_target_time * 2);
    EXPECT_GT(on_target_score, off_target_freq * 2);
}

TEST(WaterfallDspTest, ParabolicFrequencyRefinement) {
    int num_blocks = 100;
    int block_stride = 256;
    std::vector<uint8_t> mag(num_blocks * block_stride, 10);

    ToneSequence seq;
    Message msg;
    parse_message("CQ HB9IPH JN47", msg);
    encode_tones(msg, Protocol::LQ8, seq);

    for (size_t t = 0; t < seq.tones.size(); ++t) {
        int block = 10 + static_cast<int>(t);
        int bin = 100 + seq.tones[t];
        mag[block * block_stride + bin] = 240;
        mag[block * block_stride + bin - 1] = 60;
        mag[block * block_stride + bin + 1] = 70;
    }

    int out_score = 0;
    float refined_hz = lq_refine_frequency(mag.data(), num_blocks, block_stride, 10, 100, 0, 1, 0.160f, Protocol::LQ8, &out_score);

    EXPECT_GT(out_score, 1000);
    EXPECT_NEAR(refined_hz, 100.0f * 6.25f, 5.0f);
}

TEST(WaterfallDspTest, SnrEstimation) {
    int num_blocks = 100;
    int block_stride = 256;
    std::vector<float> mag2(num_blocks * block_stride, 1.0f); // Noise power = 1.0

    ToneSequence seq;
    Message msg;
    parse_message("CQ HB9IPH JN47", msg);
    encode_tones(msg, Protocol::LQ8, seq);

    for (size_t t = 0; t < seq.tones.size(); ++t) {
        int block = 10 + static_cast<int>(t);
        int bin = 50 + seq.tones[t];
        mag2[block * block_stride + bin] = 100.0f; // Signal power = 100
    }

    float snr_db = lq_guess_snr(mag2.data(), num_blocks, block_stride, 10, 50, Protocol::LQ8);
    EXPECT_GT(snr_db, -25.0f);
    EXPECT_LT(snr_db, 25.0f);
}

TEST(WaterfallDspTest, DirectLlrExtractionAndDecode) {
    int num_blocks = 90;
    int block_stride = 256;
    std::vector<uint8_t> mag(num_blocks * block_stride, 5);

    ToneSequence seq;
    Message orig_msg;
    parse_message("CALL YO1YO HB9IPH JN47 -03", orig_msg);
    encode_tones(orig_msg, Protocol::LQ8, seq);

    for (size_t t = 0; t < seq.tones.size(); ++t) {
        int block = 5 + static_cast<int>(t);
        int bin = 60 + seq.tones[t];
        mag[block * block_stride + bin] = 200;
    }

    float llrs[174];
    EXPECT_TRUE(lq_extract_llrs_from_waterfall(mag.data(), num_blocks, block_stride, 5, 60, Protocol::LQ8, llrs));

    std::vector<float> llr_vec(llrs, llrs + 174);
    Message dec_msg;
    EXPECT_TRUE(decode_soft_llrs(llr_vec, dec_msg, Protocol::LQ8));
    EXPECT_STREQ(dec_msg.get_sender_call().c_str(), "HB9IPH");
    EXPECT_STREQ(dec_msg.get_target_call().c_str(), "YO1YO");
    EXPECT_STREQ(dec_msg.locator.c_str(), "JN47");
    EXPECT_EQ(dec_msg.rst_db, -3);
}

TEST(WaterfallDspTest, SignalSubtraction) {
    int block_size = 1920; // 0.160s @ 12000 Hz
    int block_stride = 256;
    int num_blocks = 90;
    std::vector<uint8_t> mag(num_blocks * block_stride, 10);

    Message msg;
    parse_message("CQ HB9IPH JN47", msg);
    uint8_t payload[10];
    encode_message(msg, payload);

    ToneSequence seq;
    encode_payload(payload, Protocol::LQ8, seq);

    for (size_t t = 0; t < seq.tones.size(); ++t) {
        int block = static_cast<int>(t);
        int bin = 40 + seq.tones[t];
        mag[block * block_stride + bin] = 200;
    }

    float freq_hz = 40.0f * (12000.0f / 1920.0f);
    lq_subtract_signal_from_waterfall(mag.data(), static_cast<int>(mag.size()), block_size, block_stride, payload, freq_hz, 0.0f, 12000.0f, Protocol::LQ8);

    // Verify signal tone energy was attenuated
    int tone0_bin = 40 + seq.tones[0];
    EXPECT_LE(mag[0 * block_stride + tone0_bin], 60);
}
