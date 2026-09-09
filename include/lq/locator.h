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

#ifndef LQ_LOCATOR_H
#define LQ_LOCATOR_H

#include <cstdint>
#include <string>
#include <string_view>

namespace lq {

/// Special 15-bit value representing "no locator provided"
constexpr uint16_t LOCATOR_BLANK = 32400U;
constexpr uint16_t LOCATOR_MAX_VALID = 32399U;

/**
 * Check if a string is a valid 4-character Maidenhead grid locator (e.g. "JN47", "AA00", "RR99").
 */
bool is_valid_locator(std::string_view loc);

/**
 * Encode a 4-character Maidenhead grid locator into a 15-bit integer.
 * If loc is empty or blank, encodes LOCATOR_BLANK.
 * Returns true on success, false on invalid format.
 */
bool encode_locator_15(std::string_view loc, uint16_t& packed);

/**
 * Decode a 15-bit packed integer back to a 4-character grid locator.
 * If packed == LOCATOR_BLANK, returns an empty string.
 * Returns true on success, false if packed > LOCATOR_BLANK.
 */
bool decode_locator_15(uint16_t packed, std::string& loc);

} // namespace lq

#endif // LQ_LOCATOR_H
