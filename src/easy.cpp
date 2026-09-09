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

#include "lq/easy.h"
#include "lq/callsign.h"
#include "lq/callsign_nonstd.h"
#include "lq/hash.h"
#include "lq/rst.h"
#include <iostream>

namespace lq {

Message make_cq(std::string_view callsign,
                std::string_view locator,
                std::string_view modifier) {
    Message msg;
    std::string s_call(callsign);
    std::string s_loc(locator);
    std::string s_mod(modifier);

    std::string base_c;
    uint8_t suf = 0;
    if (parse_standard_callsign_suffix(s_call, base_c, suf)) {
        msg.type = MessageType::CQ_STD;
        msg.call_1 = format_callsign_with_suffix(base_c, suf);
        msg.suffix_1 = suf;
        msg.locator = s_loc;
        msg.modifier = s_mod;
    } else if (parse_any_callsign_suffix(s_call, base_c, suf)) {
        if (!s_loc.empty() && base_c.size() <= 9) {
            msg.type = MessageType::CQ_NONSTD_1;
            msg.call_1 = format_callsign_with_suffix(base_c, suf);
            msg.suffix_1 = suf;
            msg.locator = s_loc;
        } else if (!s_mod.empty() && base_c.size() <= 9) {
            msg.type = MessageType::CQ_NONSTD_2;
            msg.call_1 = format_callsign_with_suffix(base_c, suf);
            msg.suffix_1 = suf;
            msg.modifier = s_mod;
        } else {
            msg.type = MessageType::CQ_NONSTD_3;
            msg.call_1 = format_callsign_with_suffix(base_c, suf);
            msg.suffix_1 = suf;
        }
    } else {
        msg.type = MessageType::FREE_TEXT;
        msg.text = "CQ " + s_call;
    }
    return msg;
}

Message make_call(std::string_view target_call,
                  std::string_view my_call,
                  std::string_view locator,
                  int rst_db) {
    Message msg;
    std::string t_call(target_call);
    std::string m_call(my_call);

    std::string base_t, base_m;
    uint8_t suf_t = 0, suf_m = 0;
    bool std_target = parse_standard_callsign_suffix(t_call, base_t, suf_t);
    bool std_my = parse_standard_callsign_suffix(m_call, base_m, suf_m);

    if (std_my) {
        if (std_target && suf_t == 0 && suf_m == 0 && !locator.empty()) {
            msg.type = MessageType::CALL_STD_NOSUF;
            msg.call_1 = base_t;
            msg.call_2 = base_m;
            msg.locator = std::string(locator);
            msg.rst_db = rst_db;
        } else {
            msg.type = MessageType::CALL_STD_SUF;
            msg.call_1 = t_call;
            msg.hash_1 = hash_callsign_24(t_call);
            msg.call_2 = format_callsign_with_suffix(base_m, suf_m);
            msg.suffix_2 = suf_m;
            msg.locator = std::string(locator);
            msg.rst_db = rst_db;
        }
    } else {
        msg.type = MessageType::CALL_NONSTD;
        msg.call_1 = t_call;
        msg.hash_1 = hash_callsign_20(t_call);
        std::string base_m;
        uint8_t suf_m = 0;
        if (parse_any_callsign_suffix(m_call, base_m, suf_m)) {
            msg.call_2 = format_callsign_with_suffix(base_m, suf_m);
            msg.suffix_2 = suf_m;
        } else {
            msg.call_2 = m_call;
            msg.suffix_2 = 0;
        }
        msg.hash_2 = 0;
        msg.locator = "";
        msg.rst_db = rst_db;
    }
    return msg;
}

Message make_report73(std::string_view target_call,
                      std::string_view my_call,
                      int rst_db) {
    Message msg;
    std::string t_call(target_call);
    std::string m_call(my_call);

    std::string base_t, base_m;
    uint8_t suf_t = 0, suf_m = 0;
    bool std_target = parse_standard_callsign_suffix(t_call, base_t, suf_t);
    bool std_my = parse_standard_callsign_suffix(m_call, base_m, suf_m);

    if (std_target && std_my) {
        msg.type = MessageType::REPORT73_STD;
        msg.call_1 = format_callsign_with_suffix(base_t, suf_t);
        msg.suffix_1 = suf_t;
        msg.call_2 = format_callsign_with_suffix(base_m, suf_m);
        msg.suffix_2 = suf_m;
        msg.rst_db = rst_db;
    } else {
        msg.type = MessageType::MULTI_REPORT73;
        msg.call_1 = m_call;
        msg.hash_1 = hash_callsign_16(m_call);
        MultiTarget tgt;
        tgt.call = t_call;
        tgt.hash = hash_callsign_24(t_call);
        tgt.rst_db = rst_db;
        msg.multi_targets.push_back(tgt);
    }
    return msg;
}

Message make_reply73(std::string_view target_call,
                     std::string_view my_call,
                     int rst_db) {
    return make_report73(target_call, my_call, rst_db);
}

Message make_73(std::string_view target_call,
                std::string_view my_call) {
    Message msg;
    std::string t_call(target_call);
    std::string m_call(my_call);

    std::string base_t, base_m;
    uint8_t suf_t = 0, suf_m = 0;
    bool std_target = parse_standard_callsign_suffix(t_call, base_t, suf_t);
    bool std_my = parse_standard_callsign_suffix(m_call, base_m, suf_m);

    if (std_target && std_my) {
        msg.type = MessageType::M73_STD;
        msg.call_1 = format_callsign_with_suffix(base_t, suf_t);
        msg.suffix_1 = suf_t;
        msg.call_2 = format_callsign_with_suffix(base_m, suf_m);
        msg.suffix_2 = suf_m;
    } else if (parse_any_callsign_suffix(m_call, base_m, suf_m) && base_m.size() <= 9) {
        msg.type = MessageType::M73_NONSTD;
        msg.call_1 = t_call;
        msg.hash_1 = hash_callsign_24(t_call);
        msg.call_2 = format_callsign_with_suffix(base_m, suf_m);
        msg.suffix_2 = suf_m & 1;
    } else {
        msg.type = MessageType::MULTI_73;
        msg.call_1 = m_call;
        msg.hash_1 = hash_callsign_16(m_call);
        MultiTarget target;
        target.call = t_call;
        target.hash = hash_callsign_24(t_call);
        target.rst_db = 0;
        msg.multi_targets.push_back(target);
    }
    return msg;
}

Message make_multi_report73(std::string_view my_call,
                            const std::vector<MultiTarget>& targets) {
    Message msg;
    msg.type = MessageType::MULTI_REPORT73;
    msg.call_1 = std::string(my_call);
    msg.hash_1 = hash_callsign_16(my_call);
    for (size_t i = 0; i < targets.size() && i < 2; ++i) {
        msg.multi_targets.push_back(targets[i]);
    }
    return msg;
}

Message make_multi_report73(std::string_view my_call,
                            std::string_view target1_call, int rst1,
                            std::string_view target2_call, int rst2) {
    std::vector<MultiTarget> targets;
    targets.push_back({std::string(target1_call), hash_callsign_24(target1_call), rst1});
    if (!target2_call.empty()) {
        targets.push_back({std::string(target2_call), hash_callsign_24(target2_call), rst2});
    }
    return make_multi_report73(my_call, targets);
}

Message make_multi_reply73(std::string_view my_call,
                           const std::vector<MultiTarget>& targets) {
    return make_multi_report73(my_call, targets);
}

Message make_multi_reply73(std::string_view my_call,
                           std::string_view target1_call, int rst1,
                           std::string_view target2_call, int rst2) {
    return make_multi_report73(my_call, target1_call, rst1, target2_call, rst2);
}

Message make_multi_73(std::string_view my_call,
                      const std::vector<MultiTarget>& targets) {
    Message msg;
    msg.type = MessageType::MULTI_73;
    msg.call_1 = std::string(my_call);
    msg.hash_1 = hash_callsign_16(my_call);
    for (size_t i = 0; i < targets.size() && i < 2; ++i) {
        msg.multi_targets.push_back(targets[i]);
    }
    return msg;
}

Message make_multi_73(std::string_view my_call,
                      const std::vector<std::string>& targets) {
    Message msg;
    msg.type = MessageType::MULTI_73;
    msg.call_1 = std::string(my_call);
    msg.hash_1 = hash_callsign_16(my_call);
    for (size_t i = 0; i < targets.size() && i < 2; ++i) {
        MultiTarget t;
        t.call = targets[i];
        t.hash = hash_callsign_24(targets[i]);
        t.rst_db = 0;
        msg.multi_targets.push_back(t);
    }
    return msg;
}

Message make_multi_73(std::string_view my_call,
                      std::string_view target1_call,
                      std::string_view target2_call) {
    std::vector<std::string> targets;
    targets.push_back(std::string(target1_call));
    if (!target2_call.empty()) {
        targets.push_back(std::string(target2_call));
    }
    return make_multi_73(my_call, targets);
}

Message make_free_text(std::string_view text) {
    Message msg;
    msg.type = MessageType::FREE_TEXT;
    msg.text = std::string(text);
    return msg;
}

std::optional<std::vector<uint8_t>> pack(const Message& msg) {
    std::vector<uint8_t> payload(PAYLOAD_BYTES, 0);
    if (encode_message(msg, payload.data())) {
        return payload;
    }
    return std::nullopt;
}

std::optional<Message> unpack(const std::vector<uint8_t>& payload) {
    if (payload.size() < PAYLOAD_BYTES) return std::nullopt;
    return unpack(payload.data(), payload.size());
}

std::optional<Message> unpack(const uint8_t* payload_bytes, size_t len) {
    if (!payload_bytes || len < PAYLOAD_BYTES) return std::nullopt;
    Message msg;
    if (decode_message(payload_bytes, msg)) {
        return msg;
    }
    return std::nullopt;
}

std::optional<ToneSequence> text_to_tones(std::string_view text, Protocol proto) {
    Message msg;
    if (!parse_message(text, msg)) {
        return std::nullopt;
    }
    ToneSequence seq;
    if (!encode_tones(msg, proto, seq)) {
        return std::nullopt;
    }
    return seq;
}

std::optional<std::string> tones_to_text(const ToneSequence& seq) {
    if (seq.empty()) return std::nullopt;
    Message msg;
    if (!decode_tones(seq, msg)) {
        return std::nullopt;
    }
    return format_message(msg);
}

std::vector<float> text_to_audio(std::string_view text,
                                Protocol proto,
                                float base_freq_hz,
                                float sample_rate) {
    Message msg;
    if (!parse_message(text, msg)) {
        return {};
    }
    std::vector<float> samples;
    if (!message_to_audio(msg, proto, base_freq_hz, sample_rate, samples)) {
        return {};
    }
    return samples;
}

std::optional<std::string> audio_to_text(const std::vector<float>& audio_samples,
                                         float base_freq_hz,
                                         float sample_rate,
                                         Protocol proto,
                                         int num_threads) {
    Message msg;
    if (!audio_to_message(audio_samples, base_freq_hz, sample_rate, proto, msg, num_threads)) {
        return std::nullopt;
    }
    return format_message(msg);
}

// ============================================================================
// Transceiver Implementation
// ============================================================================

Transceiver::Transceiver(std::string_view my_callsign,
                         std::string_view my_grid,
                         Protocol proto,
                         float default_freq_hz,
                         float default_sample_rate,
                         int default_num_threads)
    : my_call_(my_callsign),
      my_grid_(my_grid),
      proto_(proto),
      freq_hz_(default_freq_hz),
      sample_rate_(default_sample_rate),
      num_threads_(std::max(1, default_num_threads)) {}

std::vector<float> Transceiver::generate_cq(std::string_view modifier) const {
    Message msg = make_cq(my_call_, my_grid_, modifier);
    return generate_audio(msg);
}

std::vector<float> Transceiver::generate_call(std::string_view target_call, int rst_db) const {
    Message msg = make_call(target_call, my_call_, my_grid_, rst_db);
    return generate_audio(msg);
}

std::vector<float> Transceiver::generate_reply73(std::string_view target_call, int rst_db) const {
    Message msg = make_reply73(target_call, my_call_, rst_db);
    return generate_audio(msg);
}

std::vector<float> Transceiver::generate_multi_reply73(const std::vector<MultiTarget>& targets) const {
    Message msg = make_multi_reply73(my_call_, targets);
    return generate_audio(msg);
}

std::vector<float> Transceiver::generate_multi_reply73(std::string_view target1_call, int rst1,
                                                      std::string_view target2_call, int rst2) const {
    Message msg = make_multi_reply73(my_call_, target1_call, rst1, target2_call, rst2);
    return generate_audio(msg);
}

std::vector<float> Transceiver::generate_free_text(std::string_view text) const {
    Message msg = make_free_text(text);
    return generate_audio(msg);
}

std::vector<float> Transceiver::generate_audio(const Message& msg) const {
    std::vector<float> samples;
    message_to_audio(msg, proto_, freq_hz_, sample_rate_, samples);
    return samples;
}

std::optional<Message> Transceiver::decode(const std::vector<float>& audio_samples, int num_threads) const {
    Message msg;
    int threads = (num_threads > 0) ? num_threads : num_threads_;
    if (audio_to_message(audio_samples, freq_hz_, sample_rate_, proto_, msg, threads)) {
        return msg;
    }
    return std::nullopt;
}

std::optional<std::string> Transceiver::decode_text(const std::vector<float>& audio_samples, int num_threads) const {
    Message msg;
    int threads = (num_threads > 0) ? num_threads : num_threads_;
    if (audio_to_message(audio_samples, freq_hz_, sample_rate_, proto_, msg, threads)) {
        return format_message(msg);
    }
    return std::nullopt;
}

// ============================================================================
// Standard C++ Stream Operators
// ============================================================================

std::ostream& operator<<(std::ostream& os, const Message& msg) {
    os << format_message(msg);
    return os;
}

std::ostream& operator<<(std::ostream& os, MessageType type) {
    switch (type) {
        case MessageType::CQ_STD:           os << "CQ_STD"; break;
        case MessageType::CQ_NONSTD_1:      os << "CQ_NONSTD_1"; break;
        case MessageType::CQ_NONSTD_2:      os << "CQ_NONSTD_2"; break;
        case MessageType::CQ_NONSTD_3:      os << "CQ_NONSTD_3"; break;
        case MessageType::CALL_STD_NOSUF:   os << "CALL_STD_NOSUF"; break;
        case MessageType::CALL_STD_SUF:     os << "CALL_STD_SUF"; break;
        case MessageType::CALL_NONSTD:      os << "CALL_NONSTD"; break;
        case MessageType::REPORT73_STD:     os << "REPORT73_STD"; break;
        case MessageType::M73_STD:          os << "M73_STD"; break;
        case MessageType::M73_NONSTD:       os << "M73_NONSTD"; break;
        case MessageType::MULTI_REPORT73:   os << "MULTI_REPORT73"; break;
        case MessageType::MULTI_73:         os << "MULTI_73"; break;
        case MessageType::FREE_TEXT:        os << "FREE_TEXT"; break;
        case MessageType::RESERVED_A:       os << "RESERVED_A"; break;
        case MessageType::RESERVED_B:       os << "RESERVED_B"; break;
        case MessageType::RESERVED_C:       os << "RESERVED_C"; break;
        default:                            os << "UNKNOWN"; break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, Protocol proto) {
    switch (proto) {
        case Protocol::LQ8: os << "LQ8"; break;
        case Protocol::LQ4: os << "LQ4"; break;
        case Protocol::LQ2: os << "LQ2"; break;
        case Protocol::LQ16: os << "LQ16"; break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const ToneSequence& seq) {

    os << "ToneSequence[" << seq.protocol << ", " << seq.size() << " symbols: ";
    for (size_t i = 0; i < seq.size(); ++i) {
        if (i > 0) os << " ";
        os << static_cast<int>(seq[i]);
    }
    os << "]";
    return os;
}

} // namespace lq
