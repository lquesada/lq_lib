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
#include <filesystem>
#include <cstring>
#include "lq/lq.h"
#include "lq/constants.h"

namespace {

lq::Protocol parse_protocol_name(std::string_view name) {
    if (name == "LQ16") return lq::Protocol::LQ16;
    if (name == "LQ4") return lq::Protocol::LQ4;
    if (name == "LQ2") return lq::Protocol::LQ2;
    return lq::Protocol::LQ8;
}

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

std::string crc_to_bin(uint16_t crc) {
    std::string s;
    s.resize(14);
    for (int i = 0; i < 14; ++i) {
        s[i] = ((crc >> (13 - i)) & 1) ? '1' : '0';
    }
    return s;
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

int msg_type_to_id(lq::MessageType type) {
    switch (type) {
        case lq::MessageType::CQ_STD: return 1;
        case lq::MessageType::CQ_NONSTD_1: return 2;
        case lq::MessageType::CQ_NONSTD_2: return 3;
        case lq::MessageType::CQ_NONSTD_3: return 4;
        case lq::MessageType::CALL_STD_NOSUF: return 5;
        case lq::MessageType::CALL_STD_SUF: return 6;
        case lq::MessageType::CALL_NONSTD: return 7;
        case lq::MessageType::REPORT73_STD: return 8;
        case lq::MessageType::M73_STD: return 9;
        case lq::MessageType::M73_NONSTD: return 10;
        case lq::MessageType::MULTI_REPORT73: return 11;
        case lq::MessageType::MULTI_73: return 12;
        case lq::MessageType::FREE_TEXT: return 13;
        default: return 0;
    }
}

struct TestDef {
    std::string id_suffix;
    std::string text;
    std::string description;
    bool is_paper_example;
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    std::string golden_dir = "golden_vectors";
    if (argc > 1) {
        golden_dir = argv[1];
    }
    std::filesystem::create_directories(golden_dir);

    // Primary paths
    std::string json_path = golden_dir + "/golden_vectors.json";
    std::string txt_path = golden_dir + "/golden_vectors.txt";
    std::string paper_json_path = golden_dir + "/golden_vectors_paper.json";

    // Split suites
    std::string encoding_json_path = golden_dir + "/golden_vectors_encoding.json";
    std::string physical_json_path = golden_dir + "/golden_vectors_physical.json";
    std::string physical_txt_path = golden_dir + "/golden_vectors_physical.txt";

    std::cout << "=================================================================\n";
    std::cout << "    LQ Reference Library — Golden Vectors Generator Tool         \n";
    std::cout << "=================================================================\n";
    std::cout << "Generating comprehensive golden vector suites to: " << golden_dir << "\n";

    // 1. Comprehensive list of message test definitions across all 13 types & special cases
    std::vector<TestDef> test_defs = {
        // Type 1: CQ_STD
        {"CQ_STD_01", "CQ HB9IPH JN47", "Standard CQ with 4-char Maidenhead grid", true},
        {"CQ_STD_02", "CQ YO1YO KN24", "Standard CQ from Eastern European station", true},
        {"CQ_STD_04", "CQ DX HB9IPH JN47", "CQ with modifier DX", true},
        {"CQ_STD_05", "CQ POTA HB9IPH JN47", "CQ with modifier POTA (Parks on the Air)", true},
        {"CQ_STD_06", "CQ SOTA HB9IPH JN47", "CQ with modifier SOTA (Summits on the Air)", false},
        {"CQ_STD_07", "CQ QRP HB9IPH JN47", "CQ with modifier QRP (Low Power)", false},
        {"CQ_STD_08", "CQ IOTA HB9IPH JN47", "CQ with modifier IOTA (Islands on the Air)", false},
        {"CQ_STD_09", "CQ FD HB9IPH JN47", "CQ with modifier FD (Field Day)", false},
        {"CQ_STD_10", "CQ NA HB9IPH JN47", "CQ with modifier NA (North America)", false},
        {"CQ_STD_11", "CQ EU HB9IPH JN47", "CQ with modifier EU (Europe)", false},
        {"CQ_STD_12", "CQ 100K HB9IPH JN47", "CQ with custom alphanumeric modifier 100K", true},
        {"CQ_STD_13", "CQ 4X4 HB9IPH JN47", "CQ with custom alphanumeric modifier 4X4", false},
        {"CQ_STD_14", "CQ TEST HB9IPH JN47", "CQ with modifier TEST", false},
        {"CQ_STD_15", "CQ 040 HB9IPH JN47", "CQ with 3-digit numeric modifier 040", false},

        // Type 2: CQ_NONSTD_1 (Non-standard base callsign)
        {"CQ_NONSTD1_01", "CQ 3DA0RU JN47", "CQ with non-standard prefix 3DA0RU and Maidenhead grid", true},
        {"CQ_NONSTD1_02", "CQ DP0GVN IB59", "CQ with non-standard Antarctic station DP0GVN", false},
        {"CQ_NONSTD1_03", "CQ GB100BBC IO91", "CQ with non-standard special event callsign GB100BBC", false},

        // Type 3: CQ_NONSTD_2 (Non-standard base callsign with /P suffix)
        {"CQ_NONSTD2_01", "CQ 3DA0RU/P JN47", "CQ with non-standard callsign 3DA0RU and portable suffix /P", true},
        {"CQ_NONSTD2_02", "CQ DP0GVN/P IB59", "CQ with Antarctic station DP0GVN and portable suffix /P", false},

        // Type 4: CQ_NONSTD_3 (Non-standard prefix modifier)
        {"CQ_NONSTD3_01", "CQ VP8/HB9IPH JN47", "CQ with compound prefix VP8/HB9IPH", true},
        {"CQ_NONSTD3_02", "CQ EA8/HB9IPH IL18", "CQ with compound island prefix EA8/HB9IPH", false},

        // Type 5: CALL_STD_NOSUF (Standard call initiation)
        {"CALL_STD_01", "YO1YO HB9IPH JN47 -03", "Standard QSO initiation with RST -03 dB", true},
        {"CALL_STD_02", "W1AW HB9IPH FN31 +05", "Standard QSO initiation with maximum positive RST +05 dB", true},
        {"CALL_STD_03", "YO1YO HB9IPH JN47 -26", "Standard QSO initiation with minimum negative RST -26 dB", false},
        {"CALL_STD_04", "TU2TU HB9IPH IJ85 00", "Standard QSO initiation with 00 dB signal report", false},
        {"CALL_STD_05", "K1JT HB9IPH FN20 -10", "Standard QSO initiation with -10 dB signal report", false},

        // Type 6: CALL_STD_SUF (Standard call initiation with /P suffix or hash)
        {"CALL_SUF_01", "<01a4f2> HB9IPH/P JN47 -03", "Standard QSO initiation with hashed target and compound caller", true},
        {"CALL_SUF_02", "<0a6073> W1AW/P FN31 +05", "Standard QSO initiation with portable station", false},

        // Type 7: CALL_NONSTD (Non-standard caller with 20-bit target hash and SNR)
        {"CALL_NONSTD_01", "<01a4f> EA6/HB9IP/P -03", "Non-standard caller with 20-bit target hash and SNR", true},
        {"CALL_NONSTD_02", "<HB9IPH> 3B9/HB9IP/P +02", "Non-standard DXpedition station with 20-bit hash of HB9IPH", false},
        {"CALL_NONSTD_03", "<YO1YO> 3DA0RU/P -12", "Non-standard station with 20-bit hash of YO1YO", false},

        // Type 8: REPORT73_STD (Streamlined 1-bit prefix response with 73 confirmation)
        {"RPT73_STD_01", "HB9IPH/P YO1YO R+05", "Streamlined 1-bit response with RST +05 dB and 73", true},
        {"RPT73_STD_02", "YO1YO HB9IPH R-19", "Streamlined 1-bit response with RST -19 dB and 73", false},
        {"RPT73_STD_03", "W1AW HB9IPH R00", "Streamlined 1-bit response with RST 00 dB and 73", false},
        {"RPT73_STD_04", "TU2TU HB9IPH R-03", "Streamlined 1-bit response with RST -03 dB and 73", false},

        // Type 9: M73_STD (Standard station single 73 acknowledgment)
        {"M73_STD_01", "HB9IPH/P YO1YO 73", "Standard single-station 73 contact finalization", true},
        {"M73_STD_02", "W1AW HB9IPH 73", "Standard single-station 73 contact finalization with W1AW", false},

        // Type 10: M73_NONSTD (Non-standard station 73 acknowledgment with 24-bit hash)
        {"M73_NONSTD_01", "<1fe571> EA6/HB9IP/P 73", "Non-standard station 73 contact finalization with 24-bit target hash", true},
        {"M73_NONSTD_02", "<5fe437> DP0GVN/P 73", "Non-standard station 73 contact finalization with DP0GVN/P", false},
        {"M73_NONSTD_03", "<f674cb> 3DA0RU/P 73", "Non-standard station 73 contact finalization with 3DA0RU/P", false},

        // Type 11: MULTI_REPORT73 (Multi-station parallel pileup report + 73 confirmation)
        {"MULTI_REP73_01", "<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>", "Parallel confirmation for 2 pileup stations", true},
        {"MULTI_REP73_02", "<1717> R+05 <18d7> R-03 <1ea2>", "Parallel confirmation with 24-bit hashed callsigns", false},

        // Type 12: MULTI_73 (Multi-station parallel final 73 acknowledgment)
        {"MULTI_73_01", "<YO1YO> <TU2TU> <HB9IPH> 73", "Parallel 73 confirmation acknowledging 2 stations", true},
        {"MULTI_73_02", "<1717> <18d7> <1ea2> 73", "Parallel 73 confirmation acknowledging 2 call hashes", false},

        // Type 13: FREE_TEXT (104-symbol Varicode free-form text)
        {"FREE_TEXT_01", "73 DE HB9I", "Short free-text message", true},
        {"FREE_TEXT_02", "TNX 73 GL", "Standard CW/digital greeting free-text", true},
        {"FREE_TEXT_03", "SALUDOS 73", "Free-text with greeting", false},
        {"FREE_TEXT_04", "HELLO DE EA8", "Free-text station greeting", false},
        {"FREE_TEXT_05", "ESPAÑA 73", "Free-text with Latin-1 accented character", false}
    };

    std::vector<std::string> mode_names = {"LQ8", "LQ16", "LQ4", "LQ2"};

    std::ofstream json_out(json_path);
    std::ofstream paper_json_out(paper_json_path);
    std::ofstream txt_out(txt_path);
    std::ofstream enc_json_out(encoding_json_path);
    std::ofstream phy_json_out(physical_json_path);
    std::ofstream phy_txt_out(physical_txt_path);

    if (!json_out.is_open() || !txt_out.is_open() || !paper_json_out.is_open() ||
        !enc_json_out.is_open() || !phy_json_out.is_open() || !phy_txt_out.is_open()) {
        std::cerr << "Error: Could not open output files in " << golden_dir << "\n";
        return 1;
    }

    // Unified JSON Header
    std::string json_header = "{\n"
                              "  \"$schema\": \"https://json-schema.org/draft/2020-12/schema\",\n"
                              "  \"title\": \"LQ Digital Mode Family Golden Test Vectors Suite\",\n"
                              "  \"version\": \"1.0\",\n"
                              "  \"author\": \"Luis Quesada (HB9IPH) <luis@lq8.org>\",\n"
                              "  \"license\": \"MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)\",\n"
                              "  \"portal\": \"https://lq8.org\",\n"
                              "  \"description\": \"Comprehensive golden test vector suite containing canonical conversions, 77-bit Huffman bitfields, CRC-14 checksums, LDPC(174,91) codewords, and mode-specific physical channel tone sequences across all 4 modes (LQ8, LQ16, LQ4, LQ2).\",\n"
                              "  \"test_vectors\": [\n";

    json_out << json_header;
    paper_json_out << json_header;

    // Encoding-only JSON Header
    std::string enc_json_header = "{\n"
                                  "  \"$schema\": \"https://json-schema.org/draft/2020-12/schema\",\n"
                                  "  \"title\": \"LQ Digital Mode Family — Message Encoding & Decoding Golden Vectors Suite\",\n"
                                  "  \"version\": \"1.0\",\n"
                                  "  \"author\": \"Luis Quesada (HB9IPH) <luis@lq8.org>\",\n"
                                  "  \"license\": \"MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)\",\n"
                                  "  \"portal\": \"https://lq8.org\",\n"
                                  "  \"description\": \"Comprehensive golden test vector suite for message parsing, canonical formatting, 77-bit Huffman payload serialization, CRC-14 error detection, and LDPC(174,91) forward error correction codeword generation.\",\n"
                                  "  \"test_vectors\": [\n";
    enc_json_out << enc_json_header;

    // Physical-only JSON Header
    std::string phy_json_header = "{\n"
                                  "  \"$schema\": \"https://json-schema.org/draft/2020-12/schema\",\n"
                                  "  \"title\": \"LQ Digital Mode Family — Physical Layer Golden Vectors Suite\",\n"
                                  "  \"version\": \"1.0\",\n"
                                  "  \"author\": \"Luis Quesada (HB9IPH) <luis@lq8.org>\",\n"
                                  "  \"license\": \"MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)\",\n"
                                  "  \"portal\": \"https://lq8.org\",\n"
                                  "  \"description\": \"Physical layer golden test vectors with exact RF symbol timelines, Costas synchronization arrays, ramp symbols, tone mappings, and frequency offsets across all 4 modes (LQ8, LQ16, LQ4, LQ2).\",\n"
                                  "  \"test_vectors\": [\n";
    phy_json_out << phy_json_header;

    // TXT Headers
    txt_out << "=========================================================================================================\n";
    txt_out << "                         THE LQ DIGITAL MODE FAMILY — GOLDEN TEST VECTORS SUITE                          \n";
    txt_out << "=========================================================================================================\n";
    txt_out << "Author:  Luis Quesada (HB9IPH) <luis@lq8.org>\n";
    txt_out << "License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)\n";
    txt_out << "Portal:  https://lq8.org | GitHub: https://github.com/lquesada/lq_lib\n";
    txt_out << "=========================================================================================================\n\n";

    phy_txt_out << "=========================================================================================================\n";
    phy_txt_out << "                    THE LQ DIGITAL MODE FAMILY — PHYSICAL LAYER GOLDEN VECTORS SUITE                    \n";
    phy_txt_out << "=========================================================================================================\n";
    phy_txt_out << "Author:  Luis Quesada (HB9IPH) <luis@lq8.org>\n";
    phy_txt_out << "License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)\n";
    phy_txt_out << "Portal:  https://lq8.org | GitHub: https://github.com/lquesada/lq_lib\n";
    phy_txt_out << "=========================================================================================================\n\n";

    size_t total_vectors = 0;
    size_t total_paper_vectors = 0;
    bool first_json_entry = true;
    bool first_paper_json_entry = true;
    bool first_enc_json_entry = true;
    bool first_phy_json_entry = true;

    for (const auto& tdef : test_defs) {
        // Parse message
        lq::Message msg;
        if (!lq::parse_message(tdef.text, msg)) {
            std::cerr << "Warning: Could not parse message: " << tdef.text << "\n";
            continue;
        }

        uint8_t payload[lq::PAYLOAD_BYTES] = {0};
        if (!lq::encode_message(msg, payload)) {
            std::cerr << "Warning: Could not encode message: " << tdef.text << "\n";
            continue;
        }

        std::string hex_str = lq::payload_to_hex(payload);
        std::string bin_str = lq::payload_to_binary(payload);
        std::string canonical_text = lq::format_message(msg);
        int type_id = msg_type_to_id(msg.type);
        std::string type_name = msg_type_to_str(msg.type);

        uint16_t crc14 = lq::compute_payload_crc14(payload);
        std::string crc_hex_str = crc_to_hex(crc14);
        std::string crc_bin_str = crc_to_bin(crc14);

        // 91-bit LDPC input
        std::string ldpc_input_bin = bin_str + crc_bin_str;

        // 174-bit LDPC codeword
        uint8_t in_91[lq::LDPC_INPUT_BYTES] = {0};
        lq::append_crc14(payload, in_91);

        uint8_t codeword_bytes[lq::LDPC_CODEWORD_BYTES] = {0};
        lq::ldpc_encode(in_91, codeword_bytes);

        std::string codeword_bin;
        codeword_bin.reserve(174);
        for (int i = 0; i < 174; ++i) {
            uint8_t bit = (codeword_bytes[i / 8] >> (7 - (i % 8))) & 1;
            codeword_bin.push_back(bit ? '1' : '0');
        }
        std::string codeword_hex = bytes_to_hex(codeword_bytes, 22);

        for (const auto& mode_str : mode_names) {
            lq::Protocol proto = parse_protocol_name(mode_str);
            const auto prof = lq::get_protocol_params(proto);

            lq::ToneSequence seq;
            lq::encode_payload(payload, proto, seq);

            std::string vector_id = "GV_" + tdef.id_suffix + "_" + mode_str;
            std::string enc_vector_id = "GVE_" + tdef.id_suffix + "_" + mode_str;
            std::string phy_vector_id = "GVP_" + tdef.id_suffix + "_" + mode_str;

            // -------------------------------------------------------------
            // 1. Unified JSON Entry
            // -------------------------------------------------------------
            std::ostringstream joss;
            joss << "    {\n";
            joss << "      \"id\": \"" << vector_id << "\",\n";
            joss << "      \"description\": \"" << escape_json(tdef.description) << "\",\n";
            joss << "      \"mode\": \"" << mode_str << "\",\n";
            joss << "      \"message_type\": \"" << type_name << "\",\n";
            joss << "      \"type_id\": " << type_id << ",\n";
            joss << "      \"plaintext\": \"" << escape_json(tdef.text) << "\",\n";
            joss << "      \"canonical_text\": \"" << escape_json(canonical_text) << "\",\n";
            joss << "      \"is_paper_example\": " << (tdef.is_paper_example ? "true" : "false") << ",\n";
            joss << "      \"layers\": {\n";
            joss << "        \"payload_77bit_bin\": \"" << bin_str << "\",\n";
            joss << "        \"payload_10byte_hex\": \"" << hex_str << "\",\n";
            joss << "        \"crc14_val\": " << crc14 << ",\n";
            joss << "        \"crc14_hex\": \"" << crc_hex_str << "\",\n";
            joss << "        \"crc14_bin\": \"" << crc_bin_str << "\",\n";
            joss << "        \"ldpc_input_91bit_bin\": \"" << ldpc_input_bin << "\",\n";
            joss << "        \"codeword_174bit_bin\": \"" << codeword_bin << "\",\n";
            joss << "        \"codeword_22byte_hex\": \"" << codeword_hex << "\",\n";
            joss << "        \"tones_count\": " << seq.size() << ",\n";
            joss << "        \"tones\": [";
            for (size_t k = 0; k < seq.size(); ++k) {
                if (k > 0) joss << ", ";
                joss << static_cast<int>(seq[k]);
            }
            joss << "]\n";
            joss << "      },\n";
            joss << "      \"phy_timing\": {\n";
            joss << "        \"slot_duration_s\": " << prof.slot_duration << ",\n";
            joss << "        \"tx_duration_s\": " << prof.tx_duration << ",\n";
            joss << "        \"symbol_period_ms\": " << (prof.symbol_period * 1000.0f) << ",\n";
            joss << "        \"tone_spacing_hz\": " << prof.tone_spacing << ",\n";
            joss << "        \"bandwidth_hz\": " << prof.occupied_bw << ",\n";
            joss << "        \"baud_rate\": " << (1.0f / prof.symbol_period) << ",\n";
            joss << "        \"modulation\": \"" << (prof.num_tones == 8 ? "8-GFSK" : "4-GFSK") << "\"\n";
            joss << "      }\n";
            joss << "    }";

            if (!first_json_entry) {
                json_out << ",\n";
            }
            json_out << joss.str();
            first_json_entry = false;
            ++total_vectors;

            if (tdef.is_paper_example) {
                if (!first_paper_json_entry) {
                    paper_json_out << ",\n";
                }
                paper_json_out << joss.str();
                first_paper_json_entry = false;
                ++total_paper_vectors;
            }

            // -------------------------------------------------------------
            // 2. Encoding-Only JSON Entry
            // -------------------------------------------------------------
            std::ostringstream eoss;
            eoss << "    {\n";
            eoss << "      \"id\": \"" << enc_vector_id << "\",\n";
            eoss << "      \"description\": \"" << escape_json(tdef.description) << " in " << mode_str << "\",\n";
            eoss << "      \"mode\": \"" << mode_str << "\",\n";
            eoss << "      \"message_type\": \"" << type_name << "\",\n";
            eoss << "      \"type_id\": " << type_id << ",\n";
            eoss << "      \"plaintext\": \"" << escape_json(tdef.text) << "\",\n";
            eoss << "      \"canonical_text\": \"" << escape_json(canonical_text) << "\",\n";
            eoss << "      \"is_paper_example\": " << (tdef.is_paper_example ? "true" : "false") << ",\n";
            eoss << "      \"payload_77bit_bin\": \"" << bin_str << "\",\n";
            eoss << "      \"payload_10byte_hex\": \"" << hex_str << "\",\n";
            eoss << "      \"crc14_val\": " << crc14 << ",\n";
            eoss << "      \"crc14_hex\": \"" << crc_hex_str << "\",\n";
            eoss << "      \"crc14_bin\": \"" << crc_bin_str << "\",\n";
            eoss << "      \"ldpc_input_91bit_bin\": \"" << ldpc_input_bin << "\",\n";
            eoss << "      \"codeword_174bit_bin\": \"" << codeword_bin << "\",\n";
            eoss << "      \"codeword_22byte_hex\": \"" << codeword_hex << "\"\n";
            eoss << "    }";

            if (!first_enc_json_entry) {
                enc_json_out << ",\n";
            }
            enc_json_out << eoss.str();
            first_enc_json_entry = false;

            // -------------------------------------------------------------
            // 3. Physical-Only JSON Entry (with timeline & Costas sync)
            // -------------------------------------------------------------
            std::ostringstream poss;
            poss << "    {\n";
            poss << "      \"id\": \"" << phy_vector_id << "\",\n";
            poss << "      \"description\": \"" << escape_json(tdef.description) << " in " << mode_str << "\",\n";
            poss << "      \"mode\": \"" << mode_str << "\",\n";
            poss << "      \"message_type\": \"" << type_name << "\",\n";
            poss << "      \"type_id\": " << type_id << ",\n";
            poss << "      \"plaintext\": \"" << escape_json(tdef.text) << "\",\n";
            poss << "      \"canonical_text\": \"" << escape_json(canonical_text) << "\",\n";
            poss << "      \"is_paper_example\": " << (tdef.is_paper_example ? "true" : "false") << ",\n";
            poss << "      \"phy_parameters\": {\n";
            poss << "        \"slot_duration_s\": " << std::fixed << std::setprecision(2) << prof.slot_duration << ",\n";
            poss << "        \"tx_duration_s\": " << std::fixed << std::setprecision(2) << prof.tx_duration << ",\n";
            poss << "        \"guard_duration_s\": " << std::fixed << std::setprecision(2) << (prof.slot_duration - prof.tx_duration) << ",\n";
            poss << "        \"symbol_period_s\": " << std::fixed << std::setprecision(4) << prof.symbol_period << ",\n";
            poss << "        \"symbol_period_ms\": " << std::fixed << std::setprecision(1) << (prof.symbol_period * 1000.0f) << ",\n";
            poss << "        \"tone_spacing_hz\": " << std::fixed << std::setprecision(6) << prof.tone_spacing << ",\n";
            poss << "        \"bandwidth_hz\": " << std::fixed << std::setprecision(6) << prof.occupied_bw << ",\n";
            poss << "        \"baud_rate\": " << std::fixed << std::setprecision(6) << (1.0f / prof.symbol_period) << ",\n";
            poss << "        \"modulation\": \"" << (prof.num_tones == 8 ? "8-GFSK" : "4-GFSK") << "\",\n";
            poss << "        \"total_symbols\": " << prof.total_symbols << ",\n";
            poss << "        \"sync_symbols\": " << prof.num_sync_symbols << ",\n";
            poss << "        \"data_symbols\": " << prof.num_data_symbols << ",\n";
            poss << "        \"ramp_symbols\": " << prof.num_ramp_symbols << "\n";
            poss << "      },\n";

            // Costas Synchronization Blocks
            poss << "      \"costas_sync\": {\n";
            if (prof.num_tones == 8) {
                poss << "        \"pattern\": [2, 5, 6, 1, 3, 0, 4],\n";
                poss << "        \"sync_blocks\": [\n";
                poss << "          {\"block\": 1, \"start_symbol\": 0, \"end_symbol\": 6, \"symbols_count\": 7},\n";
                poss << "          {\"block\": 2, \"start_symbol\": 36, \"end_symbol\": 42, \"symbols_count\": 7},\n";
                poss << "          {\"block\": 3, \"start_symbol\": 72, \"end_symbol\": 78, \"symbols_count\": 7}\n";
                poss << "        ]\n";
            } else {
                poss << "        \"sync_blocks\": [\n";
                poss << "          {\"block\": 1, \"start_symbol\": 1, \"end_symbol\": 4, \"symbols_count\": 4, \"pattern\": [0, 2, 3, 1]},\n";
                poss << "          {\"block\": 2, \"start_symbol\": 34, \"end_symbol\": 37, \"symbols_count\": 4, \"pattern\": [1, 3, 2, 0]},\n";
                poss << "          {\"block\": 3, \"start_symbol\": 67, \"end_symbol\": 70, \"symbols_count\": 4, \"pattern\": [2, 0, 1, 3]},\n";
                poss << "          {\"block\": 4, \"start_symbol\": 100, \"end_symbol\": 103, \"symbols_count\": 4, \"pattern\": [3, 1, 0, 2]}\n";
                poss << "        ]\n";
            }
            poss << "      },\n";

            poss << "      \"tones_count\": " << seq.size() << ",\n";
            poss << "      \"tones\": [";
            for (size_t k = 0; k < seq.size(); ++k) {
                if (k > 0) poss << ", ";
                poss << static_cast<int>(seq[k]);
            }
            poss << "],\n";

            // Symbol Timeline
            poss << "      \"symbol_timeline\": [\n";
            for (size_t i = 0; i < seq.size(); ++i) {
                double t_start = static_cast<double>(i) * prof.symbol_period;
                double t_end = static_cast<double>(i + 1) * prof.symbol_period;
                double dur = prof.symbol_period;
                uint8_t tone = seq[i];
                double freq_offset = static_cast<double>(tone) * prof.tone_spacing;

                std::string sym_type;
                std::string annotation;

                if (prof.num_tones == 8) {
                    if (i <= 6) {
                        sym_type = "SYNC";
                        annotation = "Costas Sync 1 [" + std::to_string(i) + "]";
                    } else if (i <= 35) {
                        sym_type = "DATA";
                        annotation = "Data Block 1 [" + std::to_string(i - 7) + "]";
                    } else if (i <= 42) {
                        sym_type = "SYNC";
                        annotation = "Costas Sync 2 [" + std::to_string(i - 36) + "]";
                    } else if (i <= 71) {
                        sym_type = "DATA";
                        annotation = "Data Block 2 [" + std::to_string(i - 43) + "]";
                    } else {
                        sym_type = "SYNC";
                        annotation = "Costas Sync 3 [" + std::to_string(i - 72) + "]";
                    }
                } else {
                    if (i == 0) {
                        sym_type = "RAMP";
                        annotation = "Ramp Up";
                    } else if (i <= 4) {
                        sym_type = "SYNC";
                        annotation = "Costas Sync 1 [" + std::to_string(i - 1) + "]";
                    } else if (i <= 33) {
                        sym_type = "DATA";
                        annotation = "Data Block 1 [" + std::to_string(i - 5) + "]";
                    } else if (i <= 37) {
                        sym_type = "SYNC";
                        annotation = "Costas Sync 2 [" + std::to_string(i - 34) + "]";
                    } else if (i <= 66) {
                        sym_type = "DATA";
                        annotation = "Data Block 2 [" + std::to_string(i - 38) + "]";
                    } else if (i <= 70) {
                        sym_type = "SYNC";
                        annotation = "Costas Sync 3 [" + std::to_string(i - 67) + "]";
                    } else if (i <= 99) {
                        sym_type = "DATA";
                        annotation = "Data Block 3 [" + std::to_string(i - 71) + "]";
                    } else if (i <= 103) {
                        sym_type = "SYNC";
                        annotation = "Costas Sync 4 [" + std::to_string(i - 100) + "]";
                    } else {
                        sym_type = "RAMP";
                        annotation = "Ramp Down";
                    }
                }

                poss << "        {\n";
                poss << "          \"symbol_idx\": " << i << ",\n";
                poss << "          \"start_time_s\": " << std::fixed << std::setprecision(4) << t_start << ",\n";
                poss << "          \"end_time_s\": " << std::fixed << std::setprecision(4) << t_end << ",\n";
                poss << "          \"duration_s\": " << std::fixed << std::setprecision(4) << dur << ",\n";
                poss << "          \"tone\": " << static_cast<int>(tone) << ",\n";
                poss << "          \"freq_offset_hz\": " << std::fixed << std::setprecision(2) << freq_offset << ",\n";
                poss << "          \"type\": \"" << sym_type << "\",\n";
                poss << "          \"annotation\": \"" << escape_json(annotation) << "\"\n";
                poss << "        }" << (i + 1 < seq.size() ? "," : "") << "\n";
            }
            poss << "      ]\n";
            poss << "    }";

            if (!first_phy_json_entry) {
                phy_json_out << ",\n";
            }
            phy_json_out << poss.str();
            first_phy_json_entry = false;

            // -------------------------------------------------------------
            // 4. Unified TXT Summary
            // -------------------------------------------------------------
            txt_out << "---------------------------------------------------------------------------------------------------------\n";
            txt_out << "VECTOR ID:       " << vector_id << "\n";
            txt_out << "Description:     " << tdef.description << "\n";
            txt_out << "Mode:            " << mode_str << " (" << (prof.num_tones == 8 ? "8-GFSK" : "4-GFSK")
                    << ", " << prof.slot_duration << "s Slot, " << prof.tx_duration << "s TX, " << prof.occupied_bw << " Hz BW)\n";
            txt_out << "Message Type:    Type " << type_id << " (" << type_name << ")\n";
            txt_out << "Input Text:      \"" << tdef.text << "\"\n";
            txt_out << "Canonical Text:  \"" << canonical_text << "\"\n";
            txt_out << "Payload (Hex):   " << hex_str << "\n";
            txt_out << "Payload (Bin):   " << bin_str << "\n";
            txt_out << "CRC-14:          " << crc_hex_str << " (" << crc_bin_str << ")\n";
            txt_out << "Codeword (Hex):  " << codeword_hex << "\n";
            txt_out << "Tones (" << seq.size() << " syms): [";
            for (size_t k = 0; k < seq.size(); ++k) {
                if (k > 0) txt_out << ", ";
                txt_out << static_cast<int>(seq[k]);
            }
            txt_out << "]\n\n";

            // -------------------------------------------------------------
            // 5. Physical TXT Timeline Summary
            // -------------------------------------------------------------
            phy_txt_out << "---------------------------------------------------------------------------------------------------------\n";
            phy_txt_out << "VECTOR ID:       " << phy_vector_id << "\n";
            phy_txt_out << "Description:     " << tdef.description << " in " << mode_str << "\n";
            phy_txt_out << "Mode:            " << mode_str << " (" << (prof.num_tones == 8 ? "8-GFSK" : "4-GFSK")
                        << ", " << prof.slot_duration << "s Slot, " << prof.tx_duration << "s TX, "
                        << (prof.slot_duration - prof.tx_duration) << "s Guard, "
                        << prof.occupied_bw << " Hz BW, Rs=" << (1.0f / prof.symbol_period) << " Bd, Ts="
                        << (prof.symbol_period * 1000.0f) << " ms)\n";
            phy_txt_out << "Message:         \"" << tdef.text << "\"\n";
            phy_txt_out << "Canonical Text:  \"" << canonical_text << "\"\n";
            phy_txt_out << "Tones (" << seq.size() << " syms): [";
            for (size_t k = 0; k < seq.size(); ++k) {
                if (k > 0) phy_txt_out << ", ";
                phy_txt_out << static_cast<int>(seq[k]);
            }
            phy_txt_out << "]\n";
            phy_txt_out << "Timeline:\n";
            phy_txt_out << "  Sym | Start (s) |  End (s)  | Tone | Freq (Hz) | Type | Annotation\n";
            phy_txt_out << "------+-----------+-----------+------+-----------+------+-----------------------------------\n";
            for (size_t i = 0; i < seq.size(); ++i) {
                double t_start = static_cast<double>(i) * prof.symbol_period;
                double t_end = static_cast<double>(i + 1) * prof.symbol_period;
                uint8_t tone = seq[i];
                double freq_offset = static_cast<double>(tone) * prof.tone_spacing;

                std::string sym_type;
                std::string annotation;

                if (prof.num_tones == 8) {
                    if (i <= 6) { sym_type = "SYNC"; annotation = "Costas Sync 1 [" + std::to_string(i) + "]"; }
                    else if (i <= 35) { sym_type = "DATA"; annotation = "Data Block 1 [" + std::to_string(i - 7) + "]"; }
                    else if (i <= 42) { sym_type = "SYNC"; annotation = "Costas Sync 2 [" + std::to_string(i - 36) + "]"; }
                    else if (i <= 71) { sym_type = "DATA"; annotation = "Data Block 2 [" + std::to_string(i - 43) + "]"; }
                    else { sym_type = "SYNC"; annotation = "Costas Sync 3 [" + std::to_string(i - 72) + "]"; }
                } else {
                    if (i == 0) { sym_type = "RAMP"; annotation = "Ramp Up"; }
                    else if (i <= 4) { sym_type = "SYNC"; annotation = "Costas Sync 1 [" + std::to_string(i - 1) + "]"; }
                    else if (i <= 33) { sym_type = "DATA"; annotation = "Data Block 1 [" + std::to_string(i - 5) + "]"; }
                    else if (i <= 37) { sym_type = "SYNC"; annotation = "Costas Sync 2 [" + std::to_string(i - 34) + "]"; }
                    else if (i <= 66) { sym_type = "DATA"; annotation = "Data Block 2 [" + std::to_string(i - 38) + "]"; }
                    else if (i <= 70) { sym_type = "SYNC"; annotation = "Costas Sync 3 [" + std::to_string(i - 67) + "]"; }
                    else if (i <= 99) { sym_type = "DATA"; annotation = "Data Block 3 [" + std::to_string(i - 71) + "]"; }
                    else if (i <= 103) { sym_type = "SYNC"; annotation = "Costas Sync 4 [" + std::to_string(i - 100) + "]"; }
                    else { sym_type = "RAMP"; annotation = "Ramp Down"; }
                }

                phy_txt_out << "  " << std::setw(3) << std::setfill(' ') << i << " | "
                            << std::fixed << std::setprecision(4) << std::setw(9) << t_start << " | "
                            << std::fixed << std::setprecision(4) << std::setw(9) << t_end << " | "
                            << std::setw(4) << static_cast<int>(tone) << " | "
                            << std::fixed << std::setprecision(2) << std::setw(9) << freq_offset << " | "
                            << std::setw(4) << sym_type << " | "
                            << annotation << "\n";
            }
            phy_txt_out << "\n";
        }
    }

    json_out << "\n  ]\n}\n";
    paper_json_out << "\n  ]\n}\n";
    enc_json_out << "\n  ]\n}\n";
    phy_json_out << "\n  ]\n}\n";

    json_out.close();
    paper_json_out.close();
    txt_out.close();
    enc_json_out.close();
    phy_json_out.close();
    phy_txt_out.close();

    std::cout << "✓ Successfully generated " << total_vectors << " golden test vectors ("
              << total_paper_vectors << " paper examples) across all suites:\n";
    std::cout << "   - Unified:         " << json_path << "\n";
    std::cout << "   - Paper Examples:  " << paper_json_path << "\n";
    std::cout << "   - Text Summary:    " << txt_path << "\n";
    std::cout << "   - Encoding Suite:  " << encoding_json_path << "\n";
    std::cout << "   - Physical Suite:  " << physical_json_path << "\n";
    std::cout << "   - Physical Table:  " << physical_txt_path << "\n";

    return 0;
}
