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
#include "lq/message.h"
#include "lq/hash.h"
#include "lq/easy.h"
#include <vector>
#include <string>

using namespace lq;

TEST(MessageTest, StandardQSOCycle) {
    // T1: CQ YO1YO JN47
    Message msg_cq;
    msg_cq.type = MessageType::CQ_STD;
    msg_cq.call_1 = "YO1YO";
    msg_cq.locator = "JN47";

    uint8_t payload_cq[PAYLOAD_BYTES];
    ASSERT_TRUE(encode_message(msg_cq, payload_cq));

    Message dec_cq;
    ASSERT_TRUE(decode_message(payload_cq, dec_cq));
    EXPECT_EQ(dec_cq.type, MessageType::CQ_STD);
    EXPECT_EQ(dec_cq.call_1, "YO1YO");
    EXPECT_EQ(dec_cq.locator, "JN47");
    EXPECT_EQ(format_message(dec_cq), "CQ YO1YO JN47");

    // T2: CALL YO1YO TU2TU KL22 -03 (Critical 77-bit exact message)
    Message msg_call;
    msg_call.type = MessageType::CALL_STD_NOSUF;
    msg_call.call_1 = "YO1YO";
    msg_call.call_2 = "TU2TU";
    msg_call.locator = "KL22";
    msg_call.rst_db = -3;

    uint8_t payload_call[PAYLOAD_BYTES];
    ASSERT_TRUE(encode_message(msg_call, payload_call));

    Message dec_call;
    ASSERT_TRUE(decode_message(payload_call, dec_call));
    EXPECT_EQ(dec_call.type, MessageType::CALL_STD_NOSUF);
    EXPECT_EQ(dec_call.call_1, "YO1YO");
    EXPECT_EQ(dec_call.call_2, "TU2TU");
    EXPECT_EQ(dec_call.locator, "KL22");
    EXPECT_EQ(dec_call.rst_db, -3);
    EXPECT_EQ(format_message(dec_call), "YO1YO TU2TU KL22 -03");

    // T3: REPORT+73 YO1YO TU2TU +05
    Message msg_reply;
    msg_reply.type = MessageType::REPORT73_STD;
    msg_reply.call_1 = "YO1YO";
    msg_reply.call_2 = "TU2TU";
    msg_reply.rst_db = 5;

    uint8_t payload_reply[PAYLOAD_BYTES];
    ASSERT_TRUE(encode_message(msg_reply, payload_reply));

    Message dec_reply;
    ASSERT_TRUE(decode_message(payload_reply, dec_reply));
    EXPECT_EQ(dec_reply.type, MessageType::REPORT73_STD);
    EXPECT_EQ(dec_reply.call_1, "YO1YO");
    EXPECT_EQ(dec_reply.call_2, "TU2TU");
    EXPECT_EQ(dec_reply.rst_db, 5);
    EXPECT_EQ(format_message(dec_reply), "YO1YO TU2TU R+05");

    // T4: 73 YO1YO TU2TU
    Message msg_73;
    msg_73.type = MessageType::M73_STD;
    msg_73.call_1 = "YO1YO";
    msg_73.call_2 = "TU2TU";

    uint8_t payload_73[PAYLOAD_BYTES];
    ASSERT_TRUE(encode_message(msg_73, payload_73));

    Message dec_73;
    ASSERT_TRUE(decode_message(payload_73, dec_73));
    EXPECT_EQ(dec_73.type, MessageType::M73_STD);
    EXPECT_EQ(dec_73.call_1, "YO1YO");
    EXPECT_EQ(dec_73.call_2, "TU2TU");
    EXPECT_EQ(format_message(dec_73), "YO1YO TU2TU 73");
}

TEST(MessageTest, DirectedCQ) {
    Message msg;
    msg.type = MessageType::CQ_STD;
    msg.modifier = "DX";
    msg.call_1 = "W1AW";
    msg.locator = "FN31";

    uint8_t payload[PAYLOAD_BYTES];
    ASSERT_TRUE(encode_message(msg, payload));

    Message dec;
    ASSERT_TRUE(decode_message(payload, dec));
    EXPECT_EQ(dec.type, MessageType::CQ_STD);
    EXPECT_EQ(dec.modifier, "DX");
    EXPECT_EQ(dec.call_1, "W1AW");
    EXPECT_EQ(dec.locator, "FN31");
    EXPECT_EQ(format_message(dec), "CQ DX W1AW FN31");
}

