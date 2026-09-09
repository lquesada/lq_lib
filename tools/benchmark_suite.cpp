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

#include "lq/lq.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <cmath>

using namespace lq;
using Clock = std::chrono::high_resolution_clock;

namespace {

void print_header(const std::string& title) {
    std::cout << "\n==============================================================================\n";
    std::cout << " " << title << "\n";
    std::cout << "==============================================================================\n";
}

void print_result(const std::string& benchmark_name, double ops_per_sec, const std::string& unit, double elapsed_ms) {
    std::cout << "  " << std::left << std::setw(40) << benchmark_name << " : "
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << ops_per_sec << " "
              << std::left << std::setw(14) << unit
              << " (" << std::fixed << std::setprecision(2) << elapsed_ms << " ms)\n";
}

// -----------------------------------------------------------------------------
// 1. LDPC(174,91) Codec Benchmark
// -----------------------------------------------------------------------------
void benchmark_ldpc() {
    print_header("1. LDPC(174,91) Codec & Normalized Min-Sum Decoder");

    uint8_t in_91[LDPC_INPUT_BYTES] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22, 0x33, 0x40};
    uint8_t out_174[LDPC_CODEWORD_BYTES];

    // Encode benchmark
    constexpr int ENCODE_ITERS = 500000;
    auto t0 = Clock::now();
    for (int i = 0; i < ENCODE_ITERS; ++i) {
        in_91[0] = static_cast<uint8_t>(i & 0xFF);
        ldpc_encode(in_91, out_174);
    }
    auto t1 = Clock::now();
    double encode_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("LDPC(174,91) Systematic Encoding", (ENCODE_ITERS / (encode_ms / 1000.0)), "encodes/sec", encode_ms);

    // Hard syndrome check benchmark
    constexpr int SYNDROME_ITERS = 1000000;
    t0 = Clock::now();
    int valid_count = 0;
    for (int i = 0; i < SYNDROME_ITERS; ++i) {
        if (ldpc_check_syndrome(out_174)) {
            ++valid_count;
        }
    }
    t1 = Clock::now();
    double syndrome_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("LDPC Syndrome Parity Verification", (SYNDROME_ITERS / (syndrome_ms / 1000.0)), "checks/sec", syndrome_ms);

    // Decoder benchmark (Clean channel / High SNR)
    constexpr int DECODE_ITERS = 20000;
    float llr_clean[174];
    BitBuffer bb(out_174, 174);
    for (int i = 0; i < 174; ++i) {
        llr_clean[i] = (bb.read_bits(1) == 0) ? +6.0f : -6.0f;
    }

    uint8_t decoded_91[LDPC_INPUT_BYTES];
    t0 = Clock::now();
    int total_iters_clean = 0;
    for (int i = 0; i < DECODE_ITERS; ++i) {
        int iters = ldpc_decode(llr_clean, decoded_91, 20);
        total_iters_clean += iters;
    }
    t1 = Clock::now();
    double decode_clean_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Normalized Min-Sum Decode (Clean LLR)", (DECODE_ITERS / (decode_clean_ms / 1000.0)), "decodes/sec", decode_clean_ms);

    // Decoder benchmark (Noisy channel / Marginal SNR: AWGN at SNR ~ -20 dB)
    std::mt19937 rng(42);
    std::normal_distribution<float> noise_dist(0.0f, 1.4f);
    float llr_noisy[174];
    for (int i = 0; i < 174; ++i) {
        float signal = (llr_clean[i] > 0.0f) ? +1.0f : -1.0f;
        llr_noisy[i] = signal + noise_dist(rng);
    }

    t0 = Clock::now();
    int total_iters_noisy = 0;
    for (int i = 0; i < DECODE_ITERS; ++i) {
        int iters = ldpc_decode(llr_noisy, decoded_91, 25);
        if (iters > 0) total_iters_noisy += iters;
    }
    t1 = Clock::now();
    double decode_noisy_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Normalized Min-Sum Decode (AWGN Noise)", (DECODE_ITERS / (decode_noisy_ms / 1000.0)), "decodes/sec", decode_noisy_ms);
}

