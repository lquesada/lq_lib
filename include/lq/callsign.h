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

#ifndef LQ_CALLSIGN_H
#define LQ_CALLSIGN_H

#include <cstdint>
#include <string>
#include <string_view>

namespace lq {

/// Special tokens mapped into the 28-bit standard callsign space (N >= 262,177,560)
constexpr uint32_t CALLSIGN_TOKEN_DE  = 262177560U;
constexpr uint32_t CALLSIGN_TOKEN_QRZ = 262177561U;
constexpr uint32_t CALLSIGN_TOKEN_CQ  = 262177562U;
constexpr uint32_t CALLSIGN_MAX_STD   = 262177559U;

/**
 * Check if a callsign string conforms to the standard ITU amateur radio callsign format.
 * Standard format: 1-2 alphanumeric prefix chars, 1 digit, 1-3 suffix letters.
 */
bool is_standard_callsign(std::string_view call);

/**
 * Normalise a standard callsign to exactly 6 characters:
 * - c1 (space or A-Z or 0-9)
 * - c2 (A-Z or 0-9)
 * - d  (0-9)
 * - s1 (space or A-Z)
 * - s2 (space or A-Z)
 * - s3 (space or A-Z)
 * Returns false if the callsign cannot be normalised to standard format.
 */
bool normalise_standard_callsign(std::string_view call, std::string& normalised);

/**
 * Encode a standard callsign (or special token DE/QRZ/CQ) to a 28-bit unsigned integer.
 * Returns true on success, false if call is not a valid standard callsign or token.
 */
bool encode_callsign_std(std::string_view call, uint32_t& packed);

/**
 * Decode a 28-bit unsigned integer back into a callsign string (or token).
 * Returns true on success, false on invalid packed value.
 */
bool decode_callsign_std(uint32_t packed, std::string& call);

enum class StandardSuffix : uint8_t {
    NONE = 0,
    P = 1,
    PORTABLE = 1,
    SUFFIX = 1 ///< Standard 1-bit /P suffix indicator
};

/**
 * Check if a callsign is a standard callsign with optional standard suffix (/P).
 * If valid, extracts the base callsign and sets suffix to:
 * 0 = none, 1 = suffix (/P).
 */
bool parse_standard_callsign_suffix(std::string_view call, std::string& base_call, uint8_t& suffix);

/**
 * Check if a callsign (standard or non-standard) has an optional standard suffix (/P).
 * Extracts base callsign and sets suffix to:
 * 0 = none, 1 = suffix (/P).
 */
bool parse_any_callsign_suffix(std::string_view call, std::string& base_call, uint8_t& suffix);

/**
 * Append standard suffix to base callsign:
 * 0 = none, 1 = /P.
 */
std::string format_callsign_with_suffix(std::string_view base_call, uint8_t suffix);

} // namespace lq

#endif // LQ_CALLSIGN_H
