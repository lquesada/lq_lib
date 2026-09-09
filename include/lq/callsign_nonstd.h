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

#ifndef LQ_CALLSIGN_NONSTD_H
#define LQ_CALLSIGN_NONSTD_H

#include <cstdint>
#include <string>
#include <string_view>
#include "lq/types.h"

namespace lq {

/// The 38-character alphabet for non-standard callsigns:
/// ' ' -> 0, 'A'-'Z' -> 1..26, '/' -> 27, '0'-'9' -> 28..37
constexpr std::string_view NONSTD_CHARSET = " ABCDEFGHIJKLMNOPQRSTUVWXYZ/0123456789";

/// Character count to bit count mappings:
/// 7 chars -> 37 bits (ceil(7 * log2(38)) = 37)
/// 9 chars -> 48 bits (ceil(9 * log2(38)) = 48)
/// 10 chars -> 53 bits (ceil(10 * log2(38)) = 53)
/// 13 chars -> 69 bits (ceil(13 * log2(38)) = 69)
/// 14 chars -> 74 bits (ceil(14 * log2(38)) = 74)
constexpr int nonstd_bits_for_chars(int max_chars) {
    switch (max_chars) {
        case 7:  return 37;
        case 9:  return 48;
        case 10: return 53;
        case 13: return 69;
        case 14: return 74;
        default: return 0;
    }
}

/**
 * Validate that all characters in a non-standard callsign are in the 38-character alphabet.
 */
bool is_valid_nonstd_callsign(std::string_view call, int max_chars);

/**
 * Encode a non-standard callsign of length <= max_chars into a big-integer bit representation.
 * Writes exactly nonstd_bits_for_chars(max_chars) bits into `bb`.
 * Returns true on success.
 */
bool encode_callsign_nonstd(std::string_view call, int max_chars, BitBuffer& bb);

/**
 * Decode a non-standard callsign from `bb` by reading nonstd_bits_for_chars(max_chars) bits.
 * Returns true on success, populating `call`.
 */
bool decode_callsign_nonstd(BitBuffer& bb, int max_chars, std::string& call);

/// Portable 128-bit unsigned integer representation
struct Uint128 {
    uint64_t hi = 0;
    uint64_t lo = 0;

    bool operator==(const Uint128& o) const { return hi == o.hi && lo == o.lo; }
};

/// Convenience helper working directly on byte arrays / integers
bool encode_callsign_nonstd_u128(std::string_view call, int max_chars, Uint128& val);
bool decode_callsign_nonstd_u128(Uint128 val, int max_chars, std::string& call);

} // namespace lq

#endif // LQ_CALLSIGN_NONSTD_H