// -----------------------------------------------------------------------------
// 2. Table-Driven CRC & Hash Computations
// -----------------------------------------------------------------------------
void benchmark_crc_and_hashes() {
    print_header("2. Table-Driven CRC-14, CRC-24/Q & WSJT-X Multiplicative Hashes");

    uint8_t payload[10] = {0xF8, 0x87, 0x79, 0x0B, 0xDF, 0x05, 0x67, 0x44, 0x8B, 0xB8};
    constexpr int CRC_ITERS = 5000000;

    auto t0 = Clock::now();
    uint16_t crc_sum = 0;
    for (int i = 0; i < CRC_ITERS; ++i) {
        payload[0] = static_cast<uint8_t>(i & 0xFF);
        crc_sum ^= compute_payload_crc14(payload);
    }
    auto t1 = Clock::now();
    double crc14_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("CRC-14 Payload Checksum", (CRC_ITERS / (crc14_ms / 1000.0)), "hashes/sec", crc14_ms);

    constexpr int HASH_ITERS = 3000000;
    const std::string test_calls[] = {
        "HB9IPH", "YO1YO", "EA6/HB9IP", "3B9/HB9IPH", "W1AW", "ZL1ABC", "JA1ABC", "DP0GVN"
    };
    t0 = Clock::now();
    uint32_t hash24_sum = 0;
    for (int i = 0; i < HASH_ITERS; ++i) {
        const auto& c = test_calls[i % 8];
        hash24_sum ^= hash_callsign_24(c);
    }
    t1 = Clock::now();
    double hash24_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("CRC-24/Q Callsign Hashing (H24)", (HASH_ITERS / (hash24_ms / 1000.0)), "hashes/sec", hash24_ms);

    t0 = Clock::now();
    uint32_t hash16_sum = 0;
    for (int i = 0; i < HASH_ITERS; ++i) {
        const auto& c = test_calls[i % 8];
        hash16_sum ^= hash_callsign_16(c);
    }
    t1 = Clock::now();
    double hash16_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("WSJT-X 16-bit Multiplicative Hash (H16)", (HASH_ITERS / (hash16_ms / 1000.0)), "hashes/sec", hash16_ms);
}

// -----------------------------------------------------------------------------
// 3. Free-Text Varicode & Huffman Trie Codecs
// -----------------------------------------------------------------------------
void benchmark_varicode_and_huffman() {
    print_header("3. Varicode 104-Symbol Compression & Huffman Trie Decoding");

    const std::string text = "CQ DX HB9IPH JN47 73 TNX FOR QSO";
    constexpr int VARICODE_ITERS = 1000000;

    uint8_t buffer[16];
    auto t0 = Clock::now();
    for (int i = 0; i < VARICODE_ITERS; ++i) {
        BitBuffer bb(buffer, 74);
        encode_varicode(text, bb, 74, true, true);
    }
    auto t1 = Clock::now();
    double varicode_enc_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Varicode 104-Symbol Text Encoding", (VARICODE_ITERS / (varicode_enc_ms / 1000.0)), "encodes/sec", varicode_enc_ms);

    BitBuffer bb_prep(buffer, 74);
    encode_varicode(text, bb_prep, 74, true, true);

    std::string decoded_text;
    t0 = Clock::now();
    for (int i = 0; i < VARICODE_ITERS; ++i) {
        BitBuffer bb(buffer, 74);
        decode_varicode(bb, decoded_text, 74, nullptr);
    }
    t1 = Clock::now();
    double varicode_dec_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Varicode Trie-Driven Decoding", (VARICODE_ITERS / (varicode_dec_ms / 1000.0)), "decodes/sec", varicode_dec_ms);

    constexpr int HUFFMAN_ITERS = 5000000;
    t0 = Clock::now();
    int huffman_sum = 0;
    for (int i = 0; i < HUFFMAN_ITERS; ++i) {
        MessageType type = static_cast<MessageType>((i % 13) + 1);
        BitBuffer bb_huff(buffer, 16);
        encode_huffman_prefix(type, bb_huff);
        BitBuffer bb_dec(buffer, 16);
        huffman_sum += static_cast<int>(decode_huffman_prefix(bb_dec));
    }
    t1 = Clock::now();
    double huffman_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Huffman Prefix Trie Encode & Decode", (HUFFMAN_ITERS / (huffman_ms / 1000.0)), "operations/sec", huffman_ms);
}

