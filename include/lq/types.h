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

#ifndef LQ_TYPES_H
#define LQ_TYPES_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace lq {

/// Information payload length in bits for all LQ message types
constexpr int PAYLOAD_BITS = 77;
constexpr int PAYLOAD_BYTES = (PAYLOAD_BITS + 7) / 8; // 10 bytes

/// CRC length in bits
constexpr int CRC_BITS = 14;

/// Total LDPC input length in bits
constexpr int LDPC_INPUT_BITS = PAYLOAD_BITS + CRC_BITS; // 91 bits
constexpr int LDPC_INPUT_BYTES = (LDPC_INPUT_BITS + 7) / 8; // 12 bytes

/// LDPC parity bits
constexpr int LDPC_PARITY_BITS = 83;

/// Total LDPC codeword length in bits
constexpr int LDPC_CODEWORD_BITS = LDPC_INPUT_BITS + LDPC_PARITY_BITS; // 174 bits
constexpr int LDPC_CODEWORD_BYTES = (LDPC_CODEWORD_BITS + 7) / 8; // 22 bytes

/// Maximum symbols in any transport frame (LQ4/LQ2)
constexpr int MAX_FRAME_SYMBOLS = 105;

/// Message Type Identifiers — Canonical Types (1..16) and Compatibility Aliases
enum class MessageType : uint8_t {
    // Canonical 16 Message Types (1..16)
    CQ_STD = 1,           ///< [Canonical] Type 1:  CQ std (0000000000 - 10b)
    CQ_NONSTD_1 = 2,      ///< [Canonical] Type 2:  CQ non-std 1, max 9 chars + loc (0000000001 - 10b)
    CQ_NONSTD_2 = 3,      ///< [Canonical] Type 3:  CQ non-std 2, max 9 chars + mod (00000001 - 8b)
    CQ_NONSTD_3 = 4,      ///< [Canonical] Type 4:  CQ non-std 3, max 13 chars (0000001 - 7b)
    CALL_STD_NOSUF = 5,   ///< [Canonical] Type 5:  CALL std (nosuf) (1 - 1b)
    CALL_STD_SUF = 6,     ///< [Canonical] Type 6:  CALL std+suf (0100 - 4b)
    CALL_NONSTD = 7,      ///< [Canonical] Type 7:  CALL non-std (001 - 3b)
    REPORT73_STD = 8,     ///< [Canonical] Type 8:  REPORT+73 std+suf (0000000010 - 10b)
    M73_STD = 9,          ///< [Canonical] Type 9:  73 std+suf (0000000011 - 10b)
    M73_NONSTD = 10,      ///< [Canonical] Type 10: 73 non-std (0001 - 4b)
    MULTI_REPORT73 = 11,  ///< [Canonical] Type 11: MULTI-REPORT+73 (011 - 3b)
    MULTI_73 = 12,        ///< [Canonical] Type 12: MULTI-73 (00000111 - 8b)
    FREE_TEXT = 13,       ///< [Canonical] Type 13: FREE TEXT (0101 - 4b)
    RESERVED_A = 14,      ///< [Canonical] Type 14: Reserved A (00001 - 5b, 72 payload)
    RESERVED_B = 15,      ///< [Canonical] Type 15: Reserved B (0000010 - 7b, 70 payload)
    RESERVED_C = 16,      ///< [Canonical] Type 16: Reserved C (00000110 - 8b, 69 payload)
    UNKNOWN = 0,

    // Legacy transitional aliases (retained for backward compatibility)
    CALL_STD = 5,         ///< @deprecated Use CALL_STD_NOSUF
    CALL_STD_2SUF = 6,    ///< @deprecated Use CALL_STD_SUF
    CALL_NONSTD_1 = 7,    ///< @deprecated Use CALL_NONSTD
    REPLY73_STD = 8,      ///< @deprecated Use REPORT73_STD
    SEVENTY_THREE_STD = 9,///< @deprecated Use M73_STD
    SEVENTY_THREE_NONSTD = 10, ///< @deprecated Use M73_NONSTD
    MULTI_REPLY73 = 11,   ///< @deprecated Use MULTI_REPORT73
    REPORT73_NONSTD = 11, ///< @deprecated Use MULTI_REPORT73
    REPLY73_NONSTD = 11,  ///< @deprecated Use MULTI_REPORT73
    REPLY73_NONSTD_1 = 11,///< @deprecated Use MULTI_REPORT73
    RESERVED = 14,        ///< @deprecated Use RESERVED_A
    RESERVED_4 = 14       ///< @deprecated Use RESERVED_A
};

/// Physical layer protocol variants
enum class Protocol : uint8_t {
    LQ8 = 0,  ///< 8-GFSK, 50 Hz BW, 12.64s TX, 15s slot, 79 symbols
    LQ4 = 1,  ///< 4-GFSK, 83.3 Hz BW, 5.04s TX, 7.5s slot, 105 symbols
    LQ2 = 2,  ///< 4-GFSK, 166.7 Hz BW, 2.52s TX, 3.75s slot, 105 symbols
    LQ16 = 3  ///< 8-GFSK, 25 Hz BW, 25.28s TX, 30s slot, 79 symbols
};

/// BitBuffer: Utility for bit-level reading and writing on fixed or variable byte arrays.

/// Bits are packed MSB first (big-endian bit order) consistent with amateur radio protocols.
class BitBuffer {
public:
    explicit BitBuffer(uint8_t* buffer, size_t max_bits)
        : data_(buffer), max_bits_(max_bits), bit_pos_(0), read_only_(false) {}

