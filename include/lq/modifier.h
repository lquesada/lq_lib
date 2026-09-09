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

#ifndef LQ_MODIFIER_H
#define LQ_MODIFIER_H

#include <cstdint>
#include <string>
#include <string_view>

namespace lq {

/// 20-bit modifier values
constexpr uint32_t MODIFIER_NONE = 0U;
constexpr uint32_t MODIFIER_NUMERIC_MAX = 1000U; // 000..999 mapped to 1..1000
constexpr uint32_t MODIFIER_CUSTOM_BASE = 1000U;
constexpr uint32_t MODIFIER_MAX_VAL = (1U << 20) - 1U; // 1,048,575

/**
 * Encode a CQ modifier string into 20 bits.
 * Supports 3-digit numeric designators (000..999 mapped to 1..1000) and
 * arbitrary 1-to-4 character Base-32 alphanumeric tokens (e.g., "DX", "POTA", "SOTA", "W1").
 * Returns true on success, false if the modifier cannot be encoded.
 */
bool encode_modifier_20(std::string_view mod, uint32_t& packed);

/**
 * Decode a 20-bit packed integer back to a modifier string.
 * Returns true on success, false if packed > MODIFIER_MAX_VAL.
 */
bool decode_modifier_20(uint32_t packed, std::string& mod);

/**
 * Check if a modifier string is valid and can be encoded.
 */
bool is_known_modifier(std::string_view mod);

} // namespace lq

#endif // LQ_MODIFIER_H
