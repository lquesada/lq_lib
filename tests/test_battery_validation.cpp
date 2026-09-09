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

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "lq/lq.h"

class TestBatteryValidation : public ::testing::Test {
protected:
    void SetUp() override {}
};

// -----------------------------------------------------------------------------
// 1. Direct Conversion Utilities (C++, Easy API & C ABI)
// -----------------------------------------------------------------------------
TEST_F(TestBatteryValidation, DirectHexAndBinaryConversionAPIs) {
    std::vector<std::string> test_texts = {
        "CQ HB9IPH JN47",
        "CQ POTA HB9IPH JN47",
        "CALL YO1YO HB9IPH JN47 -03",
        "REPLY73 YO1YO HB9IPH +05",
        "MULTI-REPLY73 <1ea2> <1717> +05 <18d7> -03 <3df1> +02",
        "73 DE HB9IP"
    };

    for (const auto& text : test_texts) {
        // C++ API
        std::string hex_str = lq::text_to_hex(text);
        EXPECT_FALSE(hex_str.empty()) << "Failed text_to_hex for: " << text;

        std::string bin_str = lq::text_to_binary(text);
        EXPECT_EQ(bin_str.size(), 77u) << "Binary string length mismatch for: " << text;

        std::string text_from_hex = lq::hex_to_text(hex_str);
        EXPECT_FALSE(text_from_hex.empty());

        std::string text_from_bin = lq::binary_to_text(bin_str);
        EXPECT_FALSE(text_from_bin.empty());

        // C ABI
        char c_hex[64] = {0};
        int rc = lq_c_text_to_hex(text.c_str(), c_hex, sizeof(c_hex), 1);
        EXPECT_EQ(rc, 1);
        EXPECT_STREQ(c_hex, hex_str.c_str());

        char c_bin[128] = {0};
        rc = lq_c_text_to_binary(text.c_str(), c_bin, sizeof(c_bin));
        EXPECT_EQ(rc, 1);
        EXPECT_STREQ(c_bin, bin_str.c_str());

        char c_text_from_hex[128] = {0};
        rc = lq_c_hex_to_text(c_hex, c_text_from_hex, sizeof(c_text_from_hex), nullptr, 0);
        EXPECT_EQ(rc, 1);

        char c_text_from_bin[128] = {0};
        rc = lq_c_binary_to_text(c_bin, c_text_from_bin, sizeof(c_text_from_bin), nullptr, 0);
        EXPECT_EQ(rc, 1);
    }
}

// -----------------------------------------------------------------------------
// 2. Deterministic Intent & Multi-Slot Decision Scenarios
// -----------------------------------------------------------------------------
TEST_F(TestBatteryValidation, IntentScenarioCallCQ) {
    lq::IntentEngine engine("HB9IPH", "JN47");
    lq::UserIntent intent;
    intent.action = lq::IntentAction::CALL_CQ;
    intent.modifier = "POTA";

    std::vector<lq::ReceivedFrame> rx_frames;
    lq::DecisionResult dec = engine.process_slot(rx_frames, intent);

    EXPECT_TRUE(dec.success);
    EXPECT_EQ(dec.tx_text, "CQ POTA HB9IPH JN47");
    EXPECT_FALSE(dec.tx_hex.empty());
    EXPECT_EQ(dec.tx_binary.size(), 77u);
    EXPECT_TRUE(dec.qso_completed_with.empty());
}

TEST_F(TestBatteryValidation, IntentScenarioAnswerCQ) {
    lq::IntentEngine engine("HB9IPH", "JN47");

    // Receive CQ from YO1YO
    lq::ReceivedFrame rx_cq;
    rx_cq.msg = lq::make_cq("YO1YO", "KN24");
    rx_cq.text = lq::format_message(rx_cq.msg);
    rx_cq.snr_db = -8;

    lq::UserIntent intent;
    intent.action = lq::IntentAction::CALL_STATION;
    intent.target_call_1 = "YO1YO";

    lq::DecisionResult dec = engine.process_slot({rx_cq}, intent);

    EXPECT_TRUE(dec.success);
    EXPECT_EQ(dec.tx_text, "YO1YO HB9IPH JN47 -08");
    EXPECT_FALSE(dec.tx_hex.empty());
    EXPECT_TRUE(dec.qso_completed_with.empty());
}

