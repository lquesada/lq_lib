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
#include <chrono>
#include <cmath>
#include <cstring>
#include "lq/lq.h"
#include "lq/c_api.h"
#include "lq/easy.h"
#include "lq/constants.h"

namespace {

const std::string_view KNOWN_CALLS[] = {
    "HB9IPH", "YO1YO", "TU2TU", "W1AW", "DP0GVN", "3DA0RU", "3DA0RU/P", "EA6/HB9IP/P",
    "HB9IPH/P", "YO1YO/P", "EA6/TU2TU", "3B9/HB9IPH/P", "3B9/HB9IP/P"
};
constexpr size_t NUM_KNOWN_CALLS = sizeof(KNOWN_CALLS) / sizeof(KNOWN_CALLS[0]);

const char* const KNOWN_C_CALLS[] = {
    "HB9IPH", "YO1YO", "TU2TU", "W1AW", "DP0GVN", "3DA0RU", "3DA0RU/P", "EA6/HB9IP/P",
    "HB9IPH/P", "YO1YO/P", "EA6/TU2TU", "3B9/HB9IPH/P", "3B9/HB9IP/P"
};
constexpr size_t NUM_KNOWN_C_CALLS = sizeof(KNOWN_C_CALLS) / sizeof(KNOWN_C_CALLS[0]);

inline bool expects_callsign_resolution(const std::string& canonical) {
    for (size_t i = 0; i < NUM_KNOWN_CALLS; ++i) {
        std::string tag = "<";
        tag += KNOWN_CALLS[i];
        tag += ">";
        if (canonical.find(tag) != std::string::npos) {
            return true;
        }
    }
    return false;
}

lq::Protocol parse_protocol_name(std::string_view name) {
    if (name == "LQ16") return lq::Protocol::LQ16;
    if (name == "LQ4") return lq::Protocol::LQ4;
    if (name == "LQ2") return lq::Protocol::LQ2;
    return lq::Protocol::LQ8;
}

struct GoldenVector {
    std::string id;
    std::string description;
    std::string mode_str;
    std::string message_type;
    int type_id = 0;
    std::string plaintext;
    std::string canonical_text;
    bool is_paper_example = false;
    std::string payload_77bit_bin;
    std::string payload_10byte_hex;
    uint16_t crc14_val = 0;
    std::string crc14_hex;
    std::string ldpc_input_91bit_bin;
    std::string codeword_174bit_bin;
    std::string codeword_22byte_hex;
    size_t tones_count = 0;
    std::vector<uint8_t> tones;
};

struct PhysicalSymbol {
    int symbol_idx = 0;
    double start_time_s = 0.0;
    double end_time_s = 0.0;
    double duration_s = 0.0;
    uint8_t tone = 0;
    double freq_offset_hz = 0.0;
    std::string type;
    std::string annotation;
};

struct PhysicalGoldenVector {
    std::string id;
    std::string description;
    std::string mode_str;
    std::string message_type;
    int type_id = 0;
    std::string plaintext;
    std::string canonical_text;
    bool is_paper_example = false;
    double slot_duration_s = 0.0;
    double tx_duration_s = 0.0;
    double guard_duration_s = 0.0;
    double symbol_period_s = 0.0;
    double symbol_period_ms = 0.0;
    double tone_spacing_hz = 0.0;
    double bandwidth_hz = 0.0;
    double baud_rate = 0.0;
    std::string modulation;
    int total_symbols = 0;
    int sync_symbols = 0;
    int data_symbols = 0;
    int ramp_symbols = 0;
    std::vector<uint8_t> tones;
    std::vector<PhysicalSymbol> timeline;
};

std::string extract_json_string(const std::string& obj, const std::string& key) {
    std::string search = "\"" + key + "\": \"";
    size_t pos = obj.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    size_t end = obj.find("\"", pos);
    if (end == std::string::npos) return "";
    std::string raw = obj.substr(pos, end - pos);
    std::string out;
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            if (raw[i+1] == '"') { out += '"'; ++i; }
            else if (raw[i+1] == '\\') { out += '\\'; ++i; }
            else if (raw[i+1] == 'n') { out += '\n'; ++i; }
            else if (raw[i+1] == 't') { out += '\t'; ++i; }
            else { out += raw[i]; }
        } else {
            out += raw[i];
        }
    }
    return out;
}

