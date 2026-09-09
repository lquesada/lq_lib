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
#include "lq/callsign_nonstd.h"
#include <vector>
#include <string>

using namespace lq;

TEST(CallsignNonStdTest, AlphabetValidation) {
    EXPECT_TRUE(is_valid_nonstd_callsign("EA6/W1AW", 9));
    EXPECT_TRUE(is_valid_nonstd_callsign("YO1YO/P", 7));
    EXPECT_TRUE(is_valid_nonstd_callsign("W1AW/QRP", 9));
    EXPECT_TRUE(is_valid_nonstd_callsign("TM100TOUR", 9));
    EXPECT_TRUE(is_valid_nonstd_callsign("DP0POL/MM", 9));
    EXPECT_TRUE(is_valid_nonstd_callsign("3B9/HB9IPH/P", 14));
    EXPECT_TRUE(is_valid_nonstd_callsign("ABC/123/XYZ", 14));

    // Exceeding length limits
    EXPECT_FALSE(is_valid_nonstd_callsign("EA6/W1AW", 7));  // 8 chars > 7
    EXPECT_FALSE(is_valid_nonstd_callsign("TM100TOUR", 7)); // 9 chars > 7
    EXPECT_FALSE(is_valid_nonstd_callsign("VERYLONGCALLSIGN123", 14)); // 19 chars > 14

    // Invalid characters (not in 38-char alphabet)
    EXPECT_FALSE(is_valid_nonstd_callsign("W1AW-1", 7)); // '-' not in nonstd alphabet
    EXPECT_FALSE(is_valid_nonstd_callsign("W1AW?", 7));  // '?' not in nonstd alphabet
}

TEST(CallsignNonStdTest, RoundTrip7Chars) {
    const std::vector<std::string> calls = {
        "YO1YO/P", "W1AW/1", "K1JT/0", "G4FOC/P", "9A1A/M", "3D2A/P", "A", "Z/0"
    };

    for (const auto& call : calls) {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 37);
        ASSERT_TRUE(encode_callsign_nonstd(call, 7, bb_enc)) << "Failed to encode: " << call;

        BitBuffer bb_dec(buffer, 37);
        std::string decoded;
        ASSERT_TRUE(decode_callsign_nonstd(bb_dec, 7, decoded)) << "Failed to decode: " << call;
        EXPECT_EQ(decoded, call);
    }
}

TEST(CallsignNonStdTest, RoundTrip9Chars) {
    const std::vector<std::string> calls = {
        "EA6/W1AW", "W1AW/QRP", "TM100TOUR", "DP0POL/MM", "YO1YO/QRP", "VK3ABC/M"
    };

    for (const auto& call : calls) {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 48);
        ASSERT_TRUE(encode_callsign_nonstd(call, 9, bb_enc)) << "Failed to encode: " << call;

        BitBuffer bb_dec(buffer, 48);
        std::string decoded;
        ASSERT_TRUE(decode_callsign_nonstd(bb_dec, 9, decoded)) << "Failed to decode: " << call;
        EXPECT_EQ(decoded, call);
    }
}

TEST(CallsignNonStdTest, RoundTrip10Chars) {
    const std::vector<std::string> calls = {
        "EA6/W1AW/P", "SPECIAL100", "VK3ABC/QRP", "HB9IPH/QRP"
    };

    for (const auto& call : calls) {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 53);
        ASSERT_TRUE(encode_callsign_nonstd(call, 10, bb_enc)) << "Failed to encode: " << call;

        BitBuffer bb_dec(buffer, 53);
        std::string decoded;
        ASSERT_TRUE(decode_callsign_nonstd(bb_dec, 10, decoded)) << "Failed to decode: " << call;
        EXPECT_EQ(decoded, call);
    }
}

TEST(CallsignNonStdTest, RoundTrip14Chars) {
    const std::vector<std::string> calls = {
        "3B9/HB9IPH/P", "EA6/HB9IPH/QRP", "14CHARCALLSIGN", "ZZZZZZZZZZZZZZ", "//////////////"
    };

    for (const auto& call : calls) {
        uint8_t buffer[12] = {0};
        BitBuffer bb_enc(buffer, 74);
        ASSERT_TRUE(encode_callsign_nonstd(call, 14, bb_enc)) << "Failed to encode: " << call;

        BitBuffer bb_dec(buffer, 74);
        std::string decoded;
        ASSERT_TRUE(decode_callsign_nonstd(bb_dec, 14, decoded)) << "Failed to decode: " << call;
        EXPECT_EQ(decoded, call);
    }
}