TEST_F(TestBatteryValidation, IntentScenarioSingleStationReply) {
    lq::IntentEngine engine("YO1YO", "KN24");

    // Receive CALL from HB9IPH
    lq::ReceivedFrame rx_call;
    rx_call.msg = lq::make_call("YO1YO", "HB9IPH", "JN47", -8);
    rx_call.text = lq::format_message(rx_call.msg);
    rx_call.snr_db = +3;

    lq::UserIntent intent;
    intent.action = lq::IntentAction::REPLY_TO_STATIONS;
    intent.target_call_1 = "HB9IPH";

    lq::DecisionResult dec = engine.process_slot({rx_call}, intent);

    EXPECT_TRUE(dec.success);
    EXPECT_EQ(dec.tx_text, "HB9IPH YO1YO R+03");
    EXPECT_EQ(dec.qso_completed_with.size(), 1u);

    EXPECT_EQ(dec.qso_completed_with[0], "HB9IPH");
    EXPECT_TRUE(engine.is_qso_completed("HB9IPH"));
}

TEST_F(TestBatteryValidation, IntentScenarioTwoStationMultiReplyRepeatedField3) {
    lq::IntentEngine engine("HB9IPH", "JN47");

    // Receive calls from 2 stations
    lq::ReceivedFrame rx1;
    rx1.msg = lq::make_call("HB9IPH", "YO1YO", "KN24", -3);
    rx1.text = lq::format_message(rx1.msg);
    rx1.snr_db = +5;

    lq::ReceivedFrame rx2;
    rx2.msg = lq::make_call("HB9IPH", "TU2TU", "KL22", -5);
    rx2.text = lq::format_message(rx2.msg);
    rx2.snr_db = -3;

    lq::UserIntent intent;
    intent.action = lq::IntentAction::REPLY_TO_STATIONS;
    intent.target_call_1 = "YO1YO";
    intent.target_call_2 = "TU2TU";

    lq::DecisionResult dec = engine.process_slot({rx1, rx2}, intent);

    EXPECT_TRUE(dec.success);
    EXPECT_EQ(dec.tx_message.type, lq::MessageType::MULTI_REPORT73);
    ASSERT_EQ(dec.tx_message.multi_targets.size(), 2u);

    EXPECT_EQ(dec.tx_message.multi_targets[0].call, "YO1YO");
    EXPECT_EQ(dec.tx_message.multi_targets[0].rst_db, +5);

    EXPECT_EQ(dec.tx_message.multi_targets[1].call, "TU2TU");
    EXPECT_EQ(dec.tx_message.multi_targets[1].rst_db, -3);

    EXPECT_EQ(dec.qso_completed_with.size(), 2u);
    EXPECT_TRUE(engine.is_qso_completed("YO1YO"));
    EXPECT_TRUE(engine.is_qso_completed("TU2TU"));
}

TEST_F(TestBatteryValidation, IntentScenarioThreeStationQueueMultiReply) {
    lq::IntentEngine engine("HB9IPH", "JN47");

    lq::ReceivedFrame rx1;
    rx1.msg = lq::make_call("HB9IPH", "YO1YO", "KN24", -3);
    rx1.text = lq::format_message(rx1.msg);
    rx1.snr_db = +5;

    lq::ReceivedFrame rx2;
    rx2.msg = lq::make_call("HB9IPH", "TU2TU", "KL22", -5);
    rx2.text = lq::format_message(rx2.msg);
    rx2.snr_db = -3;

    lq::ReceivedFrame rx3;
    rx3.msg = lq::make_call("HB9IPH", "DL1ABC", "JO31", -2);
    rx3.text = lq::format_message(rx3.msg);
    rx3.snr_db = +2;

    lq::UserIntent intent;
    intent.action = lq::IntentAction::REPLY_TO_STATIONS;
    intent.target_call_1 = "YO1YO";
    intent.target_call_2 = "TU2TU";

    lq::DecisionResult dec = engine.process_slot({rx1, rx2, rx3}, intent);

    EXPECT_TRUE(dec.success);
    EXPECT_EQ(dec.tx_message.type, lq::MessageType::MULTI_REPORT73);
    ASSERT_EQ(dec.tx_message.multi_targets.size(), 2u);

    EXPECT_EQ(dec.tx_message.multi_targets[0].call, "YO1YO");
    EXPECT_EQ(dec.tx_message.multi_targets[1].call, "TU2TU");

    EXPECT_EQ(dec.qso_completed_with.size(), 2u);
    EXPECT_TRUE(engine.is_qso_completed("YO1YO"));
    EXPECT_TRUE(engine.is_qso_completed("TU2TU"));
}

