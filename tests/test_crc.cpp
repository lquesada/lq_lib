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
#include "lq/crc.h"
#include <cstring>
#include <vector>

using namespace lq;

TEST(CrcTest, BasicComputation) {
    uint8_t zero_payload[10] = {0};
    uint16_t crc_zero = compute_crc14(zero_payload, 77);
    EXPECT_EQ(crc_zero, 0U);

    uint16_t crc_payload_zero = compute_payload_crc14(zero_payload);
    EXPECT_EQ(crc_payload_zero, 0U);

    uint8_t one_payload[10];
    std::memset(one_payload, 0xFF, sizeof(one_payload));
    uint16_t crc_ones = compute_crc14(one_payload, 77);
    EXPECT_NE(crc_ones, 0U);
    EXPECT_LE(crc_ones, 0x3FFFU);

    uint16_t crc_payload_ones = compute_payload_crc14(one_payload);
    EXPECT_NE(crc_payload_ones, 0U);
    EXPECT_LE(crc_payload_ones, 0x3FFFU);
}

TEST(CrcTest, AppendAndVerify) {
    uint8_t payload[10] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x00};
    uint8_t data_91[12];

    append_crc14(payload, data_91);
    EXPECT_TRUE(verify_crc14(data_91));

    uint8_t extracted[10];
    extract_payload(data_91, extracted);
    // First 9 bytes + high 5 bits of byte 9 should match
    for (int i = 0; i < 9; ++i) {
        EXPECT_EQ(extracted[i], payload[i]);
    }
    EXPECT_EQ(extracted[9] & 0xF8, payload[9] & 0xF8);
}

TEST(CrcTest, SingleBitErrorDetection) {
    uint8_t payload[10] = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0x80};
    uint8_t data_91[12];
    append_crc14(payload, data_91);
    ASSERT_TRUE(verify_crc14(data_91));

    // Flip every single bit from 0 to 90 and verify CRC failure
    for (int bit = 0; bit < 91; ++bit) {
        uint8_t corrupted[12];
        std::memcpy(corrupted, data_91, sizeof(corrupted));

        size_t byte_idx = bit / 8;
        int bit_idx = 7 - (bit % 8);
        corrupted[byte_idx] ^= static_cast<uint8_t>(1 << bit_idx);

        EXPECT_FALSE(verify_crc14(corrupted)) << "Failed to detect bit flip at position " << bit;
    }
}

TEST(CrcTest, RejectAllZerosAndNull) {
    uint8_t all_zeros[12] = {0};
    EXPECT_FALSE(verify_crc14(all_zeros));
    EXPECT_FALSE(verify_crc14(nullptr));
}

