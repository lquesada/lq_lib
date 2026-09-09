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

/**
 * @file test_ft8_compat.cpp
 * @brief Comprehensive cross-compatibility test suite between lq_lib and ft8_lib.
 *
 * This test suite validates that lq_lib's physical / transport layer is 100% compatible
 * with ft8_lib (and the standard FT8/FT4 transport layer):
 *
 * 1. CRC-14 calculation and zero-extension identity.
 * 2. LDPC(174,91) parity generator and belief propagation decoding.
 * 3. Tone sequence generation (Bits-to-tones) identity for FT8 / LQ8 (79 tones).
 * 4. Tone sequence generation (Bits-to-tones) identity for FT4 / LQ4 (105 tones).
 * 5. Cross-decoding: LQ encoded tones decoded by ft8_lib.
 * 6. Cross-decoding: FT8/FT4 encoded tones decoded by lq_lib.
 * 7. Audio baseband modulation and soft-decision demodulation.
 * 8. AWGN channel robustness.
 * 9. All 13 structured and free-text message types.
 */

#include <gtest/gtest.h>
#include <random>
#include <vector>
#include <cmath>
#include <cstring>
#include <iostream>

#include "lq/lq.h"
#include "lq/crc.h"
#include "lq/ldpc.h"
#include "lq/transport.h"
#include "lq/message.h"

extern "C" {
#include "ft8/constants.h"
#include "ft8/crc.h"
#include "ft8/encode.h"
#include "ft8/decode.h"
#include "ft8/ldpc.h"
}

namespace {

// Helper to generate deterministic random bytes
std::vector<uint8_t> make_random_payload(std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> payload(10);
    for (int i = 0; i < 10; ++i) {
        payload[i] = static_cast<uint8_t>(dist(rng));
    }
    payload[9] &= 0xF8; // 77 bits (last 3 bits clear)
    return payload;
}

// Convert 79 tones to synthetic log-likelihoods for ft8_lib bp_decode (positive for 1, negative for 0)
void tones_to_ft8_logl(const uint8_t* tones, float* log174) {
    // Inverse Gray map for FT8
    // Costas sync symbols at 0..6, 36..42, 72..78
    // Data Block 1: 7..35 (29 symbols x 3 bits = 87 bits)
    // Data Block 2: 43..71 (29 symbols x 3 bits = 87 bits)
    int bit_idx = 0;

    auto process_sym = [&](uint8_t tone) {
        uint8_t val3 = lq::GRAY_INV_MAP_8[tone & 7];
        // Bit 0 (MSB)
        log174[bit_idx++] = (val3 & 4) ? +6.0f : -6.0f;
        // Bit 1
        log174[bit_idx++] = (val3 & 2) ? +6.0f : -6.0f;
        // Bit 2 (LSB)
        log174[bit_idx++] = (val3 & 1) ? +6.0f : -6.0f;
    };

    for (int i = 7; i <= 35; ++i) {
        process_sym(tones[i]);
    }
    for (int i = 43; i <= 71; ++i) {
        process_sym(tones[i]);
    }
}

// Convert 105 FT4 tones to synthetic log-likelihoods for ft8_lib bp_decode (positive for 1, negative for 0)
void tones_to_ft4_logl(const uint8_t* tones, float* log174) {
    int bit_idx = 0;

    auto process_sym = [&](uint8_t tone) {
        uint8_t val2 = lq::GRAY_INV_MAP_4[tone & 3];
        // Bit 0 (MSB)
        log174[bit_idx++] = (val2 & 2) ? +6.0f : -6.0f;
        // Bit 1 (LSB)
        log174[bit_idx++] = (val2 & 1) ? +6.0f : -6.0f;
    };

    // Block 1: 5..33 (29 symbols x 2 bits = 58 bits)
    for (int i = 5; i <= 33; ++i) process_sym(tones[i]);
    // Block 2: 38..66 (29 symbols x 2 bits = 58 bits)
    for (int i = 38; i <= 66; ++i) process_sym(tones[i]);
    // Block 3: 71..99 (29 symbols x 2 bits = 58 bits)
    for (int i = 71; i <= 99; ++i) process_sym(tones[i]);
}

} // anonymous namespace

// ============================================================================
// 1. CRC-14 Exact Equivalence Tests
// ============================================================================

