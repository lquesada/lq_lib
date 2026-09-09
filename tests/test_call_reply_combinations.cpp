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
#include "lq/lq.h"
#include "lq/easy.h"
#include "lq/message.h"
#include "lq/hash.h"
#include "lq/callsign.h"
#include "lq/locator.h"
#include "lq/rst.h"
#include "lq/crc.h"
#include "lq/ldpc.h"
#include "lq/transport.h"
#include <vector>
#include <string>
#include <map>
#include <cmath>

using namespace lq;

namespace {

// Standard callsigns verified by is_standard_callsign()
const std::vector<std::string> STANDARD_CALLSIGNS = {
    "HB9IPH", "W1AW", "YO1YO", "TU2TU", "K1ABC", "VK2ABC",
    "ZL1ABC", "ZS6BKW", "PY2XYZ", "JA1ABC", "G4ABC", "F6ABC",
    "OH2ZZ", "SV1AA", "EA3XYZ", "VE3ABC", "DL1ABC", "BA4AA"
};

// Non-standard callsigns verified by !is_standard_callsign() (base <= 9 chars for Type 7/10 caller field)
const std::vector<std::string> NONSTANDARD_CALLSIGNS = {
    "3B9/HB9IP", "EA8/HB9IP", "3B9/HB9IP/P", "EA8/YO1YO",
    "HB9/K1ABC", "DL/ON4XYZ", "GB100BBC", "3DA0RU", "VP8/HB9IP",
    "HB9IPH/R", "VI100AIR", "EA/HB9IP", "TF/F6ABC/P", "9A/YO1YO"
};

const std::vector<std::string> TEST_LOCATORS = {
    "JN47", "FN31", "KL22", "IL18", "RE48", "AA00", "RR99", "IO91", "QF22"
};

// Valid 5-bit RST range: -26 to +5 dB
const std::vector<int> TEST_RSTS = {
    -26, -24, -20, -15, -10, -5, -3, 0, +1, +3, +5
};

} // namespace

// =============================================================================
// 1. Validation of Test Sets (Standard vs Non-Standard)
// =============================================================================

TEST(CallReplyCombinationsTest, VerifyCallsignClassification) {
    for (const auto& call : STANDARD_CALLSIGNS) {
        EXPECT_TRUE(is_standard_callsign(call)) << "Expected standard callsign: " << call;
    }
    for (const auto& call : NONSTANDARD_CALLSIGNS) {
        EXPECT_FALSE(is_standard_callsign(call)) << "Expected non-standard callsign: " << call;
    }
}

// =============================================================================
// 2. Comprehensive Permutation Tests for Message Types 6, 7, 8, 10, 11
// =============================================================================

TEST(CallReplyCombinationsTest, NonStdTargetToStdCaller_Type6_And_Type11) {
    for (const auto& target_call : NONSTANDARD_CALLSIGNS) {
        uint32_t target_hash = hash_callsign_24(target_call);

        for (const auto& caller_call : STANDARD_CALLSIGNS) {
            std::string loc = "JN47";
            int rst = -12;

            // --- Test CALL (Type 6) via make_call ---
            Message msg_call = make_call(target_call, caller_call, loc, rst);
            EXPECT_EQ(msg_call.type, MessageType::CALL_STD_SUF);
            EXPECT_EQ(msg_call.hash_1, target_hash);
            EXPECT_EQ(msg_call.call_2, caller_call);
            EXPECT_EQ(msg_call.locator, loc);
            EXPECT_EQ(msg_call.rst_db, rst);

            // Encode to 10-byte (77-bit) payload
            uint8_t payload_call[PAYLOAD_BYTES] = {0};
            ASSERT_TRUE(encode_message(msg_call, payload_call));

            // Decode from payload
            Message dec_call;
            ASSERT_TRUE(decode_message(payload_call, dec_call));
            EXPECT_EQ(dec_call.type, MessageType::CALL_STD_SUF);
            EXPECT_EQ(dec_call.hash_1, target_hash);
            EXPECT_EQ(dec_call.call_2, caller_call);
            EXPECT_EQ(dec_call.locator, loc);
            EXPECT_EQ(dec_call.rst_db, rst);

            // --- Test REPLY73 (Type 11) via make_reply73 ---
            int reply_rst = +5;
            Message msg_reply = make_reply73(target_call, caller_call, reply_rst);
            EXPECT_EQ(msg_reply.type, MessageType::MULTI_REPORT73);
            ASSERT_FALSE(msg_reply.multi_targets.empty());
            EXPECT_EQ(msg_reply.multi_targets[0].hash, target_hash);
            EXPECT_EQ(msg_reply.multi_targets[0].rst_db, reply_rst);

            // Encode REPLY73 payload
            uint8_t payload_reply[PAYLOAD_BYTES] = {0};
            ASSERT_TRUE(encode_message(msg_reply, payload_reply));

            // Decode REPLY73 payload
            Message dec_reply;
            ASSERT_TRUE(decode_message(payload_reply, dec_reply));
            EXPECT_EQ(dec_reply.type, MessageType::MULTI_REPORT73);
            ASSERT_FALSE(dec_reply.multi_targets.empty());
            EXPECT_EQ(dec_reply.multi_targets[0].hash, target_hash);
            EXPECT_EQ(dec_reply.multi_targets[0].rst_db, reply_rst);
        }
    }
}

