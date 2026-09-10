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
#include "lq/intent.h"
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <thread>
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

// =============================================================================
// 6. Canonical Precedence & Target Dehashing in Type 6 (CALL_STD_SUF)
// =============================================================================

TEST(CallReplyCombinationsTest, CanonicalPrecedence_Type6_TargetStandardOrNonStd) {
    // If I have a standard callsign without suffix (e.g. "HB9IPH"), but the CALLER is
    // suffixed or non-standard, the caller was required to use Type 6.
    // Therefore, my standard callsign MUST be resolved from hash_1 in Type 6.
    // Only if BOTH caller and candidate target are standard without suffix is the match skipped.

    const std::vector<std::string> std_targets = {"HB9IPH", "W1AW", "YO1YO", "K1ABC"};
    const std::vector<std::string> std_p_callers = {"HB9IPH/P", "W1AW/P", "YO1YO/P", "K1ABC/P"};
    const std::vector<std::string> nonstd_callers = {"3B9/HB9IP/P", "EA8/YO1YO", "3DA0RU", "GB100BBC"};
    const std::vector<std::string> std_nosuf_callers = {"DL1ABC", "ZS6BKW", "JA1ABC"};

    for (const auto& my_target : std_targets) {
        uint32_t my_h24 = hash_callsign_24(my_target);
        std::vector<std::string_view> known = {my_target, "UNRELATED"};

        // Subcase A: Caller has /P suffix -> Target is standard without suffix -> MUST RESOLVE
        for (const auto& caller : std_p_callers) {
            Message m;
            m.type = MessageType::CALL_STD_SUF;
            m.hash_1 = my_h24;
            m.call_2 = caller;
            m.suffix_2 = 1;
            m.locator = "JN47";
            m.rst_db = -5;

            bool res = resolve_callsigns(m, known.data(), known.size());
            EXPECT_TRUE(res) << "Failed to resolve standard target " << my_target << " when called by suffixed " << caller;
            EXPECT_EQ(m.call_1, my_target);
        }

        // Subcase B: Caller is Non-Standard -> Target is standard without suffix -> MUST RESOLVE
        for (const auto& caller : nonstd_callers) {
            Message m;
            m.type = MessageType::CALL_STD_SUF;
            m.hash_1 = my_h24;
            m.call_2 = caller;
            m.locator = "JN47";
            m.rst_db = -10;

            bool res = resolve_callsigns(m, known.data(), known.size());
            EXPECT_TRUE(res) << "Failed to resolve standard target " << my_target << " when called by nonstandard " << caller;
            EXPECT_EQ(m.call_1, my_target);
        }

        // Subcase C: Caller is Standard without suffix -> Target is standard without suffix -> MUST BE SKIPPED (collision)
        for (const auto& caller : std_nosuf_callers) {
            Message m;
            m.type = MessageType::CALL_STD_SUF;
            m.hash_1 = my_h24;
            m.call_2 = caller;
            m.suffix_2 = 0;
            m.locator = "JN47";
            m.rst_db = 0;

            bool res = resolve_callsigns(m, known.data(), known.size());
            EXPECT_FALSE(res) << "Accidental collision should be rejected for Type 6 when both caller " << caller << " and target " << my_target << " are standard without suffix";
            EXPECT_TRUE(m.call_1.empty() || m.call_1.front() == '<');
        }

        // Subcase D: Target has /P suffix (e.g. "HB9IPH/P") -> Caller is Standard without suffix -> MUST RESOLVE
        std::string my_target_p = my_target + "/P";
        uint32_t my_p_h24 = hash_callsign_24(my_target_p);
        std::vector<std::string_view> known_p = {my_target_p, "OTHER"};
        for (const auto& caller : std_nosuf_callers) {
            Message m;
            m.type = MessageType::CALL_STD_SUF;
            m.hash_1 = my_p_h24;
            m.call_2 = caller;
            m.suffix_2 = 0;
            m.locator = "JN47";
            m.rst_db = +3;

            bool res = resolve_callsigns(m, known_p.data(), known_p.size());
            EXPECT_TRUE(res) << "Target with /P " << my_target_p << " must resolve even if caller " << caller << " is standard without suffix";
            EXPECT_EQ(m.call_1, my_target_p);
        }
    }
}