TEST(MessageTest, NonStandardCQTypes) {
    // CQ Non-Std 1 (max 9 chars): YO1YO
    {
        Message msg;
        msg.type = MessageType::CQ_NONSTD_1;
        msg.call_1 = "YO1YO";
        msg.locator = "JN47";

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CQ_NONSTD_1);
        EXPECT_EQ(dec.call_1, "YO1YO");
        EXPECT_EQ(dec.locator, "JN47");
    }

    // CQ Non-Std 2 (max 9 chars): EA6/W1AW
    {
        Message msg;
        msg.type = MessageType::CQ_NONSTD_2;
        msg.call_1 = "EA6/W1AW";
        msg.modifier = "DX";

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CQ_NONSTD_2);
        EXPECT_EQ(dec.call_1, "EA6/W1AW");
        EXPECT_EQ(dec.modifier, "DX");
    }

    // CQ Non-Std 3 (max 13 chars): 3B9/HB9IPH/P
    {
        Message msg;
        msg.type = MessageType::CQ_NONSTD_3;
        msg.call_1 = "3B9/HB9IPH/P";

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CQ_NONSTD_3);
        EXPECT_EQ(dec.call_1, "3B9/HB9IPH/P");
    }
}

TEST(MessageTest, NonStandardCallAndReplyTypes) {
    // CALL Non-Std (Type 7)
    {
        Message msg;
        msg.type = MessageType::CALL_NONSTD;
        msg.hash_1 = hash_callsign_20("EA6/W1AW");
        msg.call_2 = "EA6/TU2TU/P";
        msg.suffix_2 = 1;
        msg.rst_db = -3;

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        std::vector<std::string_view> known = {"EA6/W1AW", "EA6/TU2TU/P"};
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec, known.data(), known.size()));
        EXPECT_EQ(dec.type, MessageType::CALL_NONSTD);
        EXPECT_EQ(dec.hash_1, msg.hash_1);
        EXPECT_EQ(dec.call_2, "EA6/TU2TU/P");
        EXPECT_EQ(dec.suffix_2, 1);
        EXPECT_EQ(dec.rst_db, -3);
    }

    // M73 Non-Std (Type 10)
    {
        Message msg;
        msg.type = MessageType::M73_NONSTD;
        msg.hash_1 = hash_callsign_24("EA6/W1AW");
        msg.call_2 = "TU2TU";
        msg.suffix_2 = 0;

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::M73_NONSTD);
        EXPECT_EQ(dec.hash_1, msg.hash_1);
        EXPECT_EQ(dec.call_2, "TU2TU");
    }
}

TEST(MessageTest, StandardCallsignPortableSuffixRoundtrip) {
    // 1. CQ with /P
    {
        Message msg;
        msg.type = MessageType::CQ_STD;
        msg.call_1 = "HB9IPH/P";
        msg.locator = "JN47";
        msg.modifier = "POTA";

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CQ_STD);
        EXPECT_EQ(dec.call_1, "HB9IPH/P");
        EXPECT_EQ(dec.suffix_1, 1);
        EXPECT_EQ(dec.locator, "JN47");
        EXPECT_EQ(dec.modifier, "POTA");
        EXPECT_EQ(format_message(dec), "CQ POTA HB9IPH/P JN47");
    }

    // 2. CALL with suffix -> Type 6 (CALL_STD_SUF) with Locator & SNR
    {
        Message msg;
        msg.type = MessageType::CALL_STD_SUF;
        msg.call_1 = "YO1YO";
        msg.hash_1 = hash_callsign_24("YO1YO");
        msg.call_2 = "HB9IPH/P";
        msg.suffix_2 = 1;
        msg.locator = "JN47";
        msg.rst_db = -3;

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::CALL_STD_SUF);
        EXPECT_EQ(dec.hash_1, msg.hash_1);
        EXPECT_EQ(dec.call_2, "HB9IPH/P");
        EXPECT_EQ(dec.suffix_2, 1);
        EXPECT_EQ(dec.locator, "JN47");
        EXPECT_EQ(dec.rst_db, -3);
    }

    // 3. REPORT+73 with both stations carrying /P -> Type 8 (REPORT73_STD with suffix_1=1, suffix_2=1)
    {
        Message msg;
        msg.type = MessageType::REPORT73_STD;
        msg.call_1 = "YO1YO/P";
        msg.call_2 = "HB9IPH/P";
        msg.rst_db = +5;

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::REPORT73_STD);
        EXPECT_EQ(dec.call_1, "YO1YO/P");
        EXPECT_EQ(dec.suffix_1, 1);
        EXPECT_EQ(dec.call_2, "HB9IPH/P");
        EXPECT_EQ(dec.suffix_2, 1);
        EXPECT_EQ(dec.rst_db, 5);
        EXPECT_EQ(format_message(dec), "YO1YO/P HB9IPH/P R+05");
    }

    // 4. 73 with both stations carrying /P -> Type 9 (M73_STD)
    {
        Message msg;
        msg.type = MessageType::M73_STD;
        msg.call_1 = "YO1YO/P";
        msg.call_2 = "HB9IPH/P";

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::M73_STD);
        EXPECT_EQ(dec.call_1, "YO1YO/P");
        EXPECT_EQ(dec.suffix_1, 1);
        EXPECT_EQ(dec.call_2, "HB9IPH/P");
        EXPECT_EQ(dec.suffix_2, 1);
        EXPECT_EQ(format_message(dec), "YO1YO/P HB9IPH/P 73");
    }
}