    explicit BitBuffer(const uint8_t* buffer, size_t max_bits)
        : data_(const_cast<uint8_t*>(buffer)), max_bits_(max_bits), bit_pos_(0), read_only_(true) {}

    size_t get_pos() const { return bit_pos_; }
    void set_pos(size_t pos) { bit_pos_ = std::min(pos, max_bits_); }
    size_t remaining() const { return max_bits_ > bit_pos_ ? max_bits_ - bit_pos_ : 0; }
    bool is_read_only() const { return read_only_; }

    /// Read a single bit with zero overhead
    inline uint8_t read_bit() {
        if (bit_pos_ >= max_bits_) return 0;
        size_t byte_idx = bit_pos_ >> 3;
        int bit_idx = 7 - static_cast<int>(bit_pos_ & 7);
        ++bit_pos_;
        return (data_[byte_idx] >> bit_idx) & 1U;
    }

    /// Write a single bit with zero overhead
    inline void write_bit(uint8_t bit) {
        if (read_only_ || bit_pos_ >= max_bits_) return;
        size_t byte_idx = bit_pos_ >> 3;
        int bit_idx = 7 - static_cast<int>(bit_pos_ & 7);
        if (bit) {
            data_[byte_idx] |= static_cast<uint8_t>(1 << bit_idx);
        } else {
            data_[byte_idx] &= static_cast<uint8_t>(~(1 << bit_idx));
        }
        ++bit_pos_;
    }

    /// Write up to 64 bits from a uint64_t value (MSB-first of the low `n_bits` of `val`)
    void write_bits(uint64_t val, int n_bits) {
        if (read_only_ || n_bits <= 0) return;
        if (n_bits == 1) {
            write_bit(static_cast<uint8_t>(val & 1ULL));
            return;
        }
        for (int i = n_bits - 1; i >= 0; --i) {
            if (bit_pos_ >= max_bits_) return;
            uint8_t bit = (val >> i) & 1ULL;
            size_t byte_idx = bit_pos_ >> 3;
            int bit_idx = 7 - static_cast<int>(bit_pos_ & 7);
            if (bit) {
                data_[byte_idx] |= static_cast<uint8_t>(1 << bit_idx);
            } else {
                data_[byte_idx] &= static_cast<uint8_t>(~(1 << bit_idx));
            }
            ++bit_pos_;
        }
    }

    /// Read up to 64 bits into a uint64_t value
    uint64_t read_bits(int n_bits) {
        if (n_bits <= 0) return 0;
        if (n_bits == 1) return read_bit();
        uint64_t val = 0;
        for (int i = 0; i < n_bits; ++i) {
            if (bit_pos_ >= max_bits_) break;
            size_t byte_idx = bit_pos_ >> 3;
            int bit_idx = 7 - static_cast<int>(bit_pos_ & 7);
            uint8_t bit = (data_[byte_idx] >> bit_idx) & 1U;
            val = (val << 1) | bit;
            ++bit_pos_;
        }
        return val;
    }

    /// Write arbitrary bytes/bits from another buffer
    void write_raw_bits(const uint8_t* src, int n_bits) {
        if (read_only_) return;
        for (int i = 0; i < n_bits; ++i) {
            if (bit_pos_ >= max_bits_) return;
            size_t src_byte = i / 8;
            int src_bit = 7 - (i % 8);
            uint8_t bit = (src[src_byte] >> src_bit) & 1U;

            size_t dst_byte = bit_pos_ / 8;
            int dst_bit = 7 - static_cast<int>(bit_pos_ % 8);
            if (bit) {
                data_[dst_byte] |= static_cast<uint8_t>(1 << dst_bit);
            } else {
                data_[dst_byte] &= static_cast<uint8_t>(~(1 << dst_bit));
            }
            ++bit_pos_;
        }
    }

    /// Read arbitrary bits into destination buffer
    void read_raw_bits(uint8_t* dst, int n_bits) {
        std::memset(dst, 0, (n_bits + 7) / 8);
        for (int i = 0; i < n_bits; ++i) {
            if (bit_pos_ >= max_bits_) break;
            size_t src_byte = bit_pos_ / 8;
            int src_bit = 7 - static_cast<int>(bit_pos_ % 8);
            uint8_t bit = (data_[src_byte] >> src_bit) & 1U;

            size_t dst_byte = i / 8;
            int dst_bit = 7 - (i % 8);
            if (bit) {
                dst[dst_byte] |= static_cast<uint8_t>(1 << dst_bit);
            }
            ++bit_pos_;
        }
    }

    /// Write n_bits zero bits
    void pad_zeros(int n_bits) {
        if (read_only_ || n_bits <= 0) return;
        write_bits(0, n_bits);
    }

    /// Zero out remaining bits up to max_bits
    void pad_zeros() {
        if (read_only_) return;
        while (bit_pos_ < max_bits_) {
            size_t byte_idx = bit_pos_ / 8;
            int bit_idx = 7 - static_cast<int>(bit_pos_ % 8);
            data_[byte_idx] &= static_cast<uint8_t>(~(1 << bit_idx));
            ++bit_pos_;
        }
    }

private:
    uint8_t* data_;
    size_t max_bits_;
    size_t bit_pos_;
    bool read_only_;
};

} // namespace lq

#endif // LQ_TYPES_H
