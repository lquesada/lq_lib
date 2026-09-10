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

#include "lq/intent.h"
#include "lq/easy.h"
#include "lq/hash.h"
#include "lq/rst.h"
#include "lq/callsign.h"
#include "lq/callsign_nonstd.h"
#include <algorithm>


namespace lq {

IntentEngine::IntentEngine(std::string_view my_callsign, std::string_view my_grid)
    : my_call_(my_callsign), my_grid_(my_grid) {
    if (!my_call_.empty()) {
        my_h24_ = hash_callsign_24(my_call_);
        my_h16_ = hash_callsign_16(my_call_);
        add_known_callsign(my_call_);
    }
}

void IntentEngine::set_my_callsign(std::string_view call) {
    my_call_ = call;
    if (!my_call_.empty()) {
        my_h24_ = hash_callsign_24(my_call_);
        my_h16_ = hash_callsign_16(my_call_);
        add_known_callsign(my_call_);
    } else {
        my_h24_ = 0;
        my_h16_ = 0;
    }
}

void IntentEngine::set_my_grid(std::string_view grid) {
    my_grid_ = grid;
}

void IntentEngine::add_known_callsign(std::string_view call) {
    if (call.empty()) return;
    std::string s(call);
    if (known_calls_set_.find(s) == known_calls_set_.end()) {
        known_calls_set_.insert(s);
        known_calls_list_.push_back(s);
        uint32_t h24 = hash_callsign_24(s);
        uint32_t h16 = hash_callsign_16(s);
        h24_map_[h24] = s;
        h16_map_[h16] = s;
    }
}

void IntentEngine::add_known_callsigns(const std::vector<std::string>& calls) {
    for (const auto& c : calls) {
        add_known_callsign(c);
    }
}

void IntentEngine::add_called_station(std::string_view call) {
    if (call.empty()) return;
    std::string s(call);
    add_known_callsign(s);
    called_stations_time_[s] = get_now();
    if (called_stations_set_.find(s) == called_stations_set_.end()) {
        called_stations_set_.insert(s);
        called_stations_list_.push_back(s);
        uint32_t h16 = hash_callsign_16(s);
        called_h16_map_[h16] = s;
    }
}

void IntentEngine::remove_called_station(std::string_view call) {
    if (call.empty()) return;
    std::string s(call);
    auto it = called_stations_set_.find(s);
    if (it != called_stations_set_.end()) {
        called_stations_set_.erase(it);
        called_stations_list_.erase(
            std::remove(called_stations_list_.begin(), called_stations_list_.end(), s),
            called_stations_list_.end()
        );
        called_stations_time_.erase(s);
        uint32_t h16 = hash_callsign_16(s);
        called_h16_map_.erase(h16);
        for (const auto& remaining : called_stations_list_) {
            if (hash_callsign_16(remaining) == h16) {
                called_h16_map_[h16] = remaining;
                break;
            }
        }
    }
}

void IntentEngine::clear_called_stations() {
    called_stations_set_.clear();
    called_stations_list_.clear();
    called_h16_map_.clear();
    called_stations_time_.clear();
}

bool IntentEngine::has_called_station(std::string_view call) const {
    auto it = called_stations_time_.find(std::string(call));
    if (it == called_stations_time_.end()) return false;
    if (called_station_ttl_sec_ > 0) {
        auto now = get_now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
        if (elapsed_ms >= static_cast<int64_t>(called_station_ttl_sec_) * 1000) return false;
    }
    return true;
}

void IntentEngine::prune_expired_called_stations() {
    prune_expired_called_stations(get_now());
}

void IntentEngine::prune_expired_called_stations(std::chrono::steady_clock::time_point now) {
    if (called_station_ttl_sec_ == 0) return;
    std::vector<std::string> expired;
    int64_t ttl_ms = static_cast<int64_t>(called_station_ttl_sec_) * 1000;
    for (const auto& pair : called_stations_time_) {
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - pair.second).count();
        if (elapsed_ms >= ttl_ms) {
            expired.push_back(pair.first);
        }
    }
    for (const auto& call : expired) {
        remove_called_station(call);
    }
}

