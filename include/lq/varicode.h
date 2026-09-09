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

#ifndef LQ_VARICODE_H
#define LQ_VARICODE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "lq/types.h"

namespace lq {

/// Standard LQ Type 13 free-text payload size (77 bits total - 4-bit Huffman prefix = 73 bits)
constexpr int VARICODE_PAYLOAD_BITS = 73;

/// Alphabet size: 104 characters (26 A-Z, 10 0-9, space, 32 ASCII symbols, 32 Latin-1, \n, ⌁/⚡, [FILL])
constexpr int VARICODE_ALPHABET_SIZE = 104;

/// Reserved special character / delimiter representation ('⌁' / ⚡ / EOM)
constexpr char SPECIAL_CHAR = '\x01'; // Reserved sentinel byte for higher-layer protocols
constexpr char EOM_CHAR = SPECIAL_CHAR; // Backward compatibility alias

/// Bit-padding fill sentinel character representation ([FILL])
constexpr char FILL_CHAR = '\x04'; // Sentinel byte used internally for [FILL]

/**
 * Check if a character (or byte) is part of the 104-character Varicode alphabet.
 */
bool is_varicode_char(char c);

/**
 * Get the exact bit length for a character in the Varicode Huffman codebook.
 * Returns 0 if character is invalid / not in alphabet.
 */
int get_varicode_char_length(char c);

/**
 * Encode a text string (ASCII, Latin-1, or UTF-8) into Varicode bits written into `bb` (up to max_bits).
 * Appends the EOM sentinel automatically if append_eom is true.
 * Fills remaining bits with pure zero-fill (consumed safely by [FILL]) if pad_zeros is true.
 * Returns the number of characters successfully encoded.
 */
int encode_varicode(std::string_view text, BitBuffer& bb, int max_bits = VARICODE_PAYLOAD_BITS, bool append_eom = true, bool pad_zeros = true);


/**
 * Decode Varicode bits from `bb` (reading up to max_bits) into UTF-8 `out`.
 * Stops decoding when the EOM sentinel is encountered, [FILL] branch is read, or max_bits are consumed.
 * Returns the number of characters decoded.
 */
int decode_varicode(BitBuffer& bb, std::string& out, int max_bits = VARICODE_PAYLOAD_BITS, bool* out_has_eom = nullptr);

/**
 * Decode a single space-delimited Varicode token from `bb` (e.g. for callsign or group fields).
 * Stops decoding when space ' ', EOM, [FILL], or max_bits are consumed.
 * Advances `bb` past the space delimiter.
 */
std::string decode_varicode_token(BitBuffer& bb, int max_bits = VARICODE_PAYLOAD_BITS);


} // namespace lq

#endif // LQ_VARICODE_H