TEST(MessageTest, FreeText) {
    Message msg;
    msg.type = MessageType::FREE_TEXT;
    msg.text = "73 DE W1AW";

    uint8_t payload[PAYLOAD_BYTES];
    ASSERT_TRUE(encode_message(msg, payload));

    Message dec;
    ASSERT_TRUE(decode_message(payload, dec));
    EXPECT_EQ(dec.type, MessageType::FREE_TEXT);
    EXPECT_EQ(dec.text, msg.text);
}

TEST(MessageTest, MultiReply73) {
    // 2 Targets
    {
        Message msg;
        msg.type = MessageType::MULTI_REPORT73;
        msg.call_1 = "HB9IPH";
        msg.hash_1 = hash_callsign_16("HB9IPH");
        msg.multi_targets = {
            {"YO1YO", hash_callsign_24("YO1YO"), 5},
            {"TU2TU", hash_callsign_24("TU2TU"), -3}
        };

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));

        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::MULTI_REPORT73);
        EXPECT_EQ(dec.hash_1, msg.hash_1);
        ASSERT_EQ(dec.multi_targets.size(), 2U);
        EXPECT_EQ(dec.multi_targets[0].hash, msg.multi_targets[0].hash);
        EXPECT_EQ(dec.multi_targets[0].rst_db, 5);
        EXPECT_EQ(dec.multi_targets[1].hash, msg.multi_targets[1].hash);
        EXPECT_EQ(dec.multi_targets[1].rst_db, -3);

        std::string formatted = format_message(msg);
        EXPECT_EQ(formatted, "<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>");

        Message parsed;
        ASSERT_TRUE(parse_message(formatted, parsed));
        EXPECT_EQ(parsed.type, MessageType::MULTI_REPORT73);
        EXPECT_EQ(parsed.call_1, "HB9IPH");
        ASSERT_EQ(parsed.multi_targets.size(), 2U);
        EXPECT_EQ(parsed.multi_targets[0].call, "YO1YO");
        EXPECT_EQ(parsed.multi_targets[0].rst_db, 5);
        EXPECT_EQ(parsed.multi_targets[1].call, "TU2TU");
        EXPECT_EQ(parsed.multi_targets[1].rst_db, -3);

        // Test Zero-Allocation / Span-based Known Callsigns Resolution
        std::string_view known_calls[] = { "DL1ABC", "YO1YO", "TU2TU", "HB9IPH", "W1AW" };
        Message resolved;
        ASSERT_TRUE(decode_message(payload, resolved, known_calls, 5));
        EXPECT_EQ(resolved.call_1, "HB9IPH");
        ASSERT_EQ(resolved.multi_targets.size(), 2U);
        EXPECT_EQ(resolved.multi_targets[0].call, "YO1YO");
        EXPECT_EQ(resolved.multi_targets[1].call, "TU2TU");
    }
}

TEST(MessageTest, Multi73) {
    // 2 Targets MULTI-73 (Type 12)
    {
        Message msg;
        msg.type = MessageType::MULTI_73;
        msg.call_1 = "HB9IPH";
        msg.hash_1 = hash_callsign_16("HB9IPH");
        msg.multi_targets = {
            {"YO1YO", hash_callsign_24("YO1YO"), 0},
            {"TU2TU", hash_callsign_24("TU2TU"), 0}
        };

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));

        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::MULTI_73);
        EXPECT_EQ(dec.hash_1, msg.hash_1);
        ASSERT_EQ(dec.multi_targets.size(), 2U);
        EXPECT_EQ(dec.multi_targets[0].hash, msg.multi_targets[0].hash);
        EXPECT_EQ(dec.multi_targets[1].hash, msg.multi_targets[1].hash);

        std::string formatted = format_message(msg);
        EXPECT_EQ(formatted, "<YO1YO> <TU2TU> <HB9IPH> 73");

        Message parsed;
        ASSERT_TRUE(parse_message(formatted, parsed));
        EXPECT_EQ(parsed.type, MessageType::MULTI_73);
        EXPECT_EQ(parsed.call_1, "HB9IPH");
        ASSERT_EQ(parsed.multi_targets.size(), 2U);
        EXPECT_EQ(parsed.multi_targets[0].call, "YO1YO");
        EXPECT_EQ(parsed.multi_targets[1].call, "TU2TU");
    }
}

TEST(MessageTest, ReservedType) {
    // 5-bit Reserved A (Type 14)
    {
        Message msg;
        msg.type = MessageType::RESERVED_A;
        msg.raw_payload = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA};

        uint8_t payload[PAYLOAD_BYTES];
        ASSERT_TRUE(encode_message(msg, payload));
        Message dec;
        ASSERT_TRUE(decode_message(payload, dec));
        EXPECT_EQ(dec.type, MessageType::RESERVED_A);
    }
}

