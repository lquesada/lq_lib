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

#include "lq/message.h"
#include "lq/huffman.h"
#include "lq/hash.h"
#include "lq/callsign.h"
#include "lq/callsign_nonstd.h"
#include "lq/locator.h"
#include "lq/modifier.h"
#include "lq/rst.h"
#include "lq/varicode.h"
#include <algorithm>
#include <cctype>

namespace lq {

namespace {

void append_hex_tag(std::string& s, uint32_t val, int width) {
    static const char hex_chars[] = "0123456789abcdef";
    s.push_back('<');
    for (int i = width - 1; i >= 0; --i) {
        s.push_back(hex_chars[(val >> (i * 4)) & 0x0F]);
    }
    s.push_back('>');
}


} // anonymous namespace

std::string Message::get_sender_call() const {
    if (is_cq()) {
        return call_1;
    }
    if (is_call() || is_report73() || is_73()) {
        if (!call_2.empty()) {
            return call_2;
        }
        if (hash_2 != 0) {
            return "<...>";
        }
    }
    if (is_multi_report73() || is_multi_73()) {
        if (!call_1.empty()) {
            return call_1;
        }
        if (hash_1 != 0) {
            return "<...>";
        }
    }
    return "";
}

std::string Message::get_target_call() const {
    if (is_cq()) {
        return "";
    }
    if ((is_multi_report73() || is_multi_73()) && !multi_targets.empty()) {
        if (!multi_targets[0].call.empty()) {
            return multi_targets[0].call;
        }
        if (multi_targets[0].hash != 0) {
            return "<...>";
        }
    }
    if (is_call() || is_report73() || is_73()) {
        if (!call_1.empty()) {
            return call_1;
        }
        if (hash_1 != 0) {
            return "<...>";
        }
    }
    return "";
}

uint32_t Message::get_sender_hash() const {
    if (is_multi_report73() || is_multi_73()) {
        if (hash_1 != 0) return hash_1;
        if (!call_1.empty()) return hash_callsign_16(call_1);
    }
    if (hash_2 != 0) return hash_2;
    if (!call_2.empty()) return hash_callsign_24(call_2);
    if (is_cq() && !call_1.empty()) return hash_callsign_24(call_1);
    return 0;
}

uint32_t Message::get_target_hash() const {
    if (is_multi_report73() || is_multi_73()) {
        if (!multi_targets.empty()) {
            if (multi_targets[0].hash != 0) return multi_targets[0].hash;
            if (!multi_targets[0].call.empty()) return hash_callsign_24(multi_targets[0].call);
        }
        return 0;
    }
    if (hash_1 != 0) return hash_1;
    if (!call_1.empty() && (is_call() || is_report73() || is_73())) return hash_callsign_24(call_1);
    return 0;
}

bool Message::is_cq() const {
    return type == MessageType::CQ_STD ||
           type == MessageType::CQ_NONSTD_1 ||
           type == MessageType::CQ_NONSTD_2 ||
           type == MessageType::CQ_NONSTD_3;
}

bool Message::is_call() const {
    return type == MessageType::CALL_STD_NOSUF ||
           type == MessageType::CALL_STD_SUF ||
           type == MessageType::CALL_NONSTD;
}

bool Message::is_report73() const {
    return type == MessageType::REPORT73_STD ||
           type == MessageType::MULTI_REPORT73;
}

bool Message::is_73() const {
    return type == MessageType::M73_STD ||
           type == MessageType::M73_NONSTD ||
           type == MessageType::MULTI_73;
}

bool Message::is_multi_report73() const {
    return type == MessageType::MULTI_REPORT73;
}

bool Message::is_multi_73() const {
    return type == MessageType::MULTI_73;
}

bool Message::is_free_text() const {
    return type == MessageType::FREE_TEXT;
}

bool encode_message(const Message& msg, uint8_t payload[PAYLOAD_BYTES]) {

    std::memset(payload, 0, PAYLOAD_BYTES);
    BitBuffer bb(payload, PAYLOAD_BITS);

    if (encode_huffman_prefix(msg.type, bb) == 0) {
        return false;
    }

    switch (msg.type) {
        case MessageType::CALL_STD_NOSUF: { // Type 5: 1 + 28 + 28 + 15 + 5 = 77
            uint32_t c1 = 0, c2 = 0;
            if (!encode_callsign_std(msg.call_1, c1)) return false;
            if (!encode_callsign_std(msg.call_2, c2)) return false;
            uint8_t r = encode_rst_5(msg.rst_db);
            uint16_t loc = 0;
            if (!encode_locator_15(msg.locator, loc)) return false;

            bb.write_bits(c1, 28);
            bb.write_bits(c2, 28);
            bb.write_bits(loc, 15);
            bb.write_bits(r, 5);
            break;
        }
        case MessageType::CALL_STD_SUF: { // Type 6: 4 + 24 + 28 + 1 + 15 + 5 = 77
            uint32_t h1 = msg.hash_1 ? (msg.hash_1 & 0xFFFFFFU) : hash_callsign_24(msg.call_1);
            std::string base_c2;
            uint8_t s2 = msg.suffix_2;
            uint32_t c2 = 0;
            if (!parse_standard_callsign_suffix(msg.call_2, base_c2, s2) || !encode_callsign_std(base_c2, c2)) {
                return false;
            }
            if (msg.suffix_2 != 0) s2 = msg.suffix_2 & 1;
            uint16_t loc = 0;
            if (!msg.locator.empty() && !encode_locator_15(msg.locator, loc)) return false;
            uint8_t r = encode_rst_5(msg.rst_db);

            bb.write_bits(h1, 24);
            bb.write_bits(c2, 28);
            bb.write_bits(s2 & 1, 1);
            bb.write_bits(loc, 15);
            bb.write_bits(r, 5);
            break;
        }
        case MessageType::CALL_NONSTD: { // Type 7: 3 + 20 + 48 + 1 + 5 = 77
            uint32_t h1 = 0;
            if (msg.hash_1 != 0) {
                if (msg.hash_1 > 0xFFFFFU) {
                    h1 = (msg.hash_1 >> 4) & 0xFFFFFU;
                } else {
                    h1 = msg.hash_1 & 0xFFFFFU;
                }
            } else if (!msg.call_1.empty()) {
                h1 = hash_callsign_20(msg.call_1);
            }

            std::string base_c2;
            uint8_t s2 = msg.suffix_2;
            if (!parse_any_callsign_suffix(msg.call_2, base_c2, s2)) {
                base_c2 = msg.call_2;
            }
            if (msg.suffix_2 != 0) s2 = msg.suffix_2 & 1;
            uint8_t r = encode_rst_5(msg.rst_db);

            bb.write_bits(h1, 20);
            if (!encode_callsign_nonstd(base_c2, 9, bb)) return false;
            bb.write_bits(s2 & 1, 1);
            bb.write_bits(r, 5);
            break;
        }
        case MessageType::M73_NONSTD: { // Type 10: 4 + 24 + 48 + 1 = 77
            uint32_t h1 = msg.hash_1 ? (msg.hash_1 & 0xFFFFFFU) : hash_callsign_24(msg.call_1);
            std::string base_c2;
            uint8_t s2 = msg.suffix_2;
            if (!parse_any_callsign_suffix(msg.call_2, base_c2, s2)) {
                base_c2 = msg.call_2;
            }
            if (msg.suffix_2 != 0) s2 = msg.suffix_2 & 1;

            bb.write_bits(h1, 24);
            if (!encode_callsign_nonstd(base_c2, 9, bb)) return false;
            bb.write_bits(s2 & 1, 1);
            break;
        }
        case MessageType::MULTI_REPORT73: { // Type 11: 3 + 16 + 29 + 29 = 77
            uint32_t dx_hash = msg.hash_1 ? (msg.hash_1 & 0xFFFFU) : (msg.call_1.empty() ? 0 : hash_callsign_16(msg.call_1));
            bb.write_bits(dx_hash, 16);

            // Target 1
            uint32_t th1 = 0;
            int rst1 = msg.rst_db;
            if (!msg.multi_targets.empty()) {
                th1 = msg.multi_targets[0].hash ? (msg.multi_targets[0].hash & 0xFFFFFFU) : (msg.multi_targets[0].call.empty() ? 0 : hash_callsign_24(msg.multi_targets[0].call));
                rst1 = msg.multi_targets[0].rst_db;
            } else if (msg.hash_2 != 0) {
                th1 = msg.hash_2 & 0xFFFFFFU;
            } else if (!msg.call_2.empty()) {
                th1 = hash_callsign_24(msg.call_2);
            }
            uint8_t r1 = encode_rst_5(rst1);
            bb.write_bits(th1, 24);
            bb.write_bits(r1, 5);

            // Target 2 (if 2 targets, write target 2; else duplicate target 1)
            uint32_t th2 = th1;
            uint8_t r2 = r1;
            if (msg.multi_targets.size() > 1) {
                th2 = msg.multi_targets[1].hash ? (msg.multi_targets[1].hash & 0xFFFFFFU) : (msg.multi_targets[1].call.empty() ? 0 : hash_callsign_24(msg.multi_targets[1].call));
                r2 = encode_rst_5(msg.multi_targets[1].rst_db);
            }
            bb.write_bits(th2, 24);
            bb.write_bits(r2, 5);
            break;
        }
        case MessageType::FREE_TEXT: { // Type 13: 4 + 73 = 77
            encode_varicode(msg.text, bb, VARICODE_PAYLOAD_BITS);
            break;
        }
        case MessageType::RESERVED_A: { // Type 14: 5 + 72 = 77
            int raw_bits = std::min(72, static_cast<int>(msg.raw_payload.size() * 8));
            for (int i = 0; i < raw_bits; ++i) {
                size_t byte_idx = i / 8;
                int bit_idx = 7 - (i % 8);
                uint64_t b = (msg.raw_payload[byte_idx] >> bit_idx) & 1U;
                bb.write_bits(b, 1);
            }
            while (raw_bits < 72) {
                bb.write_bits(0, 1);
                ++raw_bits;
            }
            break;
        }
        case MessageType::RESERVED_B: { // Type 15: 7 + 70 = 77
            int raw_bits = std::min(70, static_cast<int>(msg.raw_payload.size() * 8));
            for (int i = 0; i < raw_bits; ++i) {
                size_t byte_idx = i / 8;
                int bit_idx = 7 - (i % 8);
                uint64_t b = (msg.raw_payload[byte_idx] >> bit_idx) & 1U;
                bb.write_bits(b, 1);
            }
            while (raw_bits < 70) {
                bb.write_bits(0, 1);
                ++raw_bits;
            }
            break;
        }
        case MessageType::CQ_NONSTD_3: { // Type 4: 7 + 69 + 1 = 77
            std::string base_c1;
            uint8_t s1 = msg.suffix_1;
            if (!parse_any_callsign_suffix(msg.call_1, base_c1, s1)) {
                base_c1 = msg.call_1;
            }
            if (msg.suffix_1 != 0) s1 = msg.suffix_1 & 1;

            if (!encode_callsign_nonstd(base_c1, 13, bb)) return false;
            bb.write_bits(s1 & 1, 1);
            break;
        }
        case MessageType::CQ_NONSTD_2: { // Type 3: 8 + 48 + 1 + 20 = 77
            std::string base_c1;
            uint8_t s1 = msg.suffix_1;
            if (!parse_any_callsign_suffix(msg.call_1, base_c1, s1)) {
                base_c1 = msg.call_1;
            }
            if (msg.suffix_1 != 0) s1 = msg.suffix_1 & 1;

            if (!encode_callsign_nonstd(base_c1, 9, bb)) return false;
            bb.write_bits(s1 & 1, 1);
            uint32_t m = 0;
            if (!encode_modifier_20(msg.modifier, m)) return false;
            bb.write_bits(m, 20);
            break;
        }
        case MessageType::CQ_STD: { // Type 1: 10 + 28 + 1 + 20 + 15 = 74 (+3 pad)
            std::string base_c1;
            uint8_t s1 = msg.suffix_1;
            uint32_t c1 = 0;
            if (!parse_standard_callsign_suffix(msg.call_1, base_c1, s1) || !encode_callsign_std(base_c1, c1)) {
                return false;
            }
            if (msg.suffix_1 != 0) s1 = msg.suffix_1 & 1;
            uint32_t m = 0;
            if (!encode_modifier_20(msg.modifier, m)) return false;
            uint16_t loc = 0;
            if (!encode_locator_15(msg.locator, loc)) return false;

            bb.write_bits(c1, 28);
            bb.write_bits(s1 & 1, 1);
            bb.write_bits(m, 20);
            bb.write_bits(loc, 15);
            break;
        }
        case MessageType::CQ_NONSTD_1: { // Type 2: 10 + 48 + 1 + 15 = 74 (+3 pad)
            std::string base_c1;
            uint8_t s1 = msg.suffix_1;
            if (!parse_any_callsign_suffix(msg.call_1, base_c1, s1)) {
                base_c1 = msg.call_1;
            }
            if (msg.suffix_1 != 0) s1 = msg.suffix_1 & 1;

            if (!encode_callsign_nonstd(base_c1, 9, bb)) return false;
            bb.write_bits(s1 & 1, 1);
            uint16_t loc = 0;
            if (!encode_locator_15(msg.locator, loc)) return false;
            bb.write_bits(loc, 15);
            break;
        }
        case MessageType::REPORT73_STD: { // Type 8: 10 + 28 + 1 + 28 + 1 + 5 = 73 (+4 pad)
            std::string base_c1, base_c2;
            uint8_t s1 = msg.suffix_1, s2 = msg.suffix_2;
            uint32_t c1 = 0, c2 = 0;
            if (!parse_standard_callsign_suffix(msg.call_1, base_c1, s1) || !encode_callsign_std(base_c1, c1)) {
                return false;
            }
            if (!parse_standard_callsign_suffix(msg.call_2, base_c2, s2) || !encode_callsign_std(base_c2, c2)) {
                return false;
            }
            if (msg.suffix_1 != 0) s1 = msg.suffix_1 & 1;
            if (msg.suffix_2 != 0) s2 = msg.suffix_2 & 1;
            uint8_t r = encode_rst_5(msg.rst_db);

            bb.write_bits(c1, 28);
            bb.write_bits(s1 & 1, 1);
            bb.write_bits(c2, 28);
            bb.write_bits(s2 & 1, 1);
            bb.write_bits(r, 5);
            break;
        }
        case MessageType::M73_STD: { // Type 9: 10 + 28 + 1 + 28 + 1 = 68 (+9 pad)
            std::string base_c1, base_c2;
            uint8_t s1 = msg.suffix_1, s2 = msg.suffix_2;
            uint32_t c1 = 0, c2 = 0;
            if (!parse_standard_callsign_suffix(msg.call_1, base_c1, s1) || !encode_callsign_std(base_c1, c1)) {
                return false;
            }
            if (!parse_standard_callsign_suffix(msg.call_2, base_c2, s2) || !encode_callsign_std(base_c2, c2)) {
                return false;
            }
            if (msg.suffix_1 != 0) s1 = msg.suffix_1 & 1;
            if (msg.suffix_2 != 0) s2 = msg.suffix_2 & 1;

            bb.write_bits(c1, 28);
            bb.write_bits(s1 & 1, 1);
            bb.write_bits(c2, 28);
            bb.write_bits(s2 & 1, 1);
            break;
        }
        case MessageType::RESERVED_C: { // Type 16: 8 + 69 = 77
            int raw_bits = std::min(69, static_cast<int>(msg.raw_payload.size() * 8));
            for (int i = 0; i < raw_bits; ++i) {
                size_t byte_idx = i / 8;
                int bit_idx = 7 - (i % 8);
                uint64_t b = (msg.raw_payload[byte_idx] >> bit_idx) & 1U;
                bb.write_bits(b, 1);
            }
            while (raw_bits < 69) {
                bb.write_bits(0, 1);
                ++raw_bits;
            }
            break;
        }
        case MessageType::MULTI_73: { // Type 12: 8 + 16 + 24 + 24 = 72 (+5 pad)
            uint32_t dx_hash = msg.hash_1 ? (msg.hash_1 & 0xFFFFU) : (msg.call_1.empty() ? 0 : hash_callsign_16(msg.call_1));
            bb.write_bits(dx_hash, 16);

            // Target 1
            uint32_t th1 = 0;
            if (!msg.multi_targets.empty()) {
                th1 = msg.multi_targets[0].hash ? (msg.multi_targets[0].hash & 0xFFFFFFU) : (msg.multi_targets[0].call.empty() ? 0 : hash_callsign_24(msg.multi_targets[0].call));
            } else if (msg.hash_2 != 0) {
                th1 = msg.hash_2 & 0xFFFFFFU;
            } else if (!msg.call_2.empty()) {
                th1 = hash_callsign_24(msg.call_2);
            }
            bb.write_bits(th1, 24);

            // Target 2 (if 2 targets, write target 2; else duplicate target 1)
            uint32_t th2 = th1;
            if (msg.multi_targets.size() > 1) {
                th2 = msg.multi_targets[1].hash ? (msg.multi_targets[1].hash & 0xFFFFFFU) : (msg.multi_targets[1].call.empty() ? 0 : hash_callsign_24(msg.multi_targets[1].call));
            }
            bb.write_bits(th2, 24);
            break;
        }
        default:
            return false;
    }

    bb.pad_zeros();
    return true;
}

bool decode_message(const uint8_t payload[PAYLOAD_BYTES], Message& msg) {
    msg = Message{};
    BitBuffer bb(payload, PAYLOAD_BITS);

    msg.type = decode_huffman_prefix(bb);
    if (msg.type == MessageType::UNKNOWN) {
        return false;
    }

    switch (msg.type) {
        case MessageType::CALL_STD_NOSUF: { // Type 5
            uint32_t c1 = static_cast<uint32_t>(bb.read_bits(28));
            uint32_t c2 = static_cast<uint32_t>(bb.read_bits(28));
            uint16_t loc = static_cast<uint16_t>(bb.read_bits(15));
            uint8_t r   = static_cast<uint8_t>(bb.read_bits(5));

            if (!decode_callsign_std(c1, msg.call_1)) return false;
            if (!decode_callsign_std(c2, msg.call_2)) return false;
            if (!decode_locator_15(loc, msg.locator)) return false;
            msg.rst_db = decode_rst_5(r);
            break;
        }
        case MessageType::CALL_STD_SUF: { // Type 6
            msg.hash_1 = static_cast<uint32_t>(bb.read_bits(24));
            uint32_t c2 = static_cast<uint32_t>(bb.read_bits(28));
            uint8_t s2  = static_cast<uint8_t>(bb.read_bits(1));
            uint16_t loc = static_cast<uint16_t>(bb.read_bits(15));
            uint8_t r   = static_cast<uint8_t>(bb.read_bits(5));

            if (!decode_callsign_std(c2, msg.call_2)) return false;
            msg.suffix_2 = s2;
            msg.call_2 = format_callsign_with_suffix(msg.call_2, s2);
            if (!decode_locator_15(loc, msg.locator)) return false;
            msg.rst_db = decode_rst_5(r);
            break;
        }
        case MessageType::FREE_TEXT: { // Type 13: 4 + 73 = 77
            decode_varicode(bb, msg.text, VARICODE_PAYLOAD_BITS);
            break;
        }
        case MessageType::CALL_NONSTD: { // Type 7: 3 + 20 + 48 + 1 + 5 = 77
            msg.hash_1 = static_cast<uint32_t>(bb.read_bits(20));
            if (!decode_callsign_nonstd(bb, 9, msg.call_2)) return false;
            uint8_t s2 = static_cast<uint8_t>(bb.read_bits(1));
            msg.suffix_2 = s2;
            msg.call_2 = format_callsign_with_suffix(msg.call_2, s2);
            uint8_t r = static_cast<uint8_t>(bb.read_bits(5));
            msg.rst_db = decode_rst_5(r);
            msg.locator.clear();
            msg.hash_2 = 0;
            break;
        }
        case MessageType::M73_NONSTD: { // Type 10
            msg.hash_1 = static_cast<uint32_t>(bb.read_bits(24));
            if (!decode_callsign_nonstd(bb, 9, msg.call_2)) return false;
            uint8_t s2 = static_cast<uint8_t>(bb.read_bits(1));
            msg.suffix_2 = s2;
            msg.call_2 = format_callsign_with_suffix(msg.call_2, s2);
            break;
        }
        case MessageType::MULTI_REPORT73: { // Type 11: 3b code + 16b dx_hash + 2x (24b hash + 5b rst)
            msg.hash_1 = static_cast<uint32_t>(bb.read_bits(16));
            
            MultiTarget t1;
            t1.hash = static_cast<uint32_t>(bb.read_bits(24));
            uint8_t r1 = static_cast<uint8_t>(bb.read_bits(5));
            t1.rst_db = decode_rst_5(r1);
            msg.multi_targets.push_back(t1);

            MultiTarget t2;
            t2.hash = static_cast<uint32_t>(bb.read_bits(24));
            uint8_t r2 = static_cast<uint8_t>(bb.read_bits(5));
            t2.rst_db = decode_rst_5(r2);
            if (t2.hash != 0 && !(t2.hash == t1.hash && t2.rst_db == t1.rst_db)) {
                msg.multi_targets.push_back(t2);
            }
            break;
        }
        case MessageType::RESERVED_A: { // Type 14: 5b prefix, 72b payload
            msg.raw_payload.resize(10, 0);
            bb.read_raw_bits(msg.raw_payload.data(), 72);
            break;
        }
        case MessageType::RESERVED_B: { // Type 15: 7b prefix, 70b payload
            msg.raw_payload.resize(10, 0);
            bb.read_raw_bits(msg.raw_payload.data(), 70);
            break;
        }
        case MessageType::CQ_NONSTD_3: { // Type 4
            if (!decode_callsign_nonstd(bb, 13, msg.call_1)) return false;
            uint8_t s1 = static_cast<uint8_t>(bb.read_bits(1));
            msg.suffix_1 = s1;
            msg.call_1 = format_callsign_with_suffix(msg.call_1, s1);
            break;
        }
        case MessageType::CQ_NONSTD_2: { // Type 3
            if (!decode_callsign_nonstd(bb, 9, msg.call_1)) return false;
            uint8_t s1 = static_cast<uint8_t>(bb.read_bits(1));
            msg.suffix_1 = s1;
            msg.call_1 = format_callsign_with_suffix(msg.call_1, s1);
            uint32_t m = static_cast<uint32_t>(bb.read_bits(20));
            if (!decode_modifier_20(m, msg.modifier)) return false;
            break;
        }
        case MessageType::CQ_STD: { // Type 1
            uint32_t c1 = static_cast<uint32_t>(bb.read_bits(28));
            uint8_t s1  = static_cast<uint8_t>(bb.read_bits(1));
            uint32_t m  = static_cast<uint32_t>(bb.read_bits(20));
            uint16_t loc = static_cast<uint16_t>(bb.read_bits(15));

            if (!decode_callsign_std(c1, msg.call_1)) return false;
            msg.suffix_1 = s1;
            msg.call_1 = format_callsign_with_suffix(msg.call_1, s1);

            if (!decode_modifier_20(m, msg.modifier)) return false;
            if (!decode_locator_15(loc, msg.locator)) return false;
            break;
        }
        case MessageType::CQ_NONSTD_1: { // Type 2
            if (!decode_callsign_nonstd(bb, 9, msg.call_1)) return false;
            uint8_t s1 = static_cast<uint8_t>(bb.read_bits(1));
            msg.suffix_1 = s1;
            msg.call_1 = format_callsign_with_suffix(msg.call_1, s1);

            uint16_t loc = static_cast<uint16_t>(bb.read_bits(15));
            if (!decode_locator_15(loc, msg.locator)) return false;
            break;
        }
        case MessageType::REPORT73_STD: { // Type 8
            uint32_t c1 = static_cast<uint32_t>(bb.read_bits(28));
            uint8_t s1  = static_cast<uint8_t>(bb.read_bits(1));
            uint32_t c2 = static_cast<uint32_t>(bb.read_bits(28));
            uint8_t s2  = static_cast<uint8_t>(bb.read_bits(1));
            uint8_t r   = static_cast<uint8_t>(bb.read_bits(5));

            if (!decode_callsign_std(c1, msg.call_1)) return false;
            msg.suffix_1 = s1;
            msg.call_1 = format_callsign_with_suffix(msg.call_1, s1);

            if (!decode_callsign_std(c2, msg.call_2)) return false;
            msg.suffix_2 = s2;
            msg.call_2 = format_callsign_with_suffix(msg.call_2, s2);

            msg.rst_db = decode_rst_5(r);
            break;
        }
        case MessageType::M73_STD: { // Type 9
            uint32_t c1 = static_cast<uint32_t>(bb.read_bits(28));
            uint8_t s1  = static_cast<uint8_t>(bb.read_bits(1));
            uint32_t c2 = static_cast<uint32_t>(bb.read_bits(28));
            uint8_t s2  = static_cast<uint8_t>(bb.read_bits(1));

            if (!decode_callsign_std(c1, msg.call_1)) return false;
            msg.suffix_1 = s1;
            msg.call_1 = format_callsign_with_suffix(msg.call_1, s1);

            if (!decode_callsign_std(c2, msg.call_2)) return false;
            msg.suffix_2 = s2;
            msg.call_2 = format_callsign_with_suffix(msg.call_2, s2);
            break;
        }
        case MessageType::RESERVED_C: { // Type 16: 8b prefix, 69b payload
            msg.raw_payload.resize(10, 0);
            bb.read_raw_bits(msg.raw_payload.data(), 69);
            break;
        }
        case MessageType::MULTI_73: { // Type 12: 8b code + 16b dx_hash + 2x 24b target hash
            msg.hash_1 = static_cast<uint32_t>(bb.read_bits(16));
            
            MultiTarget t1;
            t1.hash = static_cast<uint32_t>(bb.read_bits(24));
            t1.rst_db = 0;
            msg.multi_targets.push_back(t1);

            MultiTarget t2;
            t2.hash = static_cast<uint32_t>(bb.read_bits(24));
            t2.rst_db = 0;
            if (t2.hash != 0 && t2.hash != t1.hash) {
                msg.multi_targets.push_back(t2);
            }
            break;
        }
        default:
            return false;
    }
    return true;
}

bool resolve_callsigns(Message& msg, const std::string_view* known_callsigns, size_t count) {
    if (!known_callsigns || count == 0) return false;
    bool any_resolved = false;

    // 1. Check hash_1
    if (msg.hash_1 != 0 && (msg.call_1.empty() || msg.call_1.front() == '<')) {
        std::string base_sender;
        uint8_t suf_sender = 0;
        bool sender_is_std = !msg.call_2.empty() && parse_standard_callsign_suffix(msg.call_2, base_sender, suf_sender);

        for (size_t i = 0; i < count; ++i) {
            const auto& call = known_callsigns[i];
            if (msg.type == MessageType::MULTI_REPORT73 || msg.type == MessageType::MULTI_73) {
                if (hash_callsign_16(call) == msg.hash_1) {
                    msg.call_1 = std::string(call);
                    any_resolved = true;
                    break;
                }
            } else if (msg.type == MessageType::CALL_NONSTD) {
                if ((hash_callsign_24(call) >> 4) == msg.hash_1) {
                    msg.call_1 = std::string(call);
                    any_resolved = true;
                    break;
                }
            } else {
                if (hash_callsign_24(call) == msg.hash_1) {
                    // Canonical Precedence & Disambiguation rule:
                    // 1. In CALL_STD_SUF (Type 6), if caller/sender (call_2) is standard without suffix (suf_sender == 0),
                    // and candidate target (call) is standard without suffix (suf_tgt == 0), the transmitter was required to use
                    // Type 5 (CALL_STD_NOSUF). Therefore, a Type 6 hash match cannot refer to a standard callsign without suffix.
                    if (msg.type == MessageType::CALL_STD_SUF && sender_is_std && suf_sender == 0) {
                        std::string base_tgt;
                        uint8_t suf_tgt = 0;
                        if (parse_standard_callsign_suffix(call, base_tgt, suf_tgt) && suf_tgt == 0) {
                            // Accidental hash collision with standard callsign without suffix; skip
                            continue;
                        }
                    }

                    // 2. In M73_NONSTD (Type 10), if caller/sender (call_2) is standard (suf_sender <= 1)
                    // and candidate target (call) is standard (suf_tgt <= 1), the transmitter was required to use
                    // Type 9 (M73_STD). Therefore, a Type 10 hash match cannot refer to standard callsign with suf <= 1.
                    if (msg.type == MessageType::M73_NONSTD && sender_is_std && suf_sender <= 1) {
                        std::string base_tgt;
                        uint8_t suf_tgt = 0;
                        if (parse_standard_callsign_suffix(call, base_tgt, suf_tgt) && suf_tgt <= 1) {
                            // Accidental hash collision with standard callsign that fits in Type 9; skip
                            continue;
                        }
                    }
                    msg.call_1 = std::string(call);
                    any_resolved = true;
                    break;
                }
            }
        }
    }

    // 2. Check hash_2
    if (msg.hash_2 != 0 && (msg.call_2.empty() || msg.call_2.front() == '<')) {
        for (size_t i = 0; i < count; ++i) {
            const auto& call = known_callsigns[i];
            if (hash_callsign_24(call) == msg.hash_2) {
                msg.call_2 = std::string(call);
                any_resolved = true;
                break;
            }
        }
    }

    // 3. Check multi_targets
    if (msg.type == MessageType::MULTI_REPORT73 || msg.type == MessageType::MULTI_73) {
        for (auto& tgt : msg.multi_targets) {
            if (tgt.hash != 0 && (tgt.call.empty() || tgt.call.front() == '<')) {
                for (size_t i = 0; i < count; ++i) {
                    const auto& call = known_callsigns[i];
                    if (hash_callsign_24(call) == tgt.hash) {
                        tgt.call = std::string(call);
                        any_resolved = true;
                        break;
                    }
                }
            }
        }
    }

    return any_resolved;
}

bool decode_message(const uint8_t payload[PAYLOAD_BYTES], Message& msg, const std::string_view* known_callsigns, size_t count) {
    if (!decode_message(payload, msg)) return false;
    if (known_callsigns && count > 0) {
        resolve_callsigns(msg, known_callsigns, count);
    }
    return true;
}

std::string format_message(const Message& msg) {
    std::string s;
    s.reserve(32);
    switch (msg.type) {
        case MessageType::CQ_STD:
            s.append("CQ");
            if (!msg.modifier.empty()) { s.push_back(' '); s.append(msg.modifier); }
            s.push_back(' '); s.append(format_callsign_with_suffix(msg.call_1, msg.suffix_1));
            if (!msg.locator.empty()) { s.push_back(' '); s.append(msg.locator); }
            break;
        case MessageType::CQ_NONSTD_1:
            s.append("CQ "); s.append(format_callsign_with_suffix(msg.call_1, msg.suffix_1));
            if (!msg.locator.empty()) { s.push_back(' '); s.append(msg.locator); }
            break;
        case MessageType::CQ_NONSTD_2:
            s.append("CQ");
            if (!msg.modifier.empty()) { s.push_back(' '); s.append(msg.modifier); }
            s.push_back(' '); s.append(format_callsign_with_suffix(msg.call_1, msg.suffix_1));
            break;
        case MessageType::CQ_NONSTD_3:
            s.append("CQ "); s.append(format_callsign_with_suffix(msg.call_1, msg.suffix_1));
            break;
        case MessageType::CALL_STD_NOSUF:
            s.append(format_callsign_with_suffix(msg.call_1, msg.suffix_1));
            s.push_back(' '); s.append(format_callsign_with_suffix(msg.call_2, msg.suffix_2));
            if (!msg.locator.empty()) { s.push_back(' '); s.append(msg.locator); }
            s.push_back(' '); s.append(format_rst(msg.rst_db));
            break;
        case MessageType::CALL_STD_SUF:
            if (!msg.call_1.empty()) {
                if (msg.call_1.front() == '<') {
                    s.append(msg.call_1);
                } else {
                    s.push_back('<'); s.append(msg.call_1); s.push_back('>');
                }
            } else if (msg.hash_1 != 0) {
                append_hex_tag(s, msg.hash_1, 6);
            } else {
                s.append("<...>");
            }
            s.push_back(' '); s.append(format_callsign_with_suffix(msg.call_2, msg.suffix_2));
            if (!msg.locator.empty()) { s.push_back(' '); s.append(msg.locator); }
            s.push_back(' '); s.append(format_rst(msg.rst_db));
            break;
        case MessageType::CALL_NONSTD:
            if (!msg.call_1.empty()) {
                if (msg.call_1.front() == '<') {
                    s.append(msg.call_1);
                } else {
                    s.push_back('<'); s.append(msg.call_1); s.push_back('>');
                }
            } else if (msg.hash_1 != 0) {
                append_hex_tag(s, msg.hash_1, 5);
            } else {
                s.append("<...>");
            }
            s.push_back(' ');
            s.append(format_callsign_with_suffix(msg.call_2, msg.suffix_2));
            s.push_back(' '); s.append(format_rst(msg.rst_db));
            break;
        case MessageType::REPORT73_STD:
            s.append(format_callsign_with_suffix(msg.call_1, msg.suffix_1));
            s.push_back(' ');
            s.append(format_callsign_with_suffix(msg.call_2, msg.suffix_2));
            s.append(" R");
            s.append(format_rst(msg.rst_db));
            break;
        case MessageType::M73_STD:
            s.append(format_callsign_with_suffix(msg.call_1, msg.suffix_1));
            s.push_back(' ');
            s.append(format_callsign_with_suffix(msg.call_2, msg.suffix_2));
            s.append(" 73");
            break;
        case MessageType::M73_NONSTD:
            if (!msg.call_1.empty()) {
                if (msg.call_1.front() == '<') {
                    s.append(msg.call_1);
                } else {
                    s.push_back('<'); s.append(msg.call_1); s.push_back('>');
                }
            } else if (msg.hash_1 != 0) {
                append_hex_tag(s, msg.hash_1, 6);
            } else {
                s.append("<...>");
            }
            s.push_back(' ');
            s.append(format_callsign_with_suffix(msg.call_2, msg.suffix_2));
            s.append(" 73");
            break;
        case MessageType::MULTI_REPORT73: {
            bool first = true;
            for (const auto& tgt : msg.multi_targets) {
                if (!first) s.push_back(' ');
                first = false;
                if (!tgt.call.empty()) {
                    if (tgt.call.front() == '<') {
                        s.append(tgt.call);
                    } else {
                        s.push_back('<'); s.append(tgt.call); s.push_back('>');
                    }
                } else if (tgt.hash != 0) {
                    append_hex_tag(s, tgt.hash, 6);
                } else {
                    s.append("<...>");
                }
                s.append(" R"); s.append(format_rst(tgt.rst_db));
            }
            if (!first) s.push_back(' ');
            if (!msg.call_1.empty()) {
                if (msg.call_1.front() == '<') {
                    s.append(msg.call_1);
                } else {
                    s.push_back('<'); s.append(msg.call_1); s.push_back('>');
                }
            } else if (msg.hash_1 != 0) {
                append_hex_tag(s, msg.hash_1, 4);
            } else {
                s.append("<...>");
            }
            break;
        }
        case MessageType::MULTI_73: {
            bool first = true;
            for (const auto& tgt : msg.multi_targets) {
                if (!first) s.push_back(' ');
                first = false;
                if (!tgt.call.empty()) {
                    if (tgt.call.front() == '<') {
                        s.append(tgt.call);
                    } else {
                        s.push_back('<'); s.append(tgt.call); s.push_back('>');
                    }
                } else if (tgt.hash != 0) {
                    append_hex_tag(s, tgt.hash, 6);
                } else {
                    s.append("<...>");
                }
            }
            if (!first) s.push_back(' ');
            if (!msg.call_1.empty()) {
                if (msg.call_1.front() == '<') {
                    s.append(msg.call_1);
                } else {
                    s.push_back('<'); s.append(msg.call_1); s.push_back('>');
                }
            } else if (msg.hash_1 != 0) {
                append_hex_tag(s, msg.hash_1, 4);
            } else {
                s.append("<...>");
            }
            s.append(" 73");
            break;
        }
        case MessageType::FREE_TEXT:
            s.append(msg.text);
            break;
        case MessageType::RESERVED_A:
        case MessageType::RESERVED_B:
        case MessageType::RESERVED_C:
            s.append("[RESERVED]");
            break;
        default:
            s.append("[UNKNOWN]");
            break;
    }
    return s;
}

std::string Message::to_string() const {
    return format_message(*this);
}

std::string payload_to_text(const uint8_t payload[PAYLOAD_BYTES],
                            const std::string_view* known_callsigns,
                            size_t count) {
    if (!payload) return "";
    Message msg;
    if (known_callsigns && count > 0) {
        if (!decode_message(payload, msg, known_callsigns, count)) return "";
    } else {
        if (!decode_message(payload, msg)) return "";
    }
    return format_message(msg);
}

namespace {

bool parse_hash_string(std::string_view token, uint32_t& out_hash, std::string* out_call = nullptr) {
    if (token.size() >= 3 && token.front() == '<' && token.back() == '>') {
        std::string_view inner = token.substr(1, token.size() - 2);
        if (inner == "...") {
            out_hash = 0;
            if (out_call) *out_call = "";
            return true;
        }
        std::string_view hex_str = inner;
        if (hex_str.size() >= 2 && (hex_str[0] == '0' && (hex_str[1] == 'x' || hex_str[1] == 'X'))) {
            hex_str.remove_prefix(2);
        }
        bool all_hex = !hex_str.empty() && hex_str.size() <= 8;
        uint32_t val = 0;
        if (all_hex) {
            for (char c : hex_str) {
                val <<= 4;
                if (c >= '0' && c <= '9') val |= static_cast<uint32_t>(c - '0');
                else if (c >= 'a' && c <= 'f') val |= static_cast<uint32_t>(10 + (c - 'a'));
                else if (c >= 'A' && c <= 'F') val |= static_cast<uint32_t>(10 + (c - 'A'));
                else {
                    all_hex = false;
                    break;
                }
            }
        }
        if (all_hex) {
            out_hash = val;
            if (out_call) *out_call = "";
            return true;
        }
        // It's a bracketed callsign like <HB9IPH>
        if (out_call) *out_call = std::string(inner);
        out_hash = hash_callsign_24(std::string(inner));
        return true;
    }
    return false;
}

} // anonymous namespace

bool parse_message(std::string_view text, Message& msg) {
    msg = Message{};
    if (text.empty()) return false;

    // Split text into whitespace-delimited tokens
    std::vector<std::string> tokens;
    std::string current;
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }

