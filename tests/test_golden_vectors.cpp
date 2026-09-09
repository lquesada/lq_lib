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
#include <vector>
#include <string>
#include <cmath>
#include "lq/lq.h"
#include "lq/constants.h"

namespace {

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

std::string extract_json_string(const std::string& obj, const std::string& key) {
    std::string search = "\"" + key + "\": \"";
    size_t pos = obj.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    size_t end = obj.find("\"", pos);
    if (end == std::string::npos) return "";
    std::string raw = obj.substr(pos, end - pos);
    // Unescape
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
    return std::stoi(obj.substr(pos, end - pos));
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

std::vector<GoldenVector> load_golden_vectors() {
    std::vector<GoldenVector> vectors;
    std::vector<std::string> search_paths = {
        "golden_vectors/golden_vectors.json",
        "../golden_vectors/golden_vectors.json",
        "tests/data/golden_vectors.json",
        "../tests/data/golden_vectors.json"
    };

    std::string json;
    for (const auto& p : search_paths) {
        std::ifstream f(p);
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            json = buffer.str();
            break;
        }
    }

    if (json.empty()) return vectors;

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

bool parse_codeword_hex(std::string_view hex_str, uint8_t codeword[22]) {
    std::memset(codeword, 0, 22);
    std::string clean;
    for (char c : hex_str) {
        if (std::isxdigit(static_cast<unsigned char>(c))) clean.push_back(c);
    }
    if (clean.size() < 44) return false;
    for (size_t i = 0; i < 22; ++i) {
        auto hex_val = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
            return 0;
        };
        int high = hex_val(clean[i * 2]);
        int low = hex_val(clean[i * 2 + 1]);
        codeword[i] = static_cast<uint8_t>((high << 4) | low);
    }
    return true;
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

std::vector<PhysicalGoldenVector> load_physical_golden_vectors() {
    std::vector<PhysicalGoldenVector> vectors;
    std::vector<std::string> search_paths = {
        "golden_vectors/golden_vectors_physical.json",
        "../golden_vectors/golden_vectors_physical.json",
        "tests/data/golden_vectors_physical.json",
        "../tests/data/golden_vectors_physical.json"
    };

    std::string json;
    for (const auto& p : search_paths) {
        std::ifstream f(p);
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            json = buffer.str();
            break;
        }
    }
    if (json.empty()) return vectors;

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

std::vector<GoldenVector> load_encoding_golden_vectors() {
    std::vector<GoldenVector> vectors;
    std::vector<std::string> search_paths = {
        "golden_vectors/golden_vectors_encoding.json",
        "../golden_vectors/golden_vectors_encoding.json",
        "tests/data/golden_vectors_encoding.json",
        "../tests/data/golden_vectors_encoding.json"
    };

    std::string json;
    for (const auto& p : search_paths) {
        std::ifstream f(p);
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            json = buffer.str();
            break;
        }
    }
    if (json.empty()) return vectors;

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

        if (!gv.id.empty()) {
            vectors.push_back(gv);
        }
        pos = end_obj + 1;
    }
    return vectors;
}

} // anonymous namespace

class GoldenVectorsTest : public ::testing::Test {
protected:
    static std::vector<GoldenVector> s_vectors;
    static std::vector<PhysicalGoldenVector> s_physical_vectors;
    static std::vector<GoldenVector> s_encoding_vectors;

    static void SetUpTestSuite() {
        s_vectors = load_golden_vectors();
        s_physical_vectors = load_physical_golden_vectors();
        s_encoding_vectors = load_encoding_golden_vectors();
    }
};

std::vector<GoldenVector> GoldenVectorsTest::s_vectors;
std::vector<PhysicalGoldenVector> GoldenVectorsTest::s_physical_vectors;
std::vector<GoldenVector> GoldenVectorsTest::s_encoding_vectors;

TEST_F(GoldenVectorsTest, DatasetIsPopulated) {
    ASSERT_FALSE(s_vectors.empty()) << "Golden vectors dataset must not be empty.";
    EXPECT_GE(s_vectors.size(), 100u);
}