TEST(MessageTest, TextParserRoundTrip) {
    Message msg;

    EXPECT_TRUE(parse_message("CQ YO1YO JN47", msg));
    EXPECT_EQ(msg.type, MessageType::CQ_STD);
    EXPECT_EQ(msg.call_1, "YO1YO");
    EXPECT_EQ(msg.locator, "JN47");

    EXPECT_TRUE(parse_message("CQ DX W1AW FN31", msg));
    EXPECT_EQ(msg.type, MessageType::CQ_STD);
    EXPECT_EQ(msg.modifier, "DX");
    EXPECT_EQ(msg.call_1, "W1AW");
    EXPECT_EQ(msg.locator, "FN31");

    EXPECT_TRUE(parse_message("CALL YO1YO TU2TU KL22 -03", msg));
    EXPECT_EQ(msg.type, MessageType::CALL_STD_NOSUF);
    EXPECT_EQ(msg.call_1, "YO1YO");
    EXPECT_EQ(msg.call_2, "TU2TU");
    EXPECT_EQ(msg.locator, "KL22");
    EXPECT_EQ(msg.rst_db, -3);

    EXPECT_TRUE(parse_message("REPORT+73 YO1YO TU2TU +05", msg));
    EXPECT_EQ(msg.type, MessageType::REPORT73_STD);
    EXPECT_EQ(msg.call_1, "YO1YO");
    EXPECT_EQ(msg.call_2, "TU2TU");
    EXPECT_EQ(msg.rst_db, 5);

    EXPECT_TRUE(parse_message("73 YO1YO TU2TU", msg));
    EXPECT_EQ(msg.type, MessageType::M73_STD);
    EXPECT_EQ(msg.call_1, "YO1YO");
    EXPECT_EQ(msg.call_2, "TU2TU");
}

