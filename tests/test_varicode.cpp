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
#include "lq/varicode.h"
#include <string>
#include <vector>

using namespace lq;

TEST(VaricodeTest, CharacterLengths) {
    // Verify specific tier bit lengths for the 104-character sequential codebook
    EXPECT_EQ(get_varicode_char_length(' '), 3);
    EXPECT_EQ(get_varicode_char_length('E'), 3);
    EXPECT_EQ(get_varicode_char_length('T'), 4);
    EXPECT_EQ(get_varicode_char_length('A'), 4);
    EXPECT_EQ(get_varicode_char_length('D'), 5);
    EXPECT_EQ(get_varicode_char_length('U'), 6);
    EXPECT_EQ(get_varicode_char_length('V'), 7);
    EXPECT_EQ(get_varicode_char_length('K'), 8);
    EXPECT_EQ(get_varicode_char_length(EOM_CHAR), 8); // ⌁
    EXPECT_EQ(get_varicode_char_length('X'), 9);
    EXPECT_EQ(get_varicode_char_length('1'), 10);
    EXPECT_EQ(get_varicode_char_length('0'), 11);
    EXPECT_EQ(get_varicode_char_length('7'), 12);
    EXPECT_EQ(get_varicode_char_length('='), 13);
    EXPECT_EQ(get_varicode_char_length('['), 14);
    EXPECT_EQ(get_varicode_char_length('/'), 15);
    EXPECT_EQ(get_varicode_char_length('$'), 16);
    EXPECT_EQ(get_varicode_char_length('\n'), 16);
    EXPECT_EQ(get_varicode_char_length('>'), 17);
    EXPECT_EQ(get_varicode_char_length(static_cast<char>(0xC0)), 17); // À
    EXPECT_EQ(get_varicode_char_length(static_cast<char>(0xDE)), 18); // Þ
    EXPECT_EQ(get_varicode_char_length(FILL_CHAR), 18); // [FILL]
}

TEST(VaricodeTest, AllIndividualCharacters) {
    // Test uppercase letters, digits, and symbols
    std::string test_chars = " ETAONISRHDLCUMWFGYPBVKXJ1Q!?Z,.028345967\"'-_;:=()#*[]|/+$%\n&<>@\\^`{}~";
    for (char c : test_chars) {
        EXPECT_TRUE(is_varicode_char(c)) << "Character failed: " << c;

        std::string s(1, c);
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, VARICODE_PAYLOAD_BITS);
        int n_chars = encode_varicode(s, bb_enc, VARICODE_PAYLOAD_BITS, true);
        EXPECT_EQ(n_chars, 1);

        BitBuffer bb_dec(buffer, VARICODE_PAYLOAD_BITS);
        std::string dec;
        decode_varicode(bb_dec, dec, VARICODE_PAYLOAD_BITS);
        EXPECT_EQ(dec, s);
    }
}

TEST(VaricodeTest, Latin1Support) {
    // Test UTF-8 and Latin-1 accented characters
    std::vector<std::string> latin_words = {
        "HOLA SEÑOR",
        "GRÜSSE 73",
        "CAFÉ DE HB9IPH",
        "ÇA VA BIEN"
    };

    for (const auto& w : latin_words) {
        uint8_t buffer[20] = {0};
        BitBuffer bb_enc(buffer, 128);
        int written = encode_varicode(w, bb_enc, 128, true);
        EXPECT_GT(written, 0);

        BitBuffer bb_dec(buffer, 128);
        std::string dec;
        decode_varicode(bb_dec, dec, 128);
        EXPECT_EQ(dec, w);
    }
}