    if (tokens.empty()) return false;

    // Check for explicit 73 prefix (e.g. 73 TARGET CALLER, but not "73 DE CALL" which is FREE_TEXT)
    if ((tokens[0] == "73" || tokens[0] == "RR73" || tokens[0] == "RRR") && tokens.size() >= 3 && tokens.size() <= 4 && tokens[1] != "DE" && tokens[1] != "de") {
        if (tokens.size() == 3) {
            std::string target = tokens[1];
            std::string caller = tokens[2];
            uint32_t h1 = 0;
            std::string target_call;
            bool target_is_hash = parse_hash_string(target, h1, &target_call);

            std::string base_t, base_c;
            uint8_t suf_t = 0, suf_c = 0;
            bool target_is_std = parse_standard_callsign_suffix(target, base_t, suf_t);
            bool caller_is_std = parse_standard_callsign_suffix(caller, base_c, suf_c);

            if (target_is_std && caller_is_std && !target_is_hash) {
                msg.type = MessageType::M73_STD;
                msg.call_1 = format_callsign_with_suffix(base_t, suf_t);
                msg.suffix_1 = suf_t;
                msg.call_2 = format_callsign_with_suffix(base_c, suf_c);
                msg.suffix_2 = suf_c;
                return true;
            } else {
                // Check if caller can fit in 9c nonstd (Type 10)
                std::string base_caller;
                uint8_t caller_suf = 0;
                if (parse_any_callsign_suffix(caller, base_caller, caller_suf) && base_caller.size() <= 9) {
                    msg.type = MessageType::M73_NONSTD;
                    msg.call_1 = target_is_hash ? target_call : target;
                    msg.hash_1 = target_is_hash ? (h1 & 0xFFFFFFU) : hash_callsign_24(target);
                    msg.call_2 = format_callsign_with_suffix(base_caller, caller_suf);
                    msg.suffix_2 = caller_suf;
                    return true;
                } else {
                    // Fall back to MULTI_73 (Type 12)
                    msg.type = MessageType::MULTI_73;
                    msg.call_1 = caller;
                    msg.hash_1 = hash_callsign_16(caller);
                    MultiTarget tgt;
                    tgt.call = target_is_hash ? target_call : target;
                    tgt.hash = target_is_hash ? (h1 & 0xFFFFFFU) : hash_callsign_24(target);
                    msg.multi_targets.push_back(tgt);
                    return true;
                }
            }
        } else if (tokens.size() == 4) {
            // "73 <DX> <TGT1> <TGT2>"
            msg.type = MessageType::MULTI_73;
            std::string dx_tok = tokens[1];
            uint32_t dx_h = 0;
            std::string dx_call;
            if (parse_hash_string(dx_tok, dx_h, &dx_call)) {
                if (!dx_call.empty()) {
                    msg.call_1 = dx_call;
                    msg.hash_1 = hash_callsign_16(dx_call);
                } else {
                    msg.hash_1 = dx_h & 0xFFFFU;
                }
            } else {
                msg.call_1 = dx_tok;
                msg.hash_1 = hash_callsign_16(dx_tok);
            }
            for (size_t p = 2; p < 4; ++p) {
                std::string t_tok = tokens[p];
                MultiTarget target;
                target.rst_db = 0;
                uint32_t th = 0;
                std::string t_call;
                if (parse_hash_string(t_tok, th, &t_call)) {
                    target.hash = th & 0xFFFFFFU;
                    target.call = t_call;
                } else {
                    target.call = t_tok;
                    target.hash = hash_callsign_24(t_tok);
                }
                msg.multi_targets.push_back(target);
            }
            return true;
        }
    }

