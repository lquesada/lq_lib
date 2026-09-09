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

#include "lq/c_api.h"
#include "lq/lq.h"
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace {

void safe_copy(char* dest, size_t dest_size, const std::string& src) {
    if (!dest || dest_size == 0) return;
    size_t copy_len = std::min(src.size(), dest_size - 1);
    std::memcpy(dest, src.data(), copy_len);
    dest[copy_len] = '\0';
}

lq::Protocol mode_to_proto(lq_mode_t mode) {
    switch (mode) {
        case LQ_MODE_LQ8:        return lq::Protocol::LQ8;
        case LQ_MODE_LQ4:        return lq::Protocol::LQ4;
        case LQ_MODE_LQ2:        return lq::Protocol::LQ2;
        case LQ_MODE_LQ16:       return lq::Protocol::LQ16;
        default:                 return lq::Protocol::LQ8;
    }
}

void msg_to_c(const lq::Message& cpp_msg, lq_c_message_t* c_msg) {
    if (!c_msg) return;
    std::memset(c_msg, 0, sizeof(lq_c_message_t));

    c_msg->type = static_cast<int>(cpp_msg.type);
    safe_copy(c_msg->call_from, sizeof(c_msg->call_from), cpp_msg.get_sender_call());
    safe_copy(c_msg->call_to, sizeof(c_msg->call_to), cpp_msg.get_target_call());
    safe_copy(c_msg->grid, sizeof(c_msg->grid), cpp_msg.locator);
    c_msg->rst_db = cpp_msg.rst_db;
    safe_copy(c_msg->modifier, sizeof(c_msg->modifier), cpp_msg.modifier);
    safe_copy(c_msg->text, sizeof(c_msg->text), cpp_msg.text.empty() ? lq::format_message(cpp_msg) : cpp_msg.text);

    c_msg->hash_from_24 = cpp_msg.get_sender_hash();
    c_msg->hash_to_24 = cpp_msg.get_target_hash();
    c_msg->hash_from_20 = (cpp_msg.get_sender_hash() >> 4);
    c_msg->hash_to_20 = (cpp_msg.get_target_hash() >> 4);
    c_msg->hash_from_23 = (cpp_msg.get_sender_hash() & 0x7FFFFFU);
    c_msg->hash_to_23 = (cpp_msg.get_target_hash() & 0x7FFFFFU);
    c_msg->hash_from_16 = (cpp_msg.type == lq::MessageType::MULTI_REPORT73 || cpp_msg.type == lq::MessageType::MULTI_73) ? (cpp_msg.hash_1 ? cpp_msg.hash_1 : lq::hash_callsign_16(cpp_msg.call_1)) : lq::hash_callsign_16(cpp_msg.get_sender_call());
    c_msg->hash_from_14 = lq::hash_callsign_14(cpp_msg.get_sender_call());
    c_msg->hash_from_12 = (cpp_msg.type == lq::MessageType::MULTI_REPORT73 || cpp_msg.type == lq::MessageType::MULTI_73) ? (cpp_msg.hash_1 ? cpp_msg.hash_1 : lq::hash_callsign_12(cpp_msg.call_1)) : lq::hash_callsign_12(cpp_msg.get_sender_call());
    c_msg->hash_to_12 = lq::hash_callsign_12(cpp_msg.get_target_call());

    c_msg->suffix_from = cpp_msg.is_cq() ? cpp_msg.suffix_1 : cpp_msg.suffix_2;
    c_msg->suffix_to = cpp_msg.is_cq() ? 0 : cpp_msg.suffix_1;

    c_msg->num_multi_targets = static_cast<int>(cpp_msg.multi_targets.size());
    for (size_t i = 0; i < cpp_msg.multi_targets.size() && i < 2; ++i) {
        safe_copy(c_msg->multi_targets[i].call, sizeof(c_msg->multi_targets[i].call), cpp_msg.multi_targets[i].call);
        c_msg->multi_targets[i].hash_24 = cpp_msg.multi_targets[i].hash;
        c_msg->multi_targets[i].rst_db = cpp_msg.multi_targets[i].rst_db;
    }

    c_msg->is_valid = (cpp_msg.type != lq::MessageType::UNKNOWN) ? 1 : 0;
    c_msg->is_cq = cpp_msg.is_cq() ? 1 : 0;
    c_msg->is_call = cpp_msg.is_call() ? 1 : 0;
    c_msg->is_reply73 = cpp_msg.is_report73() ? 1 : 0;
    c_msg->is_report73 = cpp_msg.is_report73() ? 1 : 0;
    c_msg->is_73 = cpp_msg.is_73() ? 1 : 0;
    c_msg->is_multi_reply73 = cpp_msg.is_multi_report73() ? 1 : 0;
    c_msg->is_multi_report73 = cpp_msg.is_multi_report73() ? 1 : 0;
    c_msg->is_multi_73 = cpp_msg.is_multi_73() ? 1 : 0;
    c_msg->is_free_text = cpp_msg.is_free_text() ? 1 : 0;

    lq::encode_message(cpp_msg, c_msg->payload);
}


void c_to_msg(const lq_c_message_t* c_msg, lq::Message& cpp_msg) {
    cpp_msg = lq::Message{};
    if (!c_msg) return;

    cpp_msg.type = static_cast<lq::MessageType>(c_msg->type);
    if (cpp_msg.type == lq::MessageType::MULTI_REPORT73 || cpp_msg.type == lq::MessageType::MULTI_73) {
        cpp_msg.call_1 = (c_msg->call_from[0] == '<') ? "" : c_msg->call_from;
        cpp_msg.hash_1 = c_msg->hash_from_16 ? c_msg->hash_from_16 : lq::hash_callsign_16(c_msg->call_from);
        for (int i = 0; i < c_msg->num_multi_targets && i < 2; ++i) {
            lq::MultiTarget t;
            t.call = (c_msg->multi_targets[i].call[0] == '<') ? "" : c_msg->multi_targets[i].call;
            t.hash = c_msg->multi_targets[i].hash_24 ? c_msg->multi_targets[i].hash_24 : lq::hash_callsign_24(c_msg->multi_targets[i].call);
            t.rst_db = c_msg->multi_targets[i].rst_db;
            cpp_msg.multi_targets.push_back(t);
        }
    } else if (c_msg->is_cq || (c_msg->type >= 1 && c_msg->type <= 4)) {
        std::string c_from = (c_msg->call_from[0] == '<') ? "" : c_msg->call_from;
        cpp_msg.call_1 = c_from;
        cpp_msg.call_2 = "";
        cpp_msg.suffix_1 = c_msg->suffix_from;
        cpp_msg.suffix_2 = 0;
    } else {
        std::string c_from = (c_msg->call_from[0] == '<') ? "" : c_msg->call_from;
        std::string c_to = (c_msg->call_to[0] == '<') ? "" : c_msg->call_to;
        cpp_msg.call_1 = c_to;
        cpp_msg.call_2 = c_from;
        cpp_msg.suffix_1 = c_msg->suffix_to;
        cpp_msg.suffix_2 = c_msg->suffix_from;
        cpp_msg.hash_1 = c_msg->hash_to_24 ? c_msg->hash_to_24 : c_msg->hash_to_20;
        cpp_msg.hash_2 = c_msg->hash_from_24 ? c_msg->hash_from_24 : c_msg->hash_from_20;
    }
    cpp_msg.modifier = c_msg->modifier;
    cpp_msg.locator = c_msg->grid;
    cpp_msg.rst_db = c_msg->rst_db;
    cpp_msg.text = c_msg->text;
}

} // anonymous namespace

