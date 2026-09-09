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

#include "lq/callsign.h"
#include "lq/callsign_nonstd.h"
#include <cctype>
#include <algorithm>

namespace lq {

namespace {

int char_to_c1(char c) {
    if (c >= '0' && c <= '9') return 1 + (c - '0');
    if (c >= 'A' && c <= 'Z') return 11 + (c - 'A');
    return (c == ' ') ? 0 : -1;
}

char c1_to_char(int idx) {
    if (idx >= 1 && idx <= 10) return static_cast<char>('0' + (idx - 1));
    if (idx >= 11 && idx <= 36) return static_cast<char>('A' + (idx - 11));
    return ' ';
}

int char_to_c2(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    return 10 + (c - 'A');
}

char c2_to_char(int idx) {
    if (idx >= 0 && idx <= 9) return static_cast<char>('0' + idx);
    return static_cast<char>('A' + (idx - 10));
}

int char_to_s(char c) {
    if (c >= 'A' && c <= 'Z') return 1 + (c - 'A');
    return (c == ' ') ? 0 : -1;
}

char s_to_char(int idx) {
    if (idx >= 1 && idx <= 26) return static_cast<char>('A' + (idx - 1));
    return ' ';
}

constexpr uint32_t RADIX_S3 = 1;
constexpr uint32_t RADIX_S2 = 27;
constexpr uint32_t RADIX_S1 = 27 * 27;       // 729
constexpr uint32_t RADIX_D  = 27 * 27 * 27;  // 19683
constexpr uint32_t RADIX_C2 = 10 * RADIX_D;  // 196830
constexpr uint32_t RADIX_C1 = 36 * RADIX_C2; // 7085880

} // anonymous namespace

bool is_standard_callsign(std::string_view call) {
    if (call == "DE" || call == "QRZ" || call == "CQ") return true;
    std::string norm;
    return normalise_standard_callsign(call, norm);
}

bool normalise_standard_callsign_buf(std::string_view call, char out_norm[6]) {
    if (call.empty()) return false;

    char s[16];
    int s_len = 0;
    for (char c : call) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            if (s_len >= 15) return false;
            s[s_len++] = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }

    if (s_len < 3 || s_len > 6) {
        return false;
    }

    int digit_idx = -1;
    for (int i = 1; i < s_len; ++i) {
        if (std::isdigit(static_cast<unsigned char>(s[i]))) {
            int letters_after = s_len - 1 - i;
            if (letters_after >= 1 && letters_after <= 3) {
                bool all_letters = true;
                for (int j = i + 1; j < s_len; ++j) {
                    if (!std::isalpha(static_cast<unsigned char>(s[j]))) {
                        all_letters = false;
                        break;
                    }
                }
                if (all_letters) {
                    digit_idx = i;
                    break;
                }
            }
        }
    }

    if (digit_idx == -1) {
        return false;
    }

    int prefix_len = digit_idx;
    int suffix_len = s_len - digit_idx - 1;

    if (prefix_len < 1 || prefix_len > 2) return false;
    if (suffix_len < 1 || suffix_len > 3) return false;

    for (int i = 0; i < prefix_len; ++i) {
        if (!std::isalnum(static_cast<unsigned char>(s[i]))) return false;
    }
    for (int i = 0; i < suffix_len; ++i) {
        if (!std::isalpha(static_cast<unsigned char>(s[digit_idx + 1 + i]))) return false;
    }

    if (prefix_len == 1) {
        out_norm[0] = ' ';
        out_norm[1] = s[0];
    } else {
        out_norm[0] = s[0];
        out_norm[1] = s[1];
    }

    out_norm[2] = s[digit_idx];

    for (int i = 0; i < suffix_len; ++i) {
        out_norm[3 + i] = s[digit_idx + 1 + i];
    }
    for (int i = suffix_len; i < 3; ++i) {
        out_norm[3 + i] = ' ';
    }

    return true;
}

bool normalise_standard_callsign(std::string_view call, std::string& normalised) {
    char norm[6];
    if (!normalise_standard_callsign_buf(call, norm)) {
        normalised.clear();
        return false;
    }
    normalised.assign(norm, 6);
    return true;
}

bool encode_callsign_std(std::string_view call, uint32_t& packed) {
    // Check tokens first
    if (call == "DE")  { packed = CALLSIGN_TOKEN_DE;  return true; }
    if (call == "QRZ") { packed = CALLSIGN_TOKEN_QRZ; return true; }
    if (call == "CQ")  { packed = CALLSIGN_TOKEN_CQ;  return true; }

    char norm[6];
    if (!normalise_standard_callsign_buf(call, norm)) {
        return false;
    }

    int c1 = char_to_c1(norm[0]);
    int c2 = char_to_c2(norm[1]);
    int d  = norm[2] - '0';
    int s1 = char_to_s(norm[3]);
    int s2 = char_to_s(norm[4]);
    int s3 = char_to_s(norm[5]);


    packed = static_cast<uint32_t>(c1) * RADIX_C1
           + static_cast<uint32_t>(c2) * RADIX_C2
           + static_cast<uint32_t>(d)  * RADIX_D
           + static_cast<uint32_t>(s1) * RADIX_S1
           + static_cast<uint32_t>(s2) * RADIX_S2
           + static_cast<uint32_t>(s3) * RADIX_S3;

    return true;
}