TEST(MessageTest, UnequivocalDeterministicTextFormattingForAllTypes) {
    // 1. Type 1: CQ_STD
    {
        Message m1;
        m1.type = MessageType::CQ_STD;
        m1.call_1 = "HB9IPH";
        m1.locator = "JN47";
        EXPECT_EQ(format_message(m1), "CQ HB9IPH JN47");
        EXPECT_EQ(m1.to_string(), "CQ HB9IPH JN47");

        Message m2;
        m2.type = MessageType::CQ_STD;
        m2.modifier = "DX";
        m2.call_1 = "HB9IPH";
        m2.locator = "JN47";
        EXPECT_EQ(format_message(m2), "CQ DX HB9IPH JN47");
        EXPECT_EQ(m2.to_string(), "CQ DX HB9IPH JN47");
    }

    // 2. Type 2: CQ_NONSTD_1
    {
        Message m;
        m.type = MessageType::CQ_NONSTD_1;
        m.call_1 = "EA6/HB9IP";
        m.locator = "JN47";
        EXPECT_EQ(format_message(m), "CQ EA6/HB9IP JN47");
        EXPECT_EQ(m.to_string(), "CQ EA6/HB9IP JN47");
    }

    // 3. Type 3: CQ_NONSTD_2
    {
        Message m;
        m.type = MessageType::CQ_NONSTD_2;
        m.modifier = "POTA";
        m.call_1 = "EA6/HB9IP";
        EXPECT_EQ(format_message(m), "CQ POTA EA6/HB9IP");
        EXPECT_EQ(m.to_string(), "CQ POTA EA6/HB9IP");
    }

    // 4. Type 4: CQ_NONSTD_3
    {
        Message m;
        m.type = MessageType::CQ_NONSTD_3;
        m.call_1 = "3B9/HB9IPH/P";
        EXPECT_EQ(format_message(m), "CQ 3B9/HB9IPH/P");
        EXPECT_EQ(m.to_string(), "CQ 3B9/HB9IPH/P");
    }

    // 5. Type 5: CALL_STD_NOSUF
    {
        Message m;
        m.type = MessageType::CALL_STD_NOSUF;
        m.call_1 = "YO1YO";
        m.call_2 = "HB9IPH";
        m.locator = "JN47";
        m.rst_db = -3;
        EXPECT_EQ(format_message(m), "YO1YO HB9IPH JN47 -03");
        EXPECT_EQ(m.to_string(), "YO1YO HB9IPH JN47 -03");
    }

    // 6. Type 6: CALL_STD_SUF
    {
        Message m;
        m.type = MessageType::CALL_STD_SUF;
        m.call_1 = "YO1YO";
        m.call_2 = "HB9IPH/P";
        m.locator = "JN47";
        m.rst_db = 5;
        EXPECT_EQ(format_message(m), "<YO1YO> HB9IPH/P JN47 +05");
        EXPECT_EQ(m.to_string(), "<YO1YO> HB9IPH/P JN47 +05");
    }

    // 7. Type 7: CALL_NONSTD
    {
        Message m_unresolved;
        m_unresolved.type = MessageType::CALL_NONSTD;
        m_unresolved.hash_1 = 0x01A4F;
        m_unresolved.call_2 = "EA6/HB9IP/P";
        m_unresolved.rst_db = -3;
        EXPECT_EQ(format_message(m_unresolved), "<01a4f> EA6/HB9IP/P -03");

        Message m_resolved;
        m_resolved.type = MessageType::CALL_NONSTD;
        m_resolved.hash_1 = 0x01A4F;
        m_resolved.call_1 = "YO1YO/P";
        m_resolved.call_2 = "EA6/HB9IP/P";
        m_resolved.rst_db = -3;
        EXPECT_EQ(format_message(m_resolved), "<YO1YO/P> EA6/HB9IP/P -03");
    }

    // 8. Type 8: REPORT73_STD
    {
        Message m;
        m.type = MessageType::REPORT73_STD;
        m.call_1 = "HB9IPH";
        m.call_2 = "YO1YO";
        m.rst_db = -10;
        EXPECT_EQ(format_message(m), "HB9IPH YO1YO R-10");
        EXPECT_EQ(m.to_string(), "HB9IPH YO1YO R-10");
    }

    // 9. Type 9: M73_STD
    {
        Message m;
        m.type = MessageType::M73_STD;
        m.call_1 = "HB9IPH";
        m.call_2 = "YO1YO";
        EXPECT_EQ(format_message(m), "HB9IPH YO1YO 73");
        EXPECT_EQ(m.to_string(), "HB9IPH YO1YO 73");
    }

    // 10. Type 10: M73_NONSTD
    {
        Message m;
        m.type = MessageType::M73_NONSTD;
        m.call_1 = "YO1YO/P";
        m.hash_1 = hash_callsign_24("YO1YO/P");
        m.call_2 = "HB9IPH";
        EXPECT_EQ(format_message(m), "<YO1YO/P> HB9IPH 73");
        EXPECT_EQ(m.to_string(), "<YO1YO/P> HB9IPH 73");
    }

    // 11. Type 11: MULTI_REPORT73
    {
        Message m;
        m.type = MessageType::MULTI_REPORT73;
        m.hash_1 = 0x1234;
        m.call_1 = "HB9IPH";
        m.multi_targets = {
            {"YO1YO", 0x112233, 5},
            {"TU2TU", 0x445566, -3}
        };
        EXPECT_EQ(format_message(m), "<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>");
        EXPECT_EQ(m.to_string(), "<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>");
    }

    // 12. Type 12: MULTI_73
    {
        Message m;
        m.type = MessageType::MULTI_73;
        m.hash_1 = 0x1234;
        m.call_1 = "HB9IPH";
        m.multi_targets = {
            {"YO1YO", 0x112233, 0},
            {"TU2TU", 0x445566, 0}
        };
        EXPECT_EQ(format_message(m), "<YO1YO> <TU2TU> <HB9IPH> 73");
        EXPECT_EQ(m.to_string(), "<YO1YO> <TU2TU> <HB9IPH> 73");
    }

    // 13. Type 13: FREE_TEXT
    {
        Message m;
        m.type = MessageType::FREE_TEXT;
        m.text = "73 DE HB9IPH";
        EXPECT_EQ(format_message(m), "73 DE HB9IPH");
        EXPECT_EQ(m.to_string(), "73 DE HB9IPH");
    }

    // 14. Types 14-16: RESERVED
    {
        Message m;
        m.type = MessageType::RESERVED_A;
        EXPECT_EQ(format_message(m), "[RESERVED]");
        EXPECT_EQ(m.to_string(), "[RESERVED]");
    }
}

TEST(MessageTest, DirectPayloadToTextConversionsAndDeterminism) {
    // Standard CQ Message payload
    Message orig;
    orig.type = MessageType::CQ_STD;
    orig.call_1 = "HB9IPH";
    orig.locator = "JN47";
    uint8_t payload[PAYLOAD_BYTES] = {0};
    ASSERT_TRUE(encode_message(orig, payload));

    // Multiple deterministic runs produce exact identical text
    for (int iter = 0; iter < 50; ++iter) {
        std::string text = payload_to_text(payload);
        EXPECT_EQ(text, "CQ HB9IPH JN47");
    }

    // Multi-report with hash resolution via payload_to_text
    Message multi;
    multi.type = MessageType::MULTI_REPORT73;
    multi.call_1 = "HB9IPH";
    multi.hash_1 = hash_callsign_16("HB9IPH");
    multi.multi_targets = {
        {"YO1YO", hash_callsign_24("YO1YO"), 5},
        {"TU2TU", hash_callsign_24("TU2TU"), -3}
    };
    uint8_t multi_payload[PAYLOAD_BYTES] = {0};
    ASSERT_TRUE(encode_message(multi, multi_payload));

    // Without known calls -> <hex> displayed for unresolved
    std::string unres_text = payload_to_text(multi_payload);
    EXPECT_EQ(unres_text, "<f674cb> R+05 <0b670e> R-03 <7a8b>");

    // With known calls -> fully resolved text with <CALL>
    std::vector<std::string_view> known = {"HB9IPH", "YO1YO", "TU2TU"};
    std::string res_text = payload_to_text(multi_payload, known.data(), known.size());
    EXPECT_EQ(res_text, "<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>");
}

