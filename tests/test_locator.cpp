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
#include "lq/locator.h"
#include <string>

using namespace lq;

TEST(LocatorTest, Validation) {
    EXPECT_TRUE(is_valid_locator("JN47"));
    EXPECT_TRUE(is_valid_locator("FN31"));
    EXPECT_TRUE(is_valid_locator("AA00"));
    EXPECT_TRUE(is_valid_locator("RR99"));
    EXPECT_TRUE(is_valid_locator("IO91"));
    EXPECT_TRUE(is_valid_locator("jn47")); // case-insensitive

    EXPECT_FALSE(is_valid_locator(""));
    EXPECT_FALSE(is_valid_locator("JN4"));    // 3 chars
    EXPECT_FALSE(is_valid_locator("JN47XX"));  // 6 chars
    EXPECT_FALSE(is_valid_locator("SA00"));    // 'S' > 'R'
    EXPECT_FALSE(is_valid_locator("AS00"));    // 'S' > 'R'
    EXPECT_FALSE(is_valid_locator("JN4A"));    // letter in square digit
    EXPECT_FALSE(is_valid_locator("1234"));    // digits in field
}

TEST(LocatorTest, BlankLocatorHandling) {
    uint16_t packed = 0;
    EXPECT_TRUE(encode_locator_15("", packed));
    EXPECT_EQ(packed, LOCATOR_BLANK);

    std::string dec;
    EXPECT_TRUE(decode_locator_15(packed, dec));
    EXPECT_TRUE(dec.empty());
}

TEST(LocatorTest, Boundaries) {
    uint16_t packed_min = 0;
    EXPECT_TRUE(encode_locator_15("AA00", packed_min));
    EXPECT_EQ(packed_min, 0U);

    std::string dec_min;
    EXPECT_TRUE(decode_locator_15(packed_min, dec_min));
    EXPECT_EQ(dec_min, "AA00");

    uint16_t packed_max = 0;
    EXPECT_TRUE(encode_locator_15("RR99", packed_max));
    EXPECT_EQ(packed_max, LOCATOR_MAX_VALID);

    std::string dec_max;
    EXPECT_TRUE(decode_locator_15(packed_max, dec_max));
    EXPECT_EQ(dec_max, "RR99");
}

TEST(LocatorTest, ExhaustiveRoundTripAll32400Locators) {
    for (int f_lon = 0; f_lon < 18; ++f_lon) {
        for (int f_lat = 0; f_lat < 18; ++f_lat) {
            for (int s_lon = 0; s_lon < 10; ++s_lon) {
                for (int s_lat = 0; s_lat < 10; ++s_lat) {
                    std::string loc;
                    loc.push_back(static_cast<char>('A' + f_lon));
                    loc.push_back(static_cast<char>('A' + f_lat));
                    loc.push_back(static_cast<char>('0' + s_lon));
                    loc.push_back(static_cast<char>('0' + s_lat));

                    uint16_t packed = 0;
                    ASSERT_TRUE(encode_locator_15(loc, packed));
                    EXPECT_LE(packed, LOCATOR_MAX_VALID);

                    std::string decoded;
                    ASSERT_TRUE(decode_locator_15(packed, decoded));
                    EXPECT_EQ(decoded, loc);
                }
            }
        }
    }
}