bool decode_callsign_std(uint32_t packed, std::string& call) {
    call.clear();

    if (packed == CALLSIGN_TOKEN_DE)  { call = "DE";  return true; }
    if (packed == CALLSIGN_TOKEN_QRZ) { call = "QRZ"; return true; }
    if (packed == CALLSIGN_TOKEN_CQ)  { call = "CQ";  return true; }

    if (packed > CALLSIGN_MAX_STD) {
        return false;
    }

    uint32_t rem = packed;

    int c1 = static_cast<int>(rem / RADIX_C1);
    rem %= RADIX_C1;

    int c2 = static_cast<int>(rem / RADIX_C2);
    rem %= RADIX_C2;

    int d = static_cast<int>(rem / RADIX_D);
    rem %= RADIX_D;

    int s1 = static_cast<int>(rem / RADIX_S1);
    rem %= RADIX_S1;

    // Standard amateur callsigns must have at least 1 letter in suffix (s1 >= 1)
    if (s1 == 0) {
        return false;
    }

    int s2 = static_cast<int>(rem / RADIX_S2);
    rem %= RADIX_S2;

    int s3 = static_cast<int>(rem / RADIX_S3);

    char ch_c1 = c1_to_char(c1);
    char ch_c2 = c2_to_char(c2);
    char ch_d  = static_cast<char>('0' + d);
    char ch_s1 = s_to_char(s1);
    char ch_s2 = s_to_char(s2);
    char ch_s3 = s_to_char(s3);

    // Build callsign string
    if (ch_c1 != ' ') call.push_back(ch_c1);
    if (ch_c2 != ' ') call.push_back(ch_c2);
    call.push_back(ch_d);
    if (ch_s1 != ' ') call.push_back(ch_s1);
    if (ch_s2 != ' ') call.push_back(ch_s2);
    if (ch_s3 != ' ') call.push_back(ch_s3);

    return true;
}

bool parse_standard_callsign_suffix(std::string_view call, std::string& base_call, uint8_t& suffix) {
    base_call.clear();
    suffix = 0;
    if (call.empty()) return false;

    // Strip leading/trailing whitespace
    size_t start = 0;
    while (start < call.size() && std::isspace(static_cast<unsigned char>(call[start]))) {
        ++start;
    }
    size_t end = call.size();
    while (end > start && std::isspace(static_cast<unsigned char>(call[end - 1]))) {
        --end;
    }
    if (start >= end) return false;

    std::string clean_call;
    clean_call.reserve(end - start);
    for (size_t i = start; i < end; ++i) {
        clean_call.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(call[i]))));
    }

    if (clean_call.size() > 2 && clean_call.substr(clean_call.size() - 2) == "/P") {
        std::string candidate = clean_call.substr(0, clean_call.size() - 2);
        if (is_standard_callsign(candidate)) {
            base_call = candidate;
            suffix = static_cast<uint8_t>(StandardSuffix::P);
            return true;
        }
        return false;
    }

    if (is_standard_callsign(clean_call)) {
        base_call = clean_call;
        suffix = static_cast<uint8_t>(StandardSuffix::NONE);
        return true;
    }

    return false;
}

bool parse_any_callsign_suffix(std::string_view call, std::string& base_call, uint8_t& suffix) {
    base_call.clear();
    suffix = 0;
    if (call.empty()) return false;

    if (parse_standard_callsign_suffix(call, base_call, suffix)) {
        return true;
    }

    std::string clean(call);
    if (clean.size() > 2 && clean.substr(clean.size() - 2) == "/P") {
        base_call = clean.substr(0, clean.size() - 2);
        suffix = static_cast<uint8_t>(StandardSuffix::P);
        return is_valid_nonstd_callsign(base_call, 13);
    }

    base_call = clean;
    suffix = static_cast<uint8_t>(StandardSuffix::NONE);
    return is_valid_nonstd_callsign(base_call, 13);
}

std::string format_callsign_with_suffix(std::string_view base_call, uint8_t suffix) {
    std::string res(base_call);
    if (suffix == static_cast<uint8_t>(StandardSuffix::P)) {
        if (res.size() < 2 || res.substr(res.size() - 2) != "/P") {
            res += "/P";
        }
    }
    return res;
}

} // namespace lq