extern "C" {

const char* lq_c_version_string(void) {
    return "1.0.0";
}

int lq_c_version_major(void) { return 1; }
int lq_c_version_minor(void) { return 0; }
int lq_c_version_patch(void) { return 0; }

int lq_c_get_mode_params(lq_mode_t mode, lq_c_mode_params_t* out_params) {
    if (!out_params) return 0;
    lq::Protocol proto = mode_to_proto(mode);
    auto p = lq::get_protocol_params(proto);

    out_params->mode_id = static_cast<int>(mode);
    out_params->name = p.name;
    out_params->total_symbols = p.total_symbols;
    out_params->num_tones = p.num_tones;
    out_params->bits_per_symbol = p.bits_per_symbol;
    out_params->symbol_period = p.symbol_period;
    out_params->tone_spacing = p.tone_spacing;
    out_params->bandwidth_hz = p.occupied_bw;
    out_params->tx_duration_sec = p.tx_duration;
    out_params->slot_duration_sec = p.slot_duration;
    out_params->threshold_snr_db = p.threshold_snr;
    return 1;
}

uint32_t lq_c_hash_callsign_24(const char* callsign) {
    if (!callsign) return 0;
    return lq::hash_callsign_24(callsign);
}

uint32_t lq_c_hash_callsign_20(const char* callsign) {
    if (!callsign) return 0;
    return lq::hash_callsign_20(callsign);
}

uint32_t lq_c_hash_callsign_23(const char* callsign) {
    if (!callsign) return 0;
    return lq::hash_callsign_23(callsign);
}

uint32_t lq_c_hash_callsign_22(const char* callsign) {
    if (!callsign) return 0;
    return lq::hash_callsign_22(callsign);
}

uint32_t lq_c_hash_callsign_16(const char* callsign) {
    if (!callsign) return 0;
    return lq::hash_callsign_16(callsign);
}

uint32_t lq_c_hash_callsign_14(const char* callsign) {
    if (!callsign) return 0;
    return lq::hash_callsign_14(callsign);
}

uint32_t lq_c_hash_callsign_12(const char* callsign) {
    if (!callsign) return 0;
    return lq::hash_callsign_12(callsign);
}

uint32_t lq_c_hash_callsign_10(const char* callsign) {
    if (!callsign) return 0;
    return lq::hash_callsign_10(callsign);
}

int lq_c_encode_message(const lq_c_message_t* msg, uint8_t payload[10]) {
    if (!msg || !payload) return 0;
    lq::Message cpp_msg;
    c_to_msg(msg, cpp_msg);
    return lq::encode_message(cpp_msg, payload) ? 1 : 0;
}

int lq_c_decode_message(const uint8_t payload[10], lq_c_message_t* out_msg) {
    if (!payload || !out_msg) return 0;
    lq::Message cpp_msg;
    if (!lq::decode_message(payload, cpp_msg)) {
        std::memset(out_msg, 0, sizeof(lq_c_message_t));
        return 0;
    }
    msg_to_c(cpp_msg, out_msg);
    return 1;
}

int lq_c_resolve_callsigns(lq_c_message_t* msg, const char* const* known_callsigns, size_t count) {
    if (!msg || !known_callsigns || count == 0) return 0;
    lq::Message cpp_msg;
    c_to_msg(msg, cpp_msg);

    std::vector<std::string_view> views;
    views.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        if (known_callsigns[i]) {
            views.push_back(std::string_view(known_callsigns[i]));
        }
    }

    bool res = lq::resolve_callsigns(cpp_msg, views.data(), views.size());
    if (res) {
        msg_to_c(cpp_msg, msg);
        return 1;
    }
    return 0;
}