TEST(MessageTest, CanonicalPrecedenceAndCollisionDisambiguation) {
    std::vector<std::string_view> known = {"HB9IPH", "HB9IPH/P", "W1AW", "W1AW/P", "EA6/W1AW", "EA8/HB9IPH/P", "YO1YO", "TU2TU"};

    // 1. Type 7: CALL_NONSTD with Non-Standard Caller -> Standard Target Hash (20-bit) IS resolved
    {
        Message m;
        m.type = MessageType::CALL_NONSTD;
        m.call_2 = "EA6/W1AW"; // Non-standard caller
        m.hash_1 = hash_callsign_20("HB9IPH"); // 20-bit standard target hash
        EXPECT_TRUE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_EQ(m.call_1, "HB9IPH");
    }

    // 2. Type 7: CALL_NONSTD with Non-Standard Target Hash (20-bit) IS resolved
    {
        Message m;
        m.type = MessageType::CALL_NONSTD;
        m.call_2 = "EA6/W1AW"; // Non-standard caller
        m.hash_1 = hash_callsign_20("EA8/HB9IPH/P"); // 20-bit non-standard target hash
        EXPECT_TRUE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_EQ(m.call_1, "EA8/HB9IPH/P");
    }

    // 3a. Type 6: CALL_STD_SUF with Standard Caller (no suffix) -> Standard Target Hash (no suffix) MUST be rejected as collision
    // (Because transmitter would have used Type 5 CALL_STD_NOSUF if target had no suffix)
    {
        Message m;
        m.type = MessageType::CALL_STD_SUF;
        m.call_2 = "W1AW"; // Standard caller without suffix
        m.suffix_2 = 0;
        m.hash_1 = hash_callsign_24("HB9IPH"); // Standard target hash without suffix
        EXPECT_FALSE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_TRUE(m.call_1.empty() || m.call_1.front() == '<');
    }

    // 3b. Type 6: CALL_STD_SUF with Standard Caller WITH suffix (/P) -> Standard Target Hash IS resolved
    // (Because caller has /P suffix which does not fit in Type 5)
    {
        Message m;
        m.type = MessageType::CALL_STD_SUF;
        m.call_2 = "W1AW/P"; // Standard caller with /P suffix
        m.suffix_2 = 1;
        m.hash_1 = hash_callsign_24("HB9IPH"); // Standard target hash
        EXPECT_TRUE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_EQ(m.call_1, "HB9IPH");
    }

    // 3c. Type 6: CALL_STD_SUF with Standard Caller (no suffix) -> Non-Standard Target Hash IS resolved
    // (Because non-standard target does not fit in Type 5)
    {
        Message m;
        m.type = MessageType::CALL_STD_SUF;
        m.call_2 = "W1AW"; // Standard caller without suffix
        m.suffix_2 = 0;
        m.hash_1 = hash_callsign_24("EA8/HB9IPH/P"); // Non-standard target hash
        EXPECT_TRUE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_EQ(m.call_1, "EA8/HB9IPH/P");
    }

    // 3d. Type 6: CALL_STD_SUF with Standard Caller (no suffix) -> Standard Target Hash WITH suffix (/P) IS resolved
    // (Because target suffix /P does not fit in Type 5)
    {
        Message m;
        m.type = MessageType::CALL_STD_SUF;
        m.call_2 = "W1AW"; // Standard caller without suffix
        m.suffix_2 = 0;
        m.hash_1 = hash_callsign_24("HB9IPH/P"); // Standard target with suffix
        EXPECT_TRUE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_EQ(m.call_1, "HB9IPH/P");
    }

    // 4. Type 10: M73_NONSTD with Standard Sender -> Standard Target Hash collision MUST be rejected
    // (Because transmitter would have used Type 9 M73_STD)
    {
        Message m;
        m.type = MessageType::M73_NONSTD;
        m.call_2 = "W1AW"; // Standard sender
        m.hash_1 = hash_callsign_24("HB9IPH"); // Standard target hash
        EXPECT_FALSE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_TRUE(m.call_1.empty() || m.call_1.front() == '<');
    }

    // 4b. Type 10: M73_NONSTD with Standard Sender (/P) -> Standard Target Hash collision MUST be rejected
    // (Because Type 9 supports 1-bit suffix for both stations)
    {
        Message m;
        m.type = MessageType::M73_NONSTD;
        m.call_2 = "W1AW/P"; // Standard sender with suffix
        m.suffix_2 = 1;
        m.hash_1 = hash_callsign_24("HB9IPH"); // Standard target hash
        EXPECT_FALSE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_TRUE(m.call_1.empty() || m.call_1.front() == '<');
    }

    // 5. Type 10: M73_NONSTD with Non-Standard Sender -> Standard Target Hash IS resolved
    {
        Message m;
        m.type = MessageType::M73_NONSTD;
        m.call_2 = "EA6/W1AW"; // Non-standard sender
        m.hash_1 = hash_callsign_24("HB9IPH"); // Standard target hash
        EXPECT_TRUE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_EQ(m.call_1, "HB9IPH");
    }

    // 6. Type 11: MULTI_REPORT73 allows standard target hashes
    {
        Message m;
        m.type = MessageType::MULTI_REPORT73;
        m.hash_1 = hash_callsign_16("W1AW");
        m.multi_targets = {
            {"", hash_callsign_24("HB9IPH"), 5},
            {"", hash_callsign_24("YO1YO"), -3}
        };
        EXPECT_TRUE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_EQ(m.call_1, "W1AW");
        EXPECT_EQ(m.multi_targets[0].call, "HB9IPH");
        EXPECT_EQ(m.multi_targets[1].call, "YO1YO");
    }

    // 7. Type 12: MULTI_73 allows standard target hashes
    {
        Message m;
        m.type = MessageType::MULTI_73;
        m.hash_1 = hash_callsign_16("W1AW");
        m.multi_targets = {
            {"", hash_callsign_24("HB9IPH"), 0},
            {"", hash_callsign_24("TU2TU"), 0}
        };
        EXPECT_TRUE(resolve_callsigns(m, known.data(), known.size()));
        EXPECT_EQ(m.call_1, "W1AW");
        EXPECT_EQ(m.multi_targets[0].call, "HB9IPH");
        EXPECT_EQ(m.multi_targets[1].call, "TU2TU");
    }
}

