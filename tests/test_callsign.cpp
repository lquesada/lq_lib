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
#include "lq/callsign.h"
#include <vector>
#include <string>

using namespace lq;

TEST(CallsignTest, StandardCallsignValidation) {
    // 1-char prefix
    EXPECT_TRUE(is_standard_callsign("W1AW"));
    EXPECT_TRUE(is_standard_callsign("K1JT"));
    EXPECT_TRUE(is_standard_callsign("G4FOC"));
    EXPECT_TRUE(is_standard_callsign("N0A"));
    EXPECT_TRUE(is_standard_callsign("W1A"));
    EXPECT_TRUE(is_standard_callsign("K9AN"));

    // 2-char prefix (letter + letter)
    EXPECT_TRUE(is_standard_callsign("YO1YO"));
    EXPECT_TRUE(is_standard_callsign("HB9IPH"));
    EXPECT_TRUE(is_standard_callsign("VK3ABC"));
    EXPECT_TRUE(is_standard_callsign("TU2TU"));
    EXPECT_TRUE(is_standard_callsign("JA1ABC"));
    EXPECT_TRUE(is_standard_callsign("ZL2B"));

    // 2-char prefix (digit + letter)
    EXPECT_TRUE(is_standard_callsign("9A1A"));
    EXPECT_TRUE(is_standard_callsign("9A1AA"));
    EXPECT_TRUE(is_standard_callsign("9A1AAA"));
    EXPECT_TRUE(is_standard_callsign("3D2AG"));
    EXPECT_TRUE(is_standard_callsign("4X4DX"));

    // Special tokens
    EXPECT_TRUE(is_standard_callsign("DE"));
    EXPECT_TRUE(is_standard_callsign("QRZ"));
    EXPECT_TRUE(is_standard_callsign("CQ"));

    // Invalid standard callsigns (compound, too long, no digits, etc.)
    EXPECT_FALSE(is_standard_callsign(""));
    EXPECT_FALSE(is_standard_callsign("W1AWXX"));   // suffix too long (4 chars)
    EXPECT_FALSE(is_standard_callsign("VK99ABC"));  // prefix has 2 digits before suffix
    EXPECT_FALSE(is_standard_callsign("YO/W1AW"));  // compound with slash -> non-standard
    EXPECT_FALSE(is_standard_callsign("HB9IPH/P")); // compound with slash -> non-standard
    EXPECT_FALSE(is_standard_callsign("ABCDEF"));   // no digit
    EXPECT_FALSE(is_standard_callsign("123456"));   // no letters
}

TEST(CallsignTest, NormalisationAlignments) {
    std::string norm;

    // 1-char prefix, 2-char suffix: " W1AW "
    EXPECT_TRUE(normalise_standard_callsign("W1AW", norm));
    EXPECT_EQ(norm, " W1AW ");

    // 1-char prefix, 3-char suffix: " G4FOC"
    EXPECT_TRUE(normalise_standard_callsign("G4FOC", norm));
    EXPECT_EQ(norm, " G4FOC");

    // 1-char prefix, 1-char suffix: " W1A  "
    EXPECT_TRUE(normalise_standard_callsign("W1A", norm));
    EXPECT_EQ(norm, " W1A  ");

    // 2-char prefix, 2-char suffix: "YO1YO "
    EXPECT_TRUE(normalise_standard_callsign("YO1YO", norm));
    EXPECT_EQ(norm, "YO1YO ");

    // 2-char prefix, 3-char suffix: "VK3ABC"
    EXPECT_TRUE(normalise_standard_callsign("VK3ABC", norm));
    EXPECT_EQ(norm, "VK3ABC");

    // Digit prefix, 1-char suffix: "9A1A  "
    EXPECT_TRUE(normalise_standard_callsign("9A1A", norm));
    EXPECT_EQ(norm, "9A1A  ");
}