TEST(CallReplyCombinationsTest, StdTargetToSuffixCaller_Type6_And_Type8) {
    const std::vector<std::string> SUFFIX_CALLSIGNS = {
        "HB9IPH/P", "W1AW/P", "YO1YO/P", "TU2TU/P", "K1ABC/P", "DL1ABC/P"
    };

    for (const auto& target_call : STANDARD_CALLSIGNS) {
        for (const auto& caller_call : SUFFIX_CALLSIGNS) {
            std::string loc = "KL22";
            int rst = -5;

            // --- Test CALL (Type 6) via make_call ---
            Message msg_call = make_call(target_call, caller_call, loc, rst);
            EXPECT_EQ(msg_call.type, MessageType::CALL_STD_SUF);
            EXPECT_EQ(msg_call.locator, loc);
            EXPECT_EQ(msg_call.rst_db, rst);

            // Encode to payload
            uint8_t payload_call[PAYLOAD_BYTES] = {0};
            ASSERT_TRUE(encode_message(msg_call, payload_call));

            // Decode from payload
            Message dec_call;
            ASSERT_TRUE(decode_message(payload_call, dec_call));
            EXPECT_EQ(dec_call.type, MessageType::CALL_STD_SUF);
            EXPECT_EQ(dec_call.locator, loc);
            EXPECT_EQ(dec_call.rst_db, rst);

            // --- Test REPLY73 (Type 8) via make_reply73 ---
            int reply_rst = +1;
            Message msg_reply = make_reply73(target_call, caller_call, reply_rst);
            EXPECT_EQ(msg_reply.type, MessageType::REPORT73_STD);
            EXPECT_EQ(msg_reply.rst_db, reply_rst);

            // Encode REPLY73 payload
            uint8_t payload_reply[PAYLOAD_BYTES] = {0};
            ASSERT_TRUE(encode_message(msg_reply, payload_reply));

            // Decode REPLY73 payload
            Message dec_reply;
            ASSERT_TRUE(decode_message(payload_reply, dec_reply));
            EXPECT_EQ(dec_reply.type, MessageType::REPORT73_STD);
            EXPECT_EQ(dec_reply.rst_db, reply_rst);
        }
    }
}