TEST(Ft8Compat, Crc14CalculationEquivalence) {
    std::mt19937 rng(42);

    for (int test = 0; test < 1000; ++test) {
        auto payload = make_random_payload(rng);

        // ft8_lib CRC computation
        uint8_t a91_ft8[FTX_LDPC_K_BYTES];
        ftx_add_crc(payload.data(), a91_ft8);
        uint16_t crc_ft8 = ftx_extract_crc(a91_ft8);

        // lq_lib CRC computation
        uint16_t crc_lq = lq::compute_payload_crc14(payload.data());
        EXPECT_EQ(crc_lq, crc_ft8) << "CRC mismatch at test iteration " << test;

        // lq_lib append_crc14
        uint8_t a91_lq[lq::LDPC_INPUT_BYTES];
        lq::append_crc14(payload.data(), a91_lq);

        // Compare packed 91-bit array bit-for-bit (first 11 bytes + top 3 bits of byte 11)
        for (int i = 0; i < 11; ++i) {
            EXPECT_EQ(a91_lq[i], a91_ft8[i]) << "a91 byte mismatch at byte " << i;
        }
        EXPECT_EQ(a91_lq[11] & 0xE0, a91_ft8[11] & 0xE0) << "a91 byte 11 top 3 bits mismatch";

        // Verify with verify_crc14
        EXPECT_TRUE(lq::verify_crc14(a91_ft8));
        EXPECT_TRUE(lq::verify_crc14(a91_lq));
    }
}

// ============================================================================
// 2. LDPC(174, 91) Encoding & Parity Generator Equivalence Tests
// ============================================================================

TEST(Ft8Compat, LdpcEncodingEquivalence) {
    std::mt19937 rng(1337);

    for (int test = 0; test < 1000; ++test) {
        auto payload = make_random_payload(rng);

        uint8_t a91[12];
        lq::append_crc14(payload.data(), a91);

        // lq_lib LDPC encode
        uint8_t codeword_lq[22];
        lq::ldpc_encode(a91, codeword_lq);

        // ft8_lib LDPC encode (via encode174 inside ft8_encode or directly)
        uint8_t tones_ft8[79];
        ft8_encode(payload.data(), tones_ft8);

        // lq_lib tone mapping
        lq::ToneSequence seq_lq;
        lq::codeword_to_tones(codeword_lq, lq::Protocol::LQ8, seq_lq);

        // Verify that data tones are 100% bit-identical
        for (size_t i = 7; i <= 35; ++i) {
            EXPECT_EQ(seq_lq[i], tones_ft8[i]) << "Data Block 1 mismatch at index " << i;
        }
        for (size_t i = 43; i <= 71; ++i) {
            EXPECT_EQ(seq_lq[i], tones_ft8[i]) << "Data Block 2 mismatch at index " << i;
        }
        // Verify unique Costas tones
        for (size_t i = 0; i < 7; ++i) {
            EXPECT_EQ(seq_lq[i], lq::COSTAS_ARRAY_8[i]);
            EXPECT_EQ(seq_lq[36 + i], lq::COSTAS_ARRAY_8[i]);
            EXPECT_EQ(seq_lq[72 + i], lq::COSTAS_ARRAY_8[i]);
        }
    }
}

// ============================================================================
// 3. Data Bits-to-Tones Tone Sequence Identity Tests (FT8 <-> LQ8)
// ============================================================================

TEST(Ft8Compat, LQ8_DataTones_FT8_DataTones_Equivalence) {
    std::mt19937 rng(2026);

    for (int test = 0; test < 1000; ++test) {
        auto payload = make_random_payload(rng);

        uint8_t ft8_tones[79];
        ft8_encode(payload.data(), ft8_tones);

        uint8_t lq_tones[79];
        ASSERT_TRUE(lq::encode_payload(payload.data(), lq_tones, lq::Protocol::LQ8));

        // Data symbols match identically
        for (int i = 7; i <= 35; ++i) {
            EXPECT_EQ(lq_tones[i], ft8_tones[i]) << "LQ8 vs FT8 data tone mismatch at index " << i << " test " << test;
        }
        for (int i = 43; i <= 71; ++i) {
            EXPECT_EQ(lq_tones[i], ft8_tones[i]) << "LQ8 vs FT8 data tone mismatch at index " << i << " test " << test;
        }
        // Costas sync symbols use unique LQ sequence
        for (int i = 0; i < 7; ++i) {
            EXPECT_EQ(lq_tones[i], lq::COSTAS_ARRAY_8[i]);
            EXPECT_EQ(lq_tones[36 + i], lq::COSTAS_ARRAY_8[i]);
            EXPECT_EQ(lq_tones[72 + i], lq::COSTAS_ARRAY_8[i]);
        }
    }
}

