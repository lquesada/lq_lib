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

#ifndef LQ_HASH_H
#define LQ_HASH_H

#include <cstdint>
#include <string>
#include <string_view>

namespace lq {

/// CRC-24/Q polynomial used for 24-bit callsign hashing
constexpr uint32_t CRC24_POLY = 0x864CFBU;

/**
 * Compute the 24-bit CRC-24/Q hash of a callsign string.
 * The callsign is first normalised: trimmed of whitespace and converted to uppercase.
 * Returns a 24-bit unsigned integer (0..16,777,215).
 */
uint32_t hash_callsign_24(std::string_view call);

/**
 * Compute the 20-bit target hash of a callsign string (H24 >> 4).
 * Returns a 20-bit unsigned integer (0..1,048,575).
 */
uint32_t hash_callsign_20(std::string_view call);

/**
 * Compute the 23-bit CRC-24/Q hash of a callsign string (truncated to 23 bits).
 * Returns a 23-bit unsigned integer (0..8,388,607).
 */
uint32_t hash_callsign_23(std::string_view call);

/**
 * Compute standard amateur radio 22-bit callsign hash (WSJT-X / FT8 compatible).
 */
uint32_t hash_callsign_22(std::string_view call);

/**
 * Compute 16-bit callsign hash (0..65,535).
 */
uint32_t hash_callsign_16(std::string_view call);

/**
 * Compute standard amateur radio 14-bit callsign hash (WSJT-X compatible multiplicative hash).
 * Returns a 14-bit unsigned integer (0..16,383).
 */
uint32_t hash_callsign_14(std::string_view call);

/**
 * Compute standard amateur radio 12-bit callsign hash (WSJT-X / FT8 compatible).
 */
uint32_t hash_callsign_12(std::string_view call);

/**
 * Compute standard amateur radio 10-bit callsign hash (WSJT-X / FT8 compatible).
 */
uint32_t hash_callsign_10(std::string_view call);

} // namespace lq

#endif // LQ_HASH_H