TEST(VaricodeTest, RoundTripCommonMessages) {
    const std::vector<std::string> messages = {
        "73",
        "HELLO WORLD",
        "CQ YO1YO",
        "TNX 73 GL",
        "QSL VIA BURO",
        "RIG KX3",
        "PWR 100W",
        "GRID JN47",
        "WX SUNNY"
    };

    for (const auto& msg : messages) {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, VARICODE_PAYLOAD_BITS);
        encode_varicode(msg, bb_enc, VARICODE_PAYLOAD_BITS, true);

        BitBuffer bb_dec(buffer, VARICODE_PAYLOAD_BITS);
        std::string dec;
        decode_varicode(bb_dec, dec, VARICODE_PAYLOAD_BITS);
        EXPECT_EQ(dec, msg);
    }
}

TEST(VaricodeTest, TruncationOnOverflow) {
    std::string long_msg = "THIS IS A VERY LONG MESSAGE THAT EXCEEDS SEVENTY FOUR BITS EASILY";

    uint8_t buffer[10] = {0};
    BitBuffer bb_enc(buffer, VARICODE_PAYLOAD_BITS);
    int written = encode_varicode(long_msg, bb_enc, VARICODE_PAYLOAD_BITS, true);
    EXPECT_GT(written, 0);
    EXPECT_LT(written, static_cast<int>(long_msg.size()));

    BitBuffer bb_dec(buffer, VARICODE_PAYLOAD_BITS);
    std::string dec;
    decode_varicode(bb_dec, dec, VARICODE_PAYLOAD_BITS);
    EXPECT_EQ(dec, long_msg.substr(0, written));
}

TEST(VaricodeTest, PureFillSequenceEmitsZeroCharacters) {
    // Pure zero-fill padding must decode to exactly 0 characters (empty string)
    for (int k = 1; k <= VARICODE_PAYLOAD_BITS; ++k) {
        uint8_t buffer[10] = {0};
        BitBuffer bb_dec(buffer, k);
        std::string dec;
        int n_decoded = decode_varicode(bb_dec, dec, k);
        EXPECT_EQ(n_decoded, 0) << "Failed for fill length k = " << k << ", decoded: " << dec;
        EXPECT_EQ(dec, "");
    }
}

TEST(VaricodeTest, TrailingBitsPaddedWithZeroes) {
    std::string short_msg = "A";

    uint8_t buffer[10] = {0};
    BitBuffer bb_enc(buffer, VARICODE_PAYLOAD_BITS);
    int written = encode_varicode(short_msg, bb_enc, VARICODE_PAYLOAD_BITS, true);
    EXPECT_EQ(written, 1);

    // Verify trailing bits are pure zeroes
    BitBuffer bb_check(buffer, VARICODE_PAYLOAD_BITS);
    bb_check.read_bits(4 + 8); // Skip 'A' (4 bits) + EOM (8 bits) = 12 bits
    for (int i = 12; i < VARICODE_PAYLOAD_BITS; ++i) {
        EXPECT_EQ(bb_check.read_bits(1), 0U) << "Bit " << i << " is not zero";
    }

    // Decode and ensure no phantom characters are emitted
    BitBuffer bb_dec(buffer, VARICODE_PAYLOAD_BITS);
    std::string dec;
    decode_varicode(bb_dec, dec, VARICODE_PAYLOAD_BITS);
    EXPECT_EQ(dec, "A");
}

TEST(VaricodeTest, DecodeVaricodeTokenBranches) {
    // 1. Standard token decoding with space delimiter
    {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 77);
        encode_varicode("HB9IPH HELLO", bb_enc, 77, true, false);

        BitBuffer bb_dec(buffer, 77);
        std::string token1 = decode_varicode_token(bb_dec, 77);
        EXPECT_EQ(token1, "HB9IPH");

        std::string token2 = decode_varicode_token(bb_dec, 77);
        EXPECT_EQ(token2, "HELLO");
    }

    // 2. Token decoding with Latin-1 characters
    {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 77);
        encode_varicode("CAFÉ PARIS", bb_enc, 77, true, false);

        BitBuffer bb_dec(buffer, 77);
        std::string token1 = decode_varicode_token(bb_dec, 77);
        EXPECT_EQ(token1, "CAFÉ");
    }

    // 3. Token decoding stopped by EOM or FILL
    {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 77);
        encode_varicode("HB9IPH", bb_enc, 77, true, false);

        BitBuffer bb_dec(buffer, 77);
        std::string token = decode_varicode_token(bb_dec, 77);
        EXPECT_EQ(token, "HB9IPH");
    }

    // 4. Token decoding invalid bit pattern accumulator reset
    {
        uint8_t buffer[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        BitBuffer bb(buffer, 40);
        std::string token = decode_varicode_token(bb, 40);
        EXPECT_GE(token.size(), 0U);
    }
}