int extract_json_int(const std::string& obj, const std::string& key) {
    std::string search = "\"" + key + "\": ";
    size_t pos = obj.find(search);
    if (pos == std::string::npos) return 0;
    pos += search.size();
    size_t end = obj.find_first_of(",}\n\r", pos);
    if (end == std::string::npos) return 0;
    try {
        return std::stoi(obj.substr(pos, end - pos));
    } catch (...) {
        return 0;
    }
}

double extract_json_double(const std::string& obj, const std::string& key) {
    std::string search = "\"" + key + "\": ";
    size_t pos = obj.find(search);
    if (pos == std::string::npos) return 0.0;
    pos += search.size();
    size_t end = obj.find_first_of(",}\n\r", pos);
    if (end == std::string::npos) return 0.0;
    try {
        return std::stod(obj.substr(pos, end - pos));
    } catch (...) {
        return 0.0;
    }
}

bool extract_json_bool(const std::string& obj, const std::string& key) {
    std::string search = "\"" + key + "\": ";
    size_t pos = obj.find(search);
    if (pos == std::string::npos) return false;
    pos += search.size();
    return (obj.compare(pos, 4, "true") == 0);
}

std::vector<uint8_t> extract_json_tones(const std::string& obj) {
    std::vector<uint8_t> tones;
    std::string search = "\"tones\": [";
    size_t pos = obj.find(search);
    if (pos == std::string::npos) return tones;
    pos += search.size();
    size_t end = obj.find("]", pos);
    if (end == std::string::npos) return tones;
    std::string arr_str = obj.substr(pos, end - pos);
    std::stringstream ss(arr_str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        size_t first = item.find_first_not_of(" \t\n\r");
        size_t last = item.find_last_not_of(" \t\n\r");
        if (first != std::string::npos && last != std::string::npos) {
            tones.push_back(static_cast<uint8_t>(std::stoi(item.substr(first, last - first + 1))));
        }
    }
    return tones;
}

std::vector<GoldenVector> load_golden_vectors(const std::string& path) {
    std::vector<GoldenVector> vectors;
    std::ifstream f(path);
    if (!f.is_open()) return vectors;

    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string json = buffer.str();

    size_t pos = json.find("\"test_vectors\": [");
    if (pos == std::string::npos) return vectors;
    pos += 17;

    while (pos < json.size()) {
        size_t start_obj = json.find("{", pos);
        if (start_obj == std::string::npos) break;
        int depth = 0;
        size_t end_obj = start_obj;
        for (; end_obj < json.size(); ++end_obj) {
            if (json[end_obj] == '{') ++depth;
            else if (json[end_obj] == '}') {
                --depth;
                if (depth == 0) break;
            }
        }
        if (depth != 0 || end_obj >= json.size()) break;

        std::string obj = json.substr(start_obj, end_obj - start_obj + 1);
        GoldenVector gv;
        gv.id = extract_json_string(obj, "id");
        gv.description = extract_json_string(obj, "description");
        gv.mode_str = extract_json_string(obj, "mode");
        gv.message_type = extract_json_string(obj, "message_type");
        gv.type_id = extract_json_int(obj, "type_id");
        gv.plaintext = extract_json_string(obj, "plaintext");
        gv.canonical_text = extract_json_string(obj, "canonical_text");
        gv.is_paper_example = extract_json_bool(obj, "is_paper_example");
        gv.payload_77bit_bin = extract_json_string(obj, "payload_77bit_bin");
        gv.payload_10byte_hex = extract_json_string(obj, "payload_10byte_hex");
        gv.crc14_val = static_cast<uint16_t>(extract_json_int(obj, "crc14_val"));
        gv.crc14_hex = extract_json_string(obj, "crc14_hex");
        gv.ldpc_input_91bit_bin = extract_json_string(obj, "ldpc_input_91bit_bin");
        gv.codeword_174bit_bin = extract_json_string(obj, "codeword_174bit_bin");
        gv.codeword_22byte_hex = extract_json_string(obj, "codeword_22byte_hex");
        gv.tones_count = static_cast<size_t>(extract_json_int(obj, "tones_count"));
        gv.tones = extract_json_tones(obj);

        if (!gv.id.empty()) {
            vectors.push_back(gv);
        }
        pos = end_obj + 1;
    }
    return vectors;
}