TEST_F(TestBatteryValidation, IntentScenarioCallerReceivesMultiReplyQSOCompletion) {
    lq::IntentEngine engine("YO1YO", "KN24");
    engine.add_known_callsigns({"HB9IPH", "TU2TU", "DL1ABC"});

    EXPECT_EQ(engine.get_my_callsign(), "YO1YO");
    EXPECT_EQ(engine.get_my_grid(), "KN24");
    EXPECT_FALSE(engine.get_known_callsigns().empty());

    // DX station sends MULTI_REPLY73 confirming YO1YO
    lq::ReceivedFrame rx_multi;
    rx_multi.msg = lq::make_multi_reply73("HB9IPH", "YO1YO", +5, "TU2TU", -3);
    rx_multi.text = lq::format_message(rx_multi.msg);
    rx_multi.snr_db = -4;

    lq::UserIntent intent;
    intent.action = lq::IntentAction::IDLE;

    lq::DecisionResult dec = engine.process_slot({rx_multi}, intent);

    EXPECT_TRUE(dec.success);
    EXPECT_EQ(dec.qso_completed_with.size(), 1u);
    EXPECT_EQ(dec.qso_completed_with[0], "HB9IPH");
    EXPECT_TRUE(engine.is_qso_completed("HB9IPH"));
    EXPECT_EQ(engine.get_all_completed_qsos().size(), 1u);

    engine.reset_completed_qsos();
    EXPECT_FALSE(engine.is_qso_completed("HB9IPH"));
    EXPECT_TRUE(engine.get_all_completed_qsos().empty());
}

TEST_F(TestBatteryValidation, IntentScenarioFreeTextAndNonStdReplies) {
    lq::IntentEngine engine("HB9IPH", "JN47");
    engine.set_my_callsign("3B9/HB9IPH");
    engine.set_my_grid("IH23");
    EXPECT_EQ(engine.get_my_callsign(), "3B9/HB9IPH");
    EXPECT_EQ(engine.get_my_grid(), "IH23");

    // Free text intent
    lq::UserIntent ft_intent;
    ft_intent.action = lq::IntentAction::SEND_FREE_TEXT;
    ft_intent.free_text = "73 DE HB9IP";
    lq::DecisionResult ft_dec = engine.process_slot({}, ft_intent);
    EXPECT_TRUE(ft_dec.success);
    EXPECT_EQ(ft_dec.tx_text, "73 DE HB9IP");

    // Reply with non-standard callsign
    lq::ReceivedFrame rx_nonstd;
    rx_nonstd.msg = lq::make_reply73("3B9/HB9IPH", "YO1YO", +4); // Type 10
    rx_nonstd.text = lq::format_message(rx_nonstd.msg);
    rx_nonstd.snr_db = +2;

    lq::UserIntent idle_intent;
    idle_intent.action = lq::IntentAction::IDLE;
    lq::DecisionResult idle_dec = engine.process_slot({rx_nonstd}, idle_intent);
    EXPECT_TRUE(idle_dec.success);
    EXPECT_EQ(idle_dec.qso_completed_with.size(), 1u);
    EXPECT_EQ(idle_dec.qso_completed_with[0], "YO1YO");

    // Error handling: missing targets
    lq::UserIntent err_call;
    err_call.action = lq::IntentAction::CALL_STATION;
    lq::DecisionResult err_dec1 = engine.process_slot({}, err_call);
    EXPECT_FALSE(err_dec1.success);

    lq::UserIntent err_reply;
    err_reply.action = lq::IntentAction::REPLY_TO_STATIONS;
    lq::DecisionResult err_dec2 = engine.process_slot({}, err_reply);
    EXPECT_FALSE(err_dec2.success);
}

