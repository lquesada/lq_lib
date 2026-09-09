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

#include "lq/callsign_nonstd.h"
#include <cctype>
#include <vector>
#include <array>

namespace lq {

namespace {

constexpr auto make_base38_lookup() {
    std::array<int8_t, 256> char_to_b38{};
    for (int i = 0; i < 256; ++i) char_to_b38[i] = -1;
    char_to_b38[static_cast<uint8_t>(' ')] = 0;
    for (char c = 'A'; c <= 'Z'; ++c) {
        char_to_b38[static_cast<uint8_t>(c)] = static_cast<int8_t>(1 + (c - 'A'));
        char_to_b38[static_cast<uint8_t>(c + 32)] = static_cast<int8_t>(1 + (c - 'A')); // lowercase
    }
    char_to_b38[static_cast<uint8_t>('/')] = 27;
    for (char c = '0'; c <= '9'; ++c) {
        char_to_b38[static_cast<uint8_t>(c)] = static_cast<int8_t>(28 + (c - '0'));
    }
    return char_to_b38;
}

constexpr auto BASE38_CHAR_TO_IDX = make_base38_lookup();

inline int char_to_base38(char c) {
    return BASE38_CHAR_TO_IDX[static_cast<uint8_t>(c)];
}

char base38_to_char(int idx) {
    if (idx > 0 && idx < static_cast<int>(NONSTD_CHARSET.size())) {
        return NONSTD_CHARSET[idx];
    }
    return ' ';
}

} // anonymous namespace

bool is_valid_nonstd_callsign(std::string_view call, int max_chars) {
    if (call.empty() || static_cast<int>(call.size()) > max_chars) {
        return false;
    }
    for (char c : call) {
        if (char_to_base38(c) < 0) {
            return false;
        }
    }
    return true;
}

bool encode_callsign_nonstd_u128(std::string_view call, int max_chars, Uint128& val) {
    val = {0, 0};
    if (call.empty() || static_cast<int>(call.size()) > max_chars) {
        return false;
    }

#if defined(__SIZEOF_INT128__)
    unsigned __int128 acc = 0;
    for (int i = 0; i < max_chars; ++i) {
        char c = (i < static_cast<int>(call.size())) ? call[i] : ' ';
        int idx = char_to_base38(c);
        if (idx < 0) return false;
        acc = acc * 38 + static_cast<uint32_t>(idx);
    }

    val.hi = static_cast<uint64_t>(acc >> 64);
    val.lo = static_cast<uint64_t>(acc & 0xFFFFFFFFFFFFFFFFULL);
    return true;
#else
    uint64_t hi = 0;
    uint64_t lo = 0;
    for (int i = 0; i < max_chars; ++i) {
        char c = (i < static_cast<int>(call.size())) ? call[i] : ' ';
        int idx = char_to_base38(c);
        if (idx < 0) return false;

        uint64_t lo_lo = lo & 0xFFFFFFFFULL;
        uint64_t lo_hi = lo >> 32;
        uint64_t prod0 = lo_lo * 38ULL + static_cast<uint32_t>(idx);
        uint64_t carry0 = prod0 >> 32;
        uint64_t prod1 = lo_hi * 38ULL + carry0;
        uint64_t carry1 = prod1 >> 32;
        lo = (prod1 << 32) | (prod0 & 0xFFFFFFFFULL);
        hi = hi * 38ULL + carry1;
    }
    val.hi = hi;
    val.lo = lo;
    return true;
#endif
}

bool decode_callsign_nonstd_u128(Uint128 val, int max_chars, std::string& call) {
    call.clear();
    if (max_chars <= 0 || max_chars > 24) {
        return false;
    }

    char chars[32];
#if defined(__SIZEOF_INT128__)
    unsigned __int128 rem = (static_cast<unsigned __int128>(val.hi) << 64) | val.lo;
    for (int i = max_chars - 1; i >= 0; --i) {
        uint32_t idx = static_cast<uint32_t>(rem % 38);
        rem /= 38;
        chars[i] = base38_to_char(idx);
    }

    if (rem != 0) {
        return false; // Overflow / invalid
    }
#else
    uint64_t hi = val.hi;
    uint64_t lo = val.lo;
    for (int i = max_chars - 1; i >= 0; --i) {
        uint64_t rem_hi = hi % 38ULL;
        hi /= 38ULL;

        uint64_t w1 = (rem_hi << 32) | (lo >> 32);
        uint64_t q1 = w1 / 38ULL;
        uint64_t r1 = w1 % 38ULL;

        uint64_t w0 = (r1 << 32) | (lo & 0xFFFFFFFFULL);
        uint64_t q0 = w0 / 38ULL;
        uint64_t r0 = w0 % 38ULL;

        lo = (q1 << 32) | q0;
        chars[i] = base38_to_char(static_cast<uint32_t>(r0));
    }
    if (hi != 0 || lo != 0) {
        return false;
    }
#endif

    int len = max_chars;
    while (len > 0 && chars[len - 1] == ' ') {
        --len;
    }
    call.assign(chars, len);
    return true;
}

bool encode_callsign_nonstd(std::string_view call, int max_chars, BitBuffer& bb) {
    Uint128 val{0, 0};
    if (!encode_callsign_nonstd_u128(call, max_chars, val)) {
        return false;
    }
    int n_bits = nonstd_bits_for_chars(max_chars);
    if (n_bits <= 0) return false;

    // Fast multi-bit chunk writing (eliminates 69 single-bit iterations)
    if (n_bits > 64) {
        int hi_bits = n_bits - 64;
        bb.write_bits(val.hi, hi_bits);
        bb.write_bits(val.lo, 64);
    } else {
        bb.write_bits(val.lo, n_bits);
    }
    return true;
}

bool decode_callsign_nonstd(BitBuffer& bb, int max_chars, std::string& call) {
    int n_bits = nonstd_bits_for_chars(max_chars);
    if (n_bits <= 0) return false;

    Uint128 val{0, 0};
    // Fast multi-bit chunk reading (eliminates 69 single-bit iterations)
    if (n_bits > 64) {
        int hi_bits = n_bits - 64;
        val.hi = bb.read_bits(hi_bits);
        val.lo = bb.read_bits(64);
    } else {
        val.hi = 0;
        val.lo = bb.read_bits(n_bits);
    }

    return decode_callsign_nonstd_u128(val, max_chars, call);
}

} // namespace lq