// =============================================================================
// 7. Canonical Precedence & Target Dehashing in Type 7 (CALL_NONSTD)
// =============================================================================

TEST(CallReplyCombinationsTest, CanonicalPrecedence_Type7_TargetHash_ExhaustiveMatrix) {
    const std::vector<std::string> all_target_categories = {
        "HB9IPH", "W1AW",
        "HB9IPH/P", "W1AW/P",
        "3B9/HB9IP", "EA8/YO1YO",
        "3B9/HB9IP/P", "TF/F6ABC/P"
    };

    const std::vector<std::string> nonstd_callers = {
        "EA8/HB9IP", "3DA0RU", "GB100BBC", "HB9/K1ABC", "VI100AIR"
    };

    for (const auto& tgt : all_target_categories) {
        uint32_t h20 = hash_callsign_20(tgt);
        std::vector<std::string_view> known = {tgt, "K1ABC", "DL1ABC"};

        for (const auto& caller : nonstd_callers) {
            Message m;
            m.type = MessageType::CALL_NONSTD;
            m.hash_1 = h20;
            m.call_2 = caller;
            m.rst_db = -8;

            bool res = resolve_callsigns(m, known.data(), known.size());
            EXPECT_TRUE(res) << "Target " << tgt << " failed to resolve from H20 in Type 7 with caller " << caller;
            EXPECT_EQ(m.call_1, tgt);
        }
    }
}

// =============================================================================
// 8. Canonical Precedence & Collision Rejection in Type 10 (M73_NONSTD)
// =============================================================================