TEST(CallReplyCombinationsTest, NonStdTargetToNonStdCaller_Type7_And_Type11) {
    for (const auto& target_call : NONSTANDARD_CALLSIGNS) {
        uint32_t target_hash = hash_callsign_24(target_call);

        for (const auto& caller_call : NONSTANDARD_CALLSIGNS) {
            std::string loc = "IL18";
            int rst = +3;

            // --- Test CALL via make_call ---
            Message msg_call = make_call(target_call, caller_call, loc, rst);
            EXPECT_EQ(msg_call.type, MessageType::CALL_NONSTD);
            EXPECT_EQ(msg_call.hash_1, hash_callsign_20(target_call));

            // Encode payload
            uint8_t payload_call[PAYLOAD_BYTES] = {0};
            ASSERT_TRUE(encode_message(msg_call, payload_call));

            // Decode payload
            Message dec_call;
            ASSERT_TRUE(decode_message(payload_call, dec_call));
            EXPECT_EQ(dec_call.type, MessageType::CALL_NONSTD);
            EXPECT_EQ(dec_call.hash_1, hash_callsign_20(target_call));
            EXPECT_EQ(dec_call.call_2, caller_call);
            EXPECT_EQ(dec_call.rst_db, rst);

            // --- Test REPLY73 (Type 11) via make_reply73 ---
            int reply_rst = -18;
            Message msg_reply = make_reply73(target_call, caller_call, reply_rst);
            EXPECT_EQ(msg_reply.type, MessageType::MULTI_REPORT73);
            ASSERT_FALSE(msg_reply.multi_targets.empty());
            EXPECT_EQ(msg_reply.multi_targets[0].hash, target_hash);
            EXPECT_EQ(msg_reply.multi_targets[0].rst_db, reply_rst);

            // Encode REPLY73 payload
            uint8_t payload_reply[PAYLOAD_BYTES] = {0};
            ASSERT_TRUE(encode_message(msg_reply, payload_reply));

            // Decode REPLY73 payload
            Message dec_reply;
            ASSERT_TRUE(decode_message(payload_reply, dec_reply));
            EXPECT_EQ(dec_reply.type, MessageType::MULTI_REPORT73);
            ASSERT_FALSE(dec_reply.multi_targets.empty());
            EXPECT_EQ(dec_reply.multi_targets[0].hash, target_hash);
            EXPECT_EQ(dec_reply.multi_targets[0].rst_db, reply_rst);
        }
    }
}

// =============================================================================
// 3. Full 3-Step QSO Cycles Across All Callsign Combinations
// =============================================================================

TEST(CallReplyCombinationsTest, Full3StepQSOCycles_AllCombinations) {
    struct QSOScenario {
        std::string desc;
        std::string station_a;       // CQ initiator & REPLY73 sender
        bool a_is_nonstd;
        std::string station_b;       // CALL sender
        bool b_is_nonstd;
        MessageType expected_call_type;
        MessageType expected_reply_type;
    };

    const std::vector<QSOScenario> scenarios = {
        {"Std <-> Std", "HB9IPH", false, "W1AW", false, MessageType::CALL_STD_NOSUF, MessageType::REPORT73_STD},
        {"Std <-> Std/P", "HB9IPH", false, "W1AW/P", false, MessageType::CALL_STD_SUF, MessageType::REPORT73_STD},
        {"Std/P <-> Std", "HB9IPH/P", false, "W1AW", false, MessageType::CALL_STD_SUF, MessageType::REPORT73_STD},
        {"Std/P <-> Std/P", "HB9IPH/P", false, "W1AW/P", false, MessageType::CALL_STD_SUF, MessageType::REPORT73_STD},
        {"NonStd CQer <-> Std Caller", "3B9/HB9IP/P", true, "W1AW", false, MessageType::CALL_STD_SUF, MessageType::MULTI_REPORT73},
        {"Std CQer <-> NonStd Caller", "HB9IPH", false, "EA8/YO1YO", true, MessageType::CALL_NONSTD, MessageType::MULTI_REPORT73},
        {"NonStd CQer <-> NonStd Caller", "3B9/HB9IP/P", true, "EA8/YO1YO", true, MessageType::CALL_NONSTD, MessageType::MULTI_REPORT73},
        {"Prefix NonStd CQer <-> Std Caller", "EA8/HB9IP", true, "TU2TU", false, MessageType::CALL_STD_SUF, MessageType::MULTI_REPORT73},
        {"Rare NonStd CQer <-> NonStd Suffix Caller", "GB100BBC", true, "TF/F6ABC/P", true, MessageType::CALL_NONSTD, MessageType::MULTI_REPORT73}
    };

    for (const auto& sc : scenarios) {
        SCOPED_TRACE(sc.desc);

        // Step 1: Station A transmits CQ
        Message cq_msg = make_cq(sc.station_a, "JN47");
        auto cq_payload = pack(cq_msg);
        ASSERT_TRUE(cq_payload.has_value());
        auto dec_cq = unpack(*cq_payload);
        ASSERT_TRUE(dec_cq.has_value());

        // Step 2: Station B receives CQ and sends CALL to Station A
        Message call_msg = make_call(sc.station_a, sc.station_b, "KL22", -8);
        EXPECT_EQ(call_msg.type, sc.expected_call_type);

        auto call_payload = pack(call_msg);
        ASSERT_TRUE(call_payload.has_value());
        auto dec_call = unpack(*call_payload);
        ASSERT_TRUE(dec_call.has_value());
        EXPECT_EQ(dec_call->type, sc.expected_call_type);

        // Step 3: Station A replies with REPLY73 to Station B
        Message reply_msg = make_reply73(sc.station_b, sc.station_a, +4);
        EXPECT_EQ(reply_msg.type, sc.expected_reply_type);

        auto reply_payload = pack(reply_msg);
        ASSERT_TRUE(reply_payload.has_value());
        auto dec_reply = unpack(*reply_payload);
        ASSERT_TRUE(dec_reply.has_value());
        EXPECT_EQ(dec_reply->type, sc.expected_reply_type);
        if (dec_reply->type == MessageType::REPORT73_STD) {
            EXPECT_EQ(dec_reply->rst_db, +4);
        } else if (dec_reply->type == MessageType::MULTI_REPORT73) {
            ASSERT_FALSE(dec_reply->multi_targets.empty());
            EXPECT_EQ(dec_reply->multi_targets[0].rst_db, +4);
        }
    }
}

