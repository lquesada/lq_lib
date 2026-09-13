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

#include "lq/ldpc.h"
#include "lq/constants.h"
#include "lq/crc.h"
#include <cmath>
#include <vector>
#include <cstring>
#include <algorithm>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace lq {

// 83 generator matrix hex strings (23 hex digits each)
constexpr const char* const LDPC_G_HEX[83] = {
    "8329ce11bf31eaf509f27fc",
    "761c264e25c259335493132",
    "dc265902fb277c6410a1bdc",
    "1b3f417858cd2dd33ec7f62",
    "09fda4fee04195fd034783a",
    "077cccc11b8873ed5c3d48a",
    "29b62afe3ca036f4fe1a9da",
    "6054faf5f35d96d3b0c8c3e",
    "e20798e4310eed27884ae90",
    "775c9c08e80e26ddae56318",
    "b0b811028c2bf997213487c",
    "18a0c9231fc60adf5c5ea32",
    "76471e8302a0721e01b12b8",
    "ffbccb80ca8341fafb47b2e",
    "66a72a158f9325a2bf67170",
    "c4243689fe85b1c51363a18",
    "0dff739414d1a1b34b1c270",
    "15b48830636c8b99894972e",
    "29a89c0d3de81d665489b0e",
    "4f126f37fa51cbe61bd6b94",
    "99c47239d0d97d3c84e0940",
    "1919b75119765621bb4f1e8",
    "09db12d731faee0b86df6b8",
    "488fc33df43fbdeea4eafb4",
    "827423ee40b675f756eb5fe",
    "abe197c484cb74757144a9a",
    "2b500e4bc0ec5a6d2bdbdd0",
    "c474aa53d70218761669360",
    "8eba1a13db3390bd6718cec",
    "753844673a27782cc42012e",
    "06ff83a145c37035a5c1268",
    "3b37417858cc2dd33ec3f62",
    "9a4a5a28ee17ca9c324842c",
    "bc29f465309c977e89610a4",
    "2663ae6ddf8b5ce2bb29488",
    "46f231efe457034c1814418",
    "3fb2ce85abe9b0c72e06fbe",
    "de87481f282c153971a0a2e",
    "fcd7ccf23c69fa99bba1412",
    "f0261447e9490ca8e474cec",
    "4410115818196f95cdd7012",
    "088fc31df4bfbde2a4eafb4",
    "b8fef1b6307729fb0a078c0",
    "5afea7acccb77bbc9d99a90",
    "49a7016ac653f65ecdc9076",
    "1944d085be4e7da8d6cc7d0",
    "251f62adc4032f0ee714002",
    "56471f8702a0721e00b12b8",
    "2b8e4923f2dd51e2d537fa0",
    "6b550a40a66f4755de95c26",
    "a18ad28d4e27fe92a4f6c84",
    "10c2e586388cb82a3d80758",
    "ef34a41817ee02133db2eb0",
    "7e9c0c54325a9c15836e000",
    "3693e572d1fde4cdf079e86",
    "bfb2cec5abe1b0c72e07fbe",
    "7ee18230c583cccc57d4b08",
    "a066cb2fedafc9f52664126",
    "bb23725abc47cc5f4cc4cd2",
    "ded9dba3bee40c59b5609b4",
    "d9a7016ac653e6decdc9036",
    "9ad46aed5f707f280ab5fc4",
    "e5921c77822587316d7d3c2",
    "4f14da8242a8b86dca73352",
    "8b8b507ad467d4441df770e",
    "22831c9cf1169467ad04b68",
    "213b838fe2ae54c38ee7180",
    "5d926b6dd71f085181a4e12",
    "66ab79d4b29ee6e69509e56",
    "958148682d748a38dd68baa",
    "b8ce020cf069c32a723ab14",
    "f4331d6d461607e95752746",
    "6da23ba424b9596133cf9c8",
    "a636bcbc7b30c5fbeae67fe",
    "5cb0d86a07df654a9089a20",
    "f11f106848780fc9ecdd80a",
    "1fbb5364fb8d2c9d730d5ba",
    "fcb86bc70a50c9d02a5d034",
    "a534433029eac15f322e34c",
    "c989d9c7c3d3b8c55d75130",
    "7bb38b2f0186d46643ae962",
    "2644ebadeb44b9467d1f42c",
    "608cc857594bfbb55d69600"
};