    // Check for explicit MULTI-73 / 73M prefix
    if ((tokens[0] == "MULTI-73" || tokens[0] == "MULTI_73" || tokens[0] == "M73" || tokens[0] == "73M" ||
         tokens[0] == "MULTI-RR73" || tokens[0] == "MULTI_RR73" || tokens[0] == "MRR73" || tokens[0] == "RR73M") &&
        (tokens.size() >= 3 && tokens.size() <= 4)) {
        msg.type = MessageType::MULTI_73;
        std::string dx_tok = tokens[1];
        uint32_t dx_h = 0;
        std::string dx_call;
        if (parse_hash_string(dx_tok, dx_h, &dx_call)) {
            msg.hash_1 = dx_h & 0xFFFFU;
            msg.call_1 = dx_call;
        } else {
            msg.call_1 = dx_tok;
            msg.hash_1 = hash_callsign_16(dx_tok);
        }

        for (size_t p = 2; p < tokens.size() && msg.multi_targets.size() < 2; ++p) {
            std::string t_tok = tokens[p];
            MultiTarget target;
            target.rst_db = 0;
            uint32_t th = 0;
            std::string t_call;
            if (parse_hash_string(t_tok, th, &t_call)) {
                target.hash = th & 0xFFFFFFU;
                target.call = t_call;
            } else {
                target.call = t_tok;
                target.hash = hash_callsign_24(t_tok);
            }
            msg.multi_targets.push_back(target);
        }
        return true;
    }