// ============================================================================
// 4. Data Bits-to-Tones Tone Sequence Identity Tests (FT4 <-> LQ4)
// ============================================================================

TEST(Ft8Compat, LQ4_DataTones_FT4_DataTones_Equivalence) {
    std::mt19937 rng(4048);

    for (int test = 0; test < 1000; ++test) {
        auto payload = make_random_payload(rng);

        uint8_t ft4_tones[105];
        ft4_encode(payload.data(), ft4_tones);

        uint8_t lq4_tones[105];
        ASSERT_TRUE(lq::encode_payload(payload.data(), lq4_tones, lq::Protocol::LQ4));

        // Data blocks match identically
        for (int i = 5; i <= 33; ++i) {
            EXPECT_EQ(lq4_tones[i], ft4_tones[i]) << "LQ4 vs FT4 Block 1 mismatch at index " << i;
        }
        for (int i = 38; i <= 66; ++i) {
            EXPECT_EQ(lq4_tones[i], ft4_tones[i]) << "LQ4 vs FT4 Block 2 mismatch at index " << i;
        }
        for (int i = 71; i <= 99; ++i) {
            EXPECT_EQ(lq4_tones[i], ft4_tones[i]) << "LQ4 vs FT4 Block 3 mismatch at index " << i;
        }
        // Costas sync blocks use unique LQ patterns
        for (int i = 0; i < 4; ++i) EXPECT_EQ(lq4_tones[1 + i], lq::COSTAS_SYNC1_4[i]);
        for (int i = 0; i < 4; ++i) EXPECT_EQ(lq4_tones[34 + i], lq::COSTAS_SYNC2_4[i]);
        for (int i = 0; i < 4; ++i) EXPECT_EQ(lq4_tones[67 + i], lq::COSTAS_SYNC3_4[i]);
        for (int i = 0; i < 4; ++i) EXPECT_EQ(lq4_tones[100 + i], lq::COSTAS_SYNC4_4[i]);
    }
}

// ============================================================================
// 5. Cross-Decoding: LQ8 -> FT8 Decode Pipeline
// ============================================================================

TEST(Ft8Compat, CrossDecode_LQ_Encode_FT8_Decode) {
    std::mt19937 rng(777);

    for (int test = 0; test < 200; ++test) {
        auto payload = make_random_payload(rng);

        // 1. Encode with lq_lib
        uint8_t lq_tones[79];
        ASSERT_TRUE(lq::encode_payload(payload.data(), lq_tones, lq::Protocol::LQ8));

        // 2. Convert tones to soft likelihoods
        float log174[FTX_LDPC_N];
        tones_to_ft8_logl(lq_tones, log174);

        // 3. Decode with ft8_lib bp_decode
        uint8_t plain174[FTX_LDPC_N];
        int errors = -1;
        bp_decode(log174, 50, plain174, &errors);
        EXPECT_EQ(errors, 0) << "ft8_lib bp_decode reported parity errors on LQ8 tones";

        // 4. Extract 91 bits in ft8_lib
        uint8_t a91_recovered[FTX_LDPC_K_BYTES] = {0};
        for (int i = 0; i < FTX_LDPC_K; ++i) {
            if (plain174[i]) {
                a91_recovered[i / 8] |= (0x80 >> (i % 8));
            }
        }

        // 5. Verify CRC in ft8_lib
        uint16_t crc_extracted = ftx_extract_crc(a91_recovered);
        a91_recovered[9] &= 0xF8;
        a91_recovered[10] = 0;
        uint16_t crc_calc = ftx_compute_crc(a91_recovered, 82);
        EXPECT_EQ(crc_extracted, crc_calc) << "ft8_lib CRC verification failed on LQ8 decoded payload";

        // 6. Check recovered payload matches original
        for (int i = 0; i < 9; ++i) {
            EXPECT_EQ(a91_recovered[i], payload[i]);
        }
        EXPECT_EQ(a91_recovered[9] & 0xF8, payload[9] & 0xF8);
    }
}

// ============================================================================
// 6. Cross-Decoding: FT8 -> LQ8 Decode Pipeline
// ============================================================================