// -----------------------------------------------------------------------------
// 4. Callsign & Locator Parsing
// -----------------------------------------------------------------------------
void benchmark_fields() {
    print_header("4. Mixed-Radix Callsign & Maidenhead Locator Arithmetic");

    constexpr int FIELD_ITERS = 2000000;
    auto t0 = Clock::now();
    uint32_t packed_call = 0;
    std::string decoded_call;
    for (int i = 0; i < FIELD_ITERS; ++i) {
        encode_callsign_std("HB9IPH", packed_call);
        decode_callsign_std(packed_call, decoded_call);
    }
    auto t1 = Clock::now();
    double call_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Standard 28-bit Callsign Codec", (FIELD_ITERS / (call_ms / 1000.0)), "ops/sec", call_ms);

    uint8_t raw_b38[16] = {0};
    t0 = Clock::now();
    for (int i = 0; i < FIELD_ITERS; ++i) {
        BitBuffer bb_enc(raw_b38, 48);
        encode_callsign_nonstd("EA6/HB9IP", 9, bb_enc);
        BitBuffer bb_dec(raw_b38, 48);
        decode_callsign_nonstd(bb_dec, 9, decoded_call);
    }
    t1 = Clock::now();
    double b38_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Base-38 Big-Integer Callsign Codec", (FIELD_ITERS / (b38_ms / 1000.0)), "ops/sec", b38_ms);

    uint16_t packed_grid = 0;
    std::string decoded_grid;
    t0 = Clock::now();
    for (int i = 0; i < FIELD_ITERS; ++i) {
        encode_locator_15("JN47", packed_grid);
        decode_locator_15(packed_grid, decoded_grid);
    }
    t1 = Clock::now();
    double loc_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Maidenhead 15-bit Grid Codec", (FIELD_ITERS / (loc_ms / 1000.0)), "ops/sec", loc_ms);
}

// -----------------------------------------------------------------------------
// 5. DSP Demodulation & Continuous-Phase GFSK Synthesis
// -----------------------------------------------------------------------------
void benchmark_dsp() {
    print_header("5. Physical Layer DSP Demodulation & 8-GFSK Synthesis");

    ToneSequence seq;
    seq.protocol = Protocol::LQ8;
    seq.symbol_period = 0.160f;
    seq.tone_spacing = 6.25f;
    seq.tx_duration = 12.64f;
    seq.tones = {
        2, 5, 6, 1, 3, 0, 4,
        0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4,
        2, 5, 6, 1, 3, 0, 4,
        0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4,
        2, 5, 6, 1, 3, 0, 4
    };

    std::vector<float> audio;
    constexpr int SYNTH_ITERS = 1000;
    auto t0 = Clock::now();
    for (int i = 0; i < SYNTH_ITERS; ++i) {
        generate_audio(seq, 1500.0f, 12000.0f, audio);
    }
    auto t1 = Clock::now();
    double synth_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double total_audio_sec = (SYNTH_ITERS * 12.64);
    print_result("8-GFSK Continuous-Phase Synthesis", (total_audio_sec / (synth_ms / 1000.0)), "audio-sec/sec", synth_ms);

    // Fast coherent FSK tone demodulation
    ToneSequence demod_out;
    constexpr int DEMOD_ITERS = 500;
    t0 = Clock::now();
    for (int i = 0; i < DEMOD_ITERS; ++i) {
        demodulate_audio(audio, 1500.0f, 12000.0f, Protocol::LQ8, demod_out);
    }
    t1 = Clock::now();
    double demod_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double demod_audio_sec = (DEMOD_ITERS * 12.64);
    print_result("8-GFSK Coherent Tone Demodulation", (demod_audio_sec / (demod_ms / 1000.0)), "audio-sec/sec", demod_ms);

    // Mock waterfall buffer for testing sync score and LLR extraction
    int num_blocks = 200;
    int block_stride = 1000;
    std::vector<uint8_t> mag(num_blocks * block_stride, 10);
    for (int k = 0; k < 7; ++k) {
        mag[k * block_stride + (50 + COSTAS_ARRAY_8[k])] = 200;
        mag[(36 + k) * block_stride + (50 + COSTAS_ARRAY_8[k])] = 200;
        mag[(72 + k) * block_stride + (50 + COSTAS_ARRAY_8[k])] = 200;
    }

    constexpr int SYNC_ITERS = 200000;
    t0 = Clock::now();
    int sync_score_sum = 0;
    for (int i = 0; i < SYNC_ITERS; ++i) {
        sync_score_sum += lq_sync_score(mag.data(), num_blocks, block_stride, 0, (i % 200) + 10, Protocol::LQ8);
    }
    t1 = Clock::now();
    double sync_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Costas Synchronizer 2D Score", (SYNC_ITERS / (sync_ms / 1000.0)), "evals/sec", sync_ms);

    // LLR Extraction
    float out_llrs[174];
    constexpr int LLR_ITERS = 200000;
    t0 = Clock::now();
    for (int i = 0; i < LLR_ITERS; ++i) {
        lq_extract_llrs_from_waterfall(mag.data(), num_blocks, block_stride, 0, 50, Protocol::LQ8, out_llrs);
    }
    t1 = Clock::now();
    double llr_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Soft LLR Extraction from Waterfall", (LLR_ITERS / (llr_ms / 1000.0)), "extractions/sec", llr_ms);
}

