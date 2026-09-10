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

#ifndef LQ_INTENT_H
#define LQ_INTENT_H

#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <optional>
#include <cstdint>
#include "lq/types.h"
#include "lq/message.h"

namespace lq {

/**
 * User intent actions for automated and assisted QSO operation.
 */
enum class IntentAction : uint8_t {
    IDLE = 0,               ///< No transmission
    CALL_CQ = 1,            ///< Call CQ (with optional modifier/grid)
    CALL_STATION = 2,       ///< Call a specific station (e.g. answering a CQ)
    REPLY_TO_STATIONS = 3,  ///< Reply to 1 or 2 calling stations with reports and 73
    SEND_FREE_TEXT = 4      ///< Send free text message
};

/**
 * Represents a decoded frame received in a time slot.
 */
struct ReceivedFrame {
    std::string hex;          ///< 10-byte hex (e.g. "F8 87 79 0B DF 05 67 44 8B B8")
    std::string binary;       ///< 77-bit binary string ("0101...")
    std::string text;         ///< Standard formatted text ("YO1YO TU2TU KL22 -03")
    int snr_db = 0;           ///< Measured SNR in dB (-26..+5)
    float freq_hz = 1500.0f;  ///< Audio subcarrier frequency in Hz
    Message msg;              ///< Decoded Message struct
};

/**
 * Represents the operator or automated controller's intent for the next slot.
 */
struct UserIntent {
    IntentAction action = IntentAction::IDLE;
    std::string target_call_1;  ///< Primary target callsign
    std::string target_call_2;  ///< Secondary target callsign (if replying to 2 stations via MULTI-REPORT+73)
    std::string modifier;       ///< CQ modifier (e.g. "DX", "POTA", "SOTA")
    std::string free_text;      ///< Free-text string if action == SEND_FREE_TEXT
    int custom_rst_1 = 999;     ///< Explicit RST for target 1 (999 = use measured SNR from rx_frames)
    int custom_rst_2 = 999;     ///< Explicit RST for target 2 (999 = use measured SNR from rx_frames)
};

/**
 * The outcome of evaluating an intent against station context and received frames.
 */
struct DecisionResult {
    bool success = false;
    Message tx_message;                         ///< Generated message to transmit
    std::string tx_text;                        ///< Standard formatted text representation
    std::string tx_hex;                         ///< 10-byte hex representation
    std::string tx_binary;                      ///< 77-bit binary representation
    std::vector<std::string> qso_completed_with;///< Callsigns of stations whose QSO is completed
    std::string status_note;                    ///< Human-readable status description
};

/**
 * Deterministic Intent & Transceiver Decision Engine.
 * 
 * Provides standardized amateur radio decision logic for:
 * 1. Initiating CQ calls.
 * 2. Answering CQ calls with proper locator and SNR reports.
 * 3. Replying to single stations via standard or non-standard REPORT+73 (RPT73).
 * 4. Replying to 2 stations simultaneously via MULTI-REPORT+73 (RPT73M).
 * 5. Detecting and tracking completed contacts (QSO logging triggers).
 */
class IntentEngine {
public:
    explicit IntentEngine(std::string_view my_callsign = "HB9IPH",
                          std::string_view my_grid = "JN47");

    void set_my_callsign(std::string_view call);
    const std::string& get_my_callsign() const { return my_call_; }

    void set_my_grid(std::string_view grid);
    const std::string& get_my_grid() const { return my_grid_; }

    void add_known_callsign(std::string_view call);
    void add_known_callsigns(const std::vector<std::string>& calls);
    const std::vector<std::string>& get_known_callsigns() const { return known_calls_list_; }

    /**
     * Process a time slot's received frames and evaluate a UserIntent.
     * Generates the deterministic transmit message and updates QSO completion state.
     */
    DecisionResult process_slot(const std::vector<ReceivedFrame>& rx_frames,
                                const UserIntent& intent);

    /**
     * Check if a contact has been completed with the given callsign.
     */
    bool is_qso_completed(std::string_view call) const;

    /**
     * Get list of all completed callsigns.
     */
    const std::vector<std::string>& get_all_completed_qsos() const { return completed_qsos_list_; }

    /**
     * Track a station that has been called by this transceiver.
     * Restricts 16-bit DX dehashing in multi-station frames to prevent accidental hash collisions.
     * Automatically updates the last-called timestamp for TTL tracking.
     */
    void add_called_station(std::string_view call);

    /**
     * Remove an actively called station (e.g. upon QSO completion or cancellation).
     */
    void remove_called_station(std::string_view call);

    /**
     * Clear all actively called stations.
     */
    void clear_called_stations();

    /**
     * Get list of currently tracked called stations.
     */
    const std::vector<std::string>& get_called_stations() const { return called_stations_list_; }

    /**
     * Check whether a callsign is in the active called stations list and not expired.
     */
    bool has_called_station(std::string_view call) const;

    /**
     * Configure Time-to-Live (TTL) in seconds for called stations tracking (default: 1800s / 30m).
     * Set to 0 to disable TTL expiration.
     */
    void set_called_station_ttl_seconds(uint32_t ttl_sec) { called_station_ttl_sec_ = ttl_sec; }
    uint32_t get_called_station_ttl_seconds() const { return called_station_ttl_sec_; }

    /**
     * Set an optional simulated time point for deterministic testing of TTL and slot processing.
     * Pass std::nullopt (default) to use real steady_clock.
     */
    void set_simulated_time(std::optional<std::chrono::steady_clock::time_point> sim_time = std::nullopt) {
        simulated_time_ = sim_time;
    }
    std::optional<std::chrono::steady_clock::time_point> get_simulated_time() const {
        return simulated_time_;
    }

    /**
     * Evict called stations whose elapsed time since last call exceeds TTL.
     */
    void prune_expired_called_stations();
    void prune_expired_called_stations(std::chrono::steady_clock::time_point now);

    /**
     * Reset completed QSOs history.
     */
    void reset_completed_qsos();

private:
    std::string my_call_;
    std::string my_grid_;
    uint32_t my_h24_ = 0;
    uint32_t my_h16_ = 0;
    std::vector<std::string> known_calls_list_;
    std::unordered_set<std::string> known_calls_set_;
    std::unordered_map<uint32_t, std::string> h24_map_;
    std::unordered_map<uint32_t, std::string> h16_map_;
    std::vector<std::string> called_stations_list_;
    std::unordered_set<std::string> called_stations_set_;
    std::unordered_map<uint32_t, std::string> called_h16_map_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> called_stations_time_;
    uint32_t called_station_ttl_sec_ = 1800;
    std::optional<std::chrono::steady_clock::time_point> simulated_time_;
    std::unordered_set<std::string> completed_qsos_set_;
    std::vector<std::string> completed_qsos_list_;

    std::chrono::steady_clock::time_point get_now() const {
        return simulated_time_.value_or(std::chrono::steady_clock::now());
    }

    // Find measured SNR for a specific station from the received frames
    int find_measured_snr(const std::vector<ReceivedFrame>& rx_frames,
                          std::string_view target_call,
                          int fallback_rst = 0) const;

    // Check if a received frame completes our QSO with another station
    void check_incoming_qso_completions(const std::vector<ReceivedFrame>& rx_frames,
                                        std::vector<std::string>& out_completed);
};

} // namespace lq

#endif // LQ_INTENT_H
