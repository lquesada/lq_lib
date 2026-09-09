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
#include "lq/rst.h"
#include <string>

using namespace lq;

TEST(RstTest, FullRangeSweep) {
    for (int db = RST_MIN_DB; db <= RST_MAX_DB; ++db) {
        uint8_t packed = encode_rst_5(db);
        EXPECT_LE(packed, RST_MAX_PACKED);

        int decoded = decode_rst_5(packed);
        EXPECT_EQ(decoded, db);
    }
}

TEST(RstTest, Clamping) {
    EXPECT_EQ(encode_rst_5(-50), 0);
    EXPECT_EQ(decode_rst_5(0), RST_MIN_DB);

    EXPECT_EQ(encode_rst_5(+10), RST_MAX_PACKED);
    EXPECT_EQ(decode_rst_5(RST_MAX_PACKED), RST_MAX_DB);
}

TEST(RstTest, StringFormattingAndParsing) {
    EXPECT_EQ(format_rst(+5), "+05");
    EXPECT_EQ(format_rst(-3), "-03");
    EXPECT_EQ(format_rst(0), "+00");
    EXPECT_EQ(format_rst(-30), "-30");
    EXPECT_EQ(format_rst(+1), "+01");

    int val = 0;
    EXPECT_TRUE(parse_rst("+05", val));
    EXPECT_EQ(val, 5);

    EXPECT_TRUE(parse_rst("-03", val));
    EXPECT_EQ(val, -3);

    EXPECT_TRUE(parse_rst("-3", val));
    EXPECT_EQ(val, -3);

    EXPECT_TRUE(parse_rst("+1", val));
    EXPECT_EQ(val, 1);

    EXPECT_TRUE(parse_rst("R-15", val));
    EXPECT_EQ(val, -15);

    EXPECT_TRUE(parse_rst("R+02", val));
    EXPECT_EQ(val, 2);
}
