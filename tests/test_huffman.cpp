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
#include "lq/huffman.h"
#include <vector>
#include <string>

using namespace lq;

TEST(HuffmanTest, PrefixFreeProperty) {
    const std::vector<MessageType> types = {
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

    EXPECT_EQ(types.size(), 16U);

    // Verify prefix-free condition pairwise
    for (size_t i = 0; i < types.size(); ++i) {
        const auto* e1 = get_huffman_entry(types[i]);
        ASSERT_NE(e1, nullptr);
        std::string s1(e1->bit_string);

        for (size_t j = 0; j < types.size(); ++j) {
            if (i == j) continue;
            const auto* e2 = get_huffman_entry(types[j]);
            ASSERT_NE(e2, nullptr);
            std::string s2(e2->bit_string);

            // Neither should be a prefix of the other
            if (s1.size() <= s2.size()) {
                EXPECT_NE(s2.substr(0, s1.size()), s1)
                    << s1 << " (type " << static_cast<int>(types[i]) << ") is a prefix of "
                    << s2 << " (type " << static_cast<int>(types[j]) << ")";
            }
        }
    }
}

TEST(HuffmanTest, RoundTripAll16Types) {
    const std::vector<MessageType> types = {
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
        uint8_t buffer[4] = {0};
        BitBuffer bb_enc(buffer, 32);
        int len = encode_huffman_prefix(t, bb_enc);
        EXPECT_GT(len, 0);
        EXPECT_LE(len, 10);

        BitBuffer bb_dec(buffer, 32);
        MessageType decoded = decode_huffman_prefix(bb_dec);
        EXPECT_EQ(decoded, t) << "Failed for message type " << static_cast<int>(t);
        EXPECT_EQ(bb_dec.get_pos(), static_cast<size_t>(len));
    }
}

TEST(HuffmanTest, DecodeInvalidPrefixReturnsUnknown) {
    BitBuffer bb_empty(static_cast<const uint8_t*>(nullptr), 0);
    EXPECT_EQ(decode_huffman_prefix(bb_empty), MessageType::UNKNOWN);

    uint8_t zeros[2] = {0x00, 0x00};
    BitBuffer bb_trunc(zeros, 6);
    EXPECT_EQ(decode_huffman_prefix(bb_trunc), MessageType::UNKNOWN);
}