// ----------------------------------------------------------------------------
// 1. Exhaustive Forward Pipeline Encoding & Compliance
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, ExhaustiveForwardEncoding) {
    ASSERT_FALSE(s_vectors.empty());

    for (const auto& gv : s_vectors) {
        SCOPED_TRACE(gv.id);

        // 1. Text Parsing
        lq::Message msg;
        ASSERT_TRUE(lq::parse_message(gv.plaintext, msg)) << "Failed parse_message for: " << gv.plaintext;

        // 2. 77-Bit Payload Serialization
        uint8_t payload[lq::PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(lq::encode_message(msg, payload)) << "Failed encode_message for: " << gv.plaintext;

        std::string hex_str = lq::payload_to_hex(payload);
        std::string bin_str = lq::payload_to_binary(payload);
        EXPECT_EQ(hex_str, gv.payload_10byte_hex);
        EXPECT_EQ(bin_str, gv.payload_77bit_bin);

        // Direct Text-to-Hex and Text-to-Binary
        EXPECT_EQ(lq::text_to_hex(gv.plaintext), gv.payload_10byte_hex);
        EXPECT_EQ(lq::text_to_binary(gv.plaintext), gv.payload_77bit_bin);

        // 3. Canonical Text Formatting
        std::string canonical = lq::format_message(msg);
        EXPECT_EQ(canonical, gv.canonical_text);
        EXPECT_EQ(msg.to_string(), gv.canonical_text);

        // 4. CRC-14 Checksum
        uint16_t crc14 = lq::compute_payload_crc14(payload);
        EXPECT_EQ(crc14, gv.crc14_val);

        uint8_t in_91[lq::LDPC_INPUT_BYTES] = {0};
        lq::append_crc14(payload, in_91);
        EXPECT_TRUE(lq::verify_crc14(in_91));

        // 5. LDPC(174,91) Codeword
        uint8_t codeword_bytes[lq::LDPC_CODEWORD_BYTES] = {0};
        lq::ldpc_encode(in_91, codeword_bytes);

        std::string codeword_bin;
        codeword_bin.reserve(174);
        for (int i = 0; i < 174; ++i) {
            uint8_t bit = (codeword_bytes[i / 8] >> (7 - (i % 8))) & 1;
            codeword_bin.push_back(bit ? '1' : '0');
        }
        EXPECT_EQ(codeword_bin, gv.codeword_174bit_bin);

        // 6. Physical Modulation Tones Synthesis
        lq::Protocol proto = parse_protocol_name(gv.mode_str);
        lq::ToneSequence seq;
        ASSERT_TRUE(lq::encode_payload(payload, proto, seq));
        EXPECT_EQ(seq.size(), gv.tones_count);
        EXPECT_EQ(seq.tones, gv.tones);
    }
}

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

// ----------------------------------------------------------------------------
// 2. Exhaustive Binary to Text Representation & Inversion
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, ExhaustiveBinaryToTextRepresentation) {
    ASSERT_FALSE(s_vectors.empty());

    for (const auto& gv : s_vectors) {
        SCOPED_TRACE(gv.id);
        bool resolve = expects_callsign_resolution(gv.canonical_text);
        const std::string_view* calls = resolve ? KNOWN_CALLS : nullptr;
        size_t num_calls = resolve ? NUM_KNOWN_CALLS : 0;

        // A. Direct 77-bit Binary String to Human-Readable Text
        std::string text_from_bin = lq::binary_to_text(gv.payload_77bit_bin, calls, num_calls);
        EXPECT_EQ(text_from_bin, gv.canonical_text) << "binary_to_text failed for " << gv.id;

        // B. Direct 10-byte Hex String to Human-Readable Text
        std::string text_from_hex = lq::hex_to_text(gv.payload_10byte_hex, calls, num_calls);
        EXPECT_EQ(text_from_hex, gv.canonical_text) << "hex_to_text failed for " << gv.id;

        // C. Binary String to 10-byte Payload -> Structured Message -> Text
        uint8_t payload_from_bin[lq::PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(lq::binary_to_payload(gv.payload_77bit_bin, payload_from_bin));
        EXPECT_EQ(lq::payload_to_hex(payload_from_bin), gv.payload_10byte_hex);

        lq::Message msg_from_bin;
        ASSERT_TRUE(lq::decode_message(payload_from_bin, msg_from_bin, calls, num_calls));
        EXPECT_EQ(lq::format_message(msg_from_bin), gv.canonical_text);
        EXPECT_EQ(msg_from_bin.to_string(), gv.canonical_text);
        EXPECT_EQ(static_cast<int>(msg_from_bin.type), gv.type_id);

        // D. Hex String to 10-byte Payload -> Structured Message -> Text
        uint8_t payload_from_hex[lq::PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(lq::hex_to_payload(gv.payload_10byte_hex, payload_from_hex));
        EXPECT_EQ(lq::payload_to_binary(payload_from_hex), gv.payload_77bit_bin);

        lq::Message msg_from_hex;
        ASSERT_TRUE(lq::decode_message(payload_from_hex, msg_from_hex, calls, num_calls));
        EXPECT_EQ(lq::format_message(msg_from_hex), gv.canonical_text);
        EXPECT_EQ(static_cast<int>(msg_from_hex.type), gv.type_id);

        // E. Canonical Text Re-encoding to Binary & Hex
        EXPECT_EQ(lq::text_to_hex(gv.canonical_text), gv.payload_10byte_hex);
        EXPECT_EQ(lq::text_to_binary(gv.canonical_text), gv.payload_77bit_bin);
    }
}

// ----------------------------------------------------------------------------
// 3. Exhaustive Reverse Channel & Codeword Decoding
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, ExhaustiveReverseChannelDecoding) {
    ASSERT_FALSE(s_vectors.empty());

    for (const auto& gv : s_vectors) {
        SCOPED_TRACE(gv.id);
        lq::Protocol proto = parse_protocol_name(gv.mode_str);
        bool resolve = expects_callsign_resolution(gv.canonical_text);
        const std::string_view* calls = resolve ? KNOWN_CALLS : nullptr;
        size_t num_calls = resolve ? NUM_KNOWN_CALLS : 0;

        // A. Reverse Demodulation: Physical Tones to 77-bit Payload
        uint8_t payload_from_tones[lq::PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(lq::decode_payload(gv.tones.data(), payload_from_tones, proto))
            << "decode_payload failed for " << gv.id;
        EXPECT_EQ(lq::payload_to_hex(payload_from_tones), gv.payload_10byte_hex);
        EXPECT_EQ(lq::payload_to_binary(payload_from_tones), gv.payload_77bit_bin);

        // B. Reverse Demodulation: Physical ToneSequence to Structured Message -> Text
        lq::ToneSequence seq;
        seq.protocol = proto;
        seq.tones = gv.tones;

        lq::Message msg_from_tones;
        ASSERT_TRUE(lq::decode_tones(seq, msg_from_tones));
        if (resolve) {
            lq::resolve_callsigns(msg_from_tones, KNOWN_CALLS, NUM_KNOWN_CALLS);
        }
        EXPECT_EQ(lq::format_message(msg_from_tones), gv.canonical_text);

        // C. Easy API: ToneSequence directly to human-readable text
        auto opt_text = lq::tones_to_text(seq);
        ASSERT_TRUE(opt_text.has_value());
        if (gv.plaintext.find('<') != std::string::npos || gv.canonical_text.find('<') != std::string::npos) {
            EXPECT_NE(opt_text->find('<'), std::string::npos);
        } else {
            EXPECT_EQ(*opt_text, gv.canonical_text);
        }

        // D. Reverse LDPC Codeword Decoding
        uint8_t codeword_bytes[lq::LDPC_CODEWORD_BYTES] = {0};
        ASSERT_TRUE(parse_codeword_hex(gv.codeword_22byte_hex, codeword_bytes));

        // Syndrome check
        EXPECT_TRUE(lq::ldpc_check_syndrome(codeword_bytes));

        // Hard-decision belief propagation decode
        uint8_t decoded_91[lq::LDPC_INPUT_BYTES] = {0};
        int iters = lq::ldpc_decode_hard_bits(codeword_bytes, decoded_91, 25);
        EXPECT_GT(iters, 0) << "LDPC hard bit decode failed for " << gv.id;
        EXPECT_TRUE(lq::verify_crc14(decoded_91));

        uint8_t extracted_payload[lq::PAYLOAD_BYTES] = {0};
        lq::extract_payload(decoded_91, extracted_payload);
        EXPECT_EQ(lq::payload_to_hex(extracted_payload), gv.payload_10byte_hex);
        EXPECT_EQ(lq::hex_to_text(lq::payload_to_hex(extracted_payload), calls, num_calls), gv.canonical_text);
    }
}

// ----------------------------------------------------------------------------
// 4. Exhaustive Pure C API Binary and Text Compliance
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, ExhaustivePureCAPIBinaryAndTextCompliance) {
    ASSERT_FALSE(s_vectors.empty());

    for (const auto& gv : s_vectors) {
        SCOPED_TRACE(gv.id);
        lq::Protocol proto = parse_protocol_name(gv.mode_str);
        lq_mode_t c_mode = static_cast<lq_mode_t>(proto);
        bool resolve = expects_callsign_resolution(gv.canonical_text);
        const char* const* c_calls = resolve ? KNOWN_C_CALLS : nullptr;
        size_t num_c_calls = resolve ? NUM_KNOWN_C_CALLS : 0;

        // 1. Text Parsing
        lq_c_message_t c_msg;
        ASSERT_EQ(lq_c_parse_message(gv.plaintext.c_str(), &c_msg), 1);
        char fmt_buf[128] = {0};
        ASSERT_EQ(lq_c_format_message(&c_msg, fmt_buf, sizeof(fmt_buf)), 1);
        EXPECT_STREQ(fmt_buf, gv.canonical_text.c_str());

        // 2. Encoding to 10-byte binary payload
        uint8_t c_payload[10] = {0};
        ASSERT_EQ(lq_c_encode_message(&c_msg, c_payload), 1);

        // 3. Payload to Hex & Binary
        char hex_buf[64] = {0};
        ASSERT_EQ(lq_c_payload_to_hex(c_payload, hex_buf, sizeof(hex_buf), 1), 1);
        EXPECT_STREQ(hex_buf, gv.payload_10byte_hex.c_str());

        char bin_buf[128] = {0};
        ASSERT_EQ(lq_c_payload_to_binary(c_payload, bin_buf, sizeof(bin_buf)), 1);
        EXPECT_STREQ(bin_buf, gv.payload_77bit_bin.c_str());

        // 4. Binary String to Text representation
        char text_from_bin[128] = {0};
        ASSERT_EQ(lq_c_binary_to_text(gv.payload_77bit_bin.c_str(), text_from_bin, sizeof(text_from_bin), c_calls, num_c_calls), 1);
        EXPECT_STREQ(text_from_bin, gv.canonical_text.c_str());

        // 5. Hex String to Text representation
        char text_from_hex[128] = {0};
        ASSERT_EQ(lq_c_hex_to_text(gv.payload_10byte_hex.c_str(), text_from_hex, sizeof(text_from_hex), c_calls, num_c_calls), 1);
        EXPECT_STREQ(text_from_hex, gv.canonical_text.c_str());

        // 6. Direct Text to Hex & Binary
        char direct_hex[64] = {0};
        ASSERT_EQ(lq_c_text_to_hex(gv.plaintext.c_str(), direct_hex, sizeof(direct_hex), 1), 1);
        EXPECT_STREQ(direct_hex, gv.payload_10byte_hex.c_str());

        char direct_bin[128] = {0};
        ASSERT_EQ(lq_c_text_to_binary(gv.plaintext.c_str(), direct_bin, sizeof(direct_bin)), 1);
        EXPECT_STREQ(direct_bin, gv.payload_77bit_bin.c_str());

        // 7. Hex and Binary to Payload conversions
        uint8_t c_payload_from_hex[10] = {0};
        ASSERT_EQ(lq_c_hex_to_payload(gv.payload_10byte_hex.c_str(), c_payload_from_hex), 1);
        EXPECT_EQ(std::memcmp(c_payload, c_payload_from_hex, 10), 0);

        uint8_t c_payload_from_bin[10] = {0};
        ASSERT_EQ(lq_c_binary_to_payload(gv.payload_77bit_bin.c_str(), c_payload_from_bin), 1);
        EXPECT_EQ(std::memcmp(c_payload, c_payload_from_bin, 10), 0);

        // 8. Tones Demodulation via C API
        uint8_t c_payload_tones[10] = {0};
        ASSERT_EQ(lq_c_decode_payload(gv.tones.data(), c_payload_tones, c_mode), 1);
        EXPECT_EQ(std::memcmp(c_payload, c_payload_tones, 10), 0);
    }
}

// ----------------------------------------------------------------------------
// 5. Exhaustive Easy API Binary and Text Compliance
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, ExhaustiveEasyAPIBinaryAndTextCompliance) {
    ASSERT_FALSE(s_vectors.empty());

    for (const auto& gv : s_vectors) {
        SCOPED_TRACE(gv.id);
        lq::Protocol proto = parse_protocol_name(gv.mode_str);
        bool resolve = expects_callsign_resolution(gv.canonical_text);

        // Text directly to tone sequence
        auto opt_tones = lq::text_to_tones(gv.plaintext, proto);
        ASSERT_TRUE(opt_tones.has_value());
        EXPECT_EQ(opt_tones->tones, gv.tones);

        // Tone sequence directly back to canonical text
        auto opt_text = lq::tones_to_text(*opt_tones);
        ASSERT_TRUE(opt_text.has_value());
        if (gv.plaintext.find('<') != std::string::npos || gv.canonical_text.find('<') != std::string::npos) {
            EXPECT_NE(opt_text->find('<'), std::string::npos);
        } else {
            EXPECT_EQ(*opt_text, gv.canonical_text);
        }

        // Fluent pack and unpack
        lq::Message msg;
        ASSERT_TRUE(lq::parse_message(gv.plaintext, msg));
        auto opt_packed = lq::pack(msg);
        ASSERT_TRUE(opt_packed.has_value());
        EXPECT_EQ(lq::payload_to_hex(opt_packed->data()), gv.payload_10byte_hex);

        auto opt_unpacked = lq::unpack(*opt_packed);
        ASSERT_TRUE(opt_unpacked.has_value());
        if (resolve) {
            lq::resolve_callsigns(*opt_unpacked, KNOWN_CALLS, NUM_KNOWN_CALLS);
        }
        EXPECT_EQ(lq::format_message(*opt_unpacked), gv.canonical_text);
    }
}