TEST(CallsignTest, RoundTripEncoding) {
    const std::vector<std::string> callsigns = {
        "W1AW", "K1JT", "YO1YO", "TU2TU", "HB9IPH", "VK3ABC", "G4FOC",
        "9A1A", "3D2AG", "4X4DX", "JA1ABC", "ZL2B", "K9AN", "AD5Q",
        "W1A", "N0A", "AA1AA", "ZZ9ZZZ", "1A1A", "9Z9ZZZ", "DE", "QRZ", "CQ"
    };

    for (const auto& call : callsigns) {
        uint32_t packed = 0;
        ASSERT_TRUE(encode_callsign_std(call, packed)) << "Failed to encode: " << call;
        EXPECT_LE(packed, (1U << 28) - 1U) << "Packed value exceeded 28 bits: " << packed;

        std::string decoded;
        ASSERT_TRUE(decode_callsign_std(packed, decoded)) << "Failed to decode packed: " << packed;
        EXPECT_EQ(decoded, call);
    }
}

TEST(CallsignTest, CaseInsensitiveEncoding) {
    uint32_t p1 = 0, p2 = 0;
    EXPECT_TRUE(encode_callsign_std("yo1yo", p1));
    EXPECT_TRUE(encode_callsign_std("YO1YO", p2));
    EXPECT_EQ(p1, p2);

    std::string dec;
    EXPECT_TRUE(decode_callsign_std(p1, dec));
    EXPECT_EQ(dec, "YO1YO");
}

TEST(CallsignTest, BoundaryValues) {
    // Smallest valid standard callsign packed value
    // " A0A  ": c1=0 (space), c2=10 ('A'), d=0, s1=1 ('A'), s2=0 (space), s3=0 (space)
    uint32_t min_p = 0;
    EXPECT_TRUE(encode_callsign_std("A0A", min_p));
    EXPECT_GT(min_p, 0U);

    std::string dec;
    EXPECT_TRUE(decode_callsign_std(min_p, dec));
    EXPECT_EQ(dec, "A0A");

    // Special token boundary values
    EXPECT_TRUE(decode_callsign_std(CALLSIGN_TOKEN_DE, dec));
    EXPECT_EQ(dec, "DE");
    EXPECT_TRUE(decode_callsign_std(CALLSIGN_TOKEN_QRZ, dec));
    EXPECT_EQ(dec, "QRZ");
    EXPECT_TRUE(decode_callsign_std(CALLSIGN_TOKEN_CQ, dec));
    EXPECT_EQ(dec, "CQ");

    // Invalid packed value beyond max
    EXPECT_FALSE(decode_callsign_std(268435456U, dec)); // 2^28
}

TEST(CallsignTest, StandardSuffixParsing) {
    std::string base;
    uint8_t suf = 0;

    EXPECT_TRUE(parse_standard_callsign_suffix("HB9IPH", base, suf));
    EXPECT_EQ(base, "HB9IPH");
    EXPECT_EQ(suf, 0);

    EXPECT_TRUE(parse_standard_callsign_suffix("HB9IPH/P", base, suf));
    EXPECT_EQ(base, "HB9IPH");
    EXPECT_EQ(suf, 1);

    // /R and /QRP are handled as non-standard callsigns, not dedicated 1-bit /P suffixes
    EXPECT_FALSE(parse_standard_callsign_suffix("HB9IPH/R", base, suf));
    EXPECT_FALSE(parse_standard_callsign_suffix("HB9IPH/QRP", base, suf));

    // Any callsign suffix parsing
    EXPECT_TRUE(parse_any_callsign_suffix("HB9IPH/R", base, suf));
    EXPECT_EQ(base, "HB9IPH/R");
    EXPECT_EQ(suf, 0);

    EXPECT_TRUE(parse_any_callsign_suffix("3B9/HB9IPH", base, suf));
    EXPECT_EQ(base, "3B9/HB9IPH");
    EXPECT_EQ(suf, 0);

    EXPECT_TRUE(parse_any_callsign_suffix("EA8/YO1YO/P", base, suf));
    EXPECT_EQ(base, "EA8/YO1YO");
    EXPECT_EQ(suf, 1);

    EXPECT_EQ(format_callsign_with_suffix("HB9IPH", 0), "HB9IPH");
    EXPECT_EQ(format_callsign_with_suffix("HB9IPH", 1), "HB9IPH/P");
}
