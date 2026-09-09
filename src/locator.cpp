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

#include "lq/locator.h"
#include <cctype>

namespace lq {

bool is_valid_locator(std::string_view loc) {
    if (loc.size() != 4) return false;
    char c0 = static_cast<char>(std::toupper(static_cast<unsigned char>(loc[0])));
    char c1 = static_cast<char>(std::toupper(static_cast<unsigned char>(loc[1])));
    char c2 = loc[2];
    char c3 = loc[3];

    return (c0 >= 'A' && c0 <= 'R') &&
           (c1 >= 'A' && c1 <= 'R') &&
           (c2 >= '0' && c2 <= '9') &&
           (c3 >= '0' && c3 <= '9');
}

bool encode_locator_15(std::string_view loc, uint16_t& packed) {
    if (loc.empty()) {
        packed = LOCATOR_BLANK;
        return true;
    }

    if (loc.size() != 4) return false;

    char c0 = static_cast<char>(std::toupper(static_cast<unsigned char>(loc[0])));
    char c1 = static_cast<char>(std::toupper(static_cast<unsigned char>(loc[1])));
    char c2 = loc[2];
    char c3 = loc[3];

    if (c0 < 'A' || c0 > 'R' || c1 < 'A' || c1 > 'R' || c2 < '0' || c2 > '9' || c3 < '0' || c3 > '9') {
        return false;
    }

    int f_lon = c0 - 'A';
    int f_lat = c1 - 'A';
    int s_lon = c2 - '0';
    int s_lat = c3 - '0';

    packed = static_cast<uint16_t>(f_lon * 1800 + f_lat * 100 + s_lon * 10 + s_lat);
    return true;
}

bool decode_locator_15(uint16_t packed, std::string& loc) {
    loc.clear();

    if (packed == LOCATOR_BLANK) {
        return true; // Blank locator
    }

    if (packed > LOCATOR_MAX_VALID) {
        return false;
    }

    uint16_t p = packed;
    int f_lon = p / 1800;
    int rem1 = p - f_lon * 1800;

    int f_lat = rem1 / 100;
    int rem2 = rem1 - f_lat * 100;

    int s_lon = rem2 / 10;
    int s_lat = rem2 - s_lon * 10;

    char buf[4] = {
        static_cast<char>('A' + f_lon),
        static_cast<char>('A' + f_lat),
        static_cast<char>('0' + s_lon),
        static_cast<char>('0' + s_lat)
    };
    loc.assign(buf, 4);

    return true;
}

} // namespace lq
