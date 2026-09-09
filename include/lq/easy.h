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

#ifndef LQ_EASY_H
#define LQ_EASY_H

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <ostream>
#include "lq/types.h"
#include "lq/message.h"
#include "lq/transport.h"

namespace lq {

// ============================================================================
// Factory Functions for Intuitive Message Creation
// ============================================================================

/**
 * Create a CQ broadcast message.
 * Supports standard callsigns, non-standard compound callsigns up to 13 chars,
 * optional grid locator, and optional modifier (e.g. "DX", "POTA", "SOTA").
 */
Message make_cq(std::string_view callsign,
                std::string_view locator = "",
                std::string_view modifier = "");

/**
 * Create a CALL message responding to CQ or initiating a contact.
 * Encodes two callsigns, caller's grid locator, and signal report in dB.
 */
Message make_call(std::string_view target_call,
                  std::string_view my_call,
                  std::string_view locator,
                  int rst_db = 0);

/**
 * Create a REPORT+73 message acknowledging contact and sending report.
 */
Message make_report73(std::string_view target_call,
                      std::string_view my_call,
                      int rst_db = 0);

Message make_reply73(std::string_view target_call,
                     std::string_view my_call,
                     int rst_db = 0);

/**
 * Create a final 73 message completing QSO.
 */
Message make_73(std::string_view target_call,
                std::string_view my_call);

/**
 * Create a MULTI-REPORT+73 message acknowledging up to 2 stations simultaneously.
 */
Message make_multi_report73(std::string_view my_call,
                            const std::vector<MultiTarget>& targets);

Message make_multi_report73(std::string_view my_call,
                            std::string_view target1_call, int rst1,
                            std::string_view target2_call = "", int rst2 = 0);

Message make_multi_reply73(std::string_view my_call,
                           const std::vector<MultiTarget>& targets);

Message make_multi_reply73(std::string_view my_call,
                           std::string_view target1_call, int rst1,
                           std::string_view target2_call = "", int rst2 = 0);

/**
 * Create a MULTI-73 message completing QSO with up to 2 stations simultaneously.
 */
Message make_multi_73(std::string_view my_call,
                      const std::vector<std::string>& targets);

Message make_multi_73(std::string_view my_call,
                      const std::vector<MultiTarget>& targets);

Message make_multi_73(std::string_view my_call,
                      std::string_view target1_call,
                      std::string_view target2_call = "");

/**
 * Create a free-text message (up to ~18 characters using 104-char Sequential Varicode).
 */
Message make_free_text(std::string_view text);

// ============================================================================
// Fluent Modern Encoding & Decoding (Optional-based)
// ============================================================================

/**
 * Pack a Message into a 10-byte (77-bit) payload vector.
 * Returns empty optional if message fields are invalid.
 */
std::optional<std::vector<uint8_t>> pack(const Message& msg);

/**
 * Unpack a 10-byte (77-bit) payload into a structured Message.
 * Returns empty optional on decode failure.
 */
std::optional<Message> unpack(const std::vector<uint8_t>& payload);
std::optional<Message> unpack(const uint8_t* payload_bytes, size_t len = PAYLOAD_BYTES);

/**
 * Convert a structured Message to human-readable text unequivocally and deterministically.
 */
inline std::string message_to_text(const Message& msg) {
    return format_message(msg);
}

// ============================================================================
// Direct 1-Line Audio Transmission & Demodulation
// ============================================================================

/**
 * Encode human-readable text line directly to a ToneSequence.
 */
std::optional<ToneSequence> text_to_tones(std::string_view text, Protocol proto = Protocol::LQ8);

/**
 * Decode a ToneSequence directly to a human-readable text line.
 */
std::optional<std::string> tones_to_text(const ToneSequence& seq);

/**
 * Encode human-readable text line directly to float audio samples.
 * Example:
 *   auto audio = lq::text_to_audio("CQ HB9IPH JN47", Protocol::LQ8, 1500.0f, 12000.0f);
 */
std::vector<float> text_to_audio(std::string_view text,
                                Protocol proto = Protocol::LQ8,
                                float base_freq_hz = 1500.0f,
                                float sample_rate = 12000.0f);

/**
 * Decode raw float audio samples directly to human-readable text line.
 * Returns empty optional if no valid LQ frame decoded.
 * `num_threads`: Number of worker threads for parallel candidate decoding (1 = single-thread fast path).
 * Example:
 *   auto text = lq::audio_to_text(samples, 1500.0f, 12000.0f, Protocol::LQ8, 4);
 */
std::optional<std::string> audio_to_text(const std::vector<float>& audio_samples,
                                         float base_freq_hz = 1500.0f,
                                         float sample_rate = 12000.0f,
                                         Protocol proto = Protocol::LQ8,
                                         int num_threads = 1);

// ============================================================================
// High-Level Transceiver Helper Class
// ============================================================================

/**
 * Lightweight, ergonomic Transceiver helper managing station state,
 * protocol variant, thread configuration, and audio generation / parsing.
 */
class Transceiver {
public:
    Transceiver(std::string_view my_callsign = "HB9IPH",
                std::string_view my_grid = "JN47",
                Protocol proto = Protocol::LQ8,
                float default_freq_hz = 1500.0f,
                float default_sample_rate = 12000.0f,
                int default_num_threads = 1);

    // Configuration
    void set_callsign(std::string_view callsign) { my_call_ = callsign; }
    const std::string& get_callsign() const { return my_call_; }

    void set_grid(std::string_view grid) { my_grid_ = grid; }
    const std::string& get_grid() const { return my_grid_; }

    void set_protocol(Protocol proto) { proto_ = proto; }
    Protocol get_protocol() const { return proto_; }

    void set_frequency(float freq_hz) { freq_hz_ = freq_hz; }
    float get_frequency() const { return freq_hz_; }

    void set_sample_rate(float sample_rate) { sample_rate_ = sample_rate; }
    float get_sample_rate() const { return sample_rate_; }

    void set_num_threads(int num_threads) { num_threads_ = std::max(1, num_threads); }
    int get_num_threads() const { return num_threads_; }

    // Audio generation helpers
    std::vector<float> generate_cq(std::string_view modifier = "") const;
    std::vector<float> generate_call(std::string_view target_call, int rst_db = 0) const;
    std::vector<float> generate_reply73(std::string_view target_call, int rst_db = 0) const;
    std::vector<float> generate_multi_reply73(const std::vector<MultiTarget>& targets) const;
    std::vector<float> generate_multi_reply73(std::string_view target1_call, int rst1,
                                              std::string_view target2_call, int rst2) const;
    std::vector<float> generate_free_text(std::string_view text) const;
    std::vector<float> generate_audio(const Message& msg) const;

    // Audio receive helpers
    std::optional<Message> decode(const std::vector<float>& audio_samples, int num_threads = 0) const;
    std::optional<std::string> decode_text(const std::vector<float>& audio_samples, int num_threads = 0) const;

private:
    std::string my_call_;
    std::string my_grid_;
    Protocol proto_;
    float freq_hz_;
    float sample_rate_;
    int num_threads_ = 1;
};

// ============================================================================
// Standard C++ Stream Operators
// ============================================================================

std::ostream& operator<<(std::ostream& os, const Message& msg);
std::ostream& operator<<(std::ostream& os, MessageType type);
std::ostream& operator<<(std::ostream& os, Protocol proto);
std::ostream& operator<<(std::ostream& os, const ToneSequence& seq);

} // namespace lq

#endif // LQ_EASY_H

