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

/**
 * =============================================================================
 * LQ Library Developer Tutorial & Usage Guide
 * =============================================================================
 *
 * This tutorial demonstrates how to use the reference implementation (lq_lib)
 * to construct, encode, serialize, modulate, synthesize, demodulate, and decode
 * messages across all layers of the LQ Digital Mode Family.
 *
 * Topics Covered:
 *   1. Constructing structured Messages (CQ, CALL, REPLY73, FREE_TEXT, etc.)
 *   2. Encoding Messages to 77-bit Binary & Hexadecimal Payloads
 *   3. Applying CRC-14 Parity & Systematic LDPC(174,91) Forward Error Correction
 *   4. Channel Modulation & Tone Sequencing across physical modes (LQ8, LQ4, LQ2, LQ16)
 *   5. Continuous-Phase GFSK Audio Synthesis
 *   6. Decoding from Raw Hex / Binary Payloads into structured C++ objects
 *   7. Demodulating & Decoding Tone Sequences & Audio Waveforms
 *   8. Using the High-Level Fluent API (lq::easy & lq::Transceiver)
 * =============================================================================
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

#include "lq/lq.h"

// -----------------------------------------------------------------------------
// Terminal Formatting & ANSI Helpers
// -----------------------------------------------------------------------------
namespace {
    const std::string RESET   = "\033[0m";
    const std::string BOLD    = "\033[1m";
    const std::string CYAN    = "\033[38;2;0;229;255m";
    const std::string GREEN   = "\033[38;2;0;230;118m";
    const std::string YELLOW  = "\033[38;2;255;234;0m";
    const std::string ORANGE  = "\033[38;2;255;109;0m";
    const std::string RED     = "\033[38;2;255;23;68m";
    const std::string WHITE   = "\033[38;2;255;255;255m";
    const std::string DIM     = "\033[38;2;140;160;180m";
    const std::string BG_RED  = "\033[48;2;180;0;0m\033[38;2;255;255;255m";

    void fatal_error(const std::string& msg) {
        std::cerr << "\n" << BG_RED << " " << BOLD << "FATAL ERROR: " << msg << " " << RESET << "\n\n";
        std::exit(1);
    }

    void print_header(const std::string& title) {
        std::cout << "\n" << CYAN << BOLD << "╔════════════════════════════════════════════════════════════════════════════════════════╗" << RESET << "\n";
        std::cout << CYAN << BOLD << "║ " << WHITE << std::left << std::setw(86) << title << CYAN << " ║" << RESET << "\n";
        std::cout << CYAN << BOLD << "╚════════════════════════════════════════════════════════════════════════════════════════╝" << RESET << "\n\n";
    }

    void print_step(int step_num, const std::string& title) {
        std::cout << YELLOW << BOLD << "▶ Step " << step_num << ": " << WHITE << title << RESET << "\n";
        std::cout << DIM << "────────────────────────────────────────────────────────────────────────────────────────" << RESET << "\n";
    }

    std::string bytes_to_hex(const uint8_t* data, size_t len) {
        std::ostringstream oss;
        for (size_t i = 0; i < len; ++i) {
            oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
            if (i + 1 < len) oss << " ";
        }
        return oss.str();
    }

    std::string bytes_to_bits(const uint8_t* data, size_t bit_count) {
        std::string bits;
        bits.reserve(bit_count + bit_count / 8);
        for (size_t i = 0; i < bit_count; ++i) {
            size_t byte_idx = i / 8;
            size_t bit_idx = 7 - (i % 8);
            bits += ((data[byte_idx] >> bit_idx) & 1) ? '1' : '0';
            if ((i + 1) % 8 == 0 && i + 1 < bit_count) {
                bits += ' ';
            }
        }
        return bits;
    }

    bool hex_to_bytes(const std::string& hex_str, uint8_t* out_bytes, size_t max_len, size_t& out_len) {
        out_len = 0;
        std::string clean;
        for (char c : hex_str) {
            if (std::isxdigit(static_cast<unsigned char>(c))) clean += c;
        }
        if (clean.length() % 2 != 0) return false;
        out_len = clean.length() / 2;
        if (out_len > max_len) return false;

        for (size_t i = 0; i < out_len; ++i) {
            std::string byte_hex = clean.substr(i * 2, 2);
            out_bytes[i] = static_cast<uint8_t>(std::stoul(byte_hex, nullptr, 16));
        }
        return true;
    }
}

// -----------------------------------------------------------------------------
// Main Tutorial Program
// -----------------------------------------------------------------------------
int main() {
    print_header("THE LQ DIGITAL MODE FAMILY — DEVELOPER TUTORIAL & API GUIDE");

    // =========================================================================
    // Step 1: Constructing Messages
    // =========================================================================
    print_step(1, "Constructing Structured Messages (lq::Message)");

    std::cout << DIM << "The fundamental unit of the protocol is " << WHITE << "lq::Message" << DIM
              << ". It encapsulates all message types, callsigns, locators, RST reports, and free text." << RESET << "\n\n";

    // 1.1 CQ Message
    lq::Message cq_msg;
    cq_msg.type = lq::MessageType::CQ_STD;
    cq_msg.call_1 = "HB9IPH";
    cq_msg.locator = "JN47";
    std::cout << "  • CQ Message:         " << GREEN << BOLD << lq::format_message(cq_msg) << RESET << "\n";

    // 1.2 CALL Message (4-step QSO step 2)
    lq::Message call_msg;
    call_msg.type = lq::MessageType::CALL_STD;
    call_msg.call_1 = "YO1YO";       // Target callsign (station being called)
    call_msg.call_2 = "HB9IPH";      // My callsign (calling station)
    call_msg.locator = "JN47";       // Caller's grid locator
    call_msg.rst_db = -3;            // Signal report in dB (-26 dB to +5 dB)
    std::cout << "  • CALL Message:       " << GREEN << BOLD << lq::format_message(call_msg) << RESET << "\n";

    // 1.3 REPORT+73 Message (4-step QSO step 3 - report & first 73)
    lq::Message reply_msg;
    reply_msg.type = lq::MessageType::REPORT73_STD;
    reply_msg.call_1 = "YO1YO";
    reply_msg.call_2 = "HB9IPH";
    reply_msg.rst_db = 5;
    std::cout << "  • REPORT+73 Message:  " << GREEN << BOLD << lq::format_message(reply_msg) << RESET << "\n";

    // 1.4 73 Message (4-step QSO step 4 - concluding 73 acknowledgment)
    lq::Message m73_msg;
    m73_msg.type = lq::MessageType::M73_STD;
    m73_msg.call_1 = "YO1YO";
    m73_msg.call_2 = "HB9IPH";
    std::cout << "  • 73 Message:         " << GREEN << BOLD << lq::format_message(m73_msg) << RESET << "\n";

    // 1.5 MULTI-REPORT+73 Message (closes up to 2 QSOs in 1 transmission during pileups)
    lq::Message multi_msg = lq::make_multi_report73("HB9IPH", "YO1YO", 5, "TU2TU", -2);
    std::cout << "  • MULTI-REPORT+73 (2x): " << GREEN << BOLD << lq::format_message(multi_msg) << RESET << "\n";

    // 1.6 Free Text Message (extended 61-char Varicode)
    lq::Message text_msg;
    text_msg.type = lq::MessageType::FREE_TEXT;
    text_msg.text = "73 DE HB9IP";
    std::cout << "  • Free Text Message:  " << GREEN << BOLD << lq::format_message(text_msg) << RESET << "\n\n";

    // =========================================================================
    // Step 2: Encoding to 77-Bit Payload (Binary & Hexadecimal)
    // =========================================================================
    print_step(2, "Encoding Messages to 77-bit Payloads (Binary & Hex)");

    std::cout << DIM << "Calling " << WHITE << "lq::encode_message(msg, payload_bytes)" << DIM
              << " packs all fields into exactly 10 bytes (77 bits)." << RESET << "\n\n";

    uint8_t payload_call[lq::PAYLOAD_BYTES] = {0};
    if (!lq::encode_message(call_msg, payload_call)) {
        fatal_error("Failed to encode CALL_STD message to payload");
    }

    std::cout << "  Formatted Message:  " << WHITE << BOLD << lq::format_message(call_msg) << RESET << "\n";
    std::cout << "  Hexadecimal (10B):  " << CYAN << BOLD << bytes_to_hex(payload_call, lq::PAYLOAD_BYTES) << RESET << "\n";
    std::cout << "  Binary (77 bits):   " << GREEN << bytes_to_bits(payload_call, 77) << RESET << "\n";
    std::cout << DIM << "  Bit Breakdown:      [1-bit Huffman: 1] [28-bit Call1] [28-bit Call2] [15-bit Grid] [5-bit RST]" << RESET << "\n\n";

    // =========================================================================
    // Step 3: Forward Error Correction (CRC-14 & LDPC(174,91))
    // =========================================================================
    print_step(3, "Applying CRC-14 Parity & Systematic LDPC(174,91) Code");

    std::cout << DIM << "To protect against ionospheric fading and noise, the 77-bit payload is protected with\n"
              << "a 14-bit CRC (0x2757) and encoded with a systematic LDPC(174, 91) code." << RESET << "\n\n";

    uint8_t input_91[lq::LDPC_INPUT_BYTES] = {0};
    lq::append_crc14(payload_call, input_91);

    uint8_t codeword_174[lq::LDPC_CODEWORD_BYTES] = {0};
    lq::ldpc_encode(input_91, codeword_174);

    std::cout << "  91-bit Input + CRC: " << CYAN << bytes_to_bits(input_91, 91) << RESET << "\n";
    std::cout << "  174-bit Codeword:   " << GREEN << bytes_to_bits(codeword_174, 174) << RESET << "\n";
    std::cout << "  Codeword Hex (22B): " << WHITE << bytes_to_hex(codeword_174, lq::LDPC_CODEWORD_BYTES) << RESET << "\n\n";

    // =========================================================================
    // Step 4: Channel Modulation & Tone Sequencing
    // =========================================================================
    print_step(4, "Channel Modulation & Tone Sequencing (LQ8, LQ4, LQ2)");

    std::cout << DIM << "The 174-bit codeword is mapped into channel tone symbols with synchronization arrays:" << RESET << "\n\n";

    lq::Protocol modes[] = { lq::Protocol::LQ8, lq::Protocol::LQ4, lq::Protocol::LQ2 };
    const char* mode_names[] = { "LQ8 (Standard DX)", "LQ4 (Fast / Contest)", "LQ2 (Turbo / Burst)" };

    for (int i = 0; i < 3; ++i) {
        lq::Protocol proto = modes[i];
        lq::ToneSequence seq;
        if (!lq::encode_tones(call_msg, proto, seq)) {
            fatal_error("Failed to encode tone sequence for protocol");
        }

        std::cout << "  " << YELLOW << BOLD << mode_names[i] << RESET << ":\n";
        std::cout << "    • Tones: " << WHITE << seq.size() << " symbols" << RESET
                  << "  • Spacing: " << CYAN << seq.tone_spacing << " Hz" << RESET
                  << "  • Duration: " << GREEN << seq.tx_duration << " s" << RESET
                  << "  • Baud: " << ORANGE << (1.0f / seq.symbol_period) << " Bd" << RESET << "\n";

        std::cout << "    • First 16 Tones: " << DIM << "[ ";
        for (size_t t = 0; t < std::min<size_t>(16, seq.size()); ++t) {
            std::cout << static_cast<int>(seq[t]) << " ";
        }
        std::cout << "... ]" << RESET << "\n";
    }
    std::cout << "\n";

    // =========================================================================
    // Step 5: Continuous-Phase GFSK Audio Generation
    // =========================================================================
    print_step(5, "Synthesizing Continuous-Phase GFSK Audio Samples");

    std::cout << DIM << "Calling " << WHITE << "lq::generate_audio(seq, base_freq_hz, sample_rate, audio_samples)" << DIM
              << " produces acoustic float samples [-1.0, 1.0]." << RESET << "\n\n";

    lq::ToneSequence lq8_seq;
    lq::encode_tones(call_msg, lq::Protocol::LQ8, lq8_seq);

    std::vector<float> audio_samples;
    float base_freq = 1500.0f;
    float sample_rate = 12000.0f;
    lq::generate_audio(lq8_seq, base_freq, sample_rate, audio_samples);

    std::cout << "  • Audio Sample Count:  " << WHITE << BOLD << audio_samples.size() << " samples" << RESET << "\n";
    std::cout << "  • Duration:            " << GREEN << (audio_samples.size() / sample_rate) << " seconds" << RESET << "\n";
    std::cout << "  • Base Frequency:      " << CYAN << base_freq << " Hz" << RESET << "\n";
    std::cout << "  • Sampling Frequency:  " << ORANGE << sample_rate << " Hz" << RESET << "\n\n";

    // =========================================================================
    // Step 6: Decoding from Raw Hex / Binary Payloads
    // =========================================================================
    print_step(6, "Decoding from Raw Hex / Binary Payloads");

    std::cout << DIM << "To decode an incoming 77-bit raw payload (from bitstream or network), call "
              << WHITE << "lq::decode_message(payload, decoded_msg)" << DIM << ":" << RESET << "\n\n";

    std::string test_hex = bytes_to_hex(payload_call, lq::PAYLOAD_BYTES);
    std::cout << "  Incoming Raw Hex:   " << CYAN << BOLD << test_hex << RESET << "\n";

    uint8_t received_payload[lq::PAYLOAD_BYTES] = {0};
    size_t parsed_len = 0;
    if (!hex_to_bytes(test_hex, received_payload, lq::PAYLOAD_BYTES, parsed_len) || parsed_len != lq::PAYLOAD_BYTES) {
        fatal_error("Failed to parse hex string back to bytes");
    }

    lq::Message decoded_msg;
    if (!lq::decode_message(received_payload, decoded_msg)) {
        fatal_error("Failed to decode message from raw payload bytes");
    }

    std::cout << "  Decoded Structure:\n";
    std::cout << "    • Type:       " << YELLOW << decoded_msg.type << RESET << "\n";
    std::cout << "    • Target:     " << GREEN << decoded_msg.call_1 << RESET << "\n";
    std::cout << "    • Caller:     " << GREEN << decoded_msg.call_2 << RESET << "\n";
    std::cout << "    • Locator:    " << CYAN << decoded_msg.locator << RESET << "\n";
    std::cout << "    • RST (dB):   " << ORANGE << decoded_msg.rst_db << " dB" << RESET << "\n";
    std::cout << "    • Formatted:  " << WHITE << BOLD << lq::format_message(decoded_msg) << RESET << "\n\n";

    // =========================================================================
    // Step 7: Demodulating & Decoding Tone Sequences & Audio
    // =========================================================================
    print_step(7, "Demodulating & Decoding Tone Sequences and Audio Waveforms");

    // 7.1 From ToneSequence
    lq::Message msg_from_tones;
    if (!lq::decode_tones(lq8_seq, msg_from_tones)) {
        fatal_error("Failed to decode message from tone sequence");
    }
    std::cout << "  • Decoded from Tones:  " << GREEN << BOLD << lq::format_message(msg_from_tones) << RESET << "\n";

    // 7.2 From Raw Audio Samples
    lq::Message msg_from_audio;
    if (!lq::audio_to_message(audio_samples, base_freq, sample_rate, lq::Protocol::LQ8, msg_from_audio)) {
        fatal_error("Failed to decode message from audio waveform");
    }
    std::cout << "  • Decoded from Audio:  " << GREEN << BOLD << lq::format_message(msg_from_audio) << RESET << "\n\n";

    // =========================================================================
    // Step 8: High-Level Modern C++ API (lq::easy & lq::Transceiver)
    // =========================================================================
    print_step(8, "Using the High-Level Fluent API (lq::easy & lq::Transceiver)");

    std::cout << DIM << "For fast development, the library provides factory helpers, std::optional return types,\n"
              << "and a stateful Transceiver class in <lq/easy.h>:" << RESET << "\n\n";

    // 8.1 Factory Helpers & Packing
    auto auto_cq = lq::make_cq("HB9IPH", "JN47", "SOTA");
    std::cout << "  • lq::make_cq:         " << WHITE << lq::format_message(auto_cq) << RESET << "\n";

    auto packed_opt = lq::pack(auto_cq);
    if (!packed_opt) {
        fatal_error("lq::pack failed to pack message");
    }
    std::cout << "  • lq::pack:            " << CYAN << bytes_to_hex(packed_opt->data(), packed_opt->size()) << RESET << "\n";
    auto unpacked_opt = lq::unpack(*packed_opt);
    if (!unpacked_opt) {
        fatal_error("lq::unpack failed to unpack message");
    }
    std::cout << "  • lq::unpack:          " << GREEN << lq::format_message(*unpacked_opt) << RESET << "\n";

    // 8.2 One-Line Text-to-Audio and Audio-to-Text
    std::cout << "\n  • One-Line Audio Pipeline:\n";
    auto fast_audio = lq::text_to_audio("CQ HB9IPH JN47", lq::Protocol::LQ4, 1500.0f, 12000.0f);
    if (fast_audio.empty()) {
        fatal_error("lq::text_to_audio generated empty audio");
    }
    std::cout << "    - Generated " << fast_audio.size() << " audio samples for LQ4.\n";

    auto decoded_text = lq::audio_to_text(fast_audio, 1500.0f, 12000.0f, lq::Protocol::LQ4);
    if (!decoded_text) {
        fatal_error("lq::audio_to_text failed to decode text");
    }
    std::cout << "    - Decoded Text: " << GREEN << BOLD << *decoded_text << RESET << "\n";

    // 8.3 Stateful Transceiver Helper
    std::cout << "\n  • Stateful lq::Transceiver Helper:\n";
    lq::Transceiver trx("HB9IPH", "JN47", lq::Protocol::LQ8);
    trx.set_frequency(1500.0f);

    auto tx_cq_audio = trx.generate_cq();
    auto rx_msg = trx.decode(tx_cq_audio);
    if (!rx_msg) {
        fatal_error("trx.decode failed on synthesized CQ audio");
    }
    std::cout << "    - Transceiver Loopback: " << GREEN << BOLD << lq::format_message(*rx_msg) << RESET << "\n\n";

    std::cout << "\n" << GREEN << BOLD << "✓ Tutorial complete! All encoding, decoding, modulation, and transceiver verified." << RESET << "\n\n";
    return 0;
}

