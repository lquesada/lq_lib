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

#include "lq/hash.h"
#include <cctype>
#include <cstring>
#include <algorithm>
#include <array>

namespace lq {

namespace {

constexpr auto make_crc24_table() {
    std::array<uint32_t, 256> table{};
    for (int byte = 0; byte < 256; ++byte) {
        uint32_t crc = static_cast<uint32_t>(byte) << 16;
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x800000U) {
                crc = ((crc << 1) ^ CRC24_POLY) & 0xFFFFFFU;
            } else {
                crc = (crc << 1) & 0xFFFFFFU;
            }
        }
        table[byte] = crc;
    }
    return table;
}

constexpr auto CRC24_TABLE = make_crc24_table();

constexpr auto make_wsjtx_char_idx() {
    std::array<int8_t, 256> idx{};
    for (int i = 0; i < 256; ++i) idx[i] = -1;
    const char chars[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/@";
    for (int i = 0; chars[i] != '\0'; ++i) {
        idx[static_cast<uint8_t>(chars[i])] = static_cast<int8_t>(i);
    }
    return idx;
}

constexpr auto WSJTX_CHAR_IDX = make_wsjtx_char_idx();

} // anonymous namespace

uint32_t compute_wsjtx_hash(std::string_view call, int bits) {
    if (bits <= 0 || bits > 32) return 0;

    // Normalise: skip leading whitespace
    size_t start = 0;
    while (start < call.size() && std::isspace(static_cast<unsigned char>(call[start]))) {
        ++start;
    }
    size_t end = call.size();
    while (end > start && std::isspace(static_cast<unsigned char>(call[end - 1]))) {
        --end;
    }

    char callsign[12];
    std::memset(callsign, ' ', 11);
    callsign[11] = '\0';

    size_t len = std::min<size_t>(end - start, 11);
    for (size_t i = 0; i < len; ++i) {
        callsign[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(call[start + i])));
    }

    uint64_t x = 0;
    for (int i = 0; i < 11; ++i) {
        uint8_t c = static_cast<uint8_t>(callsign[i]);
        int8_t idx = WSJTX_CHAR_IDX[c];
        if (idx < 0) {
            return 0;
        }
        x = 38ULL * x + static_cast<uint64_t>(idx);
    }

    // WSJT-X 48-bit multiplicative hash with intentional 64-bit unsigned overflow wrap-around
    x = x * 47055833459ULL;
    x = x >> (64 - bits);
    return static_cast<uint32_t>(x);
}

uint32_t hash_callsign_24(std::string_view call) {
    // Strip leading/trailing whitespace
    size_t start = 0;
    while (start < call.size() && std::isspace(static_cast<unsigned char>(call[start]))) {
        ++start;
    }
    size_t end = call.size();
    while (end > start && std::isspace(static_cast<unsigned char>(call[end - 1]))) {
        --end;
    }

    uint32_t crc = 0;
    for (size_t i = start; i < end; ++i) {
        uint8_t byte = static_cast<uint8_t>(std::toupper(static_cast<unsigned char>(call[i])));
        uint8_t idx = static_cast<uint8_t>(crc >> 16) ^ byte;
        crc = ((crc << 8) & 0xFFFFFFU) ^ CRC24_TABLE[idx];
    }
    return crc;
}

uint32_t hash_callsign_20(std::string_view call) {
    return hash_callsign_24(call) >> 4;
}

uint32_t hash_callsign_23(std::string_view call) {
    return hash_callsign_24(call) & 0x7FFFFFU;
}

uint32_t hash_callsign_22(std::string_view call) {
    return compute_wsjtx_hash(call, 22);
}

uint32_t hash_callsign_16(std::string_view call) {
    return compute_wsjtx_hash(call, 16);
}

uint32_t hash_callsign_14(std::string_view call) {
    return compute_wsjtx_hash(call, 14);
}

uint32_t hash_callsign_12(std::string_view call) {
    return compute_wsjtx_hash(call, 12);
}

uint32_t hash_callsign_10(std::string_view call) {
    return compute_wsjtx_hash(call, 10);
}

} // namespace lq
