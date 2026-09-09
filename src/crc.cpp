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

#include "lq/crc.h"
#include "lq/constants.h"
#include <cstring>
#include <array>

namespace lq {

namespace {

constexpr auto make_crc14_table() {
    std::array<uint16_t, 256> table{};
    for (int byte = 0; byte < 256; ++byte) {
        uint16_t crc = static_cast<uint16_t>(byte) << 6;
        for (int b = 0; b < 8; ++b) {
            bool top = (crc >> 13) & 1U;
            crc = (crc << 1) & 0x3FFFU;
            if (top) {
                crc ^= CRC14_POLY;
            }
        }
        table[byte] = crc;
    }
    return table;
}

constexpr auto CRC14_TABLE = make_crc14_table();

} // anonymous namespace

uint16_t compute_crc14(const uint8_t* data, int num_bits) {
    if (!data || num_bits <= 0) return 0;

    uint16_t crc = 0;
    int full_bytes = num_bits / 8;
    for (int i = 0; i < full_bytes; ++i) {
        uint8_t top_byte = static_cast<uint8_t>(crc >> 6);
        uint8_t idx = top_byte ^ data[i];
        crc = ((crc << 8) & 0x3FFFU) ^ CRC14_TABLE[idx];
    }

    // Process remaining trailing bits
    for (int i = full_bytes * 8; i < num_bits; ++i) {
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        uint8_t bit = (data[byte_idx] >> bit_idx) & 1U;

        bool top = ((crc >> 13) ^ bit) & 1U;
        crc = (crc << 1) & 0x3FFFU;
        if (top) {
            crc ^= CRC14_POLY;
        }
    }
    return crc;
}

uint16_t compute_payload_crc14(const uint8_t payload[PAYLOAD_BYTES]) {
    // Highly optimized unrolled CRC-14 for standard 77-bit payload zero-extended to 82 bits
    uint16_t crc = 0;
    #pragma GCC unroll 9
    for (int i = 0; i < 9; ++i) {
        uint8_t top_byte = static_cast<uint8_t>(crc >> 6);
        uint8_t idx = top_byte ^ payload[i];
        crc = ((crc << 8) & 0x3FFFU) ^ CRC14_TABLE[idx];
    }

    // 10th byte with 5 payload bits (bits 72..76) and 3 zero bits
    uint8_t top_byte = static_cast<uint8_t>(crc >> 6);
    uint8_t idx = top_byte ^ (payload[9] & 0xF8U);
    crc = ((crc << 8) & 0x3FFFU) ^ CRC14_TABLE[idx];

    // 2 trailing zero bits to reach 82 bits
    for (int b = 0; b < 2; ++b) {
        bool top = (crc >> 13) & 1U;
        crc = (crc << 1) & 0x3FFFU;
        if (top) {
            crc ^= CRC14_POLY;
        }
    }
    return crc;
}

void append_crc14(const uint8_t payload[PAYLOAD_BYTES], uint8_t out_91[LDPC_INPUT_BYTES]) {
    std::memset(out_91, 0, LDPC_INPUT_BYTES);

    // Fast copy 77 payload bits
    std::memcpy(out_91, payload, 9);
    out_91[9] = payload[9] & 0xF8U;

    // Compute CRC-14 over the 77 payload bits zero-extended to 82 bits
    uint16_t crc = compute_payload_crc14(payload);

    // Pack 14 CRC bits into bits 77..90:
    // Bit 77 is bit 5 of byte 9.
    // Bits 77..79 (3 bits): top 3 bits of CRC (crc >> 11) & 0x07
    out_91[9] |= static_cast<uint8_t>((crc >> 11) & 0x07U);
    // Bits 80..87 (8 bits): next 8 bits of CRC (crc >> 3) & 0xFF
    out_91[10] = static_cast<uint8_t>((crc >> 3) & 0xFFU);
    // Bits 88..90 (3 bits): bottom 3 bits of CRC shifted to top 3 of byte 11
    out_91[11] = static_cast<uint8_t>((crc & 0x07U) << 5);
}

bool verify_crc14(const uint8_t data_91[LDPC_INPUT_BYTES]) {
    if (!data_91) {
        return false;
    }

    // Check if the entire 91-bit sequence is all zeros (LDPC nullspace / unseeded CRC-14 nullspace)
    bool all_zero = true;
    for (int i = 0; i < LDPC_INPUT_BYTES; ++i) {
        if (data_91[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (all_zero) {
        return false;
    }

    uint8_t payload[PAYLOAD_BYTES];
    extract_payload(data_91, payload);

    uint16_t expected_crc = compute_payload_crc14(payload);

    // Extract 14 CRC bits directly from bytes 9..11
    uint16_t actual_crc = static_cast<uint16_t>((data_91[9] & 0x07U) << 11) |
                          static_cast<uint16_t>(data_91[10] << 3) |
                          static_cast<uint16_t>(data_91[11] >> 5);

    return expected_crc == actual_crc;
}

void extract_payload(const uint8_t data_91[LDPC_INPUT_BYTES], uint8_t payload[PAYLOAD_BYTES]) {
    std::memset(payload, 0, PAYLOAD_BYTES);
    std::memcpy(payload, data_91, 9);
    payload[9] = data_91[9] & 0xF8U;
}

} // namespace lq
