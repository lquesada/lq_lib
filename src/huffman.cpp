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

#include "lq/huffman.h"
#include <array>
#include <cstring>

namespace lq {

namespace {

constexpr HuffmanCodeEntry HUFFMAN_TABLE[] = {
    {MessageType::CALL_STD_NOSUF,   "1",              0x0001, 1},
    {MessageType::CALL_NONSTD,      "001",            0x0001, 3},
    {MessageType::MULTI_REPORT73,   "011",            0x0003, 3},
    {MessageType::CALL_STD_SUF,     "0100",           0x0004, 4},
    {MessageType::FREE_TEXT,        "0101",           0x0005, 4},
    {MessageType::M73_NONSTD,       "0001",           0x0001, 4},
    {MessageType::RESERVED_A,       "00001",          0x0001, 5},
    {MessageType::RESERVED_B,       "0000010",        0x0002, 7},
    {MessageType::RESERVED_C,       "00000110",       0x0006, 8},
    {MessageType::MULTI_73,         "00000111",       0x0007, 8},
    {MessageType::CQ_NONSTD_3,      "0000001",        0x0001, 7},
    {MessageType::CQ_NONSTD_2,      "00000001",       0x0001, 8},
    {MessageType::CQ_STD,           "0000000000",     0x0000, 10},
    {MessageType::CQ_NONSTD_1,      "0000000001",     0x0001, 10},
    {MessageType::REPORT73_STD,     "0000000010",     0x0002, 10},
    {MessageType::M73_STD,          "0000000011",     0x0003, 10},
};

struct HuffmanTrieNode {
    int8_t child[2];
    MessageType type;
};

struct HuffmanLookup {
    std::array<const HuffmanCodeEntry*, 32> entry_by_type;
    HuffmanTrieNode trie_nodes[64];
    int num_nodes;
};

constexpr HuffmanLookup make_huffman_lookup() {
    HuffmanLookup lk{};
    for (size_t i = 0; i < 32; ++i) {
        lk.entry_by_type[i] = nullptr;
    }
    for (const auto& entry : HUFFMAN_TABLE) {
        lk.entry_by_type[static_cast<size_t>(entry.type)] = &entry;
    }

    for (int i = 0; i < 64; ++i) {
        lk.trie_nodes[i].child[0] = -1;
        lk.trie_nodes[i].child[1] = -1;
        lk.trie_nodes[i].type = MessageType::UNKNOWN;
    }
    lk.num_nodes = 1;

    for (const auto& entry : HUFFMAN_TABLE) {
        int curr = 0;
        for (int b = entry.code_len - 1; b >= 0; --b) {
            int bit = (entry.code_val >> b) & 1U;
            if (lk.trie_nodes[curr].child[bit] == -1) {
                int next_idx = lk.num_nodes++;
                lk.trie_nodes[curr].child[bit] = static_cast<int8_t>(next_idx);
            }
            curr = lk.trie_nodes[curr].child[bit];
        }
        lk.trie_nodes[curr].type = entry.type;
    }
    return lk;
}

constexpr auto HUFFMAN_LOOKUP = make_huffman_lookup();

} // anonymous namespace

const HuffmanCodeEntry* get_huffman_entry(MessageType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx < 32) {
        return HUFFMAN_LOOKUP.entry_by_type[idx];
    }
    return nullptr;
}

int encode_huffman_prefix(MessageType type, BitBuffer& bb) {
    const auto* entry = get_huffman_entry(type);
    if (!entry) return 0;
    bb.write_bits(entry->code_val, entry->code_len);
    return entry->code_len;
}

MessageType decode_huffman_prefix(BitBuffer& bb) {
    int node = 0;
    while (bb.remaining() > 0) {
        uint8_t bit = bb.read_bit();
        node = HUFFMAN_LOOKUP.trie_nodes[node].child[bit];
        if (node < 0 || node >= 64) {
            return MessageType::UNKNOWN;
        }
        if (HUFFMAN_LOOKUP.trie_nodes[node].type != MessageType::UNKNOWN) {
            return HUFFMAN_LOOKUP.trie_nodes[node].type;
        }
    }
    return MessageType::UNKNOWN;
}

} // namespace lq