// =============================================================================
// 4. Over-the-Air Transceiver Audio Transmission for Types 6, 7, 8, 11
// =============================================================================

TEST(CallReplyCombinationsTest, OverTheAirAudioTransceiver_AllProtocols) {
    const std::vector<Protocol> protocols = {
        Protocol::LQ8, Protocol::LQ4, Protocol::LQ2, Protocol::LQ16
    };

    struct TestTransmission {
        std::string name;
        Message msg;
        MessageType expected_type;
    };

    const std::vector<TestTransmission> transmissions = {
        {"Type 6 CALL (NonStd Target, Std Caller)", make_call("3B9/HB9IP/P", "W1AW", "FN31", -10), MessageType::CALL_STD_SUF},
        {"Type 6 CALL (Std Target, Std/P Caller)", make_call("W1AW", "HB9IPH/P", "IH23", -14), MessageType::CALL_STD_SUF},
        {"Type 6 CALL (NonStd Target, Std/P Caller)", make_call("EA8/YO1YO", "HB9IPH/P", "IH23", -6), MessageType::CALL_STD_SUF},
        {"Type 7 CALL (NonStd Target, NonStd Caller)", make_call("EA8/YO1YO", "3B9/HB9IP/P", "IH23", -6), MessageType::CALL_NONSTD},
        {"Type 11 REPLY73", make_reply73("3B9/HB9IP/P", "W1AW", +3), MessageType::MULTI_REPORT73},
        {"Type 8 REPLY73 (Std->Std/P)", make_reply73("W1AW", "HB9IPH/P", +2), MessageType::REPORT73_STD},
        {"Type 11 REPLY73 (NonStd->NonStd)", make_reply73("EA8/YO1YO", "3B9/HB9IP/P", 0), MessageType::MULTI_REPORT73}
    };

    for (Protocol proto : protocols) {
        Transceiver tx("HB9IPH", "JN47", proto, 1500.0f, 12000.0f);
        Transceiver rx("W1AW", "FN31", proto, 1500.0f, 12000.0f);

        for (const auto& t : transmissions) {
            SCOPED_TRACE(t.name);

            // Generate physical audio waveform
            std::vector<float> audio = tx.generate_audio(t.msg);
            ASSERT_FALSE(audio.empty());

            // Receive and demodulate audio waveform
            auto decoded_opt = rx.decode(audio);
            ASSERT_TRUE(decoded_opt.has_value());
            EXPECT_EQ(decoded_opt->type, t.expected_type);

            if (t.expected_type == MessageType::CALL_STD_SUF) {
                EXPECT_EQ(decoded_opt->hash_1, t.msg.hash_1);
                EXPECT_EQ(decoded_opt->locator, t.msg.locator);
                EXPECT_EQ(decoded_opt->rst_db, t.msg.rst_db);
            } else if (t.expected_type == MessageType::CALL_NONSTD) {
                EXPECT_EQ(decoded_opt->hash_1, t.msg.hash_1);
                EXPECT_EQ(decoded_opt->call_2, t.msg.call_2);
                EXPECT_EQ(decoded_opt->rst_db, t.msg.rst_db);
            }
        }
    }
}

