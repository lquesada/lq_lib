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

#ifndef LQ_CRC_H
#define LQ_CRC_H

#include <cstdint>
#include <cstddef>
#include "lq/types.h"

namespace lq {

/**
 * Compute the 14-bit CRC over an array of bytes containing `num_bits` bits (MSB-first).
 * Uses polynomial 0x2757 (standard FT8/FT4 CRC-14).
 * Returns the 14-bit CRC (0..0x3FFF).
 */
uint16_t compute_crc14(const uint8_t* data, int num_bits = PAYLOAD_BITS);

/**
 * Compute the transmission 14-bit CRC for a 77-bit payload zero-extended to 82 bits
 * according to the standard FT8/FT4/LQ physical layer specification.
 */
uint16_t compute_payload_crc14(const uint8_t payload[PAYLOAD_BYTES]);

/**
 * Append the 14-bit CRC to a 77-bit payload to produce the 91-bit LDPC input.
 * `payload` must be at least 10 bytes (77 bits).
 * `out_91` will receive exactly 12 bytes (91 bits MSB-first, with 5 trailing unused bits set to 0).
 */
void append_crc14(const uint8_t payload[PAYLOAD_BYTES], uint8_t out_91[LDPC_INPUT_BYTES]);

/**
 * Verify that the 14-bit CRC matches the 77-bit payload in a 91-bit input block.
 * Returns true if valid.
 */
bool verify_crc14(const uint8_t data_91[LDPC_INPUT_BYTES]);

/**
 * Extract the 77-bit payload from a verified 91-bit input block.
 */
void extract_payload(const uint8_t data_91[LDPC_INPUT_BYTES], uint8_t payload[PAYLOAD_BYTES]);

} // namespace lq

#endif // LQ_CRC_H
