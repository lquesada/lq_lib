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

#include "lq/modifier.h"
#include <cctype>
#include <vector>
#include <string_view>
#include <array>

namespace lq {

namespace {

constexpr auto make_base32_lookup() {
    std::array<int8_t, 256> char_to_b32{};
    for (int i = 0; i < 256; ++i) char_to_b32[i] = -1;
    char_to_b32[static_cast<uint8_t>(' ')] = 0;
    for (char c = 'A'; c <= 'Z'; ++c) {
        char_to_b32[static_cast<uint8_t>(c)] = static_cast<int8_t>(1 + (c - 'A'));
        char_to_b32[static_cast<uint8_t>(c + 32)] = static_cast<int8_t>(1 + (c - 'A'));
    }
    for (char c = '0'; c <= '4'; ++c) {
        char_to_b32[static_cast<uint8_t>(c)] = static_cast<int8_t>(27 + (c - '0'));
    }
    return char_to_b32;
}

constexpr auto BASE32_CHAR_TO_IDX = make_base32_lookup();

inline int char_to_base32(char c) {
    return BASE32_CHAR_TO_IDX[static_cast<uint8_t>(c)];
}

char base32_to_char(int idx) {
    if (idx >= 1 && idx <= 26) return static_cast<char>('A' + (idx - 1));
    if (idx >= 27 && idx <= 31) return static_cast<char>('0' + (idx - 27));
    return ' ';
}

} // anonymous namespace

bool encode_modifier_20(std::string_view mod, uint32_t& packed) {
    // Trim leading and trailing whitespace
    while (!mod.empty() && std::isspace(static_cast<unsigned char>(mod.front()))) {
        mod.remove_prefix(1);
    }
    while (!mod.empty() && std::isspace(static_cast<unsigned char>(mod.back()))) {
        mod.remove_suffix(1);
    }

    if (mod.empty()) {
        packed = MODIFIER_NONE;
        return true;
    }

    // Check for 3-digit decimal numeric modifier (000..999) -> mapped to 1..1000
    if (mod.size() == 3 &&
        std::isdigit(static_cast<unsigned char>(mod[0])) &&
        std::isdigit(static_cast<unsigned char>(mod[1])) &&
        std::isdigit(static_cast<unsigned char>(mod[2]))) {
        uint32_t val = static_cast<uint32_t>(mod[0] - '0') * 100 +
                       static_cast<uint32_t>(mod[1] - '0') * 10 +
                       static_cast<uint32_t>(mod[2] - '0');
        packed = val + 1; // 1..1000
        return true;
    }

    if (mod.size() > 4) {
        return false;
    }

    // 1..4 char alphanumeric string packed into Base-32
    // Alphabet: Space -> 0, A-Z -> 1..26, 0-4 -> 27..31
    uint32_t acc = 0;
    for (int i = 0; i < 4; ++i) {
        char c = (i < static_cast<int>(mod.size())) ? mod[i] : ' ';
        int val = char_to_base32(c);
        if (val < 0) return false;
        acc = (acc << 5) | static_cast<uint32_t>(val);
    }

    packed = MODIFIER_CUSTOM_BASE + acc;
    return (packed <= MODIFIER_MAX_VAL);
}

bool decode_modifier_20(uint32_t packed, std::string& mod) {
    mod.clear();

    if (packed == MODIFIER_NONE) {
        return true;
    }

    // 3-digit numeric modifier 000..999
    if (packed >= 1 && packed <= MODIFIER_NUMERIC_MAX) {
        uint32_t val = packed - 1;
        char buf[4];
        buf[0] = static_cast<char>('0' + (val / 100));
        buf[1] = static_cast<char>('0' + ((val / 10) % 10));
        buf[2] = static_cast<char>('0' + (val % 10));
        buf[3] = '\0';
        mod.assign(buf, 3);
        return true;
    }

    // 1..4 char Base-32 alphanumeric modifier
    if (packed > MODIFIER_NUMERIC_MAX && packed <= MODIFIER_MAX_VAL) {
        uint32_t acc = packed - MODIFIER_CUSTOM_BASE;
        char chars[4];
        for (int i = 3; i >= 0; --i) {
            int val = static_cast<int>(acc & 0x1FU);
            chars[i] = base32_to_char(val);
            acc >>= 5;
        }

        int len = 4;
        while (len > 0 && chars[len - 1] == ' ') {
            --len;
        }
        mod.assign(chars, len);
        return true;
    }

    return false;
}

bool is_known_modifier(std::string_view mod) {
    uint32_t packed = 0;
    return encode_modifier_20(mod, packed);
}

} // namespace lq
