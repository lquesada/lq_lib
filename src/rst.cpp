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

#include "lq/rst.h"
#include <algorithm>
#include <cctype>

namespace lq {

uint8_t encode_rst_5(int rst_db) {
    int clamped = std::clamp(rst_db, RST_MIN_DB, RST_MAX_DB);
    return static_cast<uint8_t>(clamped - RST_MIN_DB);
}

int decode_rst_5(uint8_t packed) {
    uint8_t clamped = std::min(packed, RST_MAX_PACKED);
    return static_cast<int>(clamped) + RST_MIN_DB;
}

std::string format_rst(int rst_db) {
    char buf[4];
    int clamped = std::clamp(rst_db, -30, 30);
    if (clamped >= 0) {
        buf[0] = '+';
        buf[1] = static_cast<char>('0' + (clamped / 10));
        buf[2] = static_cast<char>('0' + (clamped % 10));
    } else {
        int pos = -clamped;
        buf[0] = '-';
        buf[1] = static_cast<char>('0' + (pos / 10));
        buf[2] = static_cast<char>('0' + (pos % 10));
    }
    buf[3] = '\0';
    return std::string(buf, 3);
}

bool parse_rst(std::string_view str, int& rst_db) {
    if (str.empty()) return false;

    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    if (start < str.size() && (str[start] == 'R' || str[start] == 'r')) {
        ++start; // Skip optional 'R' prefix (e.g. "R-05")
    }

    if (start >= str.size()) return false;

    int sign = 1;
    if (str[start] == '+') {
        sign = 1;
        ++start;
    } else if (str[start] == '-') {
        sign = -1;
        ++start;
    }

    if (start >= str.size()) return false;

    int val = 0;
    bool has_digits = false;
    while (start < str.size() && std::isdigit(static_cast<unsigned char>(str[start]))) {
        val = val * 10 + (str[start] - '0');
        has_digits = true;
        ++start;
    }

    if (!has_digits) return false;

    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    if (start < str.size()) return false;
    if (val > 30) return false;

    rst_db = std::clamp(sign * val, -30, 30);
    return true;
}

} // namespace lq