// ----------------------------------------------------------------------------
// 6. Paper Worked Examples Exact Match Verification
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, PaperWorkedExamplesAreExactMatch) {
    ASSERT_FALSE(s_vectors.empty());

    size_t paper_examples_count = 0;
    for (const auto& gv : s_vectors) {
        if (!gv.is_paper_example) continue;
        ++paper_examples_count;

        SCOPED_TRACE("Paper example: " + gv.id);
        bool resolve = expects_callsign_resolution(gv.canonical_text);
        const std::string_view* calls = resolve ? KNOWN_CALLS : nullptr;
        size_t num_calls = resolve ? NUM_KNOWN_CALLS : 0;

        lq::Message msg;
        ASSERT_TRUE(lq::parse_message(gv.plaintext, msg));
        uint8_t payload[lq::PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(lq::encode_message(msg, payload));

        EXPECT_EQ(lq::payload_to_hex(payload), gv.payload_10byte_hex);
        EXPECT_EQ(lq::compute_payload_crc14(payload), gv.crc14_val);
        EXPECT_EQ(lq::binary_to_text(gv.payload_77bit_bin, calls, num_calls), gv.canonical_text);
        EXPECT_EQ(lq::hex_to_text(gv.payload_10byte_hex, calls, num_calls), gv.canonical_text);
    }
    EXPECT_GE(paper_examples_count, 50u);
}

// ----------------------------------------------------------------------------
// 7. Physical Layer Golden Vectors Dataset Population
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, PhysicalLayerDatasetIsPopulated) {
    ASSERT_FALSE(s_physical_vectors.empty()) << "Physical layer golden vectors must not be empty.";
    EXPECT_EQ(s_physical_vectors.size(), 196u);
}