// Parity check equations: 174 rows x 3 check nodes (0-indexed)
constexpr int LDPC_BIT_TERMS[174][3] = {
    {15, 44, 72}, {24, 50, 61}, {32, 57, 77}, {0, 43, 44}, {1, 6, 60},
    {2, 5, 53}, {3, 34, 47}, {4, 12, 20}, {7, 55, 78}, {8, 63, 68},
    {9, 18, 65}, {10, 35, 59}, {11, 36, 57}, {13, 31, 42}, {14, 62, 79},
    {16, 27, 76}, {17, 73, 82}, {21, 52, 80}, {22, 29, 33}, {23, 30, 39},
    {25, 40, 75}, {26, 56, 69}, {28, 48, 64}, {2, 37, 77}, {4, 38, 81},
    {45, 49, 72}, {50, 51, 73}, {54, 70, 71}, {43, 66, 71}, {42, 67, 77},
    {0, 31, 58}, {1, 5, 70}, {3, 15, 53}, {6, 64, 66}, {7, 29, 41},
    {8, 21, 30}, {9, 17, 75}, {10, 22, 81}, {11, 27, 60}, {12, 51, 78},
    {13, 49, 50}, {14, 80, 82}, {16, 28, 59}, {18, 32, 63}, {19, 25, 72},
    {20, 33, 39}, {23, 26, 76}, {24, 54, 57}, {34, 52, 65}, {35, 47, 67},
    {36, 45, 74}, {37, 44, 46}, {38, 56, 68}, {40, 55, 61}, {19, 48, 52},
    {45, 51, 62}, {44, 69, 74}, {26, 34, 79}, {0, 14, 29}, {1, 67, 79},
    {2, 35, 50}, {3, 27, 50}, {4, 30, 55}, {5, 19, 36}, {6, 39, 81},
    {7, 59, 68}, {8, 9, 48}, {10, 43, 56}, {11, 38, 58}, {12, 23, 54},
    {13, 20, 64}, {15, 70, 77}, {16, 29, 75}, {17, 24, 79}, {18, 60, 82},
    {21, 37, 76}, {22, 40, 49}, {6, 25, 57}, {28, 31, 80}, {32, 39, 72},
    {17, 33, 47}, {12, 41, 63}, {4, 25, 42}, {46, 68, 71}, {53, 54, 69},
    {44, 61, 67}, {9, 62, 66}, {13, 65, 71}, {21, 59, 73}, {34, 38, 78},
    {0, 45, 63}, {0, 23, 65}, {1, 4, 69}, {2, 30, 64}, {3, 48, 57},
    {0, 3, 4}, {5, 59, 66}, {6, 31, 74}, {7, 47, 81}, {8, 34, 40},
    {9, 38, 61}, {10, 13, 60}, {11, 70, 73}, {12, 22, 77}, {10, 34, 54},
    {14, 15, 78}, {6, 8, 15}, {16, 53, 62}, {17, 49, 56}, {18, 29, 46},
    {19, 63, 79}, {20, 27, 68}, {21, 24, 42}, {12, 21, 36}, {1, 46, 50},
    {22, 53, 73}, {25, 33, 71}, {26, 35, 36}, {20, 35, 62}, {28, 39, 43},
    {18, 25, 56}, {2, 45, 81}, {13, 14, 57}, {32, 51, 52}, {29, 42, 51},
    {5, 8, 51}, {26, 32, 64}, {24, 68, 72}, {37, 54, 82}, {19, 38, 76},
    {17, 28, 55}, {31, 47, 70}, {41, 50, 58}, {27, 43, 78}, {33, 59, 61},
    {30, 44, 60}, {45, 67, 76}, {5, 23, 75}, {7, 9, 77}, {39, 40, 69},
    {16, 49, 52}, {41, 65, 67}, {3, 21, 71}, {35, 63, 80}, {12, 28, 46},
    {1, 7, 80}, {55, 66, 72}, {4, 37, 49}, {11, 37, 63}, {58, 71, 79},
    {2, 25, 78}, {44, 75, 80}, {0, 64, 73}, {6, 17, 76}, {10, 55, 58},
    {13, 38, 53}, {15, 36, 65}, {9, 27, 54}, {14, 59, 69}, {16, 24, 81},
    {19, 29, 30}, {11, 66, 67}, {22, 74, 79}, {26, 31, 61}, {23, 68, 74},
    {18, 20, 70}, {33, 52, 60}, {34, 45, 46}, {32, 58, 75}, {39, 42, 82},
    {40, 41, 62}, {48, 74, 82}, {19, 43, 47}, {41, 48, 56}
};