// =============================================================================
// 5. RST and Locator Boundary Sweep for Types 6, 7, 8, 11
// =============================================================================

TEST(CallReplyCombinationsTest, RSTAndLocatorBoundarySweep_Types6_7_8_11) {
    std::string nonstd1 = "3B9/HB9IP/P";
    std::string nonstd2 = "EA8/YO1YO";
    std::string std1 = "W1AW";
    std::string std2_p = "HB9IPH/P";

    for (int rst : TEST_RSTS) {
        for (const auto& loc : TEST_LOCATORS) {
            // Type 6: CALL Non-Std Target (Std Caller)
            {
                Message m = make_call(nonstd1, std1, loc, rst);
                EXPECT_EQ(m.type, MessageType::CALL_STD_SUF);
                auto payload = pack(m);
                ASSERT_TRUE(payload.has_value());
                auto dec = unpack(*payload);
                ASSERT_TRUE(dec.has_value());
                EXPECT_EQ(dec->type, MessageType::CALL_STD_SUF);
                EXPECT_EQ(dec->hash_1, hash_callsign_24(nonstd1));
                EXPECT_EQ(dec->locator, loc);
                EXPECT_EQ(dec->rst_db, rst);
            }

            // Type 6: CALL Non-Std Target (Std/P Caller)
            {
                Message m = make_call(nonstd1, std2_p, loc, rst);
                EXPECT_EQ(m.type, MessageType::CALL_STD_SUF);
                auto payload = pack(m);
                ASSERT_TRUE(payload.has_value());
                auto dec = unpack(*payload);
                ASSERT_TRUE(dec.has_value());
                EXPECT_EQ(dec->type, MessageType::CALL_STD_SUF);
                EXPECT_EQ(dec->hash_1, hash_callsign_24(nonstd1));
                EXPECT_EQ(dec->locator, loc);
                EXPECT_EQ(dec->rst_db, rst);
            }

            // Type 7: CALL Non-Std Target (NonStd Caller)
            {
                Message m = make_call(nonstd1, nonstd2, loc, rst);
                EXPECT_EQ(m.type, MessageType::CALL_NONSTD);
                auto payload = pack(m);
                ASSERT_TRUE(payload.has_value());
                auto dec = unpack(*payload);
                ASSERT_TRUE(dec.has_value());
                EXPECT_EQ(dec->type, MessageType::CALL_NONSTD);
                EXPECT_EQ(dec->hash_1, hash_callsign_20(nonstd1));
                EXPECT_EQ(dec->call_2, nonstd2);
                EXPECT_EQ(dec->rst_db, rst);
            }
        }

        // Type 11: REPLY73 Non-Std (Std Caller)
        {
            Message m = make_reply73(nonstd1, std1, rst);
            EXPECT_EQ(m.type, MessageType::MULTI_REPORT73);
            auto payload = pack(m);
            ASSERT_TRUE(payload.has_value());
            auto dec = unpack(*payload);
            ASSERT_TRUE(dec.has_value());
            EXPECT_EQ(dec->type, MessageType::MULTI_REPORT73);
            ASSERT_FALSE(dec->multi_targets.empty());
            EXPECT_EQ(dec->multi_targets[0].hash, hash_callsign_24(nonstd1));
            EXPECT_EQ(dec->multi_targets[0].rst_db, rst);
        }

        // Type 11: REPLY73 Non-Std (NonStd Caller)
        {
            Message m = make_reply73(nonstd1, nonstd2, rst);
            EXPECT_EQ(m.type, MessageType::MULTI_REPORT73);
            auto payload = pack(m);
            ASSERT_TRUE(payload.has_value());
            auto dec = unpack(*payload);
            ASSERT_TRUE(dec.has_value());
            EXPECT_EQ(dec->type, MessageType::MULTI_REPORT73);
            ASSERT_FALSE(dec->multi_targets.empty());
            EXPECT_EQ(dec->multi_targets[0].hash, hash_callsign_24(nonstd1));
            EXPECT_EQ(dec->multi_targets[0].rst_db, rst);
        }
    }
}
