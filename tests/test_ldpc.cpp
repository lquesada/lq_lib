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
#include "lq/ldpc.h"
#include <cstring>
#include <vector>
#include <random>

using namespace lq;

TEST(LdpcTest, SystematicAndSyndrome) {
    uint8_t in_91[12] = {0xAA, 0x55, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x42, 0xE0};
    uint8_t codeword[22];

    ldpc_encode(in_91, codeword);

    // Verify systematic property: first 11 bytes + high 3 bits of byte 11
    for (int i = 0; i < 11; ++i) {
        EXPECT_EQ(codeword[i], in_91[i]) << "Mismatch at byte " << i;
    }
    EXPECT_EQ(codeword[11] & 0xE0, in_91[11] & 0xE0);

    // Verify syndrome
    EXPECT_TRUE(ldpc_check_syndrome(codeword));
}

TEST(LdpcTest, CleanChannelDecode) {
    uint8_t in_91[12] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x80};
    uint8_t codeword[22];
    ldpc_encode(in_91, codeword);

    uint8_t decoded_91[12];
    int iters = ldpc_decode_hard_bits(codeword, decoded_91);
    EXPECT_GT(iters, 0);

    for (int i = 0; i < 11; ++i) {
        EXPECT_EQ(decoded_91[i], in_91[i]) << "Decoded byte mismatch at " << i;
    }
    EXPECT_EQ(decoded_91[11] & 0xE0, in_91[11] & 0xE0);
}

TEST(LdpcTest, ErrorCorrectionWithBitFlips) {
    uint8_t in_91[12] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE, 0x12, 0x34, 0x56, 0x80};
    uint8_t codeword[22];
    ldpc_encode(in_91, codeword);

    // Test correcting 1, 2, 3, 4 bit flips across the codeword
    std::mt19937 rng(1337);

    for (int num_flips = 1; num_flips <= 4; ++num_flips) {
        for (int trial = 0; trial < 10; ++trial) {
            uint8_t corrupted[22];
            std::memcpy(corrupted, codeword, sizeof(corrupted));

            std::vector<int> flip_positions;
            while (static_cast<int>(flip_positions.size()) < num_flips) {
                int pos = rng() % 174;
                if (std::find(flip_positions.begin(), flip_positions.end(), pos) == flip_positions.end()) {
                    flip_positions.push_back(pos);
                    size_t byte_idx = pos / 8;
                    int bit_idx = 7 - (pos % 8);
                    corrupted[byte_idx] ^= static_cast<uint8_t>(1 << bit_idx);
                }
            }

            uint8_t decoded_91[12];
            int iters = ldpc_decode_hard_bits(corrupted, decoded_91, 30);
            EXPECT_GT(iters, 0) << "Failed to decode with " << num_flips << " bit flips in trial " << trial;

            for (int i = 0; i < 11; ++i) {
                EXPECT_EQ(decoded_91[i], in_91[i]);
            }
            EXPECT_EQ(decoded_91[11] & 0xE0, in_91[11] & 0xE0);
        }
    }
}