TEST(MessageTest, CanonicalPrecedenceEncodingSelectionForAllMessages) {
    // 1. CALL format precedence:
    // Type 5 (CALL_STD_NOSUF) > Type 6 (CALL_STD_SUF) > Type 7 (CALL_NONSTD)
    {
        // Std caller + Std target (no suffix) -> MUST produce Type 5
        Message m1 = make_call("YO1YO", "HB9IPH", "JN47", -10);
        EXPECT_EQ(m1.type, MessageType::CALL_STD_NOSUF);
        Message pm1;
        ASSERT_TRUE(parse_message("YO1YO HB9IPH JN47 -10", pm1));
        EXPECT_EQ(pm1.type, MessageType::CALL_STD_NOSUF);

        // Std caller + Std target with /P -> MUST produce Type 6
        Message m2 = make_call("YO1YO/P", "HB9IPH", "JN47", -10);
        EXPECT_EQ(m2.type, MessageType::CALL_STD_SUF);
        Message pm2;
        ASSERT_TRUE(parse_message("YO1YO/P HB9IPH JN47 -10", pm2));
        EXPECT_EQ(pm2.type, MessageType::CALL_STD_SUF);

        // Std caller with /P + Std target -> MUST produce Type 6
        Message m3 = make_call("YO1YO", "HB9IPH/P", "JN47", -10);
        EXPECT_EQ(m3.type, MessageType::CALL_STD_SUF);
        Message pm3;
        ASSERT_TRUE(parse_message("YO1YO HB9IPH/P JN47 -10", pm3));
        EXPECT_EQ(pm3.type, MessageType::CALL_STD_SUF);

        // Std caller + NonStd target -> MUST produce Type 6
        Message m4 = make_call("EA8/HB9IP", "HB9IPH", "JN47", -10);
        EXPECT_EQ(m4.type, MessageType::CALL_STD_SUF);
        Message pm4;
        ASSERT_TRUE(parse_message("EA8/HB9IP HB9IPH JN47 -10", pm4));
        EXPECT_EQ(pm4.type, MessageType::CALL_STD_SUF);

        // NonStd caller (<=9 chars) + Std target -> MUST produce Type 7
        Message m5 = make_call("YO1YO", "EA8/HB9IP", "JN47", -10);
        EXPECT_EQ(m5.type, MessageType::CALL_NONSTD);
        Message pm5;
        ASSERT_TRUE(parse_message("YO1YO EA8/HB9IP JN47 -10", pm5));
        EXPECT_EQ(pm5.type, MessageType::CALL_NONSTD);
    }

    // 2. REPORT+73 format precedence:
    // Type 8 (REPORT73_STD) > Type 11 (MULTI_REPORT73)
    {
        // Std caller + Std target -> MUST produce Type 8
        Message m1 = make_report73("YO1YO", "HB9IPH", -10);
        EXPECT_EQ(m1.type, MessageType::REPORT73_STD);
        Message pm1;
        ASSERT_TRUE(parse_message("YO1YO HB9IPH R-10", pm1));
        EXPECT_EQ(pm1.type, MessageType::REPORT73_STD);

        // Std caller with /P + Std target with /P -> MUST produce Type 8
        Message m2 = make_report73("YO1YO/P", "HB9IPH/P", -10);
        EXPECT_EQ(m2.type, MessageType::REPORT73_STD);
        Message pm2;
        ASSERT_TRUE(parse_message("YO1YO/P HB9IPH/P R-10", pm2));
        EXPECT_EQ(pm2.type, MessageType::REPORT73_STD);

        // Std caller + NonStd target -> MUST produce Type 11
        Message m3 = make_report73("EA8/HB9IP", "HB9IPH", -10);
        EXPECT_EQ(m3.type, MessageType::MULTI_REPORT73);

        // NonStd caller + Std target -> MUST produce Type 11
        Message m4 = make_report73("YO1YO", "EA8/HB9IP", -10);
        EXPECT_EQ(m4.type, MessageType::MULTI_REPORT73);

        // Multiple targets -> MUST produce Type 11
        Message pm5;
        ASSERT_TRUE(parse_message("<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>", pm5));
        EXPECT_EQ(pm5.type, MessageType::MULTI_REPORT73);
    }

    // 3. 73 format precedence:
    // Type 9 (M73_STD) > Type 10 (M73_NONSTD) > Type 12 (MULTI_73)
    {
        // Std caller + Std target -> MUST produce Type 9
        Message m1 = make_73("YO1YO", "HB9IPH");
        EXPECT_EQ(m1.type, MessageType::M73_STD);
        Message pm1;
        ASSERT_TRUE(parse_message("YO1YO HB9IPH 73", pm1));
        EXPECT_EQ(pm1.type, MessageType::M73_STD);

        // Std caller with /P + Std target with /P -> MUST produce Type 9
        Message m2 = make_73("YO1YO/P", "HB9IPH/P");
        EXPECT_EQ(m2.type, MessageType::M73_STD);
        Message pm2;
        ASSERT_TRUE(parse_message("YO1YO/P HB9IPH/P 73", pm2));
        EXPECT_EQ(pm2.type, MessageType::M73_STD);

        // NonStd caller (<=9 chars) + Std target -> MUST produce Type 10
        Message m3 = make_73("YO1YO", "EA8/HB9IP");
        EXPECT_EQ(m3.type, MessageType::M73_NONSTD);
        Message pm3;
        ASSERT_TRUE(parse_message("YO1YO EA8/HB9IP 73", pm3));
        EXPECT_EQ(pm3.type, MessageType::M73_NONSTD);

        // Multi-73 targets -> MUST produce Type 12
        Message pm4;
        ASSERT_TRUE(parse_message("<YO1YO> <TU2TU> <HB9IPH> 73", pm4));
        EXPECT_EQ(pm4.type, MessageType::MULTI_73);
    }

    // 4. CQ format precedence:
    // Type 1 (CQ_STD) > Type 2 (CQ_NONSTD_1) > Type 3 (CQ_NONSTD_2) > Type 4 (CQ_NONSTD_3)
    {
        // Std callsign with locator -> Type 1
        Message m1 = make_cq("HB9IPH", "JN47");
        EXPECT_EQ(m1.type, MessageType::CQ_STD);

        // Std callsign with modifier and locator -> Type 1 (CQ_STD with modifier)
        Message m2 = make_cq("HB9IPH", "JN47", "DX");
        EXPECT_EQ(m2.type, MessageType::CQ_STD);

        // NonStd callsign <= 9 chars with locator -> Type 2
        Message m3 = make_cq("EA8/HB9IP", "JN47");
        EXPECT_EQ(m3.type, MessageType::CQ_NONSTD_1);

        // NonStd callsign <= 9 chars with modifier (no locator) -> Type 3
        Message m4 = make_cq("EA8/HB9IP", "", "DX");
        EXPECT_EQ(m4.type, MessageType::CQ_NONSTD_2);

        // NonStd compound callsign > 9 chars (up to 13 chars) -> Type 4
        Message m5 = make_cq("3B9/HB9IPH/P", "");
        EXPECT_EQ(m5.type, MessageType::CQ_NONSTD_3);
    }
}

TEST(MessageTest, DecodeMessageRejectsCorruptPayloads) {
    Message msg;
    uint8_t zero_payload[PAYLOAD_BYTES] = {0};
    EXPECT_FALSE(decode_message(zero_payload, msg));

    uint8_t ff_payload[PAYLOAD_BYTES] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    EXPECT_FALSE(decode_message(ff_payload, msg));
}