TEST_F(TestBatteryValidation, CAPIConversionErrorBranches) {
    // Null pointer checks
    uint8_t payload[10] = {0};
    char buf[128];
    EXPECT_EQ(lq_c_payload_to_hex(nullptr, buf, sizeof(buf), 1), 0);
    EXPECT_EQ(lq_c_payload_to_hex(payload, nullptr, sizeof(buf), 1), 0);
    EXPECT_EQ(lq_c_payload_to_hex(payload, buf, 0, 1), 0);

    EXPECT_EQ(lq_c_payload_to_binary(nullptr, buf, sizeof(buf)), 0);
    EXPECT_EQ(lq_c_payload_to_binary(payload, nullptr, sizeof(buf)), 0);
    EXPECT_EQ(lq_c_payload_to_binary(payload, buf, 0), 0);

    EXPECT_EQ(lq_c_hex_to_payload(nullptr, payload), 0);
    EXPECT_EQ(lq_c_hex_to_payload("invalid", nullptr), 0);
    EXPECT_EQ(lq_c_hex_to_payload("short", payload), 0);

    EXPECT_EQ(lq_c_binary_to_payload(nullptr, payload), 0);
    EXPECT_EQ(lq_c_binary_to_payload("1010", nullptr), 0);
    EXPECT_EQ(lq_c_binary_to_payload("", payload), 0);

    EXPECT_EQ(lq_c_hex_to_text(nullptr, buf, sizeof(buf), nullptr, 0), 0);
    EXPECT_EQ(lq_c_hex_to_text("invalid", nullptr, sizeof(buf), nullptr, 0), 0);
    EXPECT_EQ(lq_c_hex_to_text("invalid", buf, 0, nullptr, 0), 0);

    EXPECT_EQ(lq_c_binary_to_text(nullptr, buf, sizeof(buf), nullptr, 0), 0);
    EXPECT_EQ(lq_c_binary_to_text("1010", nullptr, sizeof(buf), nullptr, 0), 0);
    EXPECT_EQ(lq_c_binary_to_text("1010", buf, 0, nullptr, 0), 0);

    EXPECT_EQ(lq_c_text_to_hex(nullptr, buf, sizeof(buf), 1), 0);
    EXPECT_EQ(lq_c_text_to_hex("invalid", nullptr, sizeof(buf), 1), 0);
    EXPECT_EQ(lq_c_text_to_hex("invalid", buf, 0, 1), 0);

    EXPECT_EQ(lq_c_text_to_binary(nullptr, buf, sizeof(buf)), 0);
    EXPECT_EQ(lq_c_text_to_binary("invalid", nullptr, sizeof(buf)), 0);
    EXPECT_EQ(lq_c_text_to_binary("invalid", buf, 0), 0);
}

// -----------------------------------------------------------------------------
// 3. Exhaustive Battery JSON File Direct Verification
// -----------------------------------------------------------------------------

TEST_F(TestBatteryValidation, ExhaustiveBatteryJsonFileRoundtripAndIntentVerification) {
    std::string path = "tests/data/test_battery.json";
    std::ifstream file(path);
    if (!file.is_open()) {
        path = "../tests/data/test_battery.json";
        file.open(path);
    }
    ASSERT_TRUE(file.is_open()) << "test_battery.json must exist in tests/data/";

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    EXPECT_GT(json.size(), 10000u);

    // Verify key sections exist
    EXPECT_NE(json.find("\"codec_conversions\""), std::string::npos);
    EXPECT_NE(json.find("\"hash_verifications\""), std::string::npos);
    EXPECT_NE(json.find("\"intent_scenarios\""), std::string::npos);

    // Simple parser to count and verify entries directly in C++
    size_t count_conversions = 0;
    size_t pos = 0;
    while ((pos = json.find("\"hex\": \"", pos)) != std::string::npos) {
        pos += 8;
        size_t end = json.find("\"", pos);
        if (end != std::string::npos) {
            std::string hex_str = json.substr(pos, end - pos);
            uint8_t payload[10] = {0};
            EXPECT_TRUE(lq::hex_to_payload(hex_str, payload));
            std::string re_hex = lq::payload_to_hex(payload);
            EXPECT_EQ(re_hex, hex_str);
            ++count_conversions;
            pos = end;
        }
    }
    EXPECT_GT(count_conversions, 400u);

    size_t count_hashes = 0;
    pos = 0;
    while ((pos = json.find("\"hash_24\": ", pos)) != std::string::npos) {
        ++count_hashes;
        pos += 11;
    }
    EXPECT_GE(count_hashes, 25u);

    size_t count_hashes_23 = 0;
    pos = 0;
    while ((pos = json.find("\"hash_23\": ", pos)) != std::string::npos) {
        ++count_hashes_23;
        pos += 11;
    }
    EXPECT_GE(count_hashes_23, 25u);
}
