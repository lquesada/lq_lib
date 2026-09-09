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

#ifndef LQ_HUFFMAN_H
#define LQ_HUFFMAN_H

#include <cstdint>
#include <string_view>
#include "lq/types.h"

namespace lq {

struct HuffmanCodeEntry {
    MessageType type;
    const char* bit_string;
    uint16_t code_val;
    uint8_t code_len;
};

/**
 * Get the Huffman code entry for a given message type.
 */
const HuffmanCodeEntry* get_huffman_entry(MessageType type);

/**
 * Encode the Huffman prefix code for `type` into `bb`.
 * Returns the number of bits written (1..14).
 */
int encode_huffman_prefix(MessageType type, BitBuffer& bb);

/**
 * Decode the Huffman prefix code from `bb`, returning the detected MessageType.
 * Returns MessageType::UNKNOWN if no valid prefix matches within 10 bits.
 */
MessageType decode_huffman_prefix(BitBuffer& bb);

} // namespace lq

#endif // LQ_HUFFMAN_H
