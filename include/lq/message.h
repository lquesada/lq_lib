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

#ifndef LQ_MESSAGE_H
#define LQ_MESSAGE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include "lq/types.h"

namespace lq {

/**
 * Target entry for MULTI_REPLY73 messages.
 */
struct MultiTarget {
    std::string call;      ///< Target callsign (if known/resolved)
    uint32_t hash = 0;     ///< 24-bit callsign hash (0..16,777,215)
    int rst_db = 0;        ///< Signal report in dB (-26..+5)

    bool operator==(const MultiTarget& o) const {
        return hash == o.hash && rst_db == o.rst_db && call == o.call;
    }
};

/**
 * High-level structure representing any decoded LQ protocol message.
 */
struct Message {
    MessageType type = MessageType::UNKNOWN;

    // Field storage
    std::string call_1;    ///< Target / destination callsign, or station calling CQ (or DX station in MULTI_REPLY73)
    std::string call_2;    ///< Caller / transmitting source station callsign
    uint8_t suffix_1 = 0;  ///< Target / destination suffix (0 = None, 1 = /P)
    uint8_t suffix_2 = 0;  ///< Caller / source suffix (0 = None, 1 = /P)
    uint32_t hash_1 = 0;   ///< Target 24-bit hash (or DX 16-bit hash in MULTI_REPORT73)
    uint32_t hash_2 = 0;   ///< Caller / source 24-bit hash (for non-standard callsign messages)
    std::string modifier;  ///< CQ modifier (e.g. "DX", "POTA")
    std::string locator;   ///< 4-character Maidenhead locator
    int rst_db = 0;        ///< Signal report in dB (-26..+5)
    std::string text;      ///< Free-text message
    std::vector<uint8_t> raw_payload; ///< For reserved message types
    std::vector<MultiTarget> multi_targets; ///< Up to 2 targets for MULTI_REPLY73

    // Semantic QSO accessors
    std::string get_sender_call() const;
    std::string get_target_call() const;
    uint32_t get_sender_hash() const;
    uint32_t get_target_hash() const;

    bool is_cq() const;
    bool is_call() const;
    bool is_report73() const;
    bool is_reply73() const { return is_report73(); }
    bool is_73() const;
    bool is_multi_report73() const;
    bool is_multi_reply73() const { return is_multi_report73(); }
    bool is_multi_73() const;
    bool is_free_text() const;

    /// Return the deterministic human-readable string representation of this message.

    std::string to_string() const;

    bool operator==(const Message& o) const {
        return type == o.type &&
               call_1 == o.call_1 &&
               call_2 == o.call_2 &&
               suffix_1 == o.suffix_1 &&
               suffix_2 == o.suffix_2 &&
               hash_1 == o.hash_1 &&
               hash_2 == o.hash_2 &&
               modifier == o.modifier &&
               locator == o.locator &&
               rst_db == o.rst_db &&
               text == o.text &&
               raw_payload == o.raw_payload &&
               multi_targets == o.multi_targets;
    }
};

/**
 * Encode a structured Message into a 77-bit payload byte array (10 bytes).
 * Remaining unused bits within the 77-bit budget are zero-padded.
 * Returns true on success, false on invalid fields.
 */
bool encode_message(const Message& msg, uint8_t payload[PAYLOAD_BYTES]);

/**
 * Decode a 77-bit payload byte array (10 bytes) into a structured Message.
 * Returns true on success, false on decode failure.
 */
bool decode_message(const uint8_t payload[PAYLOAD_BYTES], Message& msg);

/**
 * Resolve any 24-bit or 14-bit callsign hashes in the message against an array/list of known callsigns.
 * If a matching hash is found, the corresponding call string (call_1, call_2, or multi_targets[i].call)
 * is populated with the matching callsign.
 * 
 * Operates without internal heap allocations.
 * Returns true if at least one hash was successfully resolved.
 */
bool resolve_callsigns(Message& msg, const std::string_view* known_callsigns, size_t count);

/**
 * Decode a 77-bit payload byte array (10 bytes) into a structured Message, automatically resolving
 * any 24-bit and 16-bit hashes against the supplied list of known callsigns.
 */
bool decode_message(const uint8_t payload[PAYLOAD_BYTES], Message& msg, const std::string_view* known_callsigns, size_t count);

/**
 * Format a Message as a standard human-readable amateur radio text line unequivocally and deterministically.
 * Examples:
 *   - "CQ YO1YO JN47"
 *   - "CQ DX W1AW FN31"
 *   - "YO1YO TU2TU KL22 -03"
 *   - "YO1YO TU2TU R+05"
 *   - "<012345> TU2TU KL22 -03"
 *   - "73 DE W1AW TNX"
 */
std::string format_message(const Message& msg);

/**
 * Convert a 10-byte payload directly to a human-readable text string unequivocally and deterministically.
 * Optionally resolves callsign hashes against known_callsigns.
 * Returns empty string on decode failure.
 */
std::string payload_to_text(const uint8_t payload[PAYLOAD_BYTES],
                            const std::string_view* known_callsigns = nullptr,
                            size_t count = 0);

/**
 * Parse a human-readable text line into a structured Message.
 * Automatically deduces the appropriate MessageType (CQ std, CALL std, REPLY73 std, etc.).
 * Supports both explicit ("CALL YO1YO TU2TU JN47 -03") and compact ("YO1YO TU2TU JN47 -03") formats.
 * Returns true on success.
 */
bool parse_message(std::string_view text, Message& msg);

// ============================================================================
// Bidirectional Hex, Binary, Text & Payload Conversion Utilities
// ============================================================================

/**
 * Convert 10-byte payload into a hex string (e.g. "F8 87 79 0B DF 05 67 44 8B B8").
 */
std::string payload_to_hex(const uint8_t payload[PAYLOAD_BYTES], bool space_separated = true);

/**
 * Convert 10-byte payload into a 77-bit binary string (e.g. "11111000...").
 */
std::string payload_to_binary(const uint8_t payload[PAYLOAD_BYTES], int total_bits = 77);

/**
 * Parse a hex string (with or without spaces/0x prefixes) into a 10-byte payload.
 * Returns true on success.
 */
bool hex_to_payload(std::string_view hex_str, uint8_t payload[PAYLOAD_BYTES]);

/**
 * Parse a binary string ('0' and '1' characters, ignoring whitespace) into a 10-byte payload.
 * Returns true on success.
 */
bool binary_to_payload(std::string_view bin_str, uint8_t payload[PAYLOAD_BYTES]);

/**
 * Direct conversion from hex string to human-readable standard text line.
 * Optionally resolves 24-bit and 16-bit hashes using known_callsigns.
 */
std::string hex_to_text(std::string_view hex_str,
                        const std::string_view* known_callsigns = nullptr,
                        size_t count = 0);

/**
 * Direct conversion from 77-bit binary string to human-readable standard text line.
 * Optionally resolves 24-bit and 16-bit hashes using known_callsigns.
 */
std::string binary_to_text(std::string_view bin_str,
                          const std::string_view* known_callsigns = nullptr,
                          size_t count = 0);

/**
 * Direct conversion from human-readable text line to 10-byte hex string.
 */
std::string text_to_hex(std::string_view text, bool space_separated = true);

/**
 * Direct conversion from human-readable text line to 77-bit binary string.
 */
std::string text_to_binary(std::string_view text, int total_bits = 77);

} // namespace lq

#endif // LQ_MESSAGE_H