// ----------------------------------------------------------------------------
// 8. Physical Layer Parameters Integrity (All 4 Modes)
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, PhysicalLayerParametersIntegrity) {
    ASSERT_FALSE(s_physical_vectors.empty());

    for (const auto& gvp : s_physical_vectors) {
        SCOPED_TRACE(gvp.id);
        lq::Protocol proto = parse_protocol_name(gvp.mode_str);
        auto prof = lq::get_protocol_params(proto);

        EXPECT_EQ(gvp.total_symbols, prof.total_symbols);
        EXPECT_EQ(gvp.sync_symbols, prof.num_sync_symbols);
        EXPECT_EQ(gvp.data_symbols, prof.num_data_symbols);
        EXPECT_EQ(gvp.ramp_symbols, prof.num_ramp_symbols);

        EXPECT_NEAR(gvp.slot_duration_s, prof.slot_duration, 0.01);
        EXPECT_NEAR(gvp.tx_duration_s, prof.tx_duration, 0.01);
        EXPECT_NEAR(gvp.guard_duration_s, prof.slot_duration - prof.tx_duration, 0.01);
        EXPECT_NEAR(gvp.symbol_period_s, prof.symbol_period, 0.001);
        EXPECT_NEAR(gvp.symbol_period_ms, prof.symbol_period * 1000.0f, 0.1);
        EXPECT_NEAR(gvp.tone_spacing_hz, prof.tone_spacing, 0.001);
        EXPECT_NEAR(gvp.bandwidth_hz, prof.occupied_bw, 0.001);
        EXPECT_NEAR(gvp.baud_rate, 1.0f / prof.symbol_period, 0.001);

        if (prof.num_tones == 8) {
            EXPECT_EQ(gvp.modulation, "8-GFSK");
        } else {
            EXPECT_EQ(gvp.modulation, "4-GFSK");
        }
    }
}