    // Check for CQ
    if (tokens[0] == "CQ") {
        if (tokens.size() == 2) {
            // CQ <call>
            std::string base_c;
            uint8_t suf = 0;
            if (parse_standard_callsign_suffix(tokens[1], base_c, suf)) {
                msg.type = MessageType::CQ_STD;
                msg.call_1 = format_callsign_with_suffix(base_c, suf);
                msg.suffix_1 = suf;
                msg.locator = "";
                return true;
            } else if (parse_any_callsign_suffix(tokens[1], base_c, suf)) {
                msg.type = MessageType::CQ_NONSTD_3;
                msg.call_1 = format_callsign_with_suffix(base_c, suf);
                msg.suffix_1 = suf;
                return true;
            }
        } else if (tokens.size() == 3) {
            // Either CQ <mod> <call> OR CQ <call> <loc>
            if (is_valid_locator(tokens[2])) {
                // CQ <call> <loc>
                std::string base_c;
                uint8_t suf = 0;
                if (parse_standard_callsign_suffix(tokens[1], base_c, suf)) {
                    msg.type = MessageType::CQ_STD;
                    msg.call_1 = format_callsign_with_suffix(base_c, suf);
                    msg.suffix_1 = suf;
                    msg.locator = tokens[2];
                    return true;
                } else if (parse_any_callsign_suffix(tokens[1], base_c, suf)) {
                    if (base_c.size() <= 9) {
                        msg.type = MessageType::CQ_NONSTD_1;
                        msg.call_1 = format_callsign_with_suffix(base_c, suf);
                        msg.suffix_1 = suf;
                        msg.locator = tokens[2];
                        return true;
                    } else if (base_c.size() <= 13) {
                        msg.type = MessageType::CQ_NONSTD_3;
                        msg.call_1 = format_callsign_with_suffix(base_c, suf);
                        msg.suffix_1 = suf;
                        return true;
                    }
                }
            } else {
                // CQ <mod> <call>
                std::string base_c;
                uint8_t suf = 0;
                if (parse_standard_callsign_suffix(tokens[2], base_c, suf)) {
                    msg.type = MessageType::CQ_STD;
                    msg.modifier = tokens[1];
                    msg.call_1 = format_callsign_with_suffix(base_c, suf);
                    msg.suffix_1 = suf;
                    return true;
                } else if (parse_any_callsign_suffix(tokens[2], base_c, suf)) {
                    if (base_c.size() <= 9) {
                        msg.type = MessageType::CQ_NONSTD_2;
                        msg.modifier = tokens[1];
                        msg.call_1 = format_callsign_with_suffix(base_c, suf);
                        msg.suffix_1 = suf;
                        return true;
                    } else if (base_c.size() <= 13) {
                        msg.type = MessageType::CQ_NONSTD_3;
                        msg.call_1 = format_callsign_with_suffix(base_c, suf);
                        msg.suffix_1 = suf;
                        return true;
                    }
                }
            }
        } else if (tokens.size() == 4) {
            // CQ <mod> <call> <loc>
            std::string base_c;
            uint8_t suf = 0;
            if (parse_standard_callsign_suffix(tokens[2], base_c, suf)) {
                msg.type = MessageType::CQ_STD;
                msg.modifier = tokens[1];
                msg.call_1 = format_callsign_with_suffix(base_c, suf);
                msg.suffix_1 = suf;
                msg.locator = tokens[3];
                return true;
            } else if (parse_any_callsign_suffix(tokens[2], base_c, suf) && base_c.size() <= 9) {
                msg.type = MessageType::CQ_NONSTD_1;
                msg.call_1 = format_callsign_with_suffix(base_c, suf);
                msg.suffix_1 = suf;
                msg.locator = tokens[3];
                return true;
            }
        }
    }