int lq_c_decode_message_with_known_calls(const uint8_t payload[10], lq_c_message_t* out_msg, const char* const* known_callsigns, size_t count) {
    if (!lq_c_decode_message(payload, out_msg)) return 0;
    if (known_callsigns && count > 0) {
        lq_c_resolve_callsigns(out_msg, known_callsigns, count);
    }
    return 1;
}

int lq_c_format_message(const lq_c_message_t* msg, char* out_str, size_t max_len) {
    if (!msg || !out_str || max_len == 0) return 0;
    lq::Message cpp_msg;
    c_to_msg(msg, cpp_msg);
    std::string formatted = lq::format_message(cpp_msg);
    safe_copy(out_str, max_len, formatted);
    return 1;
}

int lq_c_message_to_text(const lq_c_message_t* msg, char* out_str, size_t max_len) {
    return lq_c_format_message(msg, out_str, max_len);
}

int lq_c_payload_to_text(const uint8_t payload[10], char* out_text, size_t max_len, const char* const* known_callsigns, size_t count) {
    if (!payload || !out_text || max_len == 0) return 0;
    std::vector<std::string_view> views;
    if (known_callsigns && count > 0) {
        views.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            if (known_callsigns[i]) views.push_back(std::string_view(known_callsigns[i]));
        }
    }
    std::string s = lq::payload_to_text(payload, views.empty() ? nullptr : views.data(), views.size());
    if (s.empty()) {
        out_text[0] = '\0';
        return 0;
    }
    safe_copy(out_text, max_len, s);
    return 1;
}

int lq_c_parse_message(const char* text, lq_c_message_t* out_msg) {
    if (!text || !out_msg) return 0;
    lq::Message cpp_msg;
    if (!lq::parse_message(text, cpp_msg)) {
        std::memset(out_msg, 0, sizeof(lq_c_message_t));
        return 0;
    }
    msg_to_c(cpp_msg, out_msg);
    return 1;
}

int lq_c_payload_to_hex(const uint8_t payload[10], char* out_hex, size_t max_len, int space_separated) {
    if (!payload || !out_hex || max_len == 0) return 0;
    std::string s = lq::payload_to_hex(payload, space_separated != 0);
    safe_copy(out_hex, max_len, s);
    return 1;
}

int lq_c_payload_to_binary(const uint8_t payload[10], char* out_bin, size_t max_len) {
    if (!payload || !out_bin || max_len == 0) return 0;
    std::string s = lq::payload_to_binary(payload, 77);
    safe_copy(out_bin, max_len, s);
    return 1;
}