// ----------------------------------------------------------------------------
// 9. Physical Layer Costas Sync Placement and Values
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, PhysicalLayerCostasSyncPlacementAndValues) {
    ASSERT_FALSE(s_physical_vectors.empty());

    for (const auto& gvp : s_physical_vectors) {
        SCOPED_TRACE(gvp.id);
        lq::Protocol proto = parse_protocol_name(gvp.mode_str);
        auto prof = lq::get_protocol_params(proto);

        if (prof.num_tones == 8) {
            // 8-GFSK (LQ8, LQ16): 3 Costas blocks of 7 symbols at 0, 36, 72
            for (size_t k = 0; k < 7; ++k) {
                EXPECT_EQ(gvp.tones[k], lq::COSTAS_ARRAY_8[k]);
                EXPECT_EQ(gvp.tones[36 + k], lq::COSTAS_ARRAY_8[k]);
                EXPECT_EQ(gvp.tones[72 + k], lq::COSTAS_ARRAY_8[k]);
            }
        } else {
            // 4-GFSK (LQ4, LQ2): 4 Costas blocks of 4 symbols at 1, 34, 67, 100
            for (size_t k = 0; k < 4; ++k) {
                EXPECT_EQ(gvp.tones[1 + k], lq::COSTAS_SYNC1_4[k]);
                EXPECT_EQ(gvp.tones[34 + k], lq::COSTAS_SYNC2_4[k]);
                EXPECT_EQ(gvp.tones[67 + k], lq::COSTAS_SYNC3_4[k]);
                EXPECT_EQ(gvp.tones[100 + k], lq::COSTAS_SYNC4_4[k]);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// 10. Physical Layer Ramp Symbols Integrity
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, PhysicalLayerRampSymbolsIntegrity) {
    ASSERT_FALSE(s_physical_vectors.empty());

    for (const auto& gvp : s_physical_vectors) {
        SCOPED_TRACE(gvp.id);
        lq::Protocol proto = parse_protocol_name(gvp.mode_str);
        auto prof = lq::get_protocol_params(proto);

        if (prof.num_tones == 4) {
            ASSERT_EQ(gvp.tones.size(), 105u);
            EXPECT_EQ(gvp.tones[0], 0);
            EXPECT_EQ(gvp.tones[104], 0);

            ASSERT_EQ(gvp.timeline.size(), 105u);
            EXPECT_EQ(gvp.timeline[0].type, "RAMP");
            EXPECT_EQ(gvp.timeline[0].tone, 0);
            EXPECT_EQ(gvp.timeline[104].type, "RAMP");
            EXPECT_EQ(gvp.timeline[104].tone, 0);
        } else {
            EXPECT_EQ(gvp.ramp_symbols, 0);
        }
    }
}

// ----------------------------------------------------------------------------
// 11. Physical Layer Symbol Timeline Contiguity and Timing
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, PhysicalLayerSymbolTimelineContiguityAndTiming) {
    ASSERT_FALSE(s_physical_vectors.empty());

    for (const auto& gvp : s_physical_vectors) {
        SCOPED_TRACE(gvp.id);
        lq::Protocol proto = parse_protocol_name(gvp.mode_str);
        auto prof = lq::get_protocol_params(proto);

        ASSERT_EQ(gvp.timeline.size(), static_cast<size_t>(prof.total_symbols));

        for (size_t i = 0; i < gvp.timeline.size(); ++i) {
            const auto& sym = gvp.timeline[i];
            double expected_start = static_cast<double>(i) * prof.symbol_period;
            double expected_end = static_cast<double>(i + 1) * prof.symbol_period;

            EXPECT_EQ(sym.symbol_idx, static_cast<int>(i));
            EXPECT_NEAR(sym.start_time_s, expected_start, 0.001);
            EXPECT_NEAR(sym.end_time_s, expected_end, 0.001);
            EXPECT_NEAR(sym.duration_s, prof.symbol_period, 0.001);
            EXPECT_EQ(sym.tone, gvp.tones[i]);

            if (i > 0) {
                EXPECT_NEAR(sym.start_time_s, gvp.timeline[i - 1].end_time_s, 0.0001);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// 12. Physical Layer Frequency Offsets Exact Match
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, PhysicalLayerFrequencyOffsetsExact) {
    ASSERT_FALSE(s_physical_vectors.empty());

    for (const auto& gvp : s_physical_vectors) {
        SCOPED_TRACE(gvp.id);
        lq::Protocol proto = parse_protocol_name(gvp.mode_str);
        auto prof = lq::get_protocol_params(proto);

        for (const auto& sym : gvp.timeline) {
            double expected_freq = static_cast<double>(sym.tone) * prof.tone_spacing;
            EXPECT_NEAR(sym.freq_offset_hz, expected_freq, 0.05);
        }
    }
}

// ----------------------------------------------------------------------------
// 13. Physical Layer Reverse Tone Demodulation Round-Trip
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, PhysicalLayerReverseToneDemodulationRoundTrip) {
    ASSERT_FALSE(s_physical_vectors.empty());

    for (const auto& gvp : s_physical_vectors) {
        SCOPED_TRACE(gvp.id);
        lq::Protocol proto = parse_protocol_name(gvp.mode_str);
        auto prof = lq::get_protocol_params(proto);

        lq::ToneSequence seq;
        seq.protocol = proto;
        seq.tones = gvp.tones;
        seq.symbol_period = prof.symbol_period;
        seq.tone_spacing = prof.tone_spacing;
        seq.tx_duration = prof.tx_duration;

        uint8_t payload[lq::PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(lq::decode_tones(seq, payload));

        lq::Message msg;
        ASSERT_TRUE(lq::decode_message(payload, msg));

        if (expects_callsign_resolution(gvp.canonical_text)) {
            lq::resolve_callsigns(msg, KNOWN_CALLS, NUM_KNOWN_CALLS);
        }

        EXPECT_EQ(lq::format_message(msg), gvp.canonical_text);
    }
}

// ----------------------------------------------------------------------------
// 14. Encoding Golden Vectors Dataset Population
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, EncodingDatasetIsPopulated) {
    ASSERT_FALSE(s_encoding_vectors.empty()) << "Encoding golden vectors must not be empty.";
    EXPECT_EQ(s_encoding_vectors.size(), 196u);
}

// ----------------------------------------------------------------------------
// 15. Encoding Forward and Reverse Verification
// ----------------------------------------------------------------------------
TEST_F(GoldenVectorsTest, EncodingForwardAndReverseVerification) {
    ASSERT_FALSE(s_encoding_vectors.empty());

    for (const auto& gve : s_encoding_vectors) {
        SCOPED_TRACE(gve.id);
        lq::Protocol proto = parse_protocol_name(gve.mode_str);

        lq::Message msg;
        ASSERT_TRUE(lq::parse_message(gve.plaintext, msg));

        uint8_t payload[lq::PAYLOAD_BYTES] = {0};
        ASSERT_TRUE(lq::encode_message(msg, payload));

        EXPECT_EQ(lq::payload_to_hex(payload), gve.payload_10byte_hex);
        EXPECT_EQ(lq::payload_to_binary(payload), gve.payload_77bit_bin);
        EXPECT_EQ(lq::format_message(msg), gve.canonical_text);

        EXPECT_EQ(lq::compute_payload_crc14(payload), gve.crc14_val);

        uint8_t in_91[lq::LDPC_INPUT_BYTES] = {0};
        lq::append_crc14(payload, in_91);
        uint8_t codeword[lq::LDPC_CODEWORD_BYTES] = {0};
        lq::ldpc_encode(in_91, codeword);

        std::string codeword_hex = "";
        for (size_t i = 0; i < 22; ++i) {
            if (i > 0) codeword_hex += " ";
            char buf[4];
            snprintf(buf, sizeof(buf), "%02X", codeword[i]);
            codeword_hex += buf;
        }
        EXPECT_EQ(codeword_hex, gve.codeword_22byte_hex);
    }
}

