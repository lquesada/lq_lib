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

#ifndef LQ_LDPC_H
#define LQ_LDPC_H

#include <cstdint>
#include <cstddef>
#include "lq/types.h"

namespace lq {

/**
 * Encode 91 input bits (77 payload + 14 CRC) into a 174-bit LDPC codeword.
 * `in_91`: 12 bytes containing 91 bits MSB-first.
 * `out_174`: 22 bytes receiving 174 bits (91 systematic bits + 83 parity bits, remaining 2 bits 0).
 */
void ldpc_encode(const uint8_t in_91[LDPC_INPUT_BYTES], uint8_t out_174[LDPC_CODEWORD_BYTES]);

/**
 * Decode 174 log-likelihood ratios (LLRs) using belief propagation (Normalized Min-Sum).
 * LLR sign convention: positive for bit 0, negative for bit 1 (or standard log(P(0)/P(1))).
 * `llr`: array of 174 float values.
 * `out_91`: array of 12 bytes receiving 91 decoded systematic bits.
 * `max_iters`: maximum belief propagation iterations (default 25).
 * Returns number of iterations taken on success, or -1 if decoding failed (syndrome check failed).
 */
int ldpc_decode(const float llr[LDPC_CODEWORD_BITS], uint8_t out_91[LDPC_INPUT_BYTES], int max_iters = 25);

/**
 * Decode hard bit decisions (with pseudo-LLRs) from a received 174-bit codeword.
 * Useful for bit-level testing and simulation.
 * Returns iterations taken on success, or -1 on failure.
 */
int ldpc_decode_hard_bits(const uint8_t in_174[LDPC_CODEWORD_BYTES], uint8_t out_91[LDPC_INPUT_BYTES], int max_iters = 25);

/**
 * Check if a 174-bit codeword satisfies all 83 parity check equations (H * c == 0 mod 2).
 * Returns true if syndrome is all zeros.
 */
bool ldpc_check_syndrome(const uint8_t codeword[LDPC_CODEWORD_BYTES]);

} // namespace lq

#endif // LQ_LDPC_H