TEST(CallReplyCombinationsTest, CanonicalPrecedence_Type10_CollisionRejection_AllCombos) {
    const std::vector<std::string> std_calls = {"HB9IPH", "W1AW", "YO1YO", "TU2TU"};
    const std::vector<std::string> std_p_calls = {"HB9IPH/P", "W1AW/P", "YO1YO/P"};
    const std::vector<std::string> nonstd_calls = {"EA8/HB9IP", "3DA0RU", "3B9/HB9IP/P", "GB100BBC"};

    // Case 1: Standard Me + Standard Remote -> REJECT Type 10
    for (const auto& my_call : std_calls) {
        IntentEngine engine(my_call, "JN47");
        uint32_t my_h24 = hash_callsign_24(my_call);

        for (const auto& remote : std_calls) {
            ReceivedFrame rf;
            rf.msg.type = MessageType::M73_NONSTD;
            rf.msg.call_2 = remote;
            rf.msg.hash_1 = my_h24;

            auto res = engine.process_slot({rf}, UserIntent{IntentAction::IDLE});
            EXPECT_TRUE(res.qso_completed_with.empty()) << "Type 10 false acceptance between std " << my_call << " and std " << remote;
            EXPECT_FALSE(engine.is_qso_completed(remote));
        }
    }

    // Case 2: Standard Me + Standard/P Remote -> REJECT Type 10 (fits in Type 9)
    for (const auto& my_call : std_calls) {
        IntentEngine engine(my_call, "JN47");
        uint32_t my_h24 = hash_callsign_24(my_call);

        for (const auto& remote_p : std_p_calls) {
            ReceivedFrame rf;
            rf.msg.type = MessageType::M73_NONSTD;
            rf.msg.call_2 = remote_p;
            rf.msg.hash_1 = my_h24;

            auto res = engine.process_slot({rf}, UserIntent{IntentAction::IDLE});
            EXPECT_TRUE(res.qso_completed_with.empty()) << "Type 10 false acceptance between std " << my_call << " and std/p " << remote_p;
            EXPECT_FALSE(engine.is_qso_completed(remote_p));
        }
    }

    // Case 3: Standard Me + Non-Standard Remote -> ACCEPT Type 10
    for (const auto& my_call : std_calls) {
        IntentEngine engine(my_call, "JN47");
        uint32_t my_h24 = hash_callsign_24(my_call);

        for (const auto& remote_nonstd : nonstd_calls) {
            ReceivedFrame rf;
            rf.msg.type = MessageType::M73_NONSTD;
            rf.msg.call_2 = remote_nonstd;
            rf.msg.hash_1 = my_h24;

            auto res = engine.process_slot({rf}, UserIntent{IntentAction::IDLE});
            EXPECT_EQ(res.qso_completed_with.size(), 1u) << "Type 10 rejected valid nonstd remote " << remote_nonstd << " for std " << my_call;
            EXPECT_TRUE(engine.is_qso_completed(remote_nonstd));
        }
    }

    // Case 4: Non-Standard Me + Standard Remote -> ACCEPT Type 10
    for (const auto& my_nonstd : nonstd_calls) {
        IntentEngine engine(my_nonstd, "JN47");
        uint32_t my_h24 = hash_callsign_24(my_nonstd);

        for (const auto& remote_std : std_calls) {
            ReceivedFrame rf;
            rf.msg.type = MessageType::M73_NONSTD;
            rf.msg.call_2 = remote_std;
            rf.msg.hash_1 = my_h24;

            auto res = engine.process_slot({rf}, UserIntent{IntentAction::IDLE});
            EXPECT_EQ(res.qso_completed_with.size(), 1u) << "Type 10 rejected valid nonstd target " << my_nonstd << " for remote " << remote_std;
            EXPECT_TRUE(engine.is_qso_completed(remote_std));
        }
    }

    // Case 5: Non-Standard Me + Non-Standard Remote -> ACCEPT Type 10
    for (const auto& my_nonstd : nonstd_calls) {
        IntentEngine engine(my_nonstd, "JN47");
        uint32_t my_h24 = hash_callsign_24(my_nonstd);

        for (const auto& remote_nonstd : nonstd_calls) {
            if (my_nonstd == remote_nonstd) continue;
            ReceivedFrame rf;
            rf.msg.type = MessageType::M73_NONSTD;
            rf.msg.call_2 = remote_nonstd;
            rf.msg.hash_1 = my_h24;

            auto res = engine.process_slot({rf}, UserIntent{IntentAction::IDLE});
            EXPECT_EQ(res.qso_completed_with.size(), 1u) << "Type 10 rejected valid dual-nonstd QSO between " << my_nonstd << " and " << remote_nonstd;
            EXPECT_TRUE(engine.is_qso_completed(remote_nonstd));
        }
    }
}

// =============================================================================
// 9. Scoped 16-Bit DX Dehashing and Contact History States in Types 11 & 12
// =============================================================================