    // Check for explicit CALL prefix
    if (tokens[0] == "CALL" && tokens.size() >= 3) {
        std::string target = tokens[1];
        std::string caller = tokens[2];
        std::string loc;
        std::string rst_str;

        if (tokens.size() == 4) {
            int test_rst = 0;
            if (parse_rst(tokens[3], test_rst)) {
                rst_str = tokens[3];
            } else if (is_valid_locator(tokens[3])) {
                loc = tokens[3];
            }
        } else if (tokens.size() >= 5) {
            loc = tokens[3];
            rst_str = tokens[4];
        }

        int rst_val = 0;
        parse_rst(rst_str, rst_val);

        uint32_t h1 = 0;
        std::string target_call;
        bool target_is_hash = parse_hash_string(target, h1, &target_call);

        uint32_t h2 = 0;
        std::string caller_call;
        bool caller_is_hash = parse_hash_string(caller, h2, &caller_call);
        std::string clean_caller = caller_is_hash ? (caller_call.empty() ? caller : caller_call) : caller;

        std::string base_t, base_c;
        uint8_t suf_t = 0, suf_c = 0;
        bool target_is_std = parse_standard_callsign_suffix(target, base_t, suf_t);
        bool caller_is_std = parse_standard_callsign_suffix(clean_caller, base_c, suf_c);

        if (caller_is_std) {
            if (target_is_std && !target_is_hash && suf_t == 0 && suf_c == 0 && !loc.empty()) {
                // Type 5 (CALL std nosuf)
                msg.type = MessageType::CALL_STD_NOSUF;
                msg.call_1 = base_t;
                msg.call_2 = base_c;
                msg.locator = loc;
                msg.rst_db = rst_val;
                return true;
            } else {
                // Type 6 (CALL std+suf): target hash, caller callsign + 1b suf + locator + SNR
                msg.type = MessageType::CALL_STD_SUF;
                msg.call_1 = target_is_hash ? target_call : target;
                msg.hash_1 = target_is_hash ? (h1 & 0xFFFFFFU) : hash_callsign_24(target);
                msg.call_2 = format_callsign_with_suffix(base_c, suf_c);
                msg.suffix_2 = suf_c;
                msg.locator = loc;
                msg.rst_db = rst_val;
                return true;
            }
        } else {
            // Type 7 (CALL non-std)
            msg.type = MessageType::CALL_NONSTD;
            msg.call_1 = target_is_hash ? target_call : target;
            if (target_is_hash) {
                msg.hash_1 = (h1 > 0xFFFFFU) ? ((h1 >> 4) & 0xFFFFFU) : (h1 & 0xFFFFFU);
            } else {
                msg.hash_1 = hash_callsign_20(target);
            }
            msg.call_2 = clean_caller;
            msg.suffix_2 = suf_c;
            msg.hash_2 = 0;
            msg.locator = "";
            msg.rst_db = rst_val;
            return true;
        }
    }