TEST(Ft8Compat, CrossDecode_FT8_Encode_LQ_Decode) {
    std::mt19937 rng(888);

    for (int test = 0; test < 200; ++test) {
        auto payload = make_random_payload(rng);

        // 1. Encode with ft8_lib
        uint8_t ft8_tones[79];
        ft8_encode(payload.data(), ft8_tones);

        // 2. Full frame decode with lq_lib MUST reject FT8 frames due to unique Costas sequence
        uint8_t lq_recovered[lq::PAYLOAD_BYTES] = {0};
        EXPECT_FALSE(lq::decode_payload(ft8_tones, lq_recovered, lq::Protocol::LQ8));

        // 3. Verify data payload decoding directly from data tones
        lq::ToneSequence data_seq;
        data_seq.protocol = lq::Protocol::LQ8;
        data_seq.tones.assign(ft8_tones, ft8_tones + 79);
        // Replace sync bursts with LQ8 Costas array so verify_sync_tones passes
        for (int i = 0; i < 7; ++i) {
            data_seq.tones[i] = lq::COSTAS_ARRAY_8[i];
            data_seq.tones[36 + i] = lq::COSTAS_ARRAY_8[i];
            data_seq.tones[72 + i] = lq::COSTAS_ARRAY_8[i];
        }
        ASSERT_TRUE(lq::decode_tones(data_seq, lq_recovered));

        // 4. Verify exact data payload match
        for (int i = 0; i < 9; ++i) {
            EXPECT_EQ(lq_recovered[i], payload[i]) << "Payload mismatch at byte " << i;
        }
        EXPECT_EQ(lq_recovered[9] & 0xF8, payload[9] & 0xF8);
    }
}

// ============================================================================
// 7. Cross-Decoding: LQ4 -> FT4 Decode Pipeline
// ============================================================================

TEST(Ft8Compat, CrossDecode_LQ4_Encode_FT4_Decode) {
    std::mt19937 rng(999);

    for (int test = 0; test < 200; ++test) {
        auto payload = make_random_payload(rng);

        // 1. Encode with lq_lib LQ4
        uint8_t lq4_tones[105];
        ASSERT_TRUE(lq::encode_payload(payload.data(), lq4_tones, lq::Protocol::LQ4));

        // 2. Convert tones to soft likelihoods
        float log174[FTX_LDPC_N];
        tones_to_ft4_logl(lq4_tones, log174);

        // 3. Decode with ft8_lib bp_decode
        uint8_t plain174[FTX_LDPC_N];
        int errors = -1;
        bp_decode(log174, 50, plain174, &errors);
        EXPECT_EQ(errors, 0) << "ft8_lib bp_decode failed on LQ4 tones";

        // 4. Extract 91 bits in ft8_lib
        uint8_t a91_recovered[FTX_LDPC_K_BYTES] = {0};
        for (int i = 0; i < FTX_LDPC_K; ++i) {
            if (plain174[i]) {
                a91_recovered[i / 8] |= (0x80 >> (i % 8));
            }
        }

        // 5. Verify CRC in ft8_lib
        uint16_t crc_extracted = ftx_extract_crc(a91_recovered);
        a91_recovered[9] &= 0xF8;
        a91_recovered[10] = 0;
        uint16_t crc_calc = ftx_compute_crc(a91_recovered, 82);
        EXPECT_EQ(crc_extracted, crc_calc) << "ft8_lib CRC verification failed on LQ4 decoded payload";

        // 6. De-whiten FT4 XOR sequence
        for (int i = 0; i < 10; ++i) {
            a91_recovered[i] ^= kFT4_XOR_sequence[i];
        }

        // 7. Check recovered payload matches original
        for (int i = 0; i < 9; ++i) {
            EXPECT_EQ(a91_recovered[i], payload[i]);
        }
        EXPECT_EQ(a91_recovered[9] & 0xF8, payload[9] & 0xF8);
    }
}

// ============================================================================
// 8. Cross-Decoding: FT4 -> LQ4 Decode Pipeline
// ============================================================================

