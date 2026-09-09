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
#include <iomanip>
#include "lq/lq.h"

using namespace lq;

int main() {
    // 1. Define sample messages
    Message msg_cq;
    msg_cq.type = MessageType::CQ_STD;
    msg_cq.call_1 = "HB9IPH";
    msg_cq.modifier = "DX";
    msg_cq.locator = "JN47";

    Message msg_call;
    msg_call.type = MessageType::CALL_STD;
    msg_call.call_1 = "YO1YO";
    msg_call.call_2 = "HB9IPH";
    msg_call.locator = "JN47";
    msg_call.rst_db = -3;

    Message msg_reply;
    msg_reply.type = MessageType::REPLY73_STD;
    msg_reply.call_1 = "HB9IPH";
    msg_reply.call_2 = "YO1YO";
    msg_reply.rst_db = 5;

    Message msg_text;
    msg_text.type = MessageType::FREE_TEXT;
    msg_text.text = "73 DE HB9IPH";

    struct ModeConfig {
        Protocol proto;
        std::string name;
    } modes[] = {
        {Protocol::LQ8, "LQ8"},
        {Protocol::LQ4, "LQ4"},
        {Protocol::LQ2, "LQ2"},
        {Protocol::LQ16, "LQ16"}
    };

    struct MsgEntry {
        Message msg;
        std::string desc;
    } msgs[] = {
        {msg_cq, "CQ DX HB9IPH JN47"},
        {msg_call, "YO1YO HB9IPH JN47 -03"},
        {msg_reply, "HB9IPH YO1YO R+05"},
        {msg_text, "73 DE HB9IPH"}
    };

    std::ofstream out("build/tones_data.json");
    out << "{\n";
    out << "  \"modes\": {\n";

    for (size_t m = 0; m < 4; ++m) {
        out << "    \"" << modes[m].name << "\": {\n";
        for (size_t i = 0; i < 4; ++i) {
            ToneSequence seq;
            encode_tones(msgs[i].msg, modes[m].proto, seq);

            out << "      \"msg" << (i + 1) << "\": {\n";
            out << "        \"text\": \"" << msgs[i].desc << "\",\n";
            out << "        \"num_tones\": " << (modes[m].proto == Protocol::LQ8 || modes[m].proto == Protocol::LQ16 ? 8 : 4) << ",\n";
            out << "        \"symbol_period\": " << seq.symbol_period << ",\n";
            out << "        \"tone_spacing\": " << seq.tone_spacing << ",\n";
            out << "        \"tx_duration\": " << seq.tx_duration << ",\n";
            out << "        \"tones\": [";
            for (size_t t = 0; t < seq.size(); ++t) {
                out << static_cast<int>(seq[t]) << (t + 1 < seq.size() ? ", " : "");
            }
            out << "]\n";
            out << "      }" << (i + 1 < 4 ? "," : "") << "\n";
        }
        out << "    }" << (m + 1 < 4 ? "," : "") << "\n";
    }

    out << "  }\n";
    out << "}\n";

    std::cout << "Exported tone sequences to build/tones_data.json" << std::endl;
    return 0;
}