TEST(VaricodeTest, Utf8ExtendedSentinelsAndFallbacks) {
    // 1. EOM sentinels encoded from UTF-8 strings
    {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 77);
        // UTF-8 ⌁ (\xE2\x8C\x81)
        encode_varicode("73\xE2\x8C\x81", bb_enc, 77, false, false);
        BitBuffer bb_dec(buffer, 77);
        std::string dec;
        bool has_eom = false;
        decode_varicode(bb_dec, dec, 77, &has_eom);
        EXPECT_EQ(dec, "73");
        EXPECT_TRUE(has_eom);
    }
    {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 77);
        // UTF-8 ⚡ (\xE2\x9A\xA1)
        encode_varicode("73\xE2\x9A\xA1", bb_enc, 77, false, false);
        BitBuffer bb_dec(buffer, 77);
        std::string dec;
        bool has_eom = false;
        decode_varicode(bb_dec, dec, 77, &has_eom);
        EXPECT_EQ(dec, "73");
        EXPECT_TRUE(has_eom);
    }
    {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 77);
        // Other 3-byte UTF-8 character fallback (e.g. ♥ \xE2\x99\xA5)
        encode_varicode("73\xE2\x99\xA5GL", bb_enc, 77, true, false);
        BitBuffer bb_dec(buffer, 77);
        std::string dec;
        decode_varicode(bb_dec, dec, 77);
        EXPECT_EQ(dec, "73 GL");
    }

    // 2. Latin-1 multi-byte encoding (0xC2 prefix)
    {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 77);
        encode_varicode("¡HOLA!", bb_enc, 77, true, false);
        BitBuffer bb_dec(buffer, 77);
        std::string dec;
        decode_varicode(bb_dec, dec, 77);
        EXPECT_EQ(dec, "¡HOLA!");
    }

    // 3. Fallback character when unmapped byte is encountered
    {
        uint8_t buffer[10] = {0};
        BitBuffer bb_enc(buffer, 77);
        std::string non_ascii = "AB\x05" "CD";
        encode_varicode(non_ascii, bb_enc, 77, true, false);
        BitBuffer bb_dec(buffer, 77);
        std::string dec;
        decode_varicode(bb_dec, dec, 77);
        EXPECT_EQ(dec, "AB CD");
    }
}

TEST(VaricodeTest, CorruptOrInvalidBitSequencesDoNotCrash) {
    uint8_t corrupt_buffers[][4] = {
        {0x00, 0x06, 0x80, 0x00}, // 00000000001101 (invalid branch)
        {0xAA, 0xAA, 0xAA, 0xAA},
        {0x55, 0x55, 0x55, 0x55},
        {0xFF, 0x00, 0xFF, 0x00},
        {0x00, 0x00, 0x00, 0x00},
        {0xFF, 0xFF, 0xFF, 0xFF},
    };

    for (const auto& buf : corrupt_buffers) {
        BitBuffer bb_dec1(buf, 32);
        std::string dec;
        bool has_eom = false;
        EXPECT_NO_THROW(decode_varicode(bb_dec1, dec, 32, &has_eom));

        BitBuffer bb_dec2(buf, 32);
        EXPECT_NO_THROW(decode_varicode_token(bb_dec2, 32));
    }
}
