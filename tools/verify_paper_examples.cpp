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
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <cstdint>

#include "lq/lq.h"
#include "lq/crc.h"
#include "lq/hash.h"
#include "lq/locator.h"
#include "lq/modifier.h"
#include "lq/rst.h"
#include "lq/varicode.h"
#include "lq/callsign.h"
#include "lq/callsign_nonstd.h"

using namespace lq;

namespace {

int g_tests_run = 0;
int g_tests_passed = 0;

std::string bytes_to_hex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        if (i > 0) oss << " ";
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::string crc_to_hex(uint16_t crc) {
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << crc;
    return oss.str();
}

void record_test(const std::string& name, bool pass, const std::string& details = "") {
    ++g_tests_run;
    if (pass) {
        ++g_tests_passed;
        std::cout << "  [PASS] " << name << std::endl;
    } else {
        std::cout << "  [FAIL] " << name;
        if (!details.empty()) {
            std::cout << " -> " << details;
        }
        std::cout << std::endl;
    }
}

} // anonymous namespace

void verify_section_5_fields() {
    std::cout << "\n=== SECTION 5: FIELD ENCODINGS & DICTIONARY VERIFICATION ===" << std::endl;

    // 1. Standard Callsign Encoding (Section 5.1)
    {
        uint32_t val = 0;
        encode_callsign_std("HB9IPH", val);
        record_test("Standard Callsign HB9IPH -> 129,895,118", val == 129895118U, std::to_string(val));

        encode_callsign_std("YO1YO", val);
        record_test("Standard Callsign YO1YO  -> 252,768,033", val == 252768033U, std::to_string(val));

        encode_callsign_std("W1AW", val);
        record_test("Standard Callsign W1AW   -> 6,319,593", val == 6319593U, std::to_string(val));
    }

    // 2. Call-Hash Allocations (Section 5.2)
    {
        record_test("Call-Hash 24-bit (HB9IPH) -> 0x5FE437", hash_callsign_24("HB9IPH") == 0x5FE437, crc_to_hex(hash_callsign_24("HB9IPH")));
        record_test("Call-Hash 24-bit (YO1YO)  -> 0xF674CB", hash_callsign_24("YO1YO") == 0xF674CB, crc_to_hex(hash_callsign_24("YO1YO")));
        record_test("Call-Hash 24-bit (TU2TU)  -> 0x0B670E", hash_callsign_24("TU2TU") == 0x0B670E, crc_to_hex(hash_callsign_24("TU2TU")));
        record_test("Call-Hash 24-bit (W1AW)   -> 0x0A6073", hash_callsign_24("W1AW") == 0x0A6073, crc_to_hex(hash_callsign_24("W1AW")));

        record_test("Call-Hash 20-bit (HB9IPH) -> 0x5FE43", hash_callsign_20("HB9IPH") == 0x5FE43, crc_to_hex(hash_callsign_20("HB9IPH")));
        record_test("Call-Hash 20-bit (YO1YO)  -> 0xF674C", hash_callsign_20("YO1YO") == 0xF674C, crc_to_hex(hash_callsign_20("YO1YO")));
        record_test("Call-Hash 16-bit (HB9IPH) -> 0x7A8B", hash_callsign_16("HB9IPH") == 0x7A8B, crc_to_hex(hash_callsign_16("HB9IPH")));
    }

    // 3. Grid Locators (Section 5.4)
    {
        uint16_t loc_val = 0;
        encode_locator_15("JN47", loc_val);
        record_test("Grid Locator JN47 -> 17,547 (0x448B)", loc_val == 17547U, std::to_string(loc_val));

        encode_locator_15("KN24", loc_val);
        record_test("Grid Locator KN24 -> 19,324 (0x4B7C)", loc_val == 19324U, std::to_string(loc_val));

        encode_locator_15("FN31", loc_val);
        record_test("Grid Locator FN31 -> 10,331 (0x285B)", loc_val == 10331U, std::to_string(loc_val));
    }

    // 4. Modifiers (Section 5.5)
    {
        uint32_t mod_val = 0;
        encode_modifier_20("040", mod_val);
        record_test("Numeric Modifier 040 -> 41", mod_val == 41U, std::to_string(mod_val));

        encode_modifier_20("DX", mod_val);
        record_test("Alphanumeric Modifier DX -> 156,648 (0x263E8)", mod_val == 156648U, std::to_string(mod_val));

        encode_modifier_20("POTA", mod_val);
        record_test("Alphanumeric Modifier POTA -> 541,289 (0x84269)", mod_val == 541289U, std::to_string(mod_val));
    }

    // 5. Signal Reports / RST (Section 5.6)
    {
        record_test("RST -26 dB -> 0 (00000b)", encode_rst_5(-26) == 0);
        record_test("RST -21 dB -> 5 (00101b)", encode_rst_5(-21) == 5);
        record_test("RST -10 dB -> 16 (10000b)", encode_rst_5(-10) == 16);
        record_test("RST -03 dB -> 23 (10111b)", encode_rst_5(-3) == 23);
        record_test("RST  00 dB -> 26 (11010b)", encode_rst_5(0) == 26);
        record_test("RST +05 dB -> 31 (11111b)", encode_rst_5(5) == 31);
    }

    // 6. Varicode Special Sentinels (Section 5.7)
    {
        record_test("Varicode EOM Sentinel ('\\x01')", EOM_CHAR == '\x01');
        record_test("Varicode FILL Sentinel ('\\x04')", FILL_CHAR == '\x04');
        record_test("Varicode Alphabet Size (104)", VARICODE_ALPHABET_SIZE == 104);
    }
}