TEST(Ft8Compat, CrossDecode_FT4_Encode_LQ4_Decode) {
    std::mt19937 rng(1111);

    for (int test = 0; test < 200; ++test) {
        auto payload = make_random_payload(rng);

        // 1. Encode with ft8_lib ft4_encode
        uint8_t ft4_tones[105];
        ft4_encode(payload.data(), ft4_tones);

        // 2. Decode with lq_lib (injecting LQ4 sync bursts)
        lq::ToneSequence data_seq;
        data_seq.protocol = lq::Protocol::LQ4;
        data_seq.tones.assign(ft4_tones, ft4_tones + 105);
        for (int i = 0; i < 4; ++i) {
            data_seq.tones[1 + i] = lq::COSTAS_SYNC1_4[i];
            data_seq.tones[34 + i] = lq::COSTAS_SYNC2_4[i];
            data_seq.tones[67 + i] = lq::COSTAS_SYNC3_4[i];
            data_seq.tones[100 + i] = lq::COSTAS_SYNC4_4[i];
        }

        uint8_t lq_recovered[lq::PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(lq::decode_tones(data_seq, lq_recovered));

        // 3. Verify exact match
        for (int i = 0; i < 9; ++i) {
            EXPECT_EQ(lq_recovered[i], payload[i]) << "Payload mismatch at byte " << i;
        }
        EXPECT_EQ(lq_recovered[9] & 0xF8, payload[9] & 0xF8);
    }
}

// ============================================================================
// 9. Audio Waveform Cross-Demodulation & AWGN Robustness
// ============================================================================

TEST(Ft8Compat, AudioModulationAndSoftDemodulationCrossTest) {
    std::mt19937 rng(5555);
    std::normal_distribution<float> noise_dist(0.0f, 0.15f); // Clean SNR

    for (int test = 0; test < 5; ++test) {
        auto payload = make_random_payload(rng);

        lq::ToneSequence seq;
        ASSERT_TRUE(lq::encode_payload(payload.data(), lq::Protocol::LQ8, seq));

        std::vector<float> audio;
        lq::generate_audio(seq, 1500.0f, 12000.0f, audio);

        // Add slight noise
        for (auto& s : audio) {
            s += noise_dist(rng);
        }

        // Demodulate with lq_lib soft LLR demodulator
        std::vector<float> llrs;
        ASSERT_TRUE(lq::demodulate_audio_soft(audio, 0, 1500.0f, 12000.0f, lq::Protocol::LQ8, llrs));

        // Decode with ft8_lib bp_decode (ft8_lib expects positive for 1, negative for 0)
        float ft8_llrs[FTX_LDPC_N];
        for (int i = 0; i < FTX_LDPC_N; ++i) {
            ft8_llrs[i] = -llrs[i];
        }

        uint8_t plain174[FTX_LDPC_N];
        int errors = -1;
        bp_decode(ft8_llrs, 50, plain174, &errors);
        EXPECT_EQ(errors, 0) << "ft8_lib bp_decode failed on audio soft LLRs";

        // Extract 91 bits in ft8_lib
        uint8_t a91_recovered[FTX_LDPC_K_BYTES] = {0};
        for (int i = 0; i < FTX_LDPC_K; ++i) {
            if (plain174[i]) {
                a91_recovered[i / 8] |= (0x80 >> (i % 8));
            }
        }

        // Verify CRC
        uint16_t crc_extracted = ftx_extract_crc(a91_recovered);
        a91_recovered[9] &= 0xF8;
        a91_recovered[10] = 0;
        uint16_t crc_calc = ftx_compute_crc(a91_recovered, 82);
        EXPECT_EQ(crc_extracted, crc_calc);
    }
}

// ============================================================================
// 10. All 13 Structured and Free-Text Message Types Cross-Interop
// ============================================================================

TEST(Ft8Compat, AllStructuredMessageTypesCrossInterop) {
    std::vector<lq::Message> test_messages;

    // Type 1: CQ Standard
    { lq::Message m; m.type = lq::MessageType::CQ_STD; m.call_1 = "HB9IPH"; m.modifier = "DX"; m.locator = "JN47"; test_messages.push_back(m); }
    // Type 2: CQ Non-Std 1 (9-char + loc)
    { lq::Message m; m.type = lq::MessageType::CQ_NONSTD_1; m.call_1 = "EA6/HB9IP"; m.locator = "JN47"; test_messages.push_back(m); }
    // Type 3: CQ Non-Std 2 (9-char + mod)
    { lq::Message m; m.type = lq::MessageType::CQ_NONSTD_2; m.call_1 = "EA6/HB9IP"; m.modifier = "TEST"; test_messages.push_back(m); }
    // Type 4: CQ Non-Std 3 (13-char base-38 + 1b suffix)
    { lq::Message m; m.type = lq::MessageType::CQ_NONSTD_3; m.call_1 = "3B9/HB9IPH/P"; m.suffix_1 = 1; test_messages.push_back(m); }
    // Type 5: CALL Standard No-Suf
    { lq::Message m; m.type = lq::MessageType::CALL_STD_NOSUF; m.call_1 = "YO1YO"; m.call_2 = "HB9IPH"; m.locator = "JN47"; m.rst_db = -3; test_messages.push_back(m); }
    // Type 6: CALL Standard Suf
    { lq::Message m; m.type = lq::MessageType::CALL_STD_SUF; m.hash_1 = lq::hash_callsign_24("YO1YO"); m.call_2 = "HB9IPH/P"; m.suffix_2 = 1; m.locator = "JN47"; m.rst_db = -3; test_messages.push_back(m); }
    // Type 7: CALL Non-Std
    { lq::Message m; m.type = lq::MessageType::CALL_NONSTD; m.hash_1 = 0x01A4F; m.call_2 = "EA6/HB9IP"; m.rst_db = -3; test_messages.push_back(m); }
    // Type 8: REPORT+73 Standard
    { lq::Message m; m.type = lq::MessageType::REPORT73_STD; m.call_1 = "HB9IPH"; m.call_2 = "YO1YO"; m.rst_db = 5; test_messages.push_back(m); }
    // Type 9: 73 Standard
    { lq::Message m; m.type = lq::MessageType::M73_STD; m.call_1 = "HB9IPH"; m.call_2 = "YO1YO"; test_messages.push_back(m); }
    // Type 10: 73 Non-Std
    { lq::Message m; m.type = lq::MessageType::M73_NONSTD; m.hash_1 = 0x9C3B11; m.call_2 = "EA6/HB9IP"; test_messages.push_back(m); }
    // Type 11: MULTI-REPORT+73
    {
        lq::Message m;
        m.type = lq::MessageType::MULTI_REPORT73;
        m.call_1 = "HB9IPH";
        m.hash_1 = lq::hash_callsign_16("HB9IPH");
        m.multi_targets = {
            {"", lq::hash_callsign_24("YO1YO"), 5},
            {"", lq::hash_callsign_24("TU2TU"), -3}
        };
        test_messages.push_back(m);
    }
    // Type 12: MULTI-73
    {
        lq::Message m;
        m.type = lq::MessageType::MULTI_73;
        m.call_1 = "HB9IPH";
        m.hash_1 = lq::hash_callsign_16("HB9IPH");
        m.multi_targets = {
            {"", lq::hash_callsign_24("YO1YO"), 0},
            {"", lq::hash_callsign_24("TU2TU"), 0}
        };
        test_messages.push_back(m);
    }
    // Type 13: FREE TEXT
    { lq::Message m; m.type = lq::MessageType::FREE_TEXT; m.text = "73 DE HB9IPH"; test_messages.push_back(m); }
    // Type 14: RESERVED A
    { lq::Message m; m.type = lq::MessageType::RESERVED_A; m.raw_payload = {0x01, 0x02, 0x03, 0x04}; test_messages.push_back(m); }
    // Type 13: FREE TEXT 2
    { lq::Message m; m.type = lq::MessageType::FREE_TEXT; m.text = "TNX FOR QSO 7"; test_messages.push_back(m); }
    // Type 13: FREE TEXT 3
    { lq::Message m; m.type = lq::MessageType::FREE_TEXT; m.text = "TEST MSG 18"; test_messages.push_back(m); }


    for (const auto& msg : test_messages) {
        // Encode message with lq_lib
        lq::ToneSequence seq;
        ASSERT_TRUE(lq::encode_tones(msg, lq::Protocol::LQ8, seq));
        EXPECT_EQ(seq.size(), 79U);

        // Convert tones to LLRs and decode with ft8_lib bp_decode
        float log174[FTX_LDPC_N];
        tones_to_ft8_logl(seq.tones.data(), log174);

        uint8_t plain174[FTX_LDPC_N];
        int errors = -1;
        bp_decode(log174, 50, plain174, &errors);
        EXPECT_EQ(errors, 0) << "ft8_lib failed to decode message type " << static_cast<int>(msg.type);

        // Extract payload with ft8_lib
        uint8_t a91_recovered[FTX_LDPC_K_BYTES] = {0};
        for (int i = 0; i < FTX_LDPC_K; ++i) {
            if (plain174[i]) {
                a91_recovered[i / 8] |= (0x80 >> (i % 8));
            }
        }

        // Verify CRC in ft8_lib
        uint16_t crc_extracted = ftx_extract_crc(a91_recovered);
        a91_recovered[9] &= 0xF8;
        a91_recovered[10] = 0;
        uint16_t crc_calc = ftx_compute_crc(a91_recovered, 82);
        EXPECT_EQ(crc_extracted, crc_calc);

        // Decode with lq_lib decode_message
        lq::Message decoded;
        ASSERT_TRUE(lq::decode_message(a91_recovered, decoded));
        EXPECT_EQ(decoded.type, msg.type);
    }
}

// ============================================================================
// 11. Corrupted Frame & Parity Error Rejection Tests
// ============================================================================

TEST(Ft8Compat, CorruptedFrameRejection) {
    std::mt19937 rng(9999);
    auto payload = make_random_payload(rng);

    uint8_t ft8_tones[79];
    ft8_encode(payload.data(), ft8_tones);

    // Flip every individual tone to an invalid value and ensure either LDPC or CRC rejects
    for (int t = 7; t < 72; ++t) {
        if (t >= 36 && t < 43) continue; // Skip sync tones

        uint8_t corrupted_tones[79];
        std::memcpy(corrupted_tones, ft8_tones, 79);
        corrupted_tones[t] = (corrupted_tones[t] + 4) % 8; // Major tone flip

        float log174[FTX_LDPC_N];
        tones_to_ft8_logl(corrupted_tones, log174);

        uint8_t plain174[FTX_LDPC_N];
        int errors = -1;
        bp_decode(log174, 50, plain174, &errors);

        if (errors == 0) {
            // If LDPC converged, CRC MUST detect error
            uint8_t a91_rec[FTX_LDPC_K_BYTES] = {0};
            for (int i = 0; i < FTX_LDPC_K; ++i) {
                if (plain174[i]) a91_rec[i / 8] |= (0x80 >> (i % 8));
            }
            uint16_t crc_ext = ftx_extract_crc(a91_rec);
            a91_rec[9] &= 0xF8;
            a91_rec[10] = 0;
            uint16_t crc_calc = ftx_compute_crc(a91_rec, 82);
            EXPECT_TRUE(crc_ext != crc_calc || lq::verify_crc14(a91_rec) == false);
        }
    }
}

// ============================================================================
// 12. Soft LLR AWGN Channel Error Correction Performance
// ============================================================================

TEST(Ft8Compat, SoftLlrAWGNDecodingPerformance) {
    std::mt19937 rng(12345);
    std::normal_distribution<float> noise_dist(0.0f, 0.70f); // Moderate noise

    int lq_success = 0;
    int ft8_success = 0;
    const int total_trials = 50;

    for (int trial = 0; trial < total_trials; ++trial) {
        auto payload = make_random_payload(rng);

        uint8_t tones[79];
        lq::encode_payload(payload.data(), tones, lq::Protocol::LQ8);

        float log174[174];
        tones_to_ft8_logl(tones, log174);

        // Add channel noise to LLRs
        for (int i = 0; i < 174; ++i) {
            log174[i] += noise_dist(rng);
        }

        // Test ft8_lib bp_decode
        uint8_t plain174[174];
        int errors_ft8 = -1;
        bp_decode(log174, 50, plain174, &errors_ft8);
        if (errors_ft8 == 0) {
            uint8_t a91_rec[FTX_LDPC_K_BYTES] = {0};
            for (int i = 0; i < FTX_LDPC_K; ++i) {
                if (plain174[i]) a91_rec[i / 8] |= (0x80 >> (i % 8));
            }
            if (lq::verify_crc14(a91_rec)) {
                ft8_success++;
            }
        }

        // Test lq_lib ldpc_decode (convert log174 sign to lq convention)
        float lq_llrs[174];
        for (int i = 0; i < 174; ++i) lq_llrs[i] = -log174[i];
        uint8_t out_91[12];
        int iters_lq = lq::ldpc_decode(lq_llrs, out_91, 50);
        if (iters_lq >= 0 && lq::verify_crc14(out_91)) {
            lq_success++;
        }
    }

    EXPECT_GT(ft8_success, 0);
    EXPECT_GT(lq_success, 0);
    // Both LDPC decoders should have comparable performance
    EXPECT_NEAR(lq_success, ft8_success, total_trials * 0.20);
}

// ============================================================================
// 13. Direct C-Array API Usability & Ergonomics
// ============================================================================

TEST(Ft8Compat, DirectCArrayApiUsability) {
    uint8_t payload[10] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x90};

    // LQ8
    uint8_t tones8[79];
    EXPECT_TRUE(lq::encode_payload(payload, tones8, lq::Protocol::LQ8));
    uint8_t recovered8[10];
    EXPECT_TRUE(lq::decode_payload(tones8, recovered8, lq::Protocol::LQ8));
    for (int i = 0; i < 9; ++i) EXPECT_EQ(recovered8[i], payload[i]);

    // LQ4
    uint8_t tones4[105];
    EXPECT_TRUE(lq::encode_payload(payload, tones4, lq::Protocol::LQ4));
    uint8_t recovered4[10];
    EXPECT_TRUE(lq::decode_payload(tones4, recovered4, lq::Protocol::LQ4));
    for (int i = 0; i < 9; ++i) EXPECT_EQ(recovered4[i], payload[i]);

    // LQ2
    uint8_t tones2[105];
    EXPECT_TRUE(lq::encode_payload(payload, tones2, lq::Protocol::LQ2));
    uint8_t recovered2[10];
    EXPECT_TRUE(lq::decode_payload(tones2, recovered2, lq::Protocol::LQ2));
    for (int i = 0; i < 9; ++i) EXPECT_EQ(recovered2[i], payload[i]);

    // LQ16
    uint8_t tones16[79];
    EXPECT_TRUE(lq::encode_payload(payload, tones16, lq::Protocol::LQ16));
    uint8_t recovered16[10];
    EXPECT_TRUE(lq::decode_payload(tones16, recovered16, lq::Protocol::LQ16));
    for (int i = 0; i < 9; ++i) EXPECT_EQ(recovered16[i], payload[i]);
}