size_t find_matching_bracket(const std::string& str, size_t open_pos) {
    int depth = 0;
    bool in_quote = false;
    for (size_t i = open_pos; i < str.size(); ++i) {
        if (str[i] == '"' && (i == 0 || str[i - 1] != '\\')) {
            in_quote = !in_quote;
        } else if (!in_quote) {
            if (str[i] == '[') ++depth;
            else if (str[i] == ']') {
                --depth;
                if (depth == 0) return i;
            }
        }
    }
    return std::string::npos;
}

std::vector<PhysicalGoldenVector> load_physical_golden_vectors(const std::string& path) {
    std::vector<PhysicalGoldenVector> vectors;
    std::ifstream f(path);
    if (!f.is_open()) return vectors;

    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string json = buffer.str();

    size_t pos = json.find("\"test_vectors\": [");
    if (pos == std::string::npos) return vectors;
    pos += 17;

    while (pos < json.size()) {
        size_t start_obj = json.find("{", pos);
        if (start_obj == std::string::npos) break;
        int depth = 0;
        size_t end_obj = start_obj;
        for (; end_obj < json.size(); ++end_obj) {
            if (json[end_obj] == '{') ++depth;
            else if (json[end_obj] == '}') {
                --depth;
                if (depth == 0) break;
            }
        }
        if (depth != 0 || end_obj >= json.size()) break;

        std::string obj = json.substr(start_obj, end_obj - start_obj + 1);
        PhysicalGoldenVector gvp;
        gvp.id = extract_json_string(obj, "id");
        gvp.description = extract_json_string(obj, "description");
        gvp.mode_str = extract_json_string(obj, "mode");
        gvp.message_type = extract_json_string(obj, "message_type");
        gvp.type_id = extract_json_int(obj, "type_id");
        gvp.plaintext = extract_json_string(obj, "plaintext");
        gvp.canonical_text = extract_json_string(obj, "canonical_text");
        gvp.is_paper_example = extract_json_bool(obj, "is_paper_example");

        gvp.slot_duration_s = extract_json_double(obj, "slot_duration_s");
        gvp.tx_duration_s = extract_json_double(obj, "tx_duration_s");
        gvp.guard_duration_s = extract_json_double(obj, "guard_duration_s");
        gvp.symbol_period_s = extract_json_double(obj, "symbol_period_s");
        gvp.symbol_period_ms = extract_json_double(obj, "symbol_period_ms");
        gvp.tone_spacing_hz = extract_json_double(obj, "tone_spacing_hz");
        gvp.bandwidth_hz = extract_json_double(obj, "bandwidth_hz");
        gvp.baud_rate = extract_json_double(obj, "baud_rate");
        gvp.modulation = extract_json_string(obj, "modulation");
        gvp.total_symbols = extract_json_int(obj, "total_symbols");
        gvp.sync_symbols = extract_json_int(obj, "sync_symbols");
        gvp.data_symbols = extract_json_int(obj, "data_symbols");
        gvp.ramp_symbols = extract_json_int(obj, "ramp_symbols");

        gvp.tones = extract_json_tones(obj);

        // Parse symbol timeline with accurate bracket tracking
        size_t tl_key = obj.find("\"symbol_timeline\":");
        if (tl_key != std::string::npos) {
            size_t open_bracket = obj.find("[", tl_key);
            if (open_bracket != std::string::npos) {
                size_t close_bracket = find_matching_bracket(obj, open_bracket);
                if (close_bracket != std::string::npos) {
                    size_t pos_scan = open_bracket + 1;
                    while (pos_scan < close_bracket) {
                        size_t s_start = obj.find("{", pos_scan);
                        if (s_start == std::string::npos || s_start >= close_bracket) break;
                        size_t s_end = obj.find("}", s_start);
                        if (s_end == std::string::npos || s_end > close_bracket) break;
                        std::string s_obj = obj.substr(s_start, s_end - s_start + 1);

                        PhysicalSymbol sym;
                        sym.symbol_idx = extract_json_int(s_obj, "symbol_idx");
                        sym.start_time_s = extract_json_double(s_obj, "start_time_s");
                        sym.end_time_s = extract_json_double(s_obj, "end_time_s");
                        sym.duration_s = extract_json_double(s_obj, "duration_s");
                        sym.tone = static_cast<uint8_t>(extract_json_int(s_obj, "tone"));
                        sym.freq_offset_hz = extract_json_double(s_obj, "freq_offset_hz");
                        sym.type = extract_json_string(s_obj, "type");
                        sym.annotation = extract_json_string(s_obj, "annotation");

                        gvp.timeline.push_back(sym);
                        pos_scan = s_end + 1;
                    }
                }
            }
        }

        if (!gvp.id.empty()) {
            vectors.push_back(gvp);
        }
        pos = end_obj + 1;
    }
    return vectors;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    std::string base_dir = "golden_vectors";
    if (argc > 1) {
        base_dir = argv[1];
    } else {
        std::ifstream test_f(base_dir + "/golden_vectors.json");
        if (!test_f.is_open()) {
            base_dir = "../golden_vectors";
        }
    }

    std::string path_unified = base_dir + "/golden_vectors.json";
    std::string path_physical = base_dir + "/golden_vectors_physical.json";
    std::string path_encoding = base_dir + "/golden_vectors_encoding.json";

    std::cout << "=================================================================================\n";
    std::cout << "          THE LQ DIGITAL MODE FAMILY — GOLDEN VECTOR VERIFICATION SUITE          \n";
    std::cout << "=================================================================================\n";
    std::cout << "Directory: " << base_dir << "\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    // -------------------------------------------------------------------------
    // Phase 1: Verify Unified / Encoding Golden Vectors
    // -------------------------------------------------------------------------
    std::cout << "\n>>> Phase 1: Verifying Message Encoding & Decoding Suite (" << path_unified << ")...\n";
    std::vector<GoldenVector> vectors = load_golden_vectors(path_unified);
    if (vectors.empty()) {
        std::cerr << "\033[41;1;37m FATAL ERROR \033[0m Could not load golden vectors from " << path_unified << "\n";
        return 1;
    }

    size_t tests_passed = 0;
    size_t total_checks = 0;
    size_t paper_examples = 0;

    for (const auto& gv : vectors) {
        if (gv.is_paper_example) {
            ++paper_examples;
        }

        // 1. Text Parsing Check
        lq::Message msg;
        if (!lq::parse_message(gv.plaintext, msg)) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> parse_message failed for: " << gv.plaintext << "\n";
            continue;
        }
        ++total_checks;

        // 2. 77-Bit Payload Serialization Check
        uint8_t payload[lq::PAYLOAD_BYTES] = {0};
        if (!lq::encode_message(msg, payload)) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> encode_message failed for: " << gv.plaintext << "\n";
            continue;
        }
        ++total_checks;

        std::string hex_str = lq::payload_to_hex(payload);
        std::string bin_str = lq::payload_to_binary(payload);
        if (hex_str != gv.payload_10byte_hex || bin_str != gv.payload_77bit_bin) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> Hex payload mismatch!\n";
            continue;
        }
        total_checks += 2;

        // 3. Canonical Text Round-Trip Check
        std::string canonical = lq::format_message(msg);
        if (canonical != gv.canonical_text) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> Canonical text mismatch!\n";
            continue;
        }
        ++total_checks;

        // 4. CRC-14 Checksum Check
        uint16_t crc14 = lq::compute_payload_crc14(payload);
        if (crc14 != gv.crc14_val) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> CRC-14 mismatch!\n";
            continue;
        }
        ++total_checks;

        // 5. LDPC(174,91) Codeword Check
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
        if (codeword_bin != gv.codeword_174bit_bin) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> LDPC codeword mismatch!\n";
            continue;
        }
        ++total_checks;

        // 6. Physical Channel Tones Synthesis Check
        lq::Protocol proto = parse_protocol_name(gv.mode_str);
        lq_mode_t c_mode = static_cast<lq_mode_t>(proto);
        lq::ToneSequence seq;
        if (!lq::encode_payload(payload, proto, seq)) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> encode_payload failed!\n";
            continue;
        }
        if (seq.tones != gv.tones) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> Tone sequence mismatch!\n";
            continue;
        }
        ++total_checks;

        // 7. Exhaustive Binary to Text & Hex to Text Representation Checks
        bool resolve = expects_callsign_resolution(gv.canonical_text);
        const std::string_view* calls = resolve ? KNOWN_CALLS : nullptr;
        size_t num_calls = resolve ? NUM_KNOWN_CALLS : 0;
        const char* const* c_calls = resolve ? KNOWN_C_CALLS : nullptr;
        size_t num_c_calls = resolve ? NUM_KNOWN_C_CALLS : 0;

        std::string text_from_bin = lq::binary_to_text(gv.payload_77bit_bin, calls, num_calls);
        if (text_from_bin != gv.canonical_text) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> binary_to_text mismatch!\n";
            continue;
        }
        ++total_checks;

        std::string text_from_hex = lq::hex_to_text(gv.payload_10byte_hex, calls, num_calls);
        if (text_from_hex != gv.canonical_text) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> hex_to_text mismatch!\n";
            continue;
        }
        ++total_checks;

        // 8. Reverse Channel Demodulation & Syndrome Checks
        uint8_t payload_from_tones[lq::PAYLOAD_BYTES] = {0};
        if (!lq::decode_payload(gv.tones.data(), payload_from_tones, proto) ||
            lq::payload_to_hex(payload_from_tones) != gv.payload_10byte_hex) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> decode_payload mismatch!\n";
            continue;
        }
        ++total_checks;

        if (!lq::ldpc_check_syndrome(codeword_bytes)) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> ldpc_check_syndrome failed!\n";
            continue;
        }
        ++total_checks;

        // 9. Pure C API Compliance Checks
        char c_text_bin[128] = {0};
        if (!lq_c_binary_to_text(gv.payload_77bit_bin.c_str(), c_text_bin, sizeof(c_text_bin), c_calls, num_c_calls) ||
            std::string(c_text_bin) != gv.canonical_text) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> lq_c_binary_to_text mismatch!\n";
            continue;
        }
        char c_text_hex[128] = {0};
        if (!lq_c_hex_to_text(gv.payload_10byte_hex.c_str(), c_text_hex, sizeof(c_text_hex), c_calls, num_c_calls) ||
            std::string(c_text_hex) != gv.canonical_text) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> lq_c_hex_to_text mismatch!\n";
            continue;
        }
        uint8_t c_payload_tones[10] = {0};
        if (!lq_c_decode_payload(gv.tones.data(), c_payload_tones, c_mode) ||
            std::memcmp(payload, c_payload_tones, 10) != 0) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> lq_c_decode_payload mismatch!\n";
            continue;
        }
        total_checks += 3;

        // 10. Easy API Compliance Checks
        auto opt_tones = lq::text_to_tones(gv.plaintext, proto);
        if (!opt_tones || opt_tones->tones != gv.tones) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> text_to_tones mismatch!\n";
            continue;
        }
        auto opt_packed = lq::pack(msg);
        if (!opt_packed || lq::payload_to_hex(opt_packed->data()) != gv.payload_10byte_hex) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> lq::pack mismatch!\n";
            continue;
        }
        auto opt_unpacked = lq::unpack(*opt_packed);
        if (!opt_unpacked) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> lq::unpack failed!\n";
            continue;
        }
        if (resolve) {
            lq::resolve_callsigns(*opt_unpacked, KNOWN_CALLS, NUM_KNOWN_CALLS);
        }
        if (lq::format_message(*opt_unpacked) != gv.canonical_text) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gv.id << " -> lq::unpack formatted mismatch!\n";
            continue;
        }
        total_checks += 3;

        ++tests_passed;
    }

    // -------------------------------------------------------------------------
    // Phase 2: Verify Physical Layer Golden Vectors
    // -------------------------------------------------------------------------
    std::cout << "\n>>> Phase 2: Verifying Physical Layer & Timeline Suite (" << path_physical << ")...\n";
    std::vector<PhysicalGoldenVector> phy_vectors = load_physical_golden_vectors(path_physical);
    if (phy_vectors.empty()) {
        std::cerr << "\033[41;1;37m FATAL ERROR \033[0m Could not load physical vectors from " << path_physical << "\n";
        return 1;
    }

    size_t phy_passed = 0;
    size_t phy_checks = 0;

    for (const auto& gvp : phy_vectors) {
        lq::Protocol proto = parse_protocol_name(gvp.mode_str);
        auto prof = lq::get_protocol_params(proto);

        // 1. Parameter checks
        if (gvp.total_symbols != prof.total_symbols ||
            gvp.sync_symbols != prof.num_sync_symbols ||
            gvp.data_symbols != prof.num_data_symbols ||
            gvp.ramp_symbols != prof.num_ramp_symbols) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> Symbol count mismatch!\n";
            continue;
        }
        phy_checks += 4;

        if (std::abs(gvp.slot_duration_s - prof.slot_duration) > 0.01 ||
            std::abs(gvp.tx_duration_s - prof.tx_duration) > 0.01 ||
            std::abs(gvp.symbol_period_s - prof.symbol_period) > 0.001 ||
            std::abs(gvp.tone_spacing_hz - prof.tone_spacing) > 0.001) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> Physical parameter mismatch!\n";
            continue;
        }
        phy_checks += 4;

        // 2. Costas Sync Array Integrity Checks
        if (prof.num_tones == 8) {
            // LQ8 / LQ16: Costas array of 7 symbols at 0, 36, 72
            for (size_t k = 0; k < 7; ++k) {
                if (gvp.tones[k] != lq::COSTAS_ARRAY_8[k] ||
                    gvp.tones[36 + k] != lq::COSTAS_ARRAY_8[k] ||
                    gvp.tones[72 + k] != lq::COSTAS_ARRAY_8[k]) {
                    std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> Costas array 8-GFSK mismatch!\n";
                    break;
                }
            }
            phy_checks += 21;
        } else {
            // LQ4 / LQ2: 4 Costas blocks of 4 symbols at 1, 34, 67, 100
            for (size_t k = 0; k < 4; ++k) {
                if (gvp.tones[1 + k] != lq::COSTAS_SYNC1_4[k] ||
                    gvp.tones[34 + k] != lq::COSTAS_SYNC2_4[k] ||
                    gvp.tones[67 + k] != lq::COSTAS_SYNC3_4[k] ||
                    gvp.tones[100 + k] != lq::COSTAS_SYNC4_4[k]) {
                    std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> Costas array 4-GFSK mismatch!\n";
                    break;
                }
            }
            phy_checks += 16;

            // Ramp symbols
            if (gvp.tones[0] != 0 || gvp.tones[104] != 0) {
                std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> Ramp symbol tone mismatch!\n";
                continue;
            }
            phy_checks += 2;
        }

        // 3. Symbol Timeline Contiguity & Frequency Offset Checks
        if (gvp.timeline.size() != static_cast<size_t>(prof.total_symbols)) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> Timeline size mismatch!\n";
            continue;
        }
        phy_checks++;

        bool timeline_ok = true;
        for (size_t i = 0; i < gvp.timeline.size(); ++i) {
            const auto& sym = gvp.timeline[i];
            double expected_start = static_cast<double>(i) * prof.symbol_period;
            double expected_end = static_cast<double>(i + 1) * prof.symbol_period;
            double expected_freq = static_cast<double>(sym.tone) * prof.tone_spacing;

            if (sym.symbol_idx != static_cast<int>(i) ||
                std::abs(sym.start_time_s - expected_start) > 0.001 ||
                std::abs(sym.end_time_s - expected_end) > 0.001 ||
                std::abs(sym.duration_s - prof.symbol_period) > 0.001 ||
                sym.tone != gvp.tones[i] ||
                std::abs(sym.freq_offset_hz - expected_freq) > 0.05) {
                timeline_ok = false;
                break;
            }

            if (i > 0) {
                if (std::abs(sym.start_time_s - gvp.timeline[i - 1].end_time_s) > 0.0001) {
                    timeline_ok = false;
                    break;
                }
            }
        }
        if (!timeline_ok) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> Timeline validation failed!\n";
            continue;
        }
        phy_checks += (gvp.timeline.size() * 6);

        // 4. Reverse Tone Demodulation to Canonical Text
        lq::ToneSequence seq;
        seq.protocol = proto;
        seq.tones = gvp.tones;
        seq.symbol_period = prof.symbol_period;
        seq.tone_spacing = prof.tone_spacing;
        seq.tx_duration = prof.tx_duration;

        uint8_t rx_payload[lq::PAYLOAD_BYTES] = {0};
        if (!lq::decode_tones(seq, rx_payload)) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> decode_tones failed!\n";
            continue;
        }
        phy_checks++;

        lq::Message rx_msg;
        if (!lq::decode_message(rx_payload, rx_msg)) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> decode_message failed!\n";
            continue;
        }
        phy_checks++;

        if (expects_callsign_resolution(gvp.canonical_text)) {
            lq::resolve_callsigns(rx_msg, KNOWN_CALLS, NUM_KNOWN_CALLS);
        }

        std::string rx_canonical = lq::format_message(rx_msg);
        if (rx_canonical != gvp.canonical_text) {
            std::cerr << "\033[31m[FAIL]\033[0m " << gvp.id << " -> Decoded canonical text mismatch!\n";
            std::cerr << "  Expected: " << gvp.canonical_text << "\n";
            std::cerr << "  Actual:   " << rx_canonical << "\n";
            continue;
        }
        phy_checks++;

        ++phy_passed;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::cout << "\n---------------------------------------------------------------------------------\n";
    std::cout << "  COMPREHENSIVE GOLDEN VECTOR VERIFICATION REPORT:\n";
    std::cout << "   - Unified / Encoding Vectors:  " << tests_passed << " / " << vectors.size() << " passed (" << total_checks << " layer assertions)\n";
    std::cout << "   - Physical Layer Vectors:      " << phy_passed << " / " << phy_vectors.size() << " passed (" << phy_checks << " RF timeline assertions)\n";
    std::cout << "   - Published Paper Examples:    " << paper_examples << " vectors verified\n";
    std::cout << "   - Total Verification Checks:   " << (total_checks + phy_checks) << " assertions\n";
    std::cout << "   - Total Execution Time:        " << std::fixed << std::setprecision(2) << elapsed_ms << " ms\n";
    std::cout << "---------------------------------------------------------------------------------\n";

    if (tests_passed == vectors.size() && phy_passed == phy_vectors.size()) {
        std::cout << "\033[32;1m✓ ALL " << (vectors.size() + phy_vectors.size()) << " TEST VECTORS PASSED VALIDATION (100% COMPLIANT)\033[0m\n";
        std::cout << "=================================================================================\n";
        return 0;
    } else {
        std::cout << "\033[31;1m✗ FAILED: Vectors failed validation!\033[0m\n";
        std::cout << "=================================================================================\n";
        return 1;
    }
}