namespace {
// Highly optimized precomputed LDPC topology and generator matrix
struct LDPCMatrix {
    uint64_t G_bits[83][2];
    uint8_t check_bits[83][7];
    uint8_t check_var_k[83][7];      // index k in LDPC_BIT_TERMS[n][k] == m
    uint8_t var_check_edge[174][3];  // edge index in check_bits[m][edge] == n
    uint8_t check_degrees[83];
};

constexpr LDPCMatrix make_ldpc_matrix() {
    LDPCMatrix mat{};
    // 1. Build 64-bit packed generator matrix G[83][2] from hex constants
    for (int r = 0; r < 83; ++r) {
        uint64_t w0 = 0;
        uint64_t w1 = 0;
        const char* hex = LDPC_G_HEX[r];
        for (int d = 0; d < 23; ++d) {
            char c = hex[d];
            int val = 0;
            if (c >= '0' && c <= '9') val = c - '0';
            else if (c >= 'a' && c <= 'f') val = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') val = 10 + (c - 'A');
            for (int b = 0; b < 4; ++b) {
                int bit_idx = d * 4 + b;
                if (bit_idx < 91 && ((val >> (3 - b)) & 1)) {
                    if (bit_idx < 64) {
                        w0 |= (1ULL << bit_idx);
                    } else {
                        w1 |= (1ULL << (bit_idx - 64));
                    }
                }
            }
        }
        mat.G_bits[r][0] = w0;
        mat.G_bits[r][1] = w1;
    }

    // 2. Build inverse check-to-bit connections check_bits[83][7]
    for (int m = 0; m < 83; ++m) {
        mat.check_degrees[m] = 0;
        for (int i = 0; i < 7; ++i) {
            mat.check_bits[m][i] = 0;
            mat.check_var_k[m][i] = 0;
        }
    }
    for (int n = 0; n < 174; ++n) {
        for (int k = 0; k < 3; ++k) {
            mat.var_check_edge[n][k] = 0;
        }
    }
    for (int n = 0; n < 174; ++n) {
        for (int k = 0; k < 3; ++k) {
            int m = LDPC_BIT_TERMS[n][k];
            if (m >= 0 && m < 83) {
                uint8_t deg = mat.check_degrees[m]++;
                if (deg < 7) {
                    mat.check_bits[m][deg] = static_cast<uint8_t>(n);
                    mat.check_var_k[m][deg] = static_cast<uint8_t>(k);
                    mat.var_check_edge[n][k] = deg;
                }
            }
        }
    }
    return mat;
}

constexpr auto LDPC_MAT = make_ldpc_matrix();

constexpr auto make_bit_reverse_table() {
    std::array<uint8_t, 256> table{};
    for (int i = 0; i < 256; ++i) {
        uint8_t b = 0;
        for (int bit = 0; bit < 8; ++bit) {
            if ((i >> bit) & 1) b |= static_cast<uint8_t>(1 << (7 - bit));
        }
        table[i] = b;
    }
    return table;
}

constexpr auto REVERSE_BYTE = make_bit_reverse_table();

inline int popcount64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(x);
#elif defined(_MSC_VER)
    return static_cast<int>(__popcnt64(x));
#else
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return static_cast<int>((x * 0x0101010101010101ULL) >> 56);
#endif
}

inline void pack_systematic_91(const uint8_t c_hat[174], uint8_t out_91[LDPC_INPUT_BYTES]) {
    std::memset(out_91, 0, LDPC_INPUT_BYTES);
    for (int b = 0; b < 11; ++b) {
        int base = b * 8;
        out_91[b] = static_cast<uint8_t>(
            (c_hat[base + 0] << 7) | (c_hat[base + 1] << 6) |
            (c_hat[base + 2] << 5) | (c_hat[base + 3] << 4) |
            (c_hat[base + 4] << 3) | (c_hat[base + 5] << 2) |
            (c_hat[base + 6] << 1) | (c_hat[base + 7])
        );
    }
    out_91[11] = static_cast<uint8_t>(
        (c_hat[88] << 7) | (c_hat[89] << 6) | (c_hat[90] << 5)
    );
}

inline int count_unsatisfied_checks(const uint8_t c_hat[174]) {
    int errs = 0;
    for (int m = 0; m < 83; ++m) {
        uint8_t sum = 0;
        int deg = LDPC_MAT.check_degrees[m];
        for (int i = 0; i < deg; ++i) {
            sum ^= c_hat[LDPC_MAT.check_bits[m][i]];
        }
        if (sum != 0) ++errs;
    }
    return errs;
}

} // anonymous namespace