void verify_appendix_worked_examples() {
    std::cout << "\n=== APPENDIX A: WORKED EXAMPLES & SERIALIZATION VERIFICATION ===" << std::endl;

    uint8_t payload[PAYLOAD_BYTES];
    Message msg;
    Message decoded;

    // 1. CQ Standard (Type 1) --- CQ DX HB9IPH JN47
    {
        msg = Message{};
        msg.type = MessageType::CQ_STD;
        msg.call_1 = "HB9IPH";
        msg.modifier = "DX";
        msg.locator = "JN47";

        record_test("Type 1 Encode", encode_message(msg, payload));
        record_test("Type 1 Packed Hex: '00 1E F8 2B 38 4C 7D 11 22 C0'",
                    bytes_to_hex(payload, 10) == "00 1E F8 2B 38 4C 7D 11 22 C0", bytes_to_hex(payload, 10));
        record_test("Type 1 CRC-14: 0x1606", compute_payload_crc14(payload) == 0x1606, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 1 Decode Round-Trip", decode_message(payload, decoded) && decoded == msg);
    }

    // 2. CQ Non-Std 1 (Type 2, 9-char + loc) --- CQ EA6/HB9IP JN47
    {
        msg = Message{};
        msg.type = MessageType::CQ_NONSTD_1;
        msg.call_1 = "EA6/HB9IP";
        msg.locator = "JN47";

        record_test("Type 2 Encode", encode_message(msg, payload));
        record_test("Type 2 Packed Hex: '00 44 FE 1E 45 84 1A 91 22 C0'",
                    bytes_to_hex(payload, 10) == "00 44 FE 1E 45 84 1A 91 22 C0", bytes_to_hex(payload, 10));
        record_test("Type 2 CRC-14: 0x2AC8", compute_payload_crc14(payload) == 0x2AC8, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 2 Decode Round-Trip", decode_message(payload, decoded) && decoded == msg);
    }

    // 3. CQ Non-Std 2 (Type 3, 9-char + mod) --- CQ TEST EA6/HB9IP
    {
        msg = Message{};
        msg.type = MessageType::CQ_NONSTD_2;
        msg.call_1 = "EA6/HB9IP";
        msg.modifier = "TEST";

        record_test("Type 3 Encode", encode_message(msg, payload));
        record_test("Type 3 Packed Hex: '01 13 F8 79 16 10 6A 50 D2 E0'",
                    bytes_to_hex(payload, 10) == "01 13 F8 79 16 10 6A 50 D2 E0", bytes_to_hex(payload, 10));
        record_test("Type 3 CRC-14: 0x1AED", compute_payload_crc14(payload) == 0x1AED, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 3 Decode Round-Trip", decode_message(payload, decoded) && decoded == msg);
    }

    // 4. CQ Non-Std 3 (Type 4, 13-char) --- CQ 3B9/HB9IPH/P
    {
        msg = Message{};
        msg.type = MessageType::CQ_NONSTD_3;
        msg.call_1 = "3B9/HB9IPH/P";
        msg.suffix_1 = 1;

        record_test("Type 4 Encode", encode_message(msg, payload));
        record_test("Type 4 Packed Hex: '02 F4 61 78 18 E2 22 FA 76 08'",
                    bytes_to_hex(payload, 10) == "02 F4 61 78 18 E2 22 FA 76 08", bytes_to_hex(payload, 10));
        record_test("Type 4 CRC-14: 0x268E", compute_payload_crc14(payload) == 0x268E, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 4 Decode Round-Trip", decode_message(payload, decoded) && decoded.type == MessageType::CQ_NONSTD_3);
    }

    // 5. CALL Standard No Suffix (Type 5) --- CALL YO1YO HB9IPH JN47 -03
    {
        msg = Message{};
        msg.type = MessageType::CALL_STD_NOSUF;
        msg.call_1 = "YO1YO";
        msg.call_2 = "HB9IPH";
        msg.locator = "JN47";
        msg.rst_db = -3;

        record_test("Type 5 Encode", encode_message(msg, payload));
        record_test("Type 5 Packed Hex: 'F8 87 79 0B DF 05 67 44 8B B8'",
                    bytes_to_hex(payload, 10) == "F8 87 79 0B DF 05 67 44 8B B8", bytes_to_hex(payload, 10));
        record_test("Type 5 CRC-14: 0x25A0", compute_payload_crc14(payload) == 0x25A0, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 5 Decode Round-Trip", decode_message(payload, decoded) && decoded == msg);
    }

    // 6. CALL Standard + Suffix / Hash (Type 6) --- CALL <01A4F2> HB9IPH/P JN47 -03
    {
        msg = Message{};
        msg.type = MessageType::CALL_STD_SUF;
        msg.hash_1 = 0x01A4F2;
        msg.call_2 = "HB9IPH/P";
        msg.suffix_2 = 1;
        msg.locator = "JN47";
        msg.rst_db = -3;

        record_test("Type 6 Encode", encode_message(msg, payload));
        record_test("Type 6 Packed Hex: '40 1A 4F 27 BE 0A CE C4 8B B8'",
                    bytes_to_hex(payload, 10) == "40 1A 4F 27 BE 0A CE C4 8B B8", bytes_to_hex(payload, 10));
        record_test("Type 6 CRC-14: 0x3B36", compute_payload_crc14(payload) == 0x3B36, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 6 Decode Round-Trip", decode_message(payload, decoded) && decoded.type == MessageType::CALL_STD_SUF &&
                    decoded.hash_1 == msg.hash_1 && decoded.call_2 == msg.call_2 && decoded.rst_db == msg.rst_db);
    }

    // 7. CALL Non-Std (Type 7) --- <01A4F> EA6/HB9IP/P -03
    {
        msg = Message{};
        msg.type = MessageType::CALL_NONSTD;
        msg.hash_1 = 0x01A4F;
        msg.call_2 = "EA6/HB9IP/P";
        msg.suffix_2 = 1;
        msg.rst_db = -3;

        record_test("Type 7 Encode", encode_message(msg, payload));
        record_test("Type 7 Packed Hex: '20 34 9E 27 F0 F2 2C 20 D5 B8'",
                    bytes_to_hex(payload, 10) == "20 34 9E 27 F0 F2 2C 20 D5 B8", bytes_to_hex(payload, 10));
        record_test("Type 7 CRC-14: 0x14C4", compute_payload_crc14(payload) == 0x14C4, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 7 Decode Round-Trip", decode_message(payload, decoded) && decoded.type == MessageType::CALL_NONSTD &&
                    decoded.hash_1 == msg.hash_1 && decoded.call_2 == "EA6/HB9IP/P" && decoded.suffix_2 == 1 && decoded.rst_db == -3);
    }

    // 8. REPORT+73 Standard (Type 8) --- HB9IPH/P YO1YO R+05
    {
        msg = Message{};
        msg.type = MessageType::REPORT73_STD;
        msg.call_1 = "HB9IPH/P";
        msg.suffix_1 = 1;
        msg.call_2 = "YO1YO";
        msg.suffix_2 = 0;
        msg.rst_db = 5;

        record_test("Type 8 Encode", encode_message(msg, payload));
        record_test("Type 8 Packed Hex: '00 9E F8 2B 3B E2 1D E4 2F 80'",
                    bytes_to_hex(payload, 10) == "00 9E F8 2B 3B E2 1D E4 2F 80", bytes_to_hex(payload, 10));
        record_test("Type 8 CRC-14: 0x2CED", compute_payload_crc14(payload) == 0x2CED, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 8 Decode Round-Trip", decode_message(payload, decoded) && decoded.type == MessageType::REPORT73_STD);
    }

    // 9. 73 Standard (Type 9) --- HB9IPH/P YO1YO 73
    {
        msg = Message{};
        msg.type = MessageType::M73_STD;
        msg.call_1 = "HB9IPH/P";
        msg.suffix_1 = 1;
        msg.call_2 = "YO1YO";
        msg.suffix_2 = 0;

        record_test("Type 9 Encode", encode_message(msg, payload));
        record_test("Type 9 Packed Hex: '00 DE F8 2B 3B E2 1D E4 20 00'",
                    bytes_to_hex(payload, 10) == "00 DE F8 2B 3B E2 1D E4 20 00", bytes_to_hex(payload, 10));
        record_test("Type 9 CRC-14: 0x1AB2", compute_payload_crc14(payload) == 0x1AB2, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 9 Decode Round-Trip", decode_message(payload, decoded) && decoded.type == MessageType::M73_STD);
    }

    // 10. 73 Non-Standard (Type 10) --- <3B9/HB9IP> EA6/HB9IP/P 73
    {
        msg = Message{};
        msg.type = MessageType::M73_NONSTD;
        msg.hash_1 = hash_callsign_24("3B9/HB9IP");
        msg.call_2 = "EA6/HB9IP/P";
        msg.suffix_2 = 1;

        record_test("Type 10 Encode", encode_message(msg, payload));
        record_test("Type 10 Packed Hex: '1F E5 71 D1 3F 87 91 61 06 A8'",
                    bytes_to_hex(payload, 10) == "1F E5 71 D1 3F 87 91 61 06 A8", bytes_to_hex(payload, 10));
        record_test("Type 10 CRC-14: 0x2D94", compute_payload_crc14(payload) == 0x2D94, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 10 Decode Round-Trip", decode_message(payload, decoded) && decoded.type == MessageType::M73_NONSTD);
    }

    // 11. MULTI-REPORT+73 (Type 11) --- <YO1YO> R+05 <TU2TU> R-03 <HB9IPH>
    {
        msg = Message{};
        msg.type = MessageType::MULTI_REPORT73;
        msg.call_1 = "HB9IPH";
        msg.hash_1 = hash_callsign_16("HB9IPH");
        msg.multi_targets = {
            {"", hash_callsign_24("YO1YO"), 5},
            {"", hash_callsign_24("TU2TU"), -3}
        };

        record_test("Type 11 Encode", encode_message(msg, payload));
        record_test("Type 11 Packed Hex: '6F 51 7E CE 99 7F 0B 67 0E B8'",
                    bytes_to_hex(payload, 10) == "6F 51 7E CE 99 7F 0B 67 0E B8", bytes_to_hex(payload, 10));
        record_test("Type 11 CRC-14: 0x362B", compute_payload_crc14(payload) == 0x362B, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 11 Decode Round-Trip", decode_message(payload, decoded) && decoded.type == MessageType::MULTI_REPORT73 &&
                    decoded.hash_1 == msg.hash_1 && decoded.multi_targets.size() == 2U);
    }

    // 12. MULTI-73 (Type 12) --- <YO1YO> <TU2TU> <HB9IPH> 73
    {
        msg = Message{};
        msg.type = MessageType::MULTI_73;
        msg.call_1 = "HB9IPH";
        msg.hash_1 = hash_callsign_16("HB9IPH");
        msg.multi_targets = {
            {"", hash_callsign_24("YO1YO"), 0},
            {"", hash_callsign_24("TU2TU"), 0}
        };

        record_test("Type 12 Encode", encode_message(msg, payload));
        record_test("Type 12 Packed Hex: '07 7A 8B F6 74 CB 0B 67 0E 00'",
                    bytes_to_hex(payload, 10) == "07 7A 8B F6 74 CB 0B 67 0E 00", bytes_to_hex(payload, 10));
        record_test("Type 12 CRC-14: 0x38BC", compute_payload_crc14(payload) == 0x38BC, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 12 Decode Round-Trip", decode_message(payload, decoded) && decoded.type == MessageType::MULTI_73 &&
                    decoded.hash_1 == msg.hash_1 && decoded.multi_targets.size() == 2U);
    }

    // 13. FREE TEXT (Type 13) --- 73 DE HB9I
    {
        msg = Message{};
        msg.type = MessageType::FREE_TEXT;
        msg.text = "73 DE HB9I";

        record_test("Type 13 Encode", encode_message(msg, payload));
        record_test("Type 13 Packed Hex: '50 0C 01 A7 88 E8 60 12 81 C0'",
                    bytes_to_hex(payload, 10) == "50 0C 01 A7 88 E8 60 12 81 C0", bytes_to_hex(payload, 10));
        record_test("Type 13 CRC-14: 0x182D", compute_payload_crc14(payload) == 0x182D, crc_to_hex(compute_payload_crc14(payload)));
        record_test("Type 13 Decode Round-Trip", decode_message(payload, decoded) && decoded.type == MessageType::FREE_TEXT && decoded.text == "73 DE HB9I");
    }
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "  LQ DIGITAL MODE SPECIFICATION: PAPER EXAMPLES VERIFIER" << std::endl;
    std::cout << "================================================================" << std::endl;

    verify_section_5_fields();
    verify_appendix_worked_examples();

    std::cout << "\n----------------------------------------------------------------" << std::endl;
    std::cout << "  SUMMARY: " << g_tests_passed << " / " << g_tests_run << " VERIFICATIONS PASSED ("
              << (g_tests_passed * 100 / g_tests_run) << "%)" << std::endl;
    std::cout << "================================================================" << std::endl;

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}

