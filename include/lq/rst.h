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

#ifndef LQ_RST_H
#define LQ_RST_H

#include <cstdint>
#include <string>
#include <string_view>

namespace lq {

constexpr int RST_MIN_DB = -26;
constexpr int RST_MAX_DB = 5;
constexpr uint8_t RST_MAX_PACKED = 31; // 5 bits

/**
 * Encode an RST signal report in dB (-26 to +5) into a 5-bit unsigned integer (0..31).
 * Clamps out-of-range values.
 */
uint8_t encode_rst_5(int rst_db);

/**
 * Decode a 5-bit unsigned integer (0..31) into an RST signal report in dB (-26 to +5).
 */
int decode_rst_5(uint8_t packed);

/**
 * Format an RST value as a string with sign (e.g. "+05", "-03", "00", "+01", "-26").
 */
std::string format_rst(int rst_db);

/**
 * Parse an RST string (e.g. "+5", "-03", "0", "+01", "R-15") into an integer in dB.
 * Returns true on success.
 */
bool parse_rst(std::string_view str, int& rst_db);

} // namespace lq

#endif // LQ_RST_H
