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
#include "lq/hash.h"
#include <unordered_set>
#include <vector>
#include <string>

using namespace lq;

TEST(HashTest, DeterminismAndRange) {
    uint32_t h1 = hash_callsign_24("YO1YO");
    uint32_t h2 = hash_callsign_24("YO1YO");
    EXPECT_EQ(h1, h2);
    EXPECT_LE(h1, 0xFFFFFFU);

    uint32_t h_w1aw = hash_callsign_24("W1AW");
    EXPECT_NE(h1, h_w1aw);
    EXPECT_LE(h_w1aw, 0xFFFFFFU);
}

TEST(HashTest, CaseAndWhitespaceNormalization) {
    EXPECT_EQ(hash_callsign_24("yo1yo"), hash_callsign_24("YO1YO"));
    EXPECT_EQ(hash_callsign_24("  HB9IPH/P  "), hash_callsign_24("HB9IPH/P"));
    EXPECT_EQ(hash_callsign_24("ea6/w1aw"), hash_callsign_24("EA6/W1AW"));

    EXPECT_EQ(hash_callsign_22("  pj4/k1abc  "), hash_callsign_22("PJ4/K1ABC"));
    EXPECT_EQ(hash_callsign_14("  pj4/k1abc  "), hash_callsign_14("PJ4/K1ABC"));
    EXPECT_EQ(hash_callsign_12("  pj4/k1abc  "), hash_callsign_12("PJ4/K1ABC"));
    EXPECT_EQ(hash_callsign_10("  pj4/k1abc  "), hash_callsign_10("PJ4/K1ABC"));
}

TEST(HashTest, Crc24TestVectorsAndPaperWorkedExamples) {
    // 1. Worked example from Section 6.3 of the specification paper: H24("YO1YO/P") = 0xB86F47 (12087111)
    uint32_t h_yo1yo_p = hash_callsign_24("YO1YO/P");
    EXPECT_EQ(h_yo1yo_p, 0xB86F47U);
    EXPECT_EQ(h_yo1yo_p, 12087111U);

    // 2. Additional test vectors across diverse callsign formats
    uint32_t h_ea6_w1aw = hash_callsign_24("EA6/W1AW");
    EXPECT_EQ(h_ea6_w1aw, 0x69AB4BU);
    EXPECT_EQ(h_ea6_w1aw, 6925131U);

    uint32_t h_hb9iph = hash_callsign_24("HB9IPH");
    EXPECT_EQ(h_hb9iph, 0x5FE437U);
    EXPECT_EQ(h_hb9iph, 6284343U);

    uint32_t h_3b9_qrp = hash_callsign_24("3B9/HB9IPH/QRP");
    EXPECT_NE(h_3b9_qrp, 0U);
    EXPECT_LE(h_3b9_qrp, 0xFFFFFFU);
}

TEST(HashTest, WsjtxHashMathematicalPropertiesAndTestVectors) {
    // Test known standard WSJT-X callsigns
    const std::vector<std::string> test_calls = {
        "PJ4/K1ABC", "YW18FIFA", "W1AW", "HB9IPH", "K1ABC", "3DA0RU", "DP0GVN", "ZL1/VK3AMA"
    };

    for (const auto& call : test_calls) {
        uint32_t h22 = hash_callsign_22(call);
        uint32_t h16 = hash_callsign_16(call);
        uint32_t h14 = hash_callsign_14(call);
        uint32_t h12 = hash_callsign_12(call);
        uint32_t h10 = hash_callsign_10(call);

        // Verify bounds
        EXPECT_LE(h22, 0x3FFFFFU) << "22-bit hash out of range for " << call;
        EXPECT_LE(h16, 0xFFFFU) << "16-bit hash out of range for " << call;
        EXPECT_LE(h14, 0x3FFFU) << "14-bit hash out of range for " << call;
        EXPECT_LE(h12, 0xFFFU) << "12-bit hash out of range for " << call;
        EXPECT_LE(h10, 0x3FFU) << "10-bit hash out of range for " << call;

        // Mathematical consistency property: 16-bit, 14-bit, 12-bit, and 10-bit hashes are exact MSB prefixes of 22-bit hash
        EXPECT_EQ(h16, h22 >> 6) << "16-bit hash prefix mismatch for " << call;
        EXPECT_EQ(h14, h22 >> 8) << "14-bit hash prefix mismatch for " << call;
        EXPECT_EQ(h12, h22 >> 10) << "12-bit hash prefix mismatch for " << call;
        EXPECT_EQ(h10, h22 >> 12) << "10-bit hash prefix mismatch for " << call;
        EXPECT_EQ(h14, h16 >> 2) << "14-bit and 16-bit prefix mismatch for " << call;
        EXPECT_EQ(h12, h14 >> 2) << "12-bit and 14-bit prefix mismatch for " << call;
        EXPECT_EQ(h10, h14 >> 4) << "10-bit and 14-bit prefix mismatch for " << call;
    }

    // Invalid character rejection
    EXPECT_EQ(hash_callsign_22("W1AW?"), 0U);
    EXPECT_EQ(hash_callsign_16("W1AW!"), 0U);
    EXPECT_EQ(hash_callsign_14("W1AW!"), 0U);
    EXPECT_EQ(hash_callsign_12("W1AW!"), 0U);
    EXPECT_EQ(hash_callsign_10("W1AW#"), 0U);
}

TEST(HashTest, StationHashCacheResolutionRoundtrip) {
    // Simulate a receiver's active callsign hash cache
    std::unordered_map<uint32_t, std::string> cache24;
    std::unordered_map<uint32_t, std::string> cache22;

    const std::vector<std::string> stations = {
        "3B9/HB9IPH/QRP", "EA6/W1AW", "YO1YO/P", "GB100BBC", "K1ABC/QRP",
        "TO8FT", "DP0GVN", "ZL1/VK3AMA", "HB9IPH", "YO1YO"
    };

    // Populate station cache on receiving CQ or initial packets
    for (const auto& call : stations) {
        uint32_t h24 = hash_callsign_24(call);
        uint32_t h22 = hash_callsign_22(call);
        cache24[h24] = call;
        cache22[h22] = call;
    }

    // Verify 100% accurate callsign resolution from 24-bit and 22-bit hashes
    for (const auto& call : stations) {
        uint32_t h24 = hash_callsign_24(call);
        auto it24 = cache24.find(h24);
        ASSERT_NE(it24, cache24.end());
        EXPECT_EQ(it24->second, call);

        uint32_t h22 = hash_callsign_22(call);
        auto it22 = cache22.find(h22);
        ASSERT_NE(it22, cache22.end());
        EXPECT_EQ(it22->second, call);
    }

    // Verify unresolved hash handling
    uint32_t unknown_hash = 0x123456U;
    EXPECT_EQ(cache24.find(unknown_hash), cache24.end());
}

TEST(HashTest, CollisionDistributionSanityCheck) {
    std::unordered_set<uint32_t> hashes;
    const int N = 1000;
    for (int i = 0; i < N; ++i) {
        std::string call = "CALL" + std::to_string(i);
        uint32_t h = hash_callsign_24(call);
        hashes.insert(h);
    }
    // With 2^24 slots, 1000 random items should have virtually zero collisions
    EXPECT_EQ(hashes.size(), static_cast<size_t>(N));
}