// -----------------------------------------------------------------------------
// 6. Structured Messages & High-Level Transceiver Pipeline
// -----------------------------------------------------------------------------
void benchmark_messages_and_transceiver() {
    print_header("6. Structured Message Pack/Unpack & Transceiver Pipeline");

    Message msg;
    parse_message("CALL YO1YO HB9IPH JN47 -03", msg);
    uint8_t payload[PAYLOAD_BYTES];

    constexpr int MSG_ITERS = 1000000;
    auto t0 = Clock::now();
    for (int i = 0; i < MSG_ITERS; ++i) {
        encode_message(msg, payload);
    }
    auto t1 = Clock::now();
    double enc_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Message Payload Serialization (Pack)", (MSG_ITERS / (enc_ms / 1000.0)), "encodes/sec", enc_ms);

    t0 = Clock::now();
    Message dec_msg;
    for (int i = 0; i < MSG_ITERS; ++i) {
        decode_message(payload, dec_msg);
    }
    t1 = Clock::now();
    double dec_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("Message Payload Deserialization (Unpack)", (MSG_ITERS / (dec_ms / 1000.0)), "decodes/sec", dec_ms);

    constexpr int MOD_ITERS = 2000000;
    uint32_t packed_mod = 0;
    std::string decoded_mod;
    t0 = Clock::now();
    for (int i = 0; i < MOD_ITERS; ++i) {
        encode_modifier_20("SOTA", packed_mod);
        decode_modifier_20(packed_mod, decoded_mod);
    }
    t1 = Clock::now();
    double mod_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("CQ Modifier Codec (Dict & Custom)", (MOD_ITERS / (mod_ms / 1000.0)), "ops/sec", mod_ms);

    constexpr int RST_ITERS = 5000000;
    t0 = Clock::now();
    int rst_val = 0;
    std::string rst_str;
    for (int i = 0; i < RST_ITERS; ++i) {
        rst_str = format_rst(-15);
        parse_rst(rst_str, rst_val);
    }
    t1 = Clock::now();
    double rst_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("RST SNR Formatter & Parser", (RST_ITERS / (rst_ms / 1000.0)), "ops/sec", rst_ms);

    // IntentEngine decision slot benchmark
    IntentEngine engine("HB9IPH", "JN47");
    for (int i = 0; i < 500; ++i) {
        engine.add_known_callsign("W" + std::to_string(i % 10) + "ABC");
    }
    std::vector<ReceivedFrame> rx_frames;
    ReceivedFrame f1;
    parse_message("CQ POTA W1AW FN31", f1.msg);
    f1.snr_db = -10;
    rx_frames.push_back(f1);

    UserIntent intent;
    intent.action = IntentAction::CALL_STATION;
    intent.target_call_1 = "W1AW";

    constexpr int INTENT_ITERS = 500000;
    t0 = Clock::now();
    for (int i = 0; i < INTENT_ITERS; ++i) {
        engine.process_slot(rx_frames, intent);
    }
    t1 = Clock::now();
    double intent_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    print_result("IntentEngine Automated Decision Slot", (INTENT_ITERS / (intent_ms / 1000.0)), "decisions/sec", intent_ms);
}

} // anonymous namespace

int main() {
    std::cout << "\n==============================================================================";
    std::cout << "\n       LQ Digital Mode Family — Reference Library Microbenchmark Suite        ";
    std::cout << "\n==============================================================================\n";

    benchmark_ldpc();
    benchmark_crc_and_hashes();
    benchmark_varicode_and_huffman();
    benchmark_fields();
    benchmark_dsp();
    benchmark_messages_and_transceiver();

    std::cout << "\n==============================================================================\n";
    std::cout << " Benchmark suite execution completed successfully.\n";
    std::cout << "==============================================================================\n\n";
    return 0;
}