void ldpc_encode(const uint8_t in_91[LDPC_INPUT_BYTES], uint8_t out_174[LDPC_CODEWORD_BYTES]) {
    std::memset(out_174, 0, LDPC_CODEWORD_BYTES);

    // 1. Direct copy of systematic 91 bits:
    // First 11 bytes (88 bits) are identical
    std::memcpy(out_174, in_91, 11);
    // 12th byte: upper 3 bits are bits 88..90 from in_91[11]
    out_174[11] = static_cast<uint8_t>(in_91[11] & 0xE0U);

    // 2. Extract u0 (first 64 bits) and u1 (next 27 bits) in O(1) table lookups
    uint64_t u0 = (static_cast<uint64_t>(REVERSE_BYTE[in_91[0]])      ) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[1]]) <<  8) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[2]]) << 16) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[3]]) << 24) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[4]]) << 32) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[5]]) << 40) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[6]]) << 48) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[7]]) << 56);

    uint64_t u1 = (static_cast<uint64_t>(REVERSE_BYTE[in_91[8]])      ) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[9]]) <<  8) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[10]]) << 16) |
                  (static_cast<uint64_t>(REVERSE_BYTE[in_91[11]] & 0x07U) << 24);

    // 3. Compute 83 parity bits and pack directly into out_174
    #pragma GCC unroll 83
    for (int j = 0; j < 83; ++j) {
        uint64_t match0 = u0 & LDPC_MAT.G_bits[j][0];
        uint64_t match1 = u1 & LDPC_MAT.G_bits[j][1];
        int p = (popcount64(match0) ^ popcount64(match1)) & 1;
        if (p) {
            int bit_pos = 91 + j;
            out_174[bit_pos >> 3] |= static_cast<uint8_t>(1 << (7 - (bit_pos & 7)));
        }
    }
}

bool ldpc_check_syndrome(const uint8_t codeword[LDPC_CODEWORD_BYTES]) {
    // Fast check directly from codeword bytes (zero BitBuffer / unpack overhead)
    for (int m = 0; m < 83; ++m) {
        uint8_t sum = 0;
        int deg = LDPC_MAT.check_degrees[m];
        for (int i = 0; i < deg; ++i) {
            int bit_idx = LDPC_MAT.check_bits[m][i];
            sum ^= (codeword[bit_idx >> 3] >> (7 - (bit_idx & 7))) & 1U;
        }
        if (sum != 0) {
            return false;
        }
    }
    return true;
}