    // Check for explicit REPORT+73 / RPT73 / REPLY73 prefix
    if ((tokens[0] == "REPORT+73" || tokens[0] == "REPORT73" || tokens[0] == "RPT73" || tokens[0] == "REPORT" ||
         tokens[0] == "REPLY73" || tokens[0] == "REPLY") &&
        (tokens.size() == 4 || tokens.size() == 3)) {
        std::string target = tokens[1];
        std::string caller = (tokens.size() == 4) ? tokens[2] : "";
        std::string rst_str = (tokens.size() == 4) ? tokens[3] : tokens[2];
        int rst_val = 0;
        parse_rst(rst_str, rst_val);

        uint32_t h1 = 0;
        std::string target_call;
        bool target_is_hash = parse_hash_string(target, h1, &target_call);

        std::string base_t, base_c;
        uint8_t suf_t = 0, suf_c = 0;
        bool target_is_std = parse_standard_callsign_suffix(target, base_t, suf_t);
        bool caller_is_std = parse_standard_callsign_suffix(caller, base_c, suf_c);

        if (target_is_std && caller_is_std && !target_is_hash) {
            msg.type = MessageType::REPORT73_STD;
            msg.call_1 = format_callsign_with_suffix(base_t, suf_t);
            msg.suffix_1 = suf_t;
            msg.call_2 = format_callsign_with_suffix(base_c, suf_c);
            msg.suffix_2 = suf_c;
            msg.rst_db = rst_val;
            return true;
        } else {
            msg.type = MessageType::MULTI_REPORT73;
            msg.call_1 = caller;
            msg.hash_1 = hash_callsign_16(caller);
            MultiTarget tgt;
            tgt.call = target_is_hash ? target_call : target;
            tgt.hash = target_is_hash ? (h1 & 0xFFFFFFU) : hash_callsign_24(target);
            tgt.rst_db = rst_val;
            msg.multi_targets.push_back(tgt);
            return true;
        }
    }

    // Check for explicit MULTI-REPORT+73 / MRPT73 / RPT73M / MULTI-REPLY73 prefix (or RPT73 with >= 6 tokens)
    if ((tokens[0] == "MULTI-REPORT+73" || tokens[0] == "MULTI_REPORT73" || tokens[0] == "MRPT73" ||
         tokens[0] == "RPT73M" || tokens[0] == "MULTI-REPORT" || tokens[0] == "MULTI-REPLY73" || tokens[0] == "MULTI-REPLY" ||
         tokens[0] == "MULTI_REPLY73" || tokens[0] == "MREPLY73" ||
         ((tokens[0] == "RPT73" || tokens[0] == "REPORT+73") && tokens.size() >= 6)) &&
        (tokens.size() >= 4 && tokens.size() <= 6 && (tokens.size() % 2 == 0))) {
        msg.type = MessageType::MULTI_REPORT73;
        std::string dx_tok = tokens[1];
        uint32_t dx_h = 0;
        std::string dx_call;
        if (parse_hash_string(dx_tok, dx_h, &dx_call)) {
            if (!dx_call.empty()) {
                msg.call_1 = dx_call;
                msg.hash_1 = hash_callsign_16(dx_call);
            } else {
                msg.hash_1 = dx_h & 0xFFFFU;
            }
        } else {
            msg.call_1 = dx_tok;
            msg.hash_1 = hash_callsign_16(dx_tok);
        }

        size_t num_pairs = (tokens.size() - 2) / 2;
        bool all_valid = true;
        for (size_t p = 0; p < num_pairs && p < 2; ++p) {
            std::string t_tok = tokens[2 + p * 2];
            std::string r_tok = tokens[2 + p * 2 + 1];
            int rst_val = 0;
            if (!parse_rst(r_tok, rst_val)) {
                all_valid = false;
                break;
            }

            MultiTarget target;
            target.rst_db = rst_val;
            uint32_t th = 0;
            std::string t_call;
            if (parse_hash_string(t_tok, th, &t_call)) {
                target.hash = th & 0xFFFFFFU;
                target.call = t_call;
            } else {
                target.call = t_tok;
                target.hash = hash_callsign_24(t_tok);
            }
            msg.multi_targets.push_back(target);
        }
        if (all_valid && !msg.multi_targets.empty()) {
            return true;
        }
        msg = Message{};
    }