// ============================================================================
// 14. Multi-Signal 6 Simultaneous Transmissions Cross-Verification
// ============================================================================

TEST(Ft8Compat, MultiSignalSimultaneousSixTransmissionsCrossVerification) {
    std::vector<lq::Message> tx_msgs;
    std::vector<float> freqs = {400.0f, 750.0f, 1100.0f, 1450.0f, 1800.0f, 2150.0f};

    { lq::Message m; m.type = lq::MessageType::CQ_STD; m.call_1 = "W1AW"; m.locator = "FN31"; tx_msgs.push_back(m); }
    { lq::Message m; m.type = lq::MessageType::CQ_STD; m.call_1 = "HB9IPH"; m.locator = "JN47"; tx_msgs.push_back(m); }
    { lq::Message m; m.type = lq::MessageType::CQ_STD; m.call_1 = "YO1YO"; m.locator = "KN34"; tx_msgs.push_back(m); }
    { lq::Message m; m.type = lq::MessageType::CQ_STD; m.call_1 = "JA1ABC"; m.locator = "PM95"; tx_msgs.push_back(m); }
    { lq::Message m; m.type = lq::MessageType::CQ_STD; m.call_1 = "VK2BKL"; m.locator = "QF56"; tx_msgs.push_back(m); }
    { lq::Message m; m.type = lq::MessageType::CALL_STD; m.call_1 = "K1ABC"; m.call_2 = "DL1ABC"; m.locator = "JO62"; m.rst_db = -10; tx_msgs.push_back(m); }

    std::vector<std::vector<float>> audios(6);
    size_t min_len = 99999999;
    for (size_t i = 0; i < 6; ++i) {
        ASSERT_TRUE(lq::message_to_audio(tx_msgs[i], lq::Protocol::LQ8, freqs[i], 12000.0f, audios[i]));
        min_len = std::min(min_len, audios[i].size());
    }

    std::vector<float> combined(min_len, 0.0f);
    for (size_t i = 0; i < min_len; ++i) {
        for (size_t s = 0; s < 6; ++s) {
            combined[i] += (1.0f / 6.0f) * audios[s][i];
        }
    }

    auto decoded_list = lq::audio_to_messages(combined, 0.0f, 12000.0f, lq::Protocol::LQ8);

    // Verify all 6 messages are found and their payloads match ft8_lib CRC verification
    for (const auto& tx : tx_msgs) {
        bool found = false;
        for (const auto& rx : decoded_list) {
            if (rx.call_1 == tx.call_1 && rx.type == tx.type) {
                found = true;
                uint8_t payload[10];
                ASSERT_TRUE(lq::encode_message(rx, payload));
                uint8_t a91[12];
                lq::append_crc14(payload, a91);
                EXPECT_TRUE(lq::verify_crc14(a91));
                uint16_t crc_ft8 = ftx_extract_crc(a91);
                a91[9] &= 0xF8;
                a91[10] = 0;
                EXPECT_EQ(crc_ft8, ftx_compute_crc(a91, 82));
                break;
            }
        }
        EXPECT_TRUE(found) << "Failed to find decoded simultaneous signal: " << tx.call_1;
    }
}
