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

#ifndef LQ_CONSTANTS_H
#define LQ_CONSTANTS_H

#include <cstdint>
#include <array>
#include <stdexcept>
#include "lq/types.h"

namespace lq {

// ===========================================================================
// Physical Layer Parameters
// ===========================================================================

struct ProtocolParams {
    Protocol protocol;
    const char* name;          ///< "LQ8", "LQ4", "LQ2", "LQ16"
    int num_tones;             ///< 8 (LQ8, LQ16) or 4 (LQ4, LQ2)
    int bits_per_symbol;       ///< 3 (LQ8, LQ16) or 2 (LQ4, LQ2)
    float symbol_period;       ///< seconds
    float tone_spacing;        ///< Hz
    float occupied_bw;         ///< Hz
    float tx_duration;         ///< seconds
    float slot_duration;       ///< seconds
    int num_data_symbols;      ///< 58 (LQ8, LQ16) or 87 (LQ4, LQ2)
    int num_sync_symbols;      ///< 21 (LQ8, LQ16) or 16 (LQ4, LQ2)
    int num_ramp_symbols;      ///< 0 (LQ8, LQ16) or 2 (LQ4, LQ2)
    int total_symbols;         ///< 79 (LQ8, LQ16) or 105 (LQ4, LQ2)
    float threshold_snr;       ///< dB SNR in 2500 Hz reference BW
};

inline ProtocolParams get_protocol_params(Protocol p) {
    switch (p) {
        case Protocol::LQ8:
            return {
                Protocol::LQ8,
                "LQ8",
                8,
                3,
                0.160f,
                6.25f,
                50.0f,
                12.64f,
                15.0f,
                58,
                21,
                0,
                79,
                -21.0f
            };
        case Protocol::LQ4:
            return {
                Protocol::LQ4,
                "LQ4",
                4,
                2,
                0.048f,
                20.833333f,
                83.333333f,
                5.04f,
                7.5f,
                87,
                16,
                2,
                105,
                -17.5f
            };
        case Protocol::LQ2:
            return {
                Protocol::LQ2,
                "LQ2",
                4,
                2,
                0.024f,
                41.666667f,
                166.666667f,
                2.52f,
                3.75f,
                87,
                16,
                2,
                105,
                -14.0f
            };
        case Protocol::LQ16:
            return {
                Protocol::LQ16,
                "LQ16",
                8,
                3,
                0.320f,
                3.125f,
                25.0f,
                25.28f,
                30.0f,
                58,
                21,
                0,
                79,
                -24.0f
            };
        default:
            throw std::invalid_argument("Unknown protocol");
    }
}

// ===========================================================================
// Modulation and Synchronization Constants
// ===========================================================================

/// Gray mapping for 8-GFSK (LQ8, LQ16): maps 3-bit value (0..7) to tone index (0..7)
constexpr std::array<uint8_t, 8> GRAY_MAP_8 = {0, 1, 3, 2, 5, 6, 4, 7};

/// Inverse Gray mapping for 8-GFSK: maps tone index (0..7) back to 3-bit value
constexpr std::array<uint8_t, 8> GRAY_INV_MAP_8 = {0, 1, 3, 2, 6, 4, 5, 7};

/// Gray mapping for 4-GFSK (LQ4, LQ2): maps 2-bit value (0..3) to tone index (0..3)
constexpr std::array<uint8_t, 4> GRAY_MAP_4 = {0, 1, 3, 2};

/// Inverse Gray mapping for 4-GFSK: maps tone index (0..3) back to 2-bit value
constexpr std::array<uint8_t, 4> GRAY_INV_MAP_4 = {0, 1, 3, 2};

/// Unique 7-symbol Costas array for LQ8 / LQ16 synchronization (distinct from FT8)
constexpr std::array<uint8_t, 7> COSTAS_ARRAY_8 = {2, 5, 6, 1, 3, 0, 4};
constexpr std::array<int, 3> COSTAS_OFFSETS_8 = {0, 36, 72};

/// Unique 4-symbol Costas arrays for LQ4 / LQ2 synchronization (distinct from FT4)
constexpr std::array<uint8_t, 4> COSTAS_SYNC1_4 = {0, 2, 3, 1};
constexpr std::array<uint8_t, 4> COSTAS_SYNC2_4 = {1, 3, 2, 0};
constexpr std::array<uint8_t, 4> COSTAS_SYNC3_4 = {2, 0, 1, 3};
constexpr std::array<uint8_t, 4> COSTAS_SYNC4_4 = {3, 1, 0, 2};

/// FT4 / LQ4 / LQ2 whitening sequence (10 bytes / 77 bits)
constexpr std::array<uint8_t, 10> FT4_XOR_SEQUENCE = {
    0x4A, 0x5E, 0x89, 0xB4, 0xB0, 0x8A, 0x79, 0x55, 0xBE, 0x28
};


// ===========================================================================
// CRC-14 Polynomial
// ===========================================================================
constexpr uint16_t CRC14_POLY = 0x2757;

// ===========================================================================
// LDPC(174, 91) Matrices
// ===========================================================================

/// Generator matrix rows: 83 rows of 91 bits each (stored as 23-char hex strings)
extern const char* const LDPC_G_HEX[83];

/// Parity check connections: 174 rows x 3 check node indices (0..82)
extern const int LDPC_BIT_TERMS[174][3];

} // namespace lq

#endif // LQ_CONSTANTS_H