    // Multi-station interleaved report parsing: <TARGET1> R+05 <TARGET2> R-03 <CALLER>
    if (tokens.size() == 5) {
        int rst1 = 0, rst2 = 0;
        if (parse_rst(tokens[1], rst1) && parse_rst(tokens[3], rst2)) {
            uint32_t th1 = 0; std::string tc1;
            bool t1_is_hash = parse_hash_string(tokens[0], th1, &tc1);
            MultiTarget tgt1;
            tgt1.rst_db = rst1;
            tgt1.hash = t1_is_hash ? (th1 & 0xFFFFFFU) : hash_callsign_24(tokens[0]);
            tgt1.call = t1_is_hash ? tc1 : tokens[0];

            uint32_t th2 = 0; std::string tc2;
            bool t2_is_hash = parse_hash_string(tokens[2], th2, &tc2);
            MultiTarget tgt2;
            tgt2.rst_db = rst2;
            tgt2.hash = t2_is_hash ? (th2 & 0xFFFFFFU) : hash_callsign_24(tokens[2]);
            tgt2.call = t2_is_hash ? tc2 : tokens[2];

            uint32_t dx_h = 0; std::string dx_call;
            bool dx_is_hash = parse_hash_string(tokens[4], dx_h, &dx_call);
            msg.type = MessageType::MULTI_REPORT73;
            if (dx_is_hash) {
                msg.call_1 = dx_call;
                msg.hash_1 = dx_call.empty() ? (dx_h & 0xFFFFU) : hash_callsign_16(dx_call);
            } else {
                msg.call_1 = tokens[4];
                msg.hash_1 = hash_callsign_16(tokens[4]);
            }
            msg.multi_targets.push_back(tgt1);
            msg.multi_targets.push_back(tgt2);
            return true;
        }
    }

    // Multi-73 parsing: <TARGET1> <TARGET2> <CALLER> 73
    if (tokens.size() == 4 && (tokens[3] == "73" || tokens[3] == "RR73" || tokens[3] == "RRR")) {
        uint32_t th1 = 0; std::string tc1;
        bool t1_is_hash = parse_hash_string(tokens[0], th1, &tc1);
        MultiTarget tgt1;
        tgt1.rst_db = 0;
        tgt1.hash = t1_is_hash ? (th1 & 0xFFFFFFU) : hash_callsign_24(tokens[0]);
        tgt1.call = t1_is_hash ? tc1 : tokens[0];

        uint32_t th2 = 0; std::string tc2;
        bool t2_is_hash = parse_hash_string(tokens[1], th2, &tc2);
        MultiTarget tgt2;
        tgt2.rst_db = 0;
        tgt2.hash = t2_is_hash ? (th2 & 0xFFFFFFU) : hash_callsign_24(tokens[1]);
        tgt2.call = t2_is_hash ? tc2 : tokens[1];

        uint32_t dx_h = 0; std::string dx_call;
        bool dx_is_hash = parse_hash_string(tokens[2], dx_h, &dx_call);
        msg.type = MessageType::MULTI_73;
        if (dx_is_hash) {
            msg.call_1 = dx_call;
            msg.hash_1 = dx_call.empty() ? (dx_h & 0xFFFFU) : hash_callsign_16(dx_call);
        } else {
            msg.call_1 = tokens[2];
            msg.hash_1 = hash_callsign_16(tokens[2]);
        }
        msg.multi_targets.push_back(tgt1);
        msg.multi_targets.push_back(tgt2);
        return true;
    }

    // Compact standard QSO parsing (without explicit CALL/REPLY prefix)
    if (tokens.size() == 4 && is_valid_locator(tokens[2])) {
        // e.g. "YO1YO TU2TU KL22 -03" -> CALL
        int rst_val = 0;
        if (parse_rst(tokens[3], rst_val)) {
            std::string target = tokens[0];
            std::string caller = tokens[1];
            std::string loc = tokens[2];

            uint32_t h1 = 0;
            std::string target_call;
            bool target_is_hash = parse_hash_string(target, h1, &target_call);

            uint32_t h2 = 0;
            std::string caller_call;
            bool caller_is_hash = parse_hash_string(caller, h2, &caller_call);
            std::string clean_caller = caller_is_hash ? (caller_call.empty() ? caller : caller_call) : caller;

            std::string base_t, base_c;
            uint8_t suf_t = 0, suf_c = 0;
            bool target_is_std = parse_standard_callsign_suffix(target, base_t, suf_t);
            bool caller_is_std = parse_standard_callsign_suffix(clean_caller, base_c, suf_c);

            if (caller_is_std) {
                if (target_is_std && !target_is_hash && suf_t == 0 && suf_c == 0) {
                    msg.type = MessageType::CALL_STD_NOSUF;
                    msg.call_1 = base_t;
                    msg.call_2 = base_c;
                    msg.locator = loc;
                    msg.rst_db = rst_val;
                    return true;
                } else {
                    msg.type = MessageType::CALL_STD_SUF;
                    msg.call_1 = target_is_hash ? target_call : target;
                    msg.hash_1 = target_is_hash ? (h1 & 0xFFFFFFU) : hash_callsign_24(target);
                    msg.call_2 = format_callsign_with_suffix(base_c, suf_c);
                    msg.suffix_2 = suf_c;
                    msg.locator = loc;
                    msg.rst_db = rst_val;
                    return true;
                }
            } else {
                msg.type = MessageType::CALL_NONSTD;
                msg.call_1 = target_is_hash ? target_call : target;
                if (target_is_hash) {
                    msg.hash_1 = (h1 > 0xFFFFFU) ? ((h1 >> 4) & 0xFFFFFU) : (h1 & 0xFFFFFU);
                } else {
                    msg.hash_1 = hash_callsign_20(target);
                }
                msg.call_2 = clean_caller;
                msg.suffix_2 = suf_c;
                msg.hash_2 = 0;
                msg.locator = "";
                msg.rst_db = rst_val;
                return true;
            }
        }
    }