int ldpc_decode(const float llr[LDPC_CODEWORD_BITS], uint8_t out_91[LDPC_INPUT_BYTES], int max_iters) {
    // Variable-to-check messages q[n][3]
    float q[174][3];
    // Check-to-variable messages r[83][7]
    float r[83][7];
    std::memset(r, 0, sizeof(r));

    // Initialise q with channel LLRs
    for (int n = 0; n < 174; ++n) {
        q[n][0] = llr[n];
        q[n][1] = llr[n];
        q[n][2] = llr[n];
    }

    constexpr float ALPHA = 0.875f; // Normalised min-sum scaling factor

    int best_syn_errs = 999;
    uint8_t best_c_hat[174];
    float best_total_llr[174];

    for (int iter = 0; iter < max_iters; ++iter) {
        // --- Step 1: Check node update: O(deg) single pass min1/min2/sign ---
        for (int m = 0; m < 83; ++m) {
            int deg = LDPC_MAT.check_degrees[m];

            float min1 = 1e9f;
            float min2 = 1e9f;
            int min1_idx = -1;
            int total_sign = 1;
            int signs[7];

            for (int i = 0; i < deg; ++i) {
                int bit_n = LDPC_MAT.check_bits[m][i];
                int k = LDPC_MAT.check_var_k[m][i];
                float val = q[bit_n][k];
                int s = (val < 0.0f) ? -1 : 1;
                signs[i] = s;
                total_sign *= s;
                float mag = std::fabs(val);
                if (mag < min1) {
                    min2 = min1;
                    min1 = mag;
                    min1_idx = i;
                } else if (mag < min2) {
                    min2 = mag;
                }
            }

            for (int i = 0; i < deg; ++i) {
                float mag = (i == min1_idx) ? min2 : min1;
                int sign = signs[i] * total_sign;
                r[m][i] = ALPHA * static_cast<float>(sign) * mag;
            }
        }

        // --- Step 2: Variable node update & hard decisions (fully unrolled 3 edges) ---
        uint8_t c_hat[174];
        float total_llrs[174];
        for (int n = 0; n < 174; ++n) {
            int m0 = LDPC_BIT_TERMS[n][0];
            int m1 = LDPC_BIT_TERMS[n][1];
            int m2 = LDPC_BIT_TERMS[n][2];

            int e0 = LDPC_MAT.var_check_edge[n][0];
            int e1 = LDPC_MAT.var_check_edge[n][1];
            int e2 = LDPC_MAT.var_check_edge[n][2];

            float r0 = r[m0][e0];
            float r1 = r[m1][e1];
            float r2 = r[m2][e2];

            float total_llr = llr[n] + r0 + r1 + r2;
            total_llrs[n] = total_llr;
            c_hat[n] = (total_llr < 0.0f) ? 1 : 0;

            q[n][0] = total_llr - r0;
            q[n][1] = total_llr - r1;
            q[n][2] = total_llr - r2;
        }

        // --- Step 3: Syndrome check ---
        int syn_errs = 0;
        for (int m = 0; m < 83; ++m) {
            uint8_t sum = 0;
            int deg = LDPC_MAT.check_degrees[m];
            for (int i = 0; i < deg; ++i) {
                sum ^= c_hat[LDPC_MAT.check_bits[m][i]];
            }
            if (sum != 0) {
                ++syn_errs;
            }
        }

        if (syn_errs == 0) {
            pack_systematic_91(c_hat, out_91);
            return iter + 1; // Full syndrome match!
        }

        if (syn_errs < best_syn_errs) {
            best_syn_errs = syn_errs;
            std::memcpy(best_c_hat, c_hat, sizeof(best_c_hat));
            std::memcpy(best_total_llr, total_llrs, sizeof(best_total_llr));
        }

        // Trapping set check: if all 83 parity checks are satisfied, verify CRC-14
        if (syn_errs == 0) {
            pack_systematic_91(c_hat, out_91);
            if (verify_crc14(out_91)) {
                return iter + 1; // Full syndrome match and verified CRC-14!
            }
        }
    }

    // --- Step 4: CRC-Assisted Bit-Flipping Rescue on Best Candidate State ---
    // Only attempt rescue if the candidate state is near-codeword (<= 6 unsatisfied checks)
    if (best_syn_errs <= 6) {
        struct BitRel {
            float abs_llr;
            uint8_t idx;
        };
        BitRel rel[174];
        for (int n = 0; n < 174; ++n) {
            rel[n].abs_llr = std::abs(best_total_llr[n]);
            rel[n].idx = static_cast<uint8_t>(n);
        }
        std::sort(rel, rel + 174, [](const BitRel& a, const BitRel& b) {
            return a.abs_llr < b.abs_llr;
        });

        uint8_t test_c[174];
        std::memcpy(test_c, best_c_hat, sizeof(test_c));
        uint8_t test_91[LDPC_INPUT_BYTES];

        // 1. Single-bit flip rescue across all 174 codeword bits (ordered by least confident)
        // Strictly require ALL 83 parity checks satisfied (errs == 0) AND CRC-14 verification
        for (int i = 0; i < 174; ++i) {
            int bit = rel[i].idx;
            test_c[bit] ^= 1;
            int errs = count_unsatisfied_checks(test_c);
            if (errs == 0) {
                pack_systematic_91(test_c, test_91);
                if (verify_crc14(test_91)) {
                    std::memcpy(out_91, test_91, LDPC_INPUT_BYTES);
                    return max_iters + 1; // Rescued by 1-bit flip to full valid codeword
                }
            }
            test_c[bit] ^= 1; // Revert
        }

        // 2. Double-bit flip rescue on top 16 least confident systematic bits
        constexpr int MAX_2BIT = 16;
        for (int i = 0; i < MAX_2BIT; ++i) {
            int b1 = rel[i].idx;
            test_c[b1] ^= 1;
            for (int j = i + 1; j < MAX_2BIT; ++j) {
                int b2 = rel[j].idx;
                test_c[b2] ^= 1;
                int errs = count_unsatisfied_checks(test_c);
                if (errs == 0) {
                    pack_systematic_91(test_c, test_91);
                    if (verify_crc14(test_91)) {
                        std::memcpy(out_91, test_91, LDPC_INPUT_BYTES);
                        return max_iters + 2; // Rescued by 2-bit flip to full valid codeword
                    }
                }
                test_c[b2] ^= 1;
            }
            test_c[b1] ^= 1;
        }
    }

    return -1; // Decode failed within max_iters
}

int ldpc_decode_hard_bits(const uint8_t in_174[LDPC_CODEWORD_BYTES], uint8_t out_91[LDPC_INPUT_BYTES], int max_iters) {
    float llr[LDPC_CODEWORD_BITS];
    for (int i = 0; i < 174; ++i) {
        llr[i] = ((in_174[i >> 3] >> (7 - (i & 7))) & 1U) ? -10.0f : 10.0f;
    }
    return ldpc_decode(llr, out_91, max_iters);
}

} // namespace lq