int lq_c_hex_to_payload(const char* hex_str, uint8_t out_payload[10]) {
    if (!hex_str || !out_payload) return 0;
    return lq::hex_to_payload(hex_str, out_payload) ? 1 : 0;
}

int lq_c_binary_to_payload(const char* bin_str, uint8_t out_payload[10]) {
    if (!bin_str || !out_payload) return 0;
    return lq::binary_to_payload(bin_str, out_payload) ? 1 : 0;
}

int lq_c_hex_to_text(const char* hex_str, char* out_text, size_t max_len, const char* const* known_callsigns, size_t count) {
    if (!hex_str || !out_text || max_len == 0) return 0;
    std::vector<std::string_view> views;
    if (known_callsigns && count > 0) {
        views.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            if (known_callsigns[i]) views.push_back(std::string_view(known_callsigns[i]));
        }
    }
    std::string s = lq::hex_to_text(hex_str, views.empty() ? nullptr : views.data(), views.size());
    if (s.empty()) return 0;
    safe_copy(out_text, max_len, s);
    return 1;
}

int lq_c_binary_to_text(const char* bin_str, char* out_text, size_t max_len, const char* const* known_callsigns, size_t count) {
    if (!bin_str || !out_text || max_len == 0) return 0;
    std::vector<std::string_view> views;
    if (known_callsigns && count > 0) {
        views.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            if (known_callsigns[i]) views.push_back(std::string_view(known_callsigns[i]));
        }
    }
    std::string s = lq::binary_to_text(bin_str, views.empty() ? nullptr : views.data(), views.size());
    if (s.empty()) return 0;
    safe_copy(out_text, max_len, s);
    return 1;
}

int lq_c_text_to_hex(const char* text, char* out_hex, size_t max_len, int space_separated) {
    if (!text || !out_hex || max_len == 0) return 0;
    std::string s = lq::text_to_hex(text, space_separated != 0);
    if (s.empty()) return 0;
    safe_copy(out_hex, max_len, s);
    return 1;
}

int lq_c_text_to_binary(const char* text, char* out_bin, size_t max_len) {
    if (!text || !out_bin || max_len == 0) return 0;
    std::string s = lq::text_to_binary(text, 77);
    if (s.empty()) return 0;
    safe_copy(out_bin, max_len, s);
    return 1;
}

int lq_c_encode_payload(const uint8_t payload[10], uint8_t* out_tones, lq_mode_t mode) {
    if (!payload || !out_tones) return 0;
    lq::Protocol proto = mode_to_proto(mode);
    return lq::encode_payload(payload, out_tones, proto) ? 1 : 0;
}

int lq_c_decode_payload(const uint8_t* tones, uint8_t out_payload[10], lq_mode_t mode) {
    if (!tones || !out_payload) return 0;
    lq::Protocol proto = mode_to_proto(mode);
    return lq::decode_payload(tones, out_payload, proto) ? 1 : 0;
}

int lq_c_encode_tones(const lq_c_message_t* msg, uint8_t* out_tones, lq_mode_t mode) {
    if (!msg || !out_tones) return 0;
    lq::Message cpp_msg;
    c_to_msg(msg, cpp_msg);
    lq::ToneSequence seq;
    lq::Protocol proto = mode_to_proto(mode);
    if (!lq::encode_tones(cpp_msg, proto, seq)) return 0;
    std::memcpy(out_tones, seq.tones.data(), seq.tones.size());
    return 1;
}

int lq_c_decode_tones(const uint8_t* tones, lq_c_message_t* out_msg, lq_mode_t mode) {
    if (!tones || !out_msg) return 0;
    lq::Protocol proto = mode_to_proto(mode);
    auto params = lq::get_protocol_params(proto);
    lq::ToneSequence seq;
    seq.protocol = proto;
    seq.tones.assign(tones, tones + params.total_symbols);
    seq.symbol_period = params.symbol_period;
    seq.tone_spacing = params.tone_spacing;
    seq.tx_duration = params.tx_duration;

    lq::Message cpp_msg;
    if (!lq::decode_tones(seq, cpp_msg)) {
        std::memset(out_msg, 0, sizeof(lq_c_message_t));
        return 0;
    }
    msg_to_c(cpp_msg, out_msg);
    return 1;
}

void lq_c_synth_gfsk(const uint8_t* symbols, int num_symbols, float f0, float symbol_bt, float symbol_period, float sample_rate, float* signal_out) {
    lq::synth_gfsk(symbols, num_symbols, f0, symbol_bt, symbol_period, sample_rate, signal_out);
}

