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

#include "lq/varicode.h"
#include <cctype>
#include <cstring>
#include <array>

namespace lq {

namespace {

struct VaricodeEntry {
    uint8_t ch;
    uint32_t code_val; // Big-endian binary code value
    uint8_t code_len;  // Bit length (3 to 18)
};

// 104 entries from the sequential canonical Huffman codebook
constexpr VaricodeEntry VARICODE_TABLE[104] = {
    {' '   , 0x00001,  3}, // 001                (SPACE)
    {'E'   , 0x00002,  3}, // 010                (E)
    {'T'   , 0x00006,  4}, // 0110               (T)
    {'A'   , 0x00007,  4}, // 0111               (A)
    {'O'   , 0x00008,  4}, // 1000               (O)
    {'N'   , 0x00009,  4}, // 1001               (N)
    {'I'   , 0x0000A,  4}, // 1010               (I)
    {'S'   , 0x0000B,  4}, // 1011               (S)
    {'R'   , 0x0000C,  4}, // 1100               (R)
    {'H'   , 0x0000D,  4}, // 1101               (H)
    {'D'   , 0x0001C,  5}, // 11100              (D)
    {'L'   , 0x0001D,  5}, // 11101              (L)
    {'C'   , 0x0001E,  5}, // 11110              (C)
    {'U'   , 0x0003E,  6}, // 111110             (U)
    {'M'   , 0x0003F,  6}, // 111111             (M)
    {'F'   , 0x00004,  6}, // 000100             (F)
    {'W'   , 0x00005,  6}, // 000101             (W)
    {'P'   , 0x00006,  6}, // 000110             (P)
    {'G'   , 0x00007,  6}, // 000111             (G)
    {'Y'   , 0x00002,  6}, // 000010             (Y)
    {'B'   , 0x00003,  6}, // 000011             (B)
    {'V'   , 0x00002,  7}, // 0000010            (V)
    {'K'   , 0x00006,  8}, // 00000110           (K)
    {'\x01', 0x00007,  8}, // 00000111           (EOM (⌁))
    {'X'   , 0x00004,  9}, // 000000100          (X)
    {'J'   , 0x0000A, 10}, // 0000001010         (J)
    {'1'   , 0x0000B, 10}, // 0000001011         (1)
    {'Q'   , 0x0000C, 10}, // 0000001100         (Q)
    {'!'   , 0x0000D, 10}, // 0000001101         (!)
    {'?'   , 0x0000E, 10}, // 0000001110         (?)
    {'Z'   , 0x0000F, 10}, // 0000001111         (Z)
    {','   , 0x00008, 11}, // 00000001000        (,)
    {'.'   , 0x00009, 11}, // 00000001001        (.)
    {'0'   , 0x0000A, 11}, // 00000001010        (0)
    {'2'   , 0x0000B, 11}, // 00000001011        (2)
    {'8'   , 0x0000C, 11}, // 00000001100        (8)
    {'3'   , 0x0000D, 11}, // 00000001101        (3)
    {'4'   , 0x0000E, 11}, // 00000001110        (4)
    {'5'   , 0x0000F, 11}, // 00000001111        (5)
    {'9'   , 0x00004, 11}, // 00000000100        (9)
    {'6'   , 0x00005, 11}, // 00000000101        (6)
    {'7'   , 0x0000C, 12}, // 000000001100       (7)
    {'"'   , 0x0000D, 12}, // 000000001101       (")
    {'-'   , 0x0000E, 12}, // 000000001110       (-)
    {'\''  , 0x0000F, 12}, // 000000001111       (APOSTROPHE)
    {'_'   , 0x00004, 12}, // 000000000100       (_)
    {';'   , 0x00005, 12}, // 000000000101       (;)
    {':'   , 0x00006, 12}, // 000000000110       (:)
    {'='   , 0x0000E, 13}, // 0000000001110      (=)
    {'('   , 0x0000F, 13}, // 0000000001111      (()
    {')'   , 0x00004, 13}, // 0000000000100      ())
    {'#'   , 0x00005, 13}, // 0000000000101      (#)
    {'*'   , 0x00006, 13}, // 0000000000110      (*)
    {'['   , 0x0000E, 14}, // 00000000001110     ([)
    {']'   , 0x0000F, 14}, // 00000000001111     (])
    {'|'   , 0x00004, 14}, // 00000000000100     (|)
    {'/'   , 0x0000A, 15}, // 000000000001010    (/)
    {'$'   , 0x00016, 16}, // 0000000000010110   ($)
    {'+'   , 0x00017, 16}, // 0000000000010111   (+)
    {'\n'  , 0x00018, 16}, // 0000000000011000   (ENTER)
    {'%'   , 0x00019, 16}, // 0000000000011001   (%)
    {'&'   , 0x0001A, 16}, // 0000000000011010   (&)
    {'<'   , 0x0001B, 16}, // 0000000000011011   (<)
    {'>'   , 0x00038, 17}, // 00000000000111000  (>)
    {'@'   , 0x00039, 17}, // 00000000000111001  (@)
    {'\\'  , 0x0003A, 17}, // 00000000000111010  (BACKSLASH)
    {'^'   , 0x0003B, 17}, // 00000000000111011  (^)
    {'`'   , 0x0003C, 17}, // 00000000000111100  (`)
    {'{'   , 0x0003D, 17}, // 00000000000111101  ({)
    {'}'   , 0x0003E, 17}, // 00000000000111110  (})
    {'~'   , 0x0003F, 17}, // 00000000000111111  (~)
    {0xA1  , 0x00010, 17}, // 00000000000010000  (LATIN-1 ¡)
    {0xBF  , 0x00011, 17}, // 00000000000010001  (LATIN-1 ¿)
    {0xC0  , 0x00012, 17}, // 00000000000010010  (LATIN-1 À)
    {0xC1  , 0x00013, 17}, // 00000000000010011  (LATIN-1 Á)
    {0xC2  , 0x00014, 17}, // 00000000000010100  (LATIN-1 Â)
    {0xC3  , 0x00015, 17}, // 00000000000010101  (LATIN-1 Ã)
    {0xC4  , 0x00016, 17}, // 00000000000010110  (LATIN-1 Ä)
    {0xC5  , 0x00017, 17}, // 00000000000010111  (LATIN-1 Å)
    {0xC6  , 0x00018, 17}, // 00000000000011000  (LATIN-1 Æ)
    {0xC7  , 0x00019, 17}, // 00000000000011001  (LATIN-1 Ç)
    {0xC8  , 0x0001A, 17}, // 00000000000011010  (LATIN-1 È)
    {0xC9  , 0x0001B, 17}, // 00000000000011011  (LATIN-1 É)
    {0xCA  , 0x0001C, 17}, // 00000000000011100  (LATIN-1 Ê)
    {0xCB  , 0x0001D, 17}, // 00000000000011101  (LATIN-1 Ë)
    {0xCC  , 0x0001E, 17}, // 00000000000011110  (LATIN-1 Ì)
    {0xCD  , 0x0001F, 17}, // 00000000000011111  (LATIN-1 Í)
    {0xCE  , 0x00008, 17}, // 00000000000001000  (LATIN-1 Î)
    {0xCF  , 0x00009, 17}, // 00000000000001001  (LATIN-1 Ï)
    {0xD0  , 0x0000A, 17}, // 00000000000001010  (LATIN-1 Ð)
    {0xD1  , 0x0000B, 17}, // 00000000000001011  (LATIN-1 Ñ)
    {0xD2  , 0x0000C, 17}, // 00000000000001100  (LATIN-1 Ò)
    {0xD3  , 0x0000D, 17}, // 00000000000001101  (LATIN-1 Ó)
    {0xD4  , 0x0000E, 17}, // 00000000000001110  (LATIN-1 Ô)
    {0xD5  , 0x0000F, 17}, // 00000000000001111  (LATIN-1 Õ)
    {0xD6  , 0x00004, 17}, // 00000000000000100  (LATIN-1 Ö)
    {0xD8  , 0x00005, 17}, // 00000000000000101  (LATIN-1 Ø)
    {0xD9  , 0x00006, 17}, // 00000000000000110  (LATIN-1 Ù)
    {0xDA  , 0x00007, 17}, // 00000000000000111  (LATIN-1 Ú)
    {0xDB  , 0x00002, 17}, // 00000000000000010  (LATIN-1 Û)
    {0xDC  , 0x00003, 17}, // 00000000000000011  (LATIN-1 Ü)
    {0xDD  , 0x00001, 17}, // 00000000000000001  (LATIN-1 Ý)
    {0xDE  , 0x00001, 18}, // 000000000000000001 (LATIN-1 Þ)
    {'\x04', 0x00000, 18}, // 000000000000000000 (FILL ([FILL]))
};

struct VaricodeTrieNode {
    int16_t child[2];
    uint8_t ch;
    bool is_leaf;
};

// Fast O(1) decision structures
struct VaricodeLookup {
    VaricodeEntry encode_table[256];
    std::array<bool, 256> has_entry;
    VaricodeTrieNode trie_nodes[512];
    int num_nodes;
};

constexpr VaricodeLookup make_varicode_lookup() {
    VaricodeLookup lk{};
    for (int i = 0; i < 256; ++i) {
        lk.has_entry[i] = false;
        lk.encode_table[i] = {0, 0, 0};
    }
    for (const auto& entry : VARICODE_TABLE) {
        lk.encode_table[entry.ch] = entry;
        lk.has_entry[entry.ch] = true;
    }

    for (int i = 0; i < 512; ++i) {
        lk.trie_nodes[i].child[0] = -1;
        lk.trie_nodes[i].child[1] = -1;
        lk.trie_nodes[i].ch = 0;
        lk.trie_nodes[i].is_leaf = false;
    }
    lk.num_nodes = 1;

    for (const auto& entry : VARICODE_TABLE) {
        int curr = 0;
        for (int b = entry.code_len - 1; b >= 0; --b) {
            int bit = (entry.code_val >> b) & 1U;
            if (lk.trie_nodes[curr].child[bit] == -1) {
                int next_idx = lk.num_nodes++;
                lk.trie_nodes[curr].child[bit] = static_cast<int16_t>(next_idx);
            }
            curr = lk.trie_nodes[curr].child[bit];
        }
        lk.trie_nodes[curr].is_leaf = true;
        lk.trie_nodes[curr].ch = entry.ch;
    }
    return lk;
}

constexpr auto VARICODE_LOOKUP = make_varicode_lookup();

// Auto-uppercase normalization
uint8_t normalize_char_to_byte(uint8_t b) {
    if (b >= 'a' && b <= 'z') {
        return static_cast<uint8_t>(b - 32);
    }
    if (b >= 0xE0 && b <= 0xFE && b != 0xF7) {
        return static_cast<uint8_t>(b - 32);
    }
    return b;
}

const VaricodeEntry* find_entry(uint8_t b) {
    uint8_t norm = normalize_char_to_byte(b);
    if (VARICODE_LOOKUP.has_entry[norm]) {
        return &VARICODE_LOOKUP.encode_table[norm];
    }
    return nullptr;
}

} // anonymous namespace

bool is_varicode_char(char c) {
    uint8_t u = static_cast<uint8_t>(c);
    if (u == FILL_CHAR) return true;
    return find_entry(u) != nullptr;
}

int get_varicode_char_length(char c) {
    const VaricodeEntry* entry = find_entry(static_cast<uint8_t>(c));
    return entry ? static_cast<int>(entry->code_len) : 0;
}

int encode_varicode(std::string_view text, BitBuffer& bb, int max_bits, bool append_eom, bool pad_zeros) {
    size_t start_pos = bb.get_pos();
    int chars_written = 0;

    size_t i = 0;
    while (i < text.size()) {
        uint8_t b = static_cast<uint8_t>(text[i]);
        uint8_t mapped_byte = ' ';
        size_t bytes_consumed = 1;

        // Parse UTF-8 sequences
        if (b < 0x80) {
            mapped_byte = b;
        } else if ((b == 0xC2 || b == 0xC3) && (i + 1 < text.size())) {
            uint8_t b2 = static_cast<uint8_t>(text[i + 1]);
            if ((b2 & 0xC0) == 0x80) {
                bytes_consumed = 2;
                if (b == 0xC2) {
                    mapped_byte = b2;
                } else {
                    mapped_byte = static_cast<uint8_t>(b2 + 0x40);
                }
            }
        } else if (b == 0xE2 && i + 2 < text.size()) {
            // Check for EOM characters in UTF-8 (⌁ or ⚡)
            uint8_t b2 = static_cast<uint8_t>(text[i + 1]);
            uint8_t b3 = static_cast<uint8_t>(text[i + 2]);
            if ((b2 == 0x8C && b3 == 0x81) || (b2 == 0x9A && b3 == 0xA1)) {
                bytes_consumed = 3;
                mapped_byte = EOM_CHAR;
            } else {
                bytes_consumed = 3;
                mapped_byte = ' ';
            }
        } else {
            mapped_byte = b;
        }

        const VaricodeEntry* entry = find_entry(mapped_byte);
        if (!entry) {
            entry = find_entry(' ');
        }

        // Check if character fits
        int eom_reserve = append_eom ? 8 : 0;
        int total_needed = entry->code_len + eom_reserve;
        if (static_cast<int>(bb.get_pos() - start_pos) + total_needed > max_bits) {
            break; // Truncate message
        }

        bb.write_bits(entry->code_val, entry->code_len);
        ++chars_written;
        i += bytes_consumed;
    }

    // Append EOM sentinel if requested and fits
    if (append_eom) {
        const VaricodeEntry* eom = find_entry(EOM_CHAR);
        if (static_cast<int>(bb.get_pos() - start_pos) + eom->code_len <= max_bits) {
            bb.write_bits(eom->code_val, eom->code_len);
        }
    }

    // Zero-fill all remaining bits to max_bits if requested
    if (pad_zeros) {
        while (static_cast<int>(bb.get_pos() - start_pos) < max_bits) {
            bb.write_bits(0, 1);
        }
    }

    return chars_written;
}

int decode_varicode(BitBuffer& bb, std::string& out, int max_bits, bool* out_has_eom) {
    if (out_has_eom) *out_has_eom = false;
    out.clear();
    size_t start_pos = bb.get_pos();
    int chars_decoded = 0;
    int node = 0;

    while (static_cast<int>(bb.get_pos() - start_pos) < max_bits && bb.remaining() > 0) {
        uint8_t bit = bb.read_bit();
        node = VARICODE_LOOKUP.trie_nodes[node].child[bit];
        if (node < 0 || node >= VARICODE_LOOKUP.num_nodes) {
            goto done_decoding;
        }
        if (VARICODE_LOOKUP.trie_nodes[node].is_leaf) {
            uint8_t ch = VARICODE_LOOKUP.trie_nodes[node].ch;
            if (ch == FILL_CHAR) {
                goto done_decoding;
            }
            if (ch == EOM_CHAR) {
                if (out_has_eom) *out_has_eom = true;
                goto done_decoding;
            }

            if (ch < 0x80) {
                out.push_back(static_cast<char>(ch));
            } else {
                out.push_back(static_cast<char>(0xC0 | (ch >> 6)));
                out.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
            }

            ++chars_decoded;
            node = 0;
        }
    }

done_decoding:
    return chars_decoded;
}

std::string decode_varicode_token(BitBuffer& bb, int max_bits) {
    std::string token;
    size_t start_pos = bb.get_pos();
    int node = 0;

    while (static_cast<int>(bb.get_pos() - start_pos) < max_bits && bb.remaining() > 0) {
        uint8_t bit = bb.read_bit();
        node = VARICODE_LOOKUP.trie_nodes[node].child[bit];
        if (node < 0 || node >= VARICODE_LOOKUP.num_nodes) {
            return token;
        }
        if (VARICODE_LOOKUP.trie_nodes[node].is_leaf) {
            uint8_t ch = VARICODE_LOOKUP.trie_nodes[node].ch;
            if (ch == FILL_CHAR || ch == EOM_CHAR) {
                return token;
            }
            if (ch == ' ') {
                return token;
            }
            if (ch < 0x80) {
                token.push_back(static_cast<char>(ch));
            } else {
                token.push_back(static_cast<char>(0xC0 | (ch >> 6)));
                token.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
            }
            node = 0;
        }
    }
    return token;
}

} // namespace lq