TEST(CallReplyCombinationsTest, ScopedDehashing_Type11_Type12_ContactHistoryStates) {
    const std::vector<std::string> my_test_calls = {"HB9IPH", "EA8/HB9IP", "3B9/HB9IP/P"};
    const std::vector<std::string> dx_test_calls = {"W1AW", "EA8/YO1YO", "3DA0RU", "TF/F6ABC/P"};

    for (const auto& my_call : my_test_calls) {
        uint32_t my_h24 = hash_callsign_24(my_call);

        for (const auto& dx_call : dx_test_calls) {
            uint32_t dx_h16 = hash_callsign_16(dx_call);

            for (MessageType mtype : {MessageType::MULTI_REPORT73, MessageType::MULTI_73}) {
                ReceivedFrame rf_reply;
                rf_reply.msg.type = mtype;
                rf_reply.msg.call_1 = "<" + payload_to_hex(reinterpret_cast<const uint8_t*>(&dx_h16)).substr(0, 4) + ">";
                rf_reply.msg.hash_1 = dx_h16;
                MultiTarget mt;
                mt.hash = my_h24;
                mt.call = my_call;
                mt.rst_db = +1;
                rf_reply.msg.multi_targets.push_back(mt);

                // --- State 1: Actively called DX before (Fresh within TTL) ---
                {
                    IntentEngine engine(my_call, "JN47");
                    engine.add_called_station(dx_call);
                    EXPECT_TRUE(engine.has_called_station(dx_call));

                    auto dec = engine.process_slot({rf_reply}, UserIntent{IntentAction::IDLE});
                    EXPECT_EQ(dec.qso_completed_with.size(), 1u);
                    EXPECT_EQ(dec.qso_completed_with[0], dx_call);
                    EXPECT_TRUE(engine.is_qso_completed(dx_call));
                    // Automatic eviction upon QSO completion
                    EXPECT_FALSE(engine.has_called_station(dx_call));
                    EXPECT_TRUE(engine.get_called_stations().empty());
                }

                // --- State 2: Actively called DX, but TTL expired ---
                {
                    IntentEngine engine(my_call, "JN47");
                    engine.set_called_station_ttl_seconds(60);
                    auto t0 = std::chrono::steady_clock::now();
                    engine.set_simulated_time(t0);
                    engine.add_called_station(dx_call);

                    // Advance simulated time by 40s and add station Z (so Z has 30s remaining at t0+70s)
                    engine.set_simulated_time(t0 + std::chrono::seconds(40));
                    engine.add_called_station("Z3ABC");

                    // Simulate time advance past TTL for dx_call (t0 + 70s)
                    engine.set_simulated_time(t0 + std::chrono::seconds(70));
                    engine.prune_expired_called_stations();
                    EXPECT_FALSE(engine.has_called_station(dx_call));
                    EXPECT_TRUE(engine.has_called_station("Z3ABC"));

                    // Frame from dx_call arrives -> must be rejected because dx_call expired
                    // and active called station is Z3ABC
                    auto dec = engine.process_slot({rf_reply}, UserIntent{IntentAction::IDLE});
                    EXPECT_TRUE(dec.qso_completed_with.empty());
                    EXPECT_FALSE(engine.is_qso_completed(dx_call));
                }

                // --- State 3: Uncalled DX Collision Rejection ---
                // We actively called station X ("K1ABC"). DX station transmits Type 11 matching my_h24.
                // Because DX was never called, must be rejected.
                {
                    IntentEngine engine(my_call, "JN47");
                    engine.add_called_station("K1ABC");
                    engine.add_known_callsign(dx_call); // present in heard cache, but NOT called

                    auto dec = engine.process_slot({rf_reply}, UserIntent{IntentAction::IDLE});
                    EXPECT_TRUE(dec.qso_completed_with.empty());
                    EXPECT_FALSE(engine.is_qso_completed(dx_call));
                }

                // --- State 4: Passive Monitor Mode (Empty called stations list) ---
                // In monitor mode, station in heard cache resolves for display/logging
                {
                    IntentEngine engine(my_call, "JN47");
                    EXPECT_TRUE(engine.get_called_stations().empty());
                    engine.add_known_callsign(dx_call);

                    auto dec = engine.process_slot({rf_reply}, UserIntent{IntentAction::IDLE});
                    EXPECT_EQ(dec.qso_completed_with.size(), 1u);
                    EXPECT_EQ(dec.qso_completed_with[0], dx_call);
                    EXPECT_TRUE(engine.is_qso_completed(dx_call));
                }

                // --- State 5: Unheard DX station in Monitor Mode ---
                // Neither called nor heard -> DX cannot be resolved by callsign
                {
                    IntentEngine engine(my_call, "JN47");
                    EXPECT_TRUE(engine.get_called_stations().empty());

                    auto dec = engine.process_slot({rf_reply}, UserIntent{IntentAction::IDLE});
                    EXPECT_TRUE(dec.qso_completed_with.empty());
                    EXPECT_FALSE(engine.is_qso_completed(dx_call));
                }
            }
        }
    }
}

// =============================================================================
// 10. Exhaustive Combinatorial Matrix: All Message Types & All Callsign Pairs
// =============================================================================

