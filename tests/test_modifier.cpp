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
#include "lq/modifier.h"
#include <vector>
#include <string>

using namespace lq;

TEST(ModifierTest, EmptyModifier) {
    uint32_t packed = 999;
    EXPECT_TRUE(encode_modifier_20("", packed));
    EXPECT_EQ(packed, MODIFIER_NONE);

    std::string dec;
    EXPECT_TRUE(decode_modifier_20(packed, dec));
    EXPECT_TRUE(dec.empty());
}


TEST(ModifierTest, Numeric3DigitModifiers) {
    const std::vector<std::pair<std::string, uint32_t>> numeric_cases = {
        {"000", 1U},
        {"001", 2U},
        {"040", 41U},
        {"100", 101U},
        {"500", 501U},
        {"999", 1000U}
    };

    for (const auto& [mod, expected_val] : numeric_cases) {
        uint32_t packed = 0;
        ASSERT_TRUE(encode_modifier_20(mod, packed)) << "Failed to encode numeric: " << mod;
        EXPECT_EQ(packed, expected_val);
        EXPECT_LE(packed, MODIFIER_NUMERIC_MAX);

        std::string dec;
        ASSERT_TRUE(decode_modifier_20(packed, dec)) << "Failed to decode numeric: " << mod;
        EXPECT_EQ(dec, mod);
    }
}

TEST(ModifierTest, AlphanumericBase32Modifiers) {
    const std::vector<std::string> test_mods = {
        "DX", "FD", "QRP", "TEST", "POTA", "SOTA", "WWFF", "NA", "EU",
        "AF", "SA", "AS", "OC", "AN", "RTTY", "IOTA", "VHF", "UHF", "SHF", "EME",
        "W1", "JA", "VK", "G4", "A1", "0123", "P2P", "C12", "100K", "4X4"
    };

    for (const auto& mod : test_mods) {
        uint32_t packed = 0;
        ASSERT_TRUE(encode_modifier_20(mod, packed)) << "Failed to encode: " << mod;
        EXPECT_GT(packed, MODIFIER_NUMERIC_MAX);
        EXPECT_LE(packed, MODIFIER_MAX_VAL);

        std::string dec;
        ASSERT_TRUE(decode_modifier_20(packed, dec)) << "Failed to decode: " << mod;
        EXPECT_EQ(dec, mod);
    }
}

TEST(ModifierTest, CaseInsensitivityAndWhitespace) {
    uint32_t p1 = 0, p2 = 0, p3 = 0;
    EXPECT_TRUE(encode_modifier_20("dx", p1));
    EXPECT_TRUE(encode_modifier_20("DX", p2));
    EXPECT_TRUE(encode_modifier_20("  DX  ", p3));
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p2, p3);
}

TEST(ModifierTest, InvalidModifiers) {
    uint32_t packed = 0;
    // Length > 4
    EXPECT_FALSE(encode_modifier_20("CQWPX", packed));
    EXPECT_FALSE(encode_modifier_20("ABCDE", packed));

    // Invalid characters (punctuation / symbols)
    EXPECT_FALSE(encode_modifier_20("DX!", packed));
    EXPECT_FALSE(encode_modifier_20("A-B", packed));

    // Digits 5-9 in non-3-digit context (Base-32 only supports 0-4)
    EXPECT_FALSE(encode_modifier_20("A5", packed));
    EXPECT_FALSE(encode_modifier_20("DX9", packed));

    // Out of bounds decoding
    std::string dec;
    EXPECT_FALSE(decode_modifier_20(MODIFIER_MAX_VAL + 1U, dec));
}

TEST(ModifierTest, IsKnownModifierHelper) {
    EXPECT_TRUE(is_known_modifier(""));
    EXPECT_TRUE(is_known_modifier("DX"));
    EXPECT_TRUE(is_known_modifier("040"));
    EXPECT_FALSE(is_known_modifier("TOOLONG"));
}
