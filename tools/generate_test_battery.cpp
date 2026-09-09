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

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include "lq/lq.h"

namespace {

std::string escape_json(std::string_view s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string msg_type_to_str(lq::MessageType type) {
    switch (type) {
        case lq::MessageType::CQ_STD: return "CQ_STD";
        case lq::MessageType::CQ_NONSTD_1: return "CQ_NONSTD_1";
        case lq::MessageType::CQ_NONSTD_2: return "CQ_NONSTD_2";
        case lq::MessageType::CQ_NONSTD_3: return "CQ_NONSTD_3";
        case lq::MessageType::CALL_STD_NOSUF: return "CALL_STD_NOSUF";
        case lq::MessageType::CALL_STD_SUF: return "CALL_STD_SUF";
        case lq::MessageType::CALL_NONSTD: return "CALL_NONSTD";
        case lq::MessageType::REPORT73_STD: return "REPORT73_STD";
        case lq::MessageType::M73_STD: return "M73_STD";
        case lq::MessageType::M73_NONSTD: return "M73_NONSTD";
        case lq::MessageType::MULTI_REPORT73: return "MULTI_REPORT73";
        case lq::MessageType::MULTI_73: return "MULTI_73";
        case lq::MessageType::FREE_TEXT: return "FREE_TEXT";
        case lq::MessageType::RESERVED_A: return "RESERVED_A";
        case lq::MessageType::RESERVED_B: return "RESERVED_B";
        case lq::MessageType::RESERVED_C: return "RESERVED_C";
        default: return "UNKNOWN";
    }
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    std::string out_path = "tests/data/test_battery.json";
    if (argc > 1) {
        out_path = argv[1];
    }

    std::cout << "=================================================================\n";
    std::cout << "      LQ Reference Library — Test Battery Generator Tool         \n";
    std::cout << "=================================================================\n";
    std::cout << "Generating comprehensive test vectors to: " << out_path << "\n";

    std::ofstream out(out_path);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open output file " << out_path << "\n";
        return 1;
    }

    out << "{\n";
    out << "  \"version\": \"1.0\",\n";
    out << "  \"protocol\": \"The LQ Digital Mode Family (LQ8, LQ4, LQ2, LQ16)\",\n";

    out << "  \"generated_at\": \"2026-08-16T12:00:00Z\",\n";

    // ------------------------------------------------------------------------
    // SECTION 1: CODEC CONVERSIONS
    // ------------------------------------------------------------------------
    out << "  \"codec_conversions\": [\n";

    std::vector<lq::Message> test_messages;

    // 1. CQ Standard variants (Type 1)
    test_messages.push_back(lq::make_cq("HB9IPH", "JN47"));
    test_messages.push_back(lq::make_cq("HB9IPH/P", "JN47"));
    test_messages.push_back(lq::make_cq("HB9IPH", "JN47", "DX"));
    test_messages.push_back(lq::make_cq("HB9IPH", "JN47", "POTA"));
    test_messages.push_back(lq::make_cq("W1AW", "FN31", "SOTA"));
    test_messages.push_back(lq::make_cq("W1AW/P", "FN31", "SOTA"));
    test_messages.push_back(lq::make_cq("YO1YO", "KN24", "IOTA"));
    test_messages.push_back(lq::make_cq("VK3ABC", "QF22", "QRP"));
    test_messages.push_back(lq::make_cq("JA1ABC", "PM95", "NA"));
    test_messages.push_back(lq::make_cq("K1A", "FM18", "EU"));
    test_messages.push_back(lq::make_cq("9A1A", "JN75", "CONTEST"));
    test_messages.push_back(lq::make_cq("ZL1AA", "RF73", "20M"));
    test_messages.push_back(lq::make_cq("DL1ABC", "JO31", "FD"));

    // 2. CQ Non-Standard variants (Types 2, 3, 4)
    test_messages.push_back(lq::make_cq("3DA0TB", "KN24"));            // Type 2 (max 9 + loc)
    test_messages.push_back(lq::make_cq("W1AW/1", "FN31"));             // Type 2
    test_messages.push_back(lq::make_cq("EA6/HB9IP", "JM19"));          // Type 2
    test_messages.push_back(lq::make_cq("DL/HB9IP", "JN47"));           // Type 2
    test_messages.push_back(lq::make_cq("EA6/HB9IP", "", "SOTA"));      // Type 3 (max 9 + mod)
    test_messages.push_back(lq::make_cq("3B9/HB9IPH", "IH23"));         // Type 2
    test_messages.push_back(lq::make_cq("3DA0TB/P", "KG53"));           // Type 2 with /P
    test_messages.push_back(lq::make_cq("3B9/HB9IPH/P", ""));          // Type 4 (max 13)

    // 3. CALL Standard variants (Types 5, 6)
    test_messages.push_back(lq::make_call("YO1YO", "HB9IPH", "JN47", -3));
    test_messages.push_back(lq::make_call("HB9IPH", "TU2TU", "KL22", +5));
    test_messages.push_back(lq::make_call("W1AW", "DL1ABC", "JO31", -15));
    test_messages.push_back(lq::make_call("9A1A", "VK3ABC", "QF22", +2));
    test_messages.push_back(lq::make_call("K1A", "ZL1AA", "RF73", -26));
    test_messages.push_back(lq::make_call("JA1ABC", "YO1YO", "KN24", +5));
    test_messages.push_back(lq::make_call("YO1YO", "HB9IPH/P", "JN47", -3)); // Type 6

    // 4. CALL Non-Standard (Type 7)
    test_messages.push_back(lq::make_call("3B9/HB9IPH", "YO1YO", "KN24", -5));  // Type 7
    test_messages.push_back(lq::make_call("HB9IPH", "EA6/HB9IP", "JM19", +1));  // Type 7
    test_messages.push_back(lq::make_call("HB9IPH", "W1AW/P", "FN31", -10));     // Type 6
    test_messages.push_back(lq::make_call("HB9IPH/P", "W1AW", "FN31", -10));   // Type 6
    test_messages.push_back(lq::make_call("HB9IPH/P", "W1AW/P", "FN31", -10)); // Type 6
    test_messages.push_back(lq::make_call("3B9/HB9IPH", "EA6/HB9IP", "JM19", -8)); // Type 7

    // 5. REPORT73 Standard & Non-Standard (Types 8, 11)
    test_messages.push_back(lq::make_reply73("YO1YO", "HB9IPH", +5));
    test_messages.push_back(lq::make_reply73("YO1YO/P", "HB9IPH", +5));
    test_messages.push_back(lq::make_reply73("YO1YO", "HB9IPH/P", +5));
    test_messages.push_back(lq::make_reply73("YO1YO/P", "HB9IPH/P", +5));
    test_messages.push_back(lq::make_reply73("HB9IPH", "TU2TU", -3));
    test_messages.push_back(lq::make_reply73("W1AW", "DL1ABC", +2));
    test_messages.push_back(lq::make_reply73("3B9/HB9IPH", "YO1YO", +4)); // Type 11
    test_messages.push_back(lq::make_reply73("3B9/HB9IPH", "YO1YO/P", +4)); // Type 11
    test_messages.push_back(lq::make_reply73("3B9/HB9IPH", "EA6/HB9IP", -12)); // Type 11

    // 6. MULTI-REPORT73 (Type 11 with 16-bit DX hash)
    test_messages.push_back(lq::make_multi_reply73("HB9IPH", "YO1YO", +5, "TU2TU", -3));
    test_messages.push_back(lq::make_multi_reply73("W1AW", "K1A", +5, "9A1A", -1));

    // 7. FREE TEXT (Type 13)
    test_messages.push_back(lq::make_free_text("73 DE HB9IP"));
    test_messages.push_back(lq::make_free_text("TNX FOR QSO 73"));
    test_messages.push_back(lq::make_free_text("CQ POTA JN47"));
    test_messages.push_back(lq::make_free_text("TEST 123"));
    test_messages.push_back(lq::make_free_text("HELLO WORLD"));

    // 8. RESERVED Type 14
    {
        lq::Message r_msg;
        r_msg.type = lq::MessageType::RESERVED_A;
        r_msg.raw_payload = std::vector<uint8_t>{0xAA, 0x55, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x00, 0x00};
        test_messages.push_back(r_msg);
    }

    // Add multiple combinations with various grids and RSTs
    const std::vector<std::string> sample_calls = {
        "HB9IPH", "YO1YO", "TU2TU", "W1AW", "DL1ABC", "VK3ABC", "JA1ABC", "K1A", "9A1A", "ZL1AA",
        "EA6/HB9IP", "3B9/HB9IPH", "HB9IPH/QRP", "W1AW/P", "3DA0TB"
    };
    const std::vector<std::string> sample_grids = {"JN47", "KN24", "KL22", "FN31", "JO31", "QF22", "PM95", "FM18", "JN75", "RF73"};
    const std::vector<int> sample_rsts = {-26, -20, -15, -10, -5, -3, 0, +2, +5};

    for (size_t i = 0; i < sample_calls.size(); ++i) {
        for (size_t j = 0; j < sample_calls.size(); ++j) {
            if (i == j) continue;
            std::string c1 = sample_calls[i];
            std::string c2 = sample_calls[j];
            std::string grid = sample_grids[(i + j) % sample_grids.size()];
            int rst = sample_rsts[(i * 3 + j) % sample_rsts.size()];
            test_messages.push_back(lq::make_call(c1, c2, grid, rst));
            test_messages.push_back(lq::make_reply73(c1, c2, rst));
        }
    }

    size_t conv_count = 0;
    for (size_t idx = 0; idx < test_messages.size(); ++idx) {
        const auto& msg = test_messages[idx];
        uint8_t payload[lq::PAYLOAD_BYTES] = {0};
        if (!lq::encode_message(msg, payload)) continue;

        std::string hex_str = lq::payload_to_hex(payload);
        std::string bin_str = lq::payload_to_binary(payload);
        std::string formatted_text = lq::format_message(msg);
        uint16_t crc14 = lq::compute_crc14(payload, lq::PAYLOAD_BITS);

        if (conv_count > 0) out << ",\n";
        out << "    {\n";
        out << "      \"id\": \"CONV-" << std::setw(4) << std::setfill('0') << (conv_count + 1) << "\",\n";
        out << "      \"type_id\": " << static_cast<int>(msg.type) << ",\n";
        out << "      \"message_type\": \"" << msg_type_to_str(msg.type) << "\",\n";
        out << "      \"text\": \"" << escape_json(formatted_text) << "\",\n";
        out << "      \"hex\": \"" << hex_str << "\",\n";
        out << "      \"binary\": \"" << bin_str << "\",\n";
        out << "      \"crc14_hex\": \"0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << crc14 << std::dec << "\",\n";
        out << "      \"call_1\": \"" << escape_json(msg.call_1) << "\",\n";
        out << "      \"call_2\": \"" << escape_json(msg.call_2) << "\",\n";
        out << "      \"suffix_1\": " << static_cast<int>(msg.suffix_1) << ",\n";
        out << "      \"suffix_2\": " << static_cast<int>(msg.suffix_2) << ",\n";
        out << "      \"hash_1\": " << msg.hash_1 << ",\n";
        out << "      \"hash_2\": " << msg.hash_2 << ",\n";
        out << "      \"locator\": \"" << escape_json(msg.locator) << "\",\n";
        out << "      \"modifier\": \"" << escape_json(msg.modifier) << "\",\n";
        out << "      \"rst_db\": " << msg.rst_db << ",\n";
        out << "      \"free_text\": \"" << escape_json(msg.text) << "\"\n";
        out << "    }";
        ++conv_count;
    }
    out << "\n  ],\n";

    // ------------------------------------------------------------------------
    // SECTION 2: HASH VERIFICATIONS
    // ------------------------------------------------------------------------
    out << "  \"hash_verifications\": [\n";
    std::vector<std::string> hash_callsigns = {
        "HB9IPH", "YO1YO", "TU2TU", "W1AW", "DL1ABC", "VK3ABC", "JA1ABC", "K1A", "9A1A", "ZL1AA",
        "EA6/HB9IP", "3B9/HB9IPH", "HB9IPH/QRP", "W1AW/P", "3DA0TB", "K1JT", "K9AN", "G4WJS",
        "OE1AAA", "F6KBF", "OH2BH", "VE3NEA", "PY2XB", "ZS6BKW", "VK2AZ", "SM6EAN", "SP5CQI"
    };

    for (size_t idx = 0; idx < hash_callsigns.size(); ++idx) {
        const auto& call = hash_callsigns[idx];
        uint32_t h24 = lq::hash_callsign_24(call);
        uint32_t h20 = lq::hash_callsign_20(call);
        uint32_t h23 = lq::hash_callsign_23(call);
        uint32_t h22 = lq::hash_callsign_22(call);
        uint32_t h14 = lq::hash_callsign_14(call);
        uint32_t h12 = lq::hash_callsign_12(call);
        uint32_t h10 = lq::hash_callsign_10(call);

        if (idx > 0) out << ",\n";
        out << "    {\n";
        out << "      \"callsign\": \"" << escape_json(call) << "\",\n";
        out << "      \"hash_24\": " << h24 << ",\n";
        out << "      \"hash_20\": " << h20 << ",\n";
        out << "      \"hash_23\": " << h23 << ",\n";
        out << "      \"hash_22\": " << h22 << ",\n";
        out << "      \"hash_14\": " << h14 << ",\n";
        out << "      \"hash_12\": " << h12 << ",\n";
        out << "      \"hash_10\": " << h10 << "\n";
        out << "    }";
    }
    out << "\n  ],\n";

    // ------------------------------------------------------------------------
    // SECTION 3: INTENT SCENARIOS
    // ------------------------------------------------------------------------
    out << "  \"intent_scenarios\": [\n";

    struct ScenarioDef {
        std::string id;
        std::string desc;
        std::string my_call;
        std::string my_grid;
        std::vector<std::string> known_calls;
        std::vector<lq::ReceivedFrame> rx_frames;
        lq::UserIntent intent;
        std::string expected_tx_text;
        std::vector<std::string> expected_qso_completed;
    };

    std::vector<ScenarioDef> scenarios;

    // Scenario 1: Call CQ Standard
    {
        ScenarioDef sc;
        sc.id = "INTENT-001";
        sc.desc = "Operator initiates CQ standard with POTA modifier";
        sc.my_call = "HB9IPH";
        sc.my_grid = "JN47";
        sc.known_calls = {"HB9IPH"};
        sc.intent.action = lq::IntentAction::CALL_CQ;
        sc.intent.modifier = "POTA";
        sc.expected_tx_text = "CQ POTA HB9IPH JN47";
        scenarios.push_back(sc);
    }

    // Scenario 2: Answer CQ Heard in Slot
    {
        ScenarioDef sc;
        sc.id = "INTENT-002";
        sc.desc = "Operator calls YO1YO after hearing their CQ";
        sc.my_call = "HB9IPH";
        sc.my_grid = "JN47";
        sc.known_calls = {"HB9IPH", "YO1YO"};
        lq::ReceivedFrame rf;
        rf.msg = lq::make_cq("YO1YO", "KN24");
        rf.text = lq::format_message(rf.msg);
        rf.snr_db = -8;
        sc.rx_frames.push_back(rf);
        sc.intent.action = lq::IntentAction::CALL_STATION;
        sc.intent.target_call_1 = "YO1YO";
        sc.expected_tx_text = "YO1YO HB9IPH JN47 -08";
        scenarios.push_back(sc);
    }

    // Scenario 3: Single station reply
    {
        ScenarioDef sc;
        sc.id = "INTENT-003";
        sc.desc = "DX station replies to single caller HB9IPH with report +03";
        sc.my_call = "YO1YO";
        sc.my_grid = "KN24";
        sc.known_calls = {"YO1YO", "HB9IPH"};
        lq::ReceivedFrame rf;
        rf.msg = lq::make_call("YO1YO", "HB9IPH", "JN47", -8);
        rf.text = lq::format_message(rf.msg);
        rf.snr_db = +3;
        sc.rx_frames.push_back(rf);
        sc.intent.action = lq::IntentAction::REPLY_TO_STATIONS;
        sc.intent.target_call_1 = "HB9IPH";
        sc.expected_tx_text = "HB9IPH YO1YO R+03";
        sc.expected_qso_completed = {"HB9IPH"};
        scenarios.push_back(sc);
    }

    // Scenario 4: Reply to 2 stations via MULTI-REPLY73
    {
        ScenarioDef sc;
        sc.id = "INTENT-004";
        sc.desc = "DX station replies to 2 callers (YO1YO, TU2TU) via MULTI-REPLY73";
        sc.my_call = "HB9IPH";
        sc.my_grid = "JN47";
        sc.known_calls = {"HB9IPH", "YO1YO", "TU2TU"};
        lq::ReceivedFrame rf1;
        rf1.msg = lq::make_call("HB9IPH", "YO1YO", "KN24", -3);
        rf1.text = lq::format_message(rf1.msg);
        rf1.snr_db = +5;
        sc.rx_frames.push_back(rf1);
        lq::ReceivedFrame rf2;
        rf2.msg = lq::make_call("HB9IPH", "TU2TU", "KL22", -5);
        rf2.text = lq::format_message(rf2.msg);
        rf2.snr_db = -3;
        sc.rx_frames.push_back(rf2);
        sc.intent.action = lq::IntentAction::REPLY_TO_STATIONS;
        sc.intent.target_call_1 = "YO1YO";
        sc.intent.target_call_2 = "TU2TU";
        sc.expected_tx_text = "<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>";
        sc.expected_qso_completed = {"YO1YO", "TU2TU"};
        scenarios.push_back(sc);
    }

    // Scenario 5: Reply to 2 stations with custom signal reports
    {
        ScenarioDef sc;
        sc.id = "INTENT-005";
        sc.desc = "DX station replies to 2 callers (YO1YO, DL1ABC) with manual custom reports";
        sc.my_call = "HB9IPH";
        sc.my_grid = "JN47";
        sc.known_calls = {"HB9IPH", "YO1YO", "DL1ABC"};
        sc.intent.action = lq::IntentAction::REPLY_TO_STATIONS;
        sc.intent.target_call_1 = "YO1YO";
        sc.intent.target_call_2 = "DL1ABC";
        sc.intent.custom_rst_1 = +4;
        sc.intent.custom_rst_2 = -12;
        sc.expected_tx_text = "<YO1YO> R+04 <DL1ABC> R-12 <HB9IPH>";
        sc.expected_qso_completed = {"YO1YO", "DL1ABC"};
        scenarios.push_back(sc);
    }

    // Scenario 6: Calling station receives MULTI-REPLY73 and logs completed QSO
    {
        ScenarioDef sc;
        sc.id = "INTENT-006";
        sc.desc = "Station YO1YO receives MULTI-REPLY73 from HB9IPH and logs completed QSO";
        sc.my_call = "YO1YO";
        sc.my_grid = "KN24";
        sc.known_calls = {"YO1YO", "HB9IPH", "TU2TU"};
        lq::ReceivedFrame rf;
        rf.msg = lq::make_multi_reply73("HB9IPH", "YO1YO", +5, "TU2TU", -3);
        rf.text = lq::format_message(rf.msg);
        rf.snr_db = -4;
        sc.rx_frames.push_back(rf);
        sc.intent.action = lq::IntentAction::IDLE;
        sc.expected_tx_text = "";
        sc.expected_qso_completed = {"HB9IPH"};
        scenarios.push_back(sc);
    }

    // Scenario 7: Free text transmission
    {
        ScenarioDef sc;
        sc.id = "INTENT-007";
        sc.desc = "Operator sends free text message";
        sc.my_call = "HB9IPH";
        sc.my_grid = "JN47";
        sc.intent.action = lq::IntentAction::SEND_FREE_TEXT;
        sc.intent.free_text = "73 DE HB9IP";
        sc.expected_tx_text = "73 DE HB9IP";
        scenarios.push_back(sc);
    }

    for (size_t idx = 0; idx < scenarios.size(); ++idx) {
        const auto& sc = scenarios[idx];
        lq::IntentEngine engine(sc.my_call, sc.my_grid);
        engine.add_known_callsigns(sc.known_calls);

        lq::DecisionResult dec = engine.process_slot(sc.rx_frames, sc.intent);

        if (idx > 0) out << ",\n";
        out << "    {\n";
        out << "      \"id\": \"" << sc.id << "\",\n";
        out << "      \"description\": \"" << escape_json(sc.desc) << "\",\n";
        out << "      \"my_callsign\": \"" << sc.my_call << "\",\n";
        out << "      \"my_grid\": \"" << sc.my_grid << "\",\n";
        out << "      \"intent_action\": " << static_cast<int>(sc.intent.action) << ",\n";
        out << "      \"target_call_1\": \"" << escape_json(sc.intent.target_call_1) << "\",\n";
        out << "      \"target_call_2\": \"" << escape_json(sc.intent.target_call_2) << "\",\n";
        out << "      \"modifier\": \"" << escape_json(sc.intent.modifier) << "\",\n";
        out << "      \"free_text\": \"" << escape_json(sc.intent.free_text) << "\",\n";
        out << "      \"expected_tx_text\": \"" << escape_json(dec.tx_text) << "\",\n";
        out << "      \"expected_tx_hex\": \"" << dec.tx_hex << "\",\n";
        out << "      \"expected_tx_binary\": \"" << dec.tx_binary << "\",\n";
        out << "      \"expected_qso_completed\": [";
        for (size_t q = 0; q < dec.qso_completed_with.size(); ++q) {
            if (q > 0) out << ", ";
            out << "\"" << dec.qso_completed_with[q] << "\"";
        }
        out << "]\n";
        out << "    }";
    }
    out << "\n  ]\n";
    out << "}\n";

    out.close();
    std::cout << "✓ Generated " << conv_count << " codec conversions, "
              << hash_callsigns.size() << " hash vectors, and "
              << scenarios.size() << " intent scenarios into " << out_path << "\n";
    return 0;
}