int lq_c_generate_audio(const uint8_t* tones, int num_tones, float base_freq_hz, float sample_rate, lq_mode_t mode, float* audio_out, int max_samples) {
    if (!tones || num_tones <= 0 || !audio_out || max_samples <= 0) return 0;
    lq::Protocol proto = mode_to_proto(mode);
    auto params = lq::get_protocol_params(proto);
    if (num_tones < params.total_symbols) return 0;

    lq::ToneSequence seq;
    seq.protocol = proto;
    seq.tones.assign(tones, tones + params.total_symbols);
    seq.symbol_period = params.symbol_period;
    seq.tone_spacing = params.tone_spacing;
    seq.tx_duration = params.tx_duration;

    std::vector<float> audio;
    lq::generate_audio(seq, base_freq_hz, sample_rate, audio);
    if (audio.empty()) return 0;
    if (static_cast<int>(audio.size()) > max_samples) return 0;

    std::memcpy(audio_out, audio.data(), audio.size() * sizeof(float));
    return static_cast<int>(audio.size());
}

int lq_c_audio_to_message(const float* audio, size_t num_samples, float base_freq, float sample_rate, lq_mode_t mode, lq_c_message_t* out_msg, int num_threads) {
    if (!audio || num_samples == 0 || !out_msg) return 0;
    std::vector<float> audio_vec(audio, audio + num_samples);
    lq::Message cpp_msg;
    if (!lq::audio_to_message(audio_vec, base_freq, sample_rate, mode_to_proto(mode), cpp_msg, num_threads)) {
        std::memset(out_msg, 0, sizeof(lq_c_message_t));
        return 0;
    }
    msg_to_c(cpp_msg, out_msg);
    return 1;
}

int lq_c_audio_to_messages_ext(const float* audio_samples, int num_samples, float base_freq, float sample_rate, lq_mode_t mode, lq_c_message_t* out_messages, int max_messages, int num_threads, int is_deep) {
    if (!audio_samples || num_samples <= 0 || !out_messages || max_messages <= 0) return 0;
    std::vector<float> audio_vec(audio_samples, audio_samples + num_samples);
    auto cpp_msgs = lq::audio_to_messages(audio_vec, base_freq, sample_rate, mode_to_proto(mode), num_threads, is_deep != 0);
    size_t count = std::min(cpp_msgs.size(), static_cast<size_t>(max_messages));
    for (size_t i = 0; i < count; ++i) {
        msg_to_c(cpp_msgs[i], &out_messages[i]);
    }
    return static_cast<int>(count);
}

int lq_c_audio_to_messages(const float* audio, size_t num_samples, float base_freq, float sample_rate, lq_mode_t mode, lq_c_message_t* out_msgs, size_t max_msgs, int num_threads) {
    return lq_c_audio_to_messages_ext(audio, static_cast<int>(num_samples), base_freq, sample_rate, mode, out_msgs, static_cast<int>(max_msgs), num_threads, 0);
}

int lq_c_waterfall_sync_score(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, lq_mode_t mode) {
    return lq::lq_sync_score(mag, num_blocks, block_stride, time_offset, freq_offset, mode_to_proto(mode));
}

float lq_c_waterfall_refine_frequency(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, int freq_sub, int freq_osr, float symbol_period, lq_mode_t mode, int* out_score) {
    return lq::lq_refine_frequency(mag, num_blocks, block_stride, time_offset, freq_offset, freq_sub, freq_osr, symbol_period, mode_to_proto(mode), out_score);
}

float lq_c_waterfall_guess_snr(const float* mag2, int num_blocks, int block_stride, int time_offset, int freq_offset, lq_mode_t mode) {
    return lq::lq_guess_snr(mag2, num_blocks, block_stride, time_offset, freq_offset, mode_to_proto(mode));
}

int lq_c_waterfall_extract_llrs(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, lq_mode_t mode, float out_llrs_174[174]) {
    return lq::lq_extract_llrs_from_waterfall(mag, num_blocks, block_stride, time_offset, freq_offset, mode_to_proto(mode), out_llrs_174) ? 1 : 0;
}

void lq_c_waterfall_subtract_signal(uint8_t* mag, int max_mag_size, int block_size, int block_stride, const uint8_t payload[10], float freq_hz, float time_sec, float sample_rate, lq_mode_t mode) {
    lq::lq_subtract_signal_from_waterfall(mag, max_mag_size, block_size, block_stride, payload, freq_hz, time_sec, sample_rate, mode_to_proto(mode));
}

} // extern "C"