int IntentEngine::find_measured_snr(const std::vector<ReceivedFrame>& rx_frames,
                                    std::string_view target_call,
                                    int fallback_rst) const {
    if (target_call.empty()) return fallback_rst;
    uint32_t h24 = hash_callsign_24(target_call);
    uint32_t h16 = hash_callsign_16(target_call);

    for (const auto& f : rx_frames) {
        const Message& m = f.msg;
        if (m.call_1 == target_call || m.call_2 == target_call) {
            return f.snr_db;
        }
        if (m.get_sender_call() == target_call || m.get_target_call() == target_call) {
            return f.snr_db;
        }
        if (m.type == MessageType::MULTI_REPORT73 || m.type == MessageType::MULTI_73) {
            if (m.hash_1 == h16) return f.snr_db;
            for (const auto& tgt : m.multi_targets) {
                if (tgt.call == target_call || tgt.hash == h24) return f.snr_db;
            }
        } else if (m.type == MessageType::CALL_NONSTD) {
            uint32_t h20 = hash_callsign_20(target_call);
            if (m.hash_1 == h20) {
                return f.snr_db;
            }
        } else {
            if (m.hash_1 == h24 || m.hash_2 == h24) {
                return f.snr_db;
            }
        }
    }
    return fallback_rst;
}

void IntentEngine::check_incoming_qso_completions(const std::vector<ReceivedFrame>& rx_frames,
                                                  std::vector<std::string>& out_completed) {
    if (my_call_.empty()) return;
    uint32_t my_h24 = my_h24_;

    for (const auto& f : rx_frames) {
        const Message& m = f.msg;

        if (m.is_multi_report73() || m.is_multi_73()) {
            // Check if we are one of the targets in MULTI-REPORT+73 / MULTI-73
            for (const auto& t : m.multi_targets) {
                if (t.call == my_call_ || t.hash == my_h24) {
                    // DX replied to us!
                    std::string dx_station = m.call_1;
                    if ((dx_station.empty() || dx_station.front() == '<') && m.hash_1 != 0) {
                        // Scoped 16-bit DX Dehashing:
                        // Restrict 16-bit DX hash resolution to stations actively called by this transceiver.
                        // If no called stations are registered, fallback to the general heard cache.
                        if (!called_stations_set_.empty()) {
                            auto it16 = called_h16_map_.find(m.hash_1);
                            if (it16 != called_h16_map_.end()) {
                                dx_station = it16->second;
                            } else {
                                // 16-bit hash does not match any station we called -> reject as collision
                                break;
                            }
                        } else {
                            auto it16 = h16_map_.find(m.hash_1);
                            if (it16 != h16_map_.end()) {
                                dx_station = it16->second;
                            }
                        }
                    } else if (!m.call_1.empty() && m.call_1.front() != '<') {
                        // If DX station was cleartext or already resolved, verify against called stations if active
                        if (!called_stations_set_.empty() && called_stations_set_.find(m.call_1) == called_stations_set_.end()) {
                            break;
                        }
                    }
                    if (!dx_station.empty() && dx_station.front() != '<' && completed_qsos_set_.find(dx_station) == completed_qsos_set_.end()) {
                        completed_qsos_set_.insert(dx_station);
                        completed_qsos_list_.push_back(dx_station);
                        out_completed.push_back(dx_station);
                    }
                    if (!dx_station.empty() && dx_station.front() != '<') {
                        remove_called_station(dx_station);
                    }
                    break;
                }
            }
        } else if (m.is_report73() || m.is_73()) {
            // Check if we are the target of this REPORT+73 or 73
            bool directed_to_me = false;
            if ((m.type == MessageType::REPORT73_STD || m.type == MessageType::M73_STD) && m.call_1 == my_call_) {
                directed_to_me = true;
            } else if (m.type == MessageType::M73_NONSTD) {
                if (m.call_1 == my_call_) {
                    directed_to_me = true;
                } else if (m.hash_1 == my_h24) {
                    // Canonical Precedence rule: If my_call_ is standard AND m.call_2 is standard,
                    // the sender was required to use M73_STD, so this hash match cannot be for me.
                    std::string base_my, base_remote;
                    uint8_t suf_my = 0, suf_remote = 0;
                    bool my_is_std = parse_standard_callsign_suffix(my_call_, base_my, suf_my);
                    bool remote_is_std = !m.call_2.empty() && parse_standard_callsign_suffix(m.call_2, base_remote, suf_remote);

                    if (!(my_is_std && remote_is_std)) {
                        directed_to_me = true;
                    }
                }
            }

            if (directed_to_me) {
                std::string remote_station = m.call_2;
                if ((remote_station.empty() || remote_station.front() == '<') && m.hash_2 != 0) {
                    auto it24 = h24_map_.find(m.hash_2);
                    if (it24 != h24_map_.end()) {
                        remote_station = it24->second;
                    }
                }
                if (!remote_station.empty() && remote_station.front() != '<' && completed_qsos_set_.find(remote_station) == completed_qsos_set_.end()) {
                    completed_qsos_set_.insert(remote_station);
                    completed_qsos_list_.push_back(remote_station);
                    out_completed.push_back(remote_station);
                }
                if (!remote_station.empty() && remote_station.front() != '<') {
                    remove_called_station(remote_station);
                }
            }
        }

    }
}