TEST(CallReplyCombinationsTest, FullCombinatorialMatrix_AllTypes_AllCallPairs_DehashingAndIntent) {
    const std::vector<std::pair<std::string, std::string>> test_actor_pairs = {
        // Std x Std
        {"HB9IPH", "W1AW"},
        {"YO1YO", "TU2TU"},
        // Std x Std/P
        {"HB9IPH", "W1AW/P"},
        {"W1AW/P", "HB9IPH"},
        // Std x NonStd Short
        {"HB9IPH", "EA8/HB9IP"},
        {"EA8/HB9IP", "HB9IPH"},
        // Std x NonStd Complex
        {"HB9IPH", "3B9/HB9IP/P"},
        {"3B9/HB9IP/P", "HB9IPH"},
        // Std/P x Std/P
        {"HB9IPH/P", "W1AW/P"},
        // Std/P x NonStd Short
        {"HB9IPH/P", "3DA0RU"},
        {"3DA0RU", "HB9IPH/P"},
        // Std/P x NonStd Complex
        {"HB9IPH/P", "TF/F6ABC/P"},
        // NonStd Short x NonStd Short
        {"EA8/HB9IP", "3DA0RU"},
        {"GB100BBC", "HB9/K1ABC"},
        // NonStd Short x NonStd Complex
        {"EA8/HB9IP", "3B9/HB9IP/P"},
        // NonStd Complex x NonStd Complex
        {"3B9/HB9IP/P", "TF/F6ABC/P"},
        {"EA8/YO1YO", "DL/ON4XYZ"}
    };

    for (const auto& [my_call, remote_call] : test_actor_pairs) {
        // 1. Test CALL initiation (CALL_STATION intent)
        {
            IntentEngine engine(my_call, "JN47");
            UserIntent call_intent;
            call_intent.action = IntentAction::CALL_STATION;
            call_intent.target_call_1 = remote_call;
            call_intent.custom_rst_1 = -7;

            auto dec = engine.process_slot({}, call_intent);
            EXPECT_TRUE(dec.success) << "Failed CALL_STATION from " << my_call << " to " << remote_call;
            EXPECT_TRUE(engine.has_called_station(remote_call));
            EXPECT_FALSE(dec.tx_hex.empty());
            EXPECT_FALSE(dec.tx_binary.empty());

            // Check that transmitted message unpacks cleanly
            uint8_t payload[PAYLOAD_BYTES] = {0};
            ASSERT_TRUE(hex_to_payload(dec.tx_hex, payload));
            Message dec_msg;
            ASSERT_TRUE(decode_message(payload, dec_msg));
        }

        // 2. Test REPORT+73 generation (REPLY_TO_STATIONS intent)
        {
            IntentEngine engine(my_call, "JN47");
            UserIntent reply_intent;
            reply_intent.action = IntentAction::REPLY_TO_STATIONS;
            reply_intent.target_call_1 = remote_call;
            reply_intent.custom_rst_1 = +2;

            auto dec = engine.process_slot({}, reply_intent);
            EXPECT_TRUE(dec.success) << "Failed REPLY_TO_STATIONS from " << my_call << " to " << remote_call;
            EXPECT_TRUE(engine.is_qso_completed(remote_call));
            EXPECT_FALSE(engine.has_called_station(remote_call)); // evicted
        }

        // 3. Test MULTI-REPORT+73 dual target reply
        {
            IntentEngine engine(my_call, "JN47");
            UserIntent multi_intent;
            multi_intent.action = IntentAction::REPLY_TO_STATIONS;
            multi_intent.target_call_1 = remote_call;
            multi_intent.target_call_2 = "K1ABC";
            multi_intent.custom_rst_1 = 0;
            multi_intent.custom_rst_2 = -12;

            auto dec = engine.process_slot({}, multi_intent);
            EXPECT_TRUE(dec.success);
            EXPECT_TRUE(engine.is_qso_completed(remote_call));
            EXPECT_TRUE(engine.is_qso_completed("K1ABC"));
        }

        // 4. Test format_message round-trip on direct calls
        {
            Message m = make_call(remote_call, my_call, "FN31", -15);
            auto payload = pack(m);
            ASSERT_TRUE(payload.has_value());
            auto unpacked = unpack(*payload);
            ASSERT_TRUE(unpacked.has_value());

            std::vector<std::string_view> known = {my_call, remote_call};
            resolve_callsigns(*unpacked, known.data(), known.size());
            std::string text = format_message(*unpacked);
            EXPECT_FALSE(text.empty());
        }
    }
}