    if (tokens.size() == 3) {
        // Check for single-target multi-report: <TARGET1> R+05 <CALLER>
        bool has_r = (!tokens[1].empty() && (tokens[1][0] == 'R' || tokens[1][0] == 'r'));
        int mid_rst = 0;
        if (has_r && parse_rst(tokens[1], mid_rst)) {
            uint32_t th1 = 0; std::string tc1;
            bool t1_is_hash = parse_hash_string(tokens[0], th1, &tc1);
            MultiTarget tgt1;
            tgt1.rst_db = mid_rst;
            tgt1.hash = t1_is_hash ? (th1 & 0xFFFFFFU) : hash_callsign_24(tokens[0]);
            tgt1.call = t1_is_hash ? tc1 : tokens[0];

            uint32_t dx_h = 0; std::string dx_call;
            bool dx_is_hash = parse_hash_string(tokens[2], dx_h, &dx_call);
            msg.type = MessageType::MULTI_REPORT73;
            if (dx_is_hash) {
                msg.call_1 = dx_call;
                msg.hash_1 = dx_call.empty() ? (dx_h & 0xFFFFU) : hash_callsign_16(dx_call);
            } else {
                msg.call_1 = tokens[2];
                msg.hash_1 = hash_callsign_16(tokens[2]);
            }
            msg.multi_targets.push_back(tgt1);
            return true;
        }

        // Could be e.g. "YO1YO TU2TU R+05" (REPORT+73), "YO1YO TU2TU 73" (73 std), or "YO1YO TU2TU JN47" (CALL)
        std::string target = tokens[0];
        std::string caller = tokens[1];
        std::string token3 = tokens[2];
        uint32_t h1 = 0;
        std::string target_call;
        bool target_is_hash = parse_hash_string(target, h1, &target_call);

        std::string base_t, base_c;
        uint8_t suf_t = 0, suf_c = 0;
        bool target_is_std = parse_standard_callsign_suffix(target, base_t, suf_t);
        bool caller_is_std = parse_standard_callsign_suffix(caller, base_c, suf_c);

        bool target_valid = target_is_std || is_valid_nonstd_callsign(target, 13) || target_is_hash;
        bool caller_valid = caller_is_std || is_valid_nonstd_callsign(caller, 13);

        if (target_valid && caller_valid) {
            bool is_73 = (token3 == "73" || token3 == "RR73" || token3 == "RRR");
            int rst_val = 0;
            bool is_rst = parse_rst(token3, rst_val);
            bool is_loc = (!is_73 && is_valid_locator(token3));

            if (is_73) {
                // Formatted as 73 std, 73 non-std, or MULTI_73
                if (target_is_std && caller_is_std && !target_is_hash) {
                    msg.type = MessageType::M73_STD;
                    msg.call_1 = format_callsign_with_suffix(base_t, suf_t);
                    msg.suffix_1 = suf_t;
                    msg.call_2 = format_callsign_with_suffix(base_c, suf_c);
                    msg.suffix_2 = suf_c;
                    return true;
                } else {
                    std::string base_caller;
                    uint8_t caller_suf = 0;
                    if (parse_any_callsign_suffix(caller, base_caller, caller_suf) && base_caller.size() <= 9) {
                        msg.type = MessageType::M73_NONSTD;
                        msg.call_1 = target_is_hash ? target_call : target;
                        msg.hash_1 = target_is_hash ? (h1 & 0xFFFFFFU) : hash_callsign_24(target);
                        msg.call_2 = format_callsign_with_suffix(base_caller, caller_suf);
                        msg.suffix_2 = caller_suf;
                        return true;
                    } else {
                        msg.type = MessageType::MULTI_73;
                        msg.call_1 = caller;
                        msg.hash_1 = hash_callsign_16(caller);
                        MultiTarget tgt;
                        tgt.call = target_is_hash ? target_call : target;
                        tgt.hash = target_is_hash ? (h1 & 0xFFFFFFU) : hash_callsign_24(target);
                        msg.multi_targets.push_back(tgt);
                        return true;
                    }
                }
            } else if (is_loc) {
                // Treated as CALL with locator
                if (target_is_std && caller_is_std && !target_is_hash && suf_t == 0 && suf_c == 0) {
                    msg.type = MessageType::CALL_STD_NOSUF;
                    msg.call_1 = base_t;
                    msg.call_2 = base_c;
                    msg.locator = token3;
                    msg.rst_db = 0;
                    return true;
                } else if (caller_is_std) {
                    msg.type = MessageType::CALL_STD_SUF;
                    msg.call_1 = target_is_hash ? target_call : target;
                    msg.hash_1 = target_is_hash ? (h1 & 0xFFFFFFU) : hash_callsign_24(target);
                    msg.call_2 = format_callsign_with_suffix(base_c, suf_c);
                    msg.suffix_2 = suf_c;
                    msg.locator = token3;
                    msg.rst_db = 0;
                    return true;
                } else {
                    msg.type = MessageType::CALL_NONSTD;
                    msg.call_1 = target_is_hash ? target_call : target;
                    if (target_is_hash) {
                        msg.hash_1 = (h1 > 0xFFFFFU) ? ((h1 >> 4) & 0xFFFFFU) : (h1 & 0xFFFFFU);
                    } else {
                        msg.hash_1 = hash_callsign_20(target);
                    }
                    std::string base_caller; uint8_t suf_caller = 0;
                    if (parse_any_callsign_suffix(caller, base_caller, suf_caller)) {
                        msg.call_2 = format_callsign_with_suffix(base_caller, suf_caller);
                        msg.suffix_2 = suf_caller;
                    } else {
                        msg.call_2 = caller;
                        msg.suffix_2 = 0;
                    }
                    msg.hash_2 = 0;
                    msg.locator = "";
                    msg.rst_db = 0;
                    return true;
                }
            } else if (is_rst) {
                bool has_r_prefix = (!token3.empty() && (token3[0] == 'R' || token3[0] == 'r'));
                // If caller is non-standard and NO 'R' prefix, it is CALL_NONSTD (Type 7)
                if (!caller_is_std && !has_r_prefix) {
                    msg.type = MessageType::CALL_NONSTD;
                    msg.call_1 = target_is_hash ? target_call : target;
                    if (target_is_hash) {
                        msg.hash_1 = (h1 > 0xFFFFFU) ? ((h1 >> 4) & 0xFFFFFU) : (h1 & 0xFFFFFU);
                    } else {
                        msg.hash_1 = hash_callsign_20(target);
                    }
                    std::string base_caller; uint8_t suf_caller = 0;
                    if (parse_any_callsign_suffix(caller, base_caller, suf_caller)) {
                        msg.call_2 = format_callsign_with_suffix(base_caller, suf_caller);
                        msg.suffix_2 = suf_caller;
                    } else {
                        msg.call_2 = caller;
                        msg.suffix_2 = 0;
                    }
                    msg.hash_2 = 0;
                    msg.locator = "";
                    msg.rst_db = rst_val;
                    return true;
                }
                // Treated as REPORT+73
                if (target_is_std && caller_is_std && !target_is_hash) {
                    msg.type = MessageType::REPORT73_STD;
                    msg.call_1 = format_callsign_with_suffix(base_t, suf_t);
                    msg.suffix_1 = suf_t;
                    msg.call_2 = format_callsign_with_suffix(base_c, suf_c);
                    msg.suffix_2 = suf_c;
                    msg.rst_db = rst_val;
                    return true;
                } else {
                    msg.type = MessageType::MULTI_REPORT73;
                    msg.call_1 = caller;
                    msg.hash_1 = hash_callsign_16(caller);
                    MultiTarget tgt;
                    tgt.call = target_is_hash ? target_call : target;
                    tgt.hash = target_is_hash ? (h1 & 0xFFFFFFU) : hash_callsign_24(target);
                    tgt.rst_db = rst_val;
                    msg.multi_targets.push_back(tgt);
                    return true;
                }
            }
        }
    }

    // Otherwise, treat as FREE_TEXT
    msg.type = MessageType::FREE_TEXT;
    msg.text = std::string(text);
    return true;
}

std::string payload_to_hex(const uint8_t payload[PAYLOAD_BYTES], bool space_separated) {
    static const char hex_digits[] = "0123456789ABCDEF";
    std::string s;
    s.reserve(space_separated ? (PAYLOAD_BYTES * 3 - 1) : (PAYLOAD_BYTES * 2));
    for (size_t i = 0; i < PAYLOAD_BYTES; ++i) {
        if (space_separated && i > 0) s.push_back(' ');
        s.push_back(hex_digits[(payload[i] >> 4) & 0x0F]);
        s.push_back(hex_digits[payload[i] & 0x0F]);
    }
    return s;
}

std::string payload_to_binary(const uint8_t payload[PAYLOAD_BYTES], int total_bits) {
    std::string s;
    s.reserve(total_bits);
    for (int i = 0; i < total_bits; ++i) {
        size_t byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        if (byte_idx < PAYLOAD_BYTES) {
            uint8_t bit = (payload[byte_idx] >> bit_idx) & 1U;
            s.push_back(bit ? '1' : '0');
        } else {
            s.push_back('0');
        }
    }
    return s;
}

bool hex_to_payload(std::string_view hex_str, uint8_t payload[PAYLOAD_BYTES]) {
    std::memset(payload, 0, PAYLOAD_BYTES);
    std::string clean;
    clean.reserve(hex_str.size());
    for (size_t i = 0; i < hex_str.size(); ++i) {
        char c = hex_str[i];
        if (std::isspace(static_cast<unsigned char>(c)) || c == ':' || c == '-') continue;
        if (c == '0' && i + 1 < hex_str.size() && (hex_str[i+1] == 'x' || hex_str[i+1] == 'X')) {
            ++i; // skip 'x'
            continue;
        }
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            clean.push_back(c);
        }
    }
    if (clean.size() < 20) return false;
    for (size_t i = 0; i < PAYLOAD_BYTES; ++i) {
        auto hex_val = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            return 10 + (c - 'A');
        };
        int high = hex_val(clean[i * 2]);
        int low = hex_val(clean[i * 2 + 1]);
        payload[i] = static_cast<uint8_t>((high << 4) | low);
    }
    return true;
}

bool binary_to_payload(std::string_view bin_str, uint8_t payload[PAYLOAD_BYTES]) {
    std::memset(payload, 0, PAYLOAD_BYTES);
    std::string clean;
    clean.reserve(bin_str.size());
    for (char c : bin_str) {
        if (c == '0' || c == '1') {
            clean.push_back(c);
        }
    }
    if (clean.empty()) return false;
    for (size_t i = 0; i < clean.size() && i < 77; ++i) {
        if (clean[i] == '1') {
            size_t byte_idx = i / 8;
            int bit_idx = 7 - (i % 8);
            payload[byte_idx] |= static_cast<uint8_t>(1 << bit_idx);
        }
    }
    return true;
}

std::string hex_to_text(std::string_view hex_str,
                        const std::string_view* known_callsigns,
                        size_t count) {
    uint8_t payload[PAYLOAD_BYTES] = {0};
    if (!hex_to_payload(hex_str, payload)) return "";
    Message msg;
    if (known_callsigns && count > 0) {
        if (!decode_message(payload, msg, known_callsigns, count)) return "";
    } else {
        if (!decode_message(payload, msg)) return "";
    }
    return format_message(msg);
}

std::string binary_to_text(std::string_view bin_str,
                          const std::string_view* known_callsigns,
                          size_t count) {
    uint8_t payload[PAYLOAD_BYTES] = {0};
    if (!binary_to_payload(bin_str, payload)) return "";
    Message msg;
    if (known_callsigns && count > 0) {
        if (!decode_message(payload, msg, known_callsigns, count)) return "";
    } else {
        if (!decode_message(payload, msg)) return "";
    }
    return format_message(msg);
}

std::string text_to_hex(std::string_view text, bool space_separated) {
    Message msg;
    if (!parse_message(text, msg)) return "";
    uint8_t payload[PAYLOAD_BYTES] = {0};
    if (!encode_message(msg, payload)) return "";
    return payload_to_hex(payload, space_separated);
}

std::string text_to_binary(std::string_view text, int total_bits) {
    Message msg;
    if (!parse_message(text, msg)) return "";
    uint8_t payload[PAYLOAD_BYTES] = {0};
    if (!encode_message(msg, payload)) return "";
    return payload_to_binary(payload, total_bits);
}

} // namespace lq