DecisionResult IntentEngine::process_slot(const std::vector<ReceivedFrame>& rx_frames,
                                          const UserIntent& intent) {
    DecisionResult result;

    // 0. Prune expired called stations based on TTL
    prune_expired_called_stations();

    // 1. Ingest any callsigns present in received frames into our known calls database
    for (const auto& f : rx_frames) {
        if (!f.msg.call_1.empty()) add_known_callsign(f.msg.call_1);
        if (!f.msg.call_2.empty()) add_known_callsign(f.msg.call_2);
        for (const auto& t : f.msg.multi_targets) {
            if (!t.call.empty()) add_known_callsign(t.call);
        }
    }

    // 2. Check for incoming QSO completions
    check_incoming_qso_completions(rx_frames, result.qso_completed_with);

    // 3. Process the desired user intent
    switch (intent.action) {
        case IntentAction::IDLE: {
            result.success = true;
            result.status_note = "Idle (RX only)";
            break;
        }

        case IntentAction::CALL_CQ: {
            result.tx_message = make_cq(my_call_, my_grid_, intent.modifier);
            uint8_t payload[PAYLOAD_BYTES] = {0};
            if (encode_message(result.tx_message, payload)) {
                result.tx_text = format_message(result.tx_message);
                result.tx_hex = payload_to_hex(payload);
                result.tx_binary = payload_to_binary(payload);
                result.success = true;
                result.status_note = "Calling CQ";
            }
            break;
        }

        case IntentAction::CALL_STATION: {
            if (intent.target_call_1.empty()) {
                result.success = false;
                result.status_note = "Error: target_call_1 required for CALL_STATION";
                return result;
            }
            add_called_station(intent.target_call_1);

            int rst = (intent.custom_rst_1 != 999) ? intent.custom_rst_1
                                                   : find_measured_snr(rx_frames, intent.target_call_1, -5);
            rst = std::clamp(rst, RST_MIN_DB, RST_MAX_DB);

            result.tx_message = make_call(intent.target_call_1, my_call_, my_grid_, rst);
            uint8_t payload[PAYLOAD_BYTES] = {0};
            if (encode_message(result.tx_message, payload)) {
                result.tx_text = format_message(result.tx_message);
                result.tx_hex = payload_to_hex(payload);
                result.tx_binary = payload_to_binary(payload);
                result.success = true;
                result.status_note = "Calling " + intent.target_call_1;
            }
            break;
        }

        case IntentAction::REPLY_TO_STATIONS: {
            if (intent.target_call_1.empty()) {
                result.success = false;
                result.status_note = "Error: At least 1 target required for REPLY_TO_STATIONS";
                return result;
            }

            add_known_callsign(intent.target_call_1);
            int rst1 = (intent.custom_rst_1 != 999) ? intent.custom_rst_1
                                                    : find_measured_snr(rx_frames, intent.target_call_1, 0);
            rst1 = std::clamp(rst1, RST_MIN_DB, RST_MAX_DB);

            if (intent.target_call_2.empty()) {
                // Single station reply -> Standard or non-standard REPORT+73
                result.tx_message = make_report73(intent.target_call_1, my_call_, rst1);
                uint8_t payload[PAYLOAD_BYTES] = {0};
                if (encode_message(result.tx_message, payload)) {
                    result.tx_text = format_message(result.tx_message);
                    result.tx_hex = payload_to_hex(payload);
                    result.tx_binary = payload_to_binary(payload);
                    result.success = true;
                    result.status_note = "Sent REPORT+73 to " + intent.target_call_1;

                    // Record completed QSO with target 1 on our end
                    if (completed_qsos_set_.find(intent.target_call_1) == completed_qsos_set_.end()) {
                        completed_qsos_set_.insert(intent.target_call_1);
                        completed_qsos_list_.push_back(intent.target_call_1);
                        result.qso_completed_with.push_back(intent.target_call_1);
                    }
                    remove_called_station(intent.target_call_1);
                }
            } else {
                // Two stations reply -> MULTI-REPORT+73 dual confirmation
                add_known_callsign(intent.target_call_2);
                int rst2 = (intent.custom_rst_2 != 999) ? intent.custom_rst_2
                                                        : find_measured_snr(rx_frames, intent.target_call_2, 0);
                rst2 = std::clamp(rst2, RST_MIN_DB, RST_MAX_DB);

                std::vector<MultiTarget> targets;
                targets.push_back({intent.target_call_1, hash_callsign_24(intent.target_call_1), rst1});
                targets.push_back({intent.target_call_2, hash_callsign_24(intent.target_call_2), rst2});

                result.tx_message = make_multi_report73(my_call_, targets);
                uint8_t payload[PAYLOAD_BYTES] = {0};
                if (encode_message(result.tx_message, payload)) {
                    result.tx_text = format_message(result.tx_message);
                    result.tx_hex = payload_to_hex(payload);
                    result.tx_binary = payload_to_binary(payload);
                    result.success = true;
                    result.status_note = "Sent MULTI-REPORT+73 to " + intent.target_call_1 + " and " + intent.target_call_2;

                    // Record completed QSOs
                    for (const auto& c : {intent.target_call_1, intent.target_call_2}) {
                        if (completed_qsos_set_.find(c) == completed_qsos_set_.end()) {
                            completed_qsos_set_.insert(c);
                            completed_qsos_list_.push_back(c);
                            result.qso_completed_with.push_back(c);
                        }
                        remove_called_station(c);
                    }
                }
            }
            break;
        }

        case IntentAction::SEND_FREE_TEXT: {
            result.tx_message = make_free_text(intent.free_text);
            uint8_t payload[PAYLOAD_BYTES] = {0};
            if (encode_message(result.tx_message, payload)) {
                result.tx_text = format_message(result.tx_message);
                result.tx_hex = payload_to_hex(payload);
                result.tx_binary = payload_to_binary(payload);
                result.success = true;
                result.status_note = "Sent free text: " + intent.free_text;
            }
            break;
        }
    }

    return result;
}

bool IntentEngine::is_qso_completed(std::string_view call) const {
    return completed_qsos_set_.find(std::string(call)) != completed_qsos_set_.end();
}

void IntentEngine::reset_completed_qsos() {
    completed_qsos_set_.clear();
    completed_qsos_list_.clear();
}

} // namespace lq
