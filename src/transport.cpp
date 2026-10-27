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

#include "lq/transport.h"
#include "lq/crc.h"
#include "lq/ldpc.h"
#include <cmath>
#include <complex>

#include <cstring>
#include <fstream>
#include <algorithm>
#include <array>
#include <atomic>
#include <thread>

namespace lq {

namespace {

constexpr double TWO_PI = 6.28318530717958647692;
constexpr float PI_F = 3.14159265358979323846f;

struct GfskPulseCache {
    int n_spsym = 0;
    float symbol_bt = 0.0f;
    float tone_spacing = 0.0f;
    float sample_rate = 0.0f;
    std::vector<float> scaled_pulse;
    std::vector<float> ramp_table;
};

thread_local GfskPulseCache tl_pulse_cache;
thread_local std::vector<float> tl_dphi_arr;

} // anonymous namespace

bool codeword_to_tones(const uint8_t codeword[LDPC_CODEWORD_BYTES], Protocol proto, ToneSequence& seq) {
    seq.protocol = proto;
    auto params = get_protocol_params(proto);
    seq.symbol_period = params.symbol_period;
    seq.tone_spacing = params.tone_spacing;
    seq.tx_duration = params.tx_duration;
    seq.tones.clear();
    seq.tones.resize(params.total_symbols, 0);

    BitBuffer bb(codeword, LDPC_CODEWORD_BITS);

    if (proto == Protocol::LQ8 || proto == Protocol::LQ16) {
        // 58 data symbols, 3 bits per symbol -> 8 tones
        uint8_t data_tones[58];
        for (int i = 0; i < 58; ++i) {
            uint8_t val3 = static_cast<uint8_t>(bb.read_bits(3));
            data_tones[i] = GRAY_MAP_8[val3 & 7U];
        }

        // Place Costas arrays and data symbols
        // Sync 1 (0..6)
        for (int i = 0; i < 7; ++i) seq[i] = COSTAS_ARRAY_8[i];
        // Data Block 1 (7..35)
        for (int i = 0; i < 29; ++i) seq[7 + i] = data_tones[i];
        // Sync 2 (36..42)
        for (int i = 0; i < 7; ++i) seq[36 + i] = COSTAS_ARRAY_8[i];
        // Data Block 2 (43..71)
        for (int i = 0; i < 29; ++i) seq[43 + i] = data_tones[29 + i];
        // Sync 3 (72..78)
        for (int i = 0; i < 7; ++i) seq[72 + i] = COSTAS_ARRAY_8[i];

        return true;
    } else {
        // 87 data symbols, 2 bits per symbol -> 4 tones
        uint8_t data_tones[87];
        for (int i = 0; i < 87; ++i) {
            uint8_t val2 = static_cast<uint8_t>(bb.read_bits(2));
            data_tones[i] = GRAY_MAP_4[val2 & 3U];
        }

        // Frame structure: R Sa D1 Sb D2 Sc D3 Sd R (105 symbols)
        seq[0] = 0; // Ramp symbol
        // Sync 1 (1..4)
        for (int i = 0; i < 4; ++i) seq[1 + i] = COSTAS_SYNC1_4[i];
        // Data Block 1 (5..33)
        for (int i = 0; i < 29; ++i) seq[5 + i] = data_tones[i];
        // Sync 2 (34..37)
        for (int i = 0; i < 4; ++i) seq[34 + i] = COSTAS_SYNC2_4[i];
        // Data Block 2 (38..66)
        for (int i = 0; i < 29; ++i) seq[38 + i] = data_tones[29 + i];
        // Sync 3 (67..70)
        for (int i = 0; i < 4; ++i) seq[67 + i] = COSTAS_SYNC3_4[i];
        // Data Block 3 (71..99)
        for (int i = 0; i < 29; ++i) seq[71 + i] = data_tones[58 + i];
        // Sync 4 (100..103)
        for (int i = 0; i < 4; ++i) seq[100 + i] = COSTAS_SYNC4_4[i];
        seq[104] = 0; // Ramp symbol

        return true;
    }
}

bool tones_to_codeword(const ToneSequence& seq, uint8_t codeword[LDPC_CODEWORD_BYTES]) {
    std::memset(codeword, 0, LDPC_CODEWORD_BYTES);
    BitBuffer bb(codeword, LDPC_CODEWORD_BITS);

    if (seq.protocol == Protocol::LQ8 || seq.protocol == Protocol::LQ16) {
        if (seq.size() < 79) return false;

        // Extract 58 data symbols
        // Data Block 1 (7..35)
        for (int i = 0; i < 29; ++i) {
            uint8_t tone = seq[7 + i] & 7U;
            uint8_t val3 = GRAY_INV_MAP_8[tone];
            bb.write_bits(val3, 3);
        }
        // Data Block 2 (43..71)
        for (int i = 0; i < 29; ++i) {
            uint8_t tone = seq[43 + i] & 7U;
            uint8_t val3 = GRAY_INV_MAP_8[tone];
            bb.write_bits(val3, 3);
        }
        bb.pad_zeros();
        return true;
    } else if (seq.protocol == Protocol::LQ4 || seq.protocol == Protocol::LQ2) {
        if (seq.size() < 105) return false;

        // Extract 87 data symbols (3 blocks of 29)
        // Data Block 1 (5..33)
        for (int i = 0; i < 29; ++i) {
            uint8_t tone = seq[5 + i] & 3U;
            uint8_t val2 = GRAY_INV_MAP_4[tone];
            bb.write_bits(val2, 2);
        }
        // Data Block 2 (38..66)
        for (int i = 0; i < 29; ++i) {
            uint8_t tone = seq[38 + i] & 3U;
            uint8_t val2 = GRAY_INV_MAP_4[tone];
            bb.write_bits(val2, 2);
        }
        // Data Block 3 (71..99)
        for (int i = 0; i < 29; ++i) {
            uint8_t tone = seq[71 + i] & 3U;
            uint8_t val2 = GRAY_INV_MAP_4[tone];
            bb.write_bits(val2, 2);
        }
        bb.pad_zeros();
        return true;
    }

    return false;
}

bool encode_payload(const uint8_t payload[PAYLOAD_BYTES], Protocol proto, ToneSequence& seq) {
    uint8_t processed_payload[PAYLOAD_BYTES];
    std::memcpy(processed_payload, payload, PAYLOAD_BYTES);

    if (proto == Protocol::LQ4 || proto == Protocol::LQ2) {
        for (size_t i = 0; i < PAYLOAD_BYTES; ++i) {
            processed_payload[i] ^= FT4_XOR_SEQUENCE[i];
        }
    }

    // 1. Append CRC-14 to produce 91 bits
    uint8_t in_91[LDPC_INPUT_BYTES];
    append_crc14(processed_payload, in_91);

    // 2. LDPC(174, 91) encode to produce 174 bits
    uint8_t codeword[LDPC_CODEWORD_BYTES];
    ldpc_encode(in_91, codeword);

    // 3. Map codeword to tones
    return codeword_to_tones(codeword, proto, seq);
}

bool encode_payload(const uint8_t* payload, uint8_t* tones, Protocol proto) {
    if (!payload || !tones) return false;
    ToneSequence seq;
    if (!encode_payload(payload, proto, seq)) return false;
    std::memcpy(tones, seq.tones.data(), seq.tones.size());
    return true;
}

bool encode_tones(const Message& msg, Protocol proto, ToneSequence& seq) {
    // 1. Encode message into 77-bit payload
    uint8_t payload[PAYLOAD_BYTES];
    if (!encode_message(msg, payload)) {
        return false;
    }

    return encode_payload(payload, proto, seq);
}

bool verify_sync_tones(const ToneSequence& seq) {
    if (seq.protocol == Protocol::LQ8 || seq.protocol == Protocol::LQ16) {
        if (seq.size() < 79) return false;
        int match = 0;
        for (size_t i = 0; i < 7; ++i) {
            if (seq[i] == COSTAS_ARRAY_8[i]) match++;
            if (seq[36 + i] == COSTAS_ARRAY_8[i]) match++;
            if (seq[72 + i] == COSTAS_ARRAY_8[i]) match++;
        }
        return match >= 5;
    } else if (seq.protocol == Protocol::LQ4 || seq.protocol == Protocol::LQ2) {
        if (seq.size() < 105) return false;
        int match = 0;
        for (size_t i = 0; i < 4; ++i) {
            if (seq[1 + i] == COSTAS_SYNC1_4[i]) match++;
            if (seq[34 + i] == COSTAS_SYNC2_4[i]) match++;
            if (seq[67 + i] == COSTAS_SYNC3_4[i]) match++;
            if (seq[100 + i] == COSTAS_SYNC4_4[i]) match++;
        }
        return match >= 5;
    }
    return false;
}

bool decode_tones(const ToneSequence& seq, uint8_t payload[PAYLOAD_BYTES]) {
    // 0. Verify synchronization arrays
    if (!verify_sync_tones(seq)) {
        return false;
    }

    // 1. Extract codeword from tones
    uint8_t codeword[LDPC_CODEWORD_BYTES];
    tones_to_codeword(seq, codeword);

    // 2. Decode LDPC codeword (fast 25 iters, deep 100 iters fallback)
    uint8_t decoded_91[LDPC_INPUT_BYTES];
    if (ldpc_decode_hard_bits(codeword, decoded_91, 25) < 0) {
        if (ldpc_decode_hard_bits(codeword, decoded_91, 100) < 0) {
            return false; // LDPC decode failure
        }
    }

    // 3. Verify CRC-14
    if (!verify_crc14(decoded_91)) {
        return false; // CRC mismatch
    }

    // 4. Extract 77-bit payload
    extract_payload(decoded_91, payload);

    // 5. De-whiten for FT4/LQ4/LQ2
    if (seq.protocol == Protocol::LQ4 || seq.protocol == Protocol::LQ2) {
        for (size_t i = 0; i < PAYLOAD_BYTES; ++i) {
            payload[i] ^= FT4_XOR_SEQUENCE[i];
        }
    }

    return true;
}

bool decode_payload(const uint8_t* tones, uint8_t* payload, Protocol proto) {
    if (!tones || !payload) return false;
    ToneSequence seq;
    seq.protocol = proto;
    auto params = get_protocol_params(proto);
    seq.tones.assign(tones, tones + params.total_symbols);
    seq.symbol_period = params.symbol_period;
    seq.tone_spacing = params.tone_spacing;
    seq.tx_duration = params.tx_duration;
    return decode_tones(seq, payload);
}

bool decode_tones(const ToneSequence& seq, Message& msg) {
    uint8_t payload[PAYLOAD_BYTES];
    if (!decode_tones(seq, payload)) {
        return false;
    }

    // Decode structured message
    if (decode_message(payload, msg)) {
        msg.raw_payload.assign(payload, payload + PAYLOAD_BYTES);
        return true;
    }
    return false;
}


void synth_gfsk(const uint8_t* symbols, int num_symbols, float f0, float symbol_bt, float symbol_period, float sample_rate, float* signal) {
    if (!symbols || num_symbols <= 0 || sample_rate <= 0.0f || !signal) return;

    int n_spsym = static_cast<int>(std::round(symbol_period * sample_rate));
    if (n_spsym <= 0) return;

    double dt = 1.0 / static_cast<double>(sample_rate);
    double two_pi_dt = TWO_PI * dt;
    double base_omega = TWO_PI * static_cast<double>(f0) * dt;
    float tone_spacing = 1.0f / symbol_period;
    double pulse_scale = two_pi_dt * static_cast<double>(tone_spacing);

    int n_pulse = 3 * n_spsym;
    int n_ramp = n_spsym / 2;

    // Cache pulse filter and ramp table across calls to avoid repeated std::erff / std::cos loops and heap allocations
    if (tl_pulse_cache.n_spsym != n_spsym || tl_pulse_cache.symbol_bt != symbol_bt ||
        tl_pulse_cache.sample_rate != sample_rate || tl_pulse_cache.tone_spacing != tone_spacing) {
        tl_pulse_cache.n_spsym = n_spsym;
        tl_pulse_cache.symbol_bt = symbol_bt;
        tl_pulse_cache.sample_rate = sample_rate;
        tl_pulse_cache.tone_spacing = tone_spacing;

        tl_pulse_cache.scaled_pulse.resize(n_pulse);
        float k = 5.336446f * symbol_bt;
        for (int i = 0; i < n_pulse; ++i) {
            float t = (static_cast<float>(i) - 1.5f * static_cast<float>(n_spsym)) / static_cast<float>(n_spsym);
            float p = 0.5f * (std::erff(k * (t + 0.5f)) - std::erff(k * (t - 0.5f)));
            tl_pulse_cache.scaled_pulse[i] = static_cast<float>(static_cast<double>(p) * pulse_scale);
        }

        tl_pulse_cache.ramp_table.resize(n_ramp);
        for (int i = 0; i < n_ramp; ++i) {
            tl_pulse_cache.ramp_table[i] = 0.5f * (1.0f - std::cos(PI_F * static_cast<float>(i) / static_cast<float>(n_ramp)));
        }
    }

    const float* scaled_pulse = tl_pulse_cache.scaled_pulse.data();
    const float* ramp_table = tl_pulse_cache.ramp_table.data();

    // Reusable thread-local buffer for frequency offsets convolution (avoids heap allocations)
    int total_samples = num_symbols * n_spsym;
    tl_dphi_arr.assign(total_samples, 0.0f);
    float* dphi_arr = tl_dphi_arr.data();

    for (int sym = 0; sym < num_symbols; ++sym) {
        uint8_t sym_val = symbols[sym];
        if (sym_val == 0) continue;
        float sym_f = static_cast<float>(sym_val);
        int sym_start = sym * n_spsym - n_spsym; // Center pulse at symbol center

        int p_start = std::max(0, -sym_start);
        int p_end = std::min(n_pulse, total_samples - sym_start);

        for (int p = p_start; p < p_end; ++p) {
            dphi_arr[sym_start + p] += sym_f * scaled_pulse[p];
        }
    }

    double phase = 0.0;

    // 1. Ramp-up samples
    for (int i = 0; i < n_ramp && i < total_samples; ++i) {
        phase += base_omega + static_cast<double>(dphi_arr[i]);
        if (phase >= TWO_PI) phase -= TWO_PI;
        signal[i] = ramp_table[i] * static_cast<float>(std::sin(phase));
    }

    // 2. Middle samples (amp = 1.0f)
    int middle_end = total_samples - n_ramp;
    for (int i = n_ramp; i < middle_end; ++i) {
        phase += base_omega + static_cast<double>(dphi_arr[i]);
        if (phase >= TWO_PI) phase -= TWO_PI;
        signal[i] = static_cast<float>(std::sin(phase));
    }

    // 3. Ramp-down samples
    for (int i = std::max(n_ramp, middle_end); i < total_samples; ++i) {
        phase += base_omega + static_cast<double>(dphi_arr[i]);
        if (phase >= TWO_PI) phase -= TWO_PI;
        int ramp_idx = total_samples - 1 - i;
        signal[i] = ramp_table[ramp_idx] * static_cast<float>(std::sin(phase));
    }
}

void generate_audio(const ToneSequence& seq, float base_freq_hz, float sample_rate, std::vector<float>& audio_samples) {
    audio_samples.clear();
    if (seq.tones.empty() || sample_rate <= 0.0f) return;

    size_t samples_per_symbol = static_cast<size_t>(std::round(seq.symbol_period * sample_rate));
    size_t total_samples = samples_per_symbol * seq.tones.size();
    audio_samples.resize(total_samples, 0.0f);

    float symbol_bt = (seq.protocol == Protocol::LQ8 || seq.protocol == Protocol::LQ16) ? 2.0f : 1.0f;
    synth_gfsk(seq.tones.data(), static_cast<int>(seq.tones.size()), base_freq_hz, symbol_bt, seq.symbol_period, sample_rate, audio_samples.data());
}

int lq_sync_score(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, Protocol proto) {
    if (!mag || num_blocks <= 0 || block_stride <= 0 || time_offset < 0 || freq_offset < 0) return 0;

    int total_score = 0;

    if (proto == Protocol::LQ8 || proto == Protocol::LQ16) {
        if (time_offset + 79 > num_blocks || freq_offset + 8 > block_stride) return 0;

        const int sync_starts[3] = {0, 36, 72};
        for (int s = 0; s < 3; ++s) {
            for (int k = 0; k < 7; ++k) {
                int t = time_offset + sync_starts[s] + k;
                const uint8_t* row = mag + (t * block_stride + freq_offset);
                int target_tone = COSTAS_ARRAY_8[k];
                int target_val = row[target_tone];

                int row_sum = 0;
                #pragma GCC unroll 8
                for (int tone = 0; tone < 8; ++tone) {
                    row_sum += row[tone];
                }
                int score = 8 * target_val - row_sum;
                if (score > 0) total_score += score;
            }
        }
    } else if (proto == Protocol::LQ4 || proto == Protocol::LQ2) {
        if (time_offset + 105 > num_blocks || freq_offset + 4 > block_stride) return 0;

        const int sync_starts[4] = {1, 34, 67, 100};
        const uint8_t* sync_patterns[4] = {COSTAS_SYNC1_4.data(), COSTAS_SYNC2_4.data(), COSTAS_SYNC3_4.data(), COSTAS_SYNC4_4.data()};

        for (int s = 0; s < 4; ++s) {
            for (int k = 0; k < 4; ++k) {
                int t = time_offset + sync_starts[s] + k;
                const uint8_t* row = mag + (t * block_stride + freq_offset);
                int target_tone = sync_patterns[s][k];
                int target_val = row[target_tone];

                int row_sum = 0;
                #pragma GCC unroll 4
                for (int tone = 0; tone < 4; ++tone) {
                    row_sum += row[tone];
                }
                int score = 4 * target_val - row_sum;
                if (score > 0) total_score += score;
            }
        }
    }

    return total_score;
}

float lq_refine_frequency(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, int freq_sub, int freq_osr, float symbol_period, Protocol proto, int* out_score) {
    if (freq_osr <= 0) freq_osr = 1;
    float df = (1.0f / symbol_period) / static_cast<float>(freq_osr);

    int score_m1 = lq_sync_score(mag, num_blocks, block_stride, time_offset, freq_offset > 0 ? freq_offset - 1 : 0, proto);
    int score_0  = lq_sync_score(mag, num_blocks, block_stride, time_offset, freq_offset, proto);
    int score_p1 = lq_sync_score(mag, num_blocks, block_stride, time_offset, freq_offset + 1 < block_stride ? freq_offset + 1 : freq_offset, proto);

    if (out_score) *out_score = score_0;

    // 3-point parabolic interpolation
    float delta = 0.0f;
    float denom = 2.0f * (2.0f * static_cast<float>(score_0) - static_cast<float>(score_m1) - static_cast<float>(score_p1));
    if (denom > 1e-4f) {
        delta = (static_cast<float>(score_p1) - static_cast<float>(score_m1)) / denom;
        if (delta < -0.5f) delta = -0.5f;
        if (delta > 0.5f) delta = 0.5f;
    }

    float refined_hz = (static_cast<float>(freq_offset) + static_cast<float>(freq_sub) / static_cast<float>(freq_osr) + delta) * df;
    return refined_hz;
}

float lq_guess_snr(const float* mag2, int num_blocks, int block_stride, int time_offset, int freq_offset, Protocol proto) {
    if (!mag2 || num_blocks <= 0 || block_stride <= 0 || time_offset < 0 || freq_offset < 0) return -30.0f;

    auto params = get_protocol_params(proto);
    int total_syms = params.total_symbols;
    int num_tones = params.num_tones;

    if (time_offset + total_syms > num_blocks || freq_offset + num_tones > block_stride) return -30.0f;

    double signal_power = 0.0;
    double noise_power = 0.0;
    int noise_bins = 0;

    for (int t = 0; t < total_syms; ++t) {
        int block_idx = time_offset + t;
        float max_p = 0.0f;
        for (int tone = 0; tone < num_tones; ++tone) {
            float p = mag2[block_idx * block_stride + (freq_offset + tone)];
            if (p > max_p) max_p = p;
        }
        signal_power += max_p;

        // Estimate noise from surrounding passband (outside the signal bandwidth)
        for (int n = -4; n < 0; ++n) {
            int bin = freq_offset + n;
            if (bin >= 0 && bin < block_stride) {
                noise_power += mag2[block_idx * block_stride + bin];
                noise_bins++;
            }
        }
        for (int n = num_tones; n < num_tones + 4; ++n) {
            int bin = freq_offset + n;
            if (bin >= 0 && bin < block_stride) {
                noise_power += mag2[block_idx * block_stride + bin];
                noise_bins++;
            }
        }
    }

    if (noise_bins == 0 || noise_power <= 1e-12 || signal_power <= 1e-12) return -30.0f;

    double avg_noise_per_bin = noise_power / noise_bins;
    double avg_sig = (signal_power / total_syms) - avg_noise_per_bin;
    if (avg_sig <= 0.0) return -30.0f;

    // Standard amateur reference bandwidth 2500 Hz
    double snr_linear = (avg_sig / (avg_noise_per_bin * (2500.0 / params.tone_spacing)));
    float snr_db = static_cast<float>(10.0 * std::log10(std::max(snr_linear, 1e-4)));
    return std::clamp(snr_db, -30.0f, +30.0f);
}

bool lq_extract_llrs_from_waterfall(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, Protocol proto, float out_llrs_174[LDPC_CODEWORD_BITS]) {
    if (!mag || !out_llrs_174 || num_blocks <= 0 || block_stride <= 0 || time_offset < 0 || freq_offset < 0) return false;

    if (proto == Protocol::LQ8 || proto == Protocol::LQ16) {
        if (time_offset + 79 > num_blocks || freq_offset + 8 > block_stride) return false;

        int data_block_1 = 7;
        int data_block_2 = 43;
        int bit_idx = 0;
        constexpr float SCALE = 0.25f;

        for (int block = 0; block < 2; ++block) {
            int start_t = (block == 0) ? (time_offset + data_block_1) : (time_offset + data_block_2);
            for (int s = 0; s < 29; ++s) {
                int t = start_t + s;
                const uint8_t* row = mag + (t * block_stride + freq_offset);

                float m0 = static_cast<float>(row[0]);
                float m1 = static_cast<float>(row[1]);
                float m2 = static_cast<float>(row[2]);
                float m3 = static_cast<float>(row[3]);
                float m4 = static_cast<float>(row[4]);
                float m5 = static_cast<float>(row[5]);
                float m6 = static_cast<float>(row[6]);
                float m7 = static_cast<float>(row[7]);

                float max_01 = std::max(m0, m1);
                float max_23 = std::max(m2, m3);
                float max_45 = std::max(m4, m5);
                float max_67 = std::max(m6, m7);

                float max0_b0 = std::max(max_01, max_23);
                float max1_b0 = std::max(max_45, max_67);

                float max0_b1 = std::max(max_01, std::max(m5, m6));
                float max1_b1 = std::max(max_23, std::max(m4, m7));

                float max0_b2 = std::max(std::max(m0, m3), max_45);
                float max1_b2 = std::max(std::max(m1, m2), max_67);

                out_llrs_174[bit_idx++] = (max0_b0 - max1_b0) * SCALE;
                out_llrs_174[bit_idx++] = (max0_b1 - max1_b1) * SCALE;
                out_llrs_174[bit_idx++] = (max0_b2 - max1_b2) * SCALE;
            }
        }
        return true;
    } else {
        if (time_offset + 105 > num_blocks || freq_offset + 4 > block_stride) return false;

        const int data_starts[3] = {5, 38, 71};
        int bit_idx = 0;
        constexpr float SCALE = 0.25f;

        for (int block = 0; block < 3; ++block) {
            int start_t = time_offset + data_starts[block];
            for (int s = 0; s < 29; ++s) {
                int t = start_t + s;
                const uint8_t* row = mag + (t * block_stride + freq_offset);

                float m0 = static_cast<float>(row[0]);
                float m1 = static_cast<float>(row[1]);
                float m2 = static_cast<float>(row[2]);
                float m3 = static_cast<float>(row[3]);

                float max0_b0 = std::max(m0, m1);
                float max1_b0 = std::max(m2, m3);

                float max0_b1 = std::max(m0, m3);
                float max1_b1 = std::max(m1, m2);

                out_llrs_174[bit_idx++] = (max0_b0 - max1_b0) * SCALE;
                out_llrs_174[bit_idx++] = (max0_b1 - max1_b1) * SCALE;
            }
        }
        return true;
    }
}

void lq_subtract_signal_from_waterfall(uint8_t* mag, int max_mag_size, int block_size, int block_stride, const uint8_t* payload, float freq_hz, float time_sec, float sample_rate, Protocol proto) {
    if (!mag || !payload || max_mag_size <= 0 || block_size <= 0 || block_stride <= 0 || sample_rate <= 0.0f) return;

    ToneSequence seq;
    if (!encode_payload(payload, proto, seq)) return;

    auto params = get_protocol_params(proto);
    float dt_block = static_cast<float>(block_size) / sample_rate;
    int time_start_block = static_cast<int>(std::round(time_sec / dt_block));
    float df = sample_rate / static_cast<float>(block_size);
    int base_bin = static_cast<int>(std::round(freq_hz / df));

    int blocks_per_sym = static_cast<int>(std::round(params.symbol_period / dt_block));
    if (blocks_per_sym <= 0) blocks_per_sym = 1;

    for (size_t sym = 0; sym < seq.tones.size(); ++sym) {
        uint8_t tone = seq.tones[sym];
        int tone_bin = base_bin + static_cast<int>(std::round((static_cast<float>(tone) * params.tone_spacing) / df));

        for (int b = 0; b < blocks_per_sym; ++b) {
            int t = time_start_block + static_cast<int>(sym) * blocks_per_sym + b;
            int offset = t * block_stride + tone_bin;
            if (offset >= 0 && offset < max_mag_size && tone_bin >= 0 && tone_bin < block_stride) {
                // Attenuate tone bin energy
                mag[offset] = static_cast<uint8_t>(mag[offset] / 4);
                if (tone_bin > 0) mag[offset - 1] = static_cast<uint8_t>(mag[offset - 1] / 2);
                if (tone_bin + 1 < block_stride) mag[offset + 1] = static_cast<uint8_t>(mag[offset + 1] / 2);
            }
        }
    }
}

bool demodulate_audio_offset(const std::vector<float>& audio_samples, size_t offset, float base_freq_hz, float sample_rate, Protocol proto, ToneSequence& seq) {
    auto params = get_protocol_params(proto);
    seq.protocol = proto;
    seq.symbol_period = params.symbol_period;
    seq.tone_spacing = params.tone_spacing;
    seq.tx_duration = params.tx_duration;
    seq.tones.clear();
    seq.tones.resize(params.total_symbols, 0);

    size_t samples_per_sym = static_cast<size_t>(std::round(params.symbol_period * sample_rate));
    if (samples_per_sym == 0) return false;
    size_t total_samples_needed = samples_per_sym * params.total_symbols;
    if (audio_samples.size() - offset < total_samples_needed) {
        return false;
    }

    double total_energy = 0.0;
    for (size_t i = 0; i < total_samples_needed; ++i) {
        float s = audio_samples[offset + i];
        total_energy += static_cast<double>(s * s);
    }
    if (total_energy < 1e-5) {
        return false;
    }

    float dt = 1.0f / sample_rate;

    // Precalculate Goertzel IIR filter coefficients once
    float tone_coeff[16];
    for (int tone = 0; tone < params.num_tones; ++tone) {
        float tone_freq = base_freq_hz + static_cast<float>(tone) * params.tone_spacing;
        float omega = static_cast<float>(TWO_PI) * tone_freq * dt;
        tone_coeff[tone] = 2.0f * std::cos(omega);
    }

    auto compute_tone_energy = [&](size_t start_sample, int tone) -> float {
        float coeff = tone_coeff[tone];
        float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f;
        for (size_t n = 0; n < samples_per_sym; ++n) {
            s0 = audio_samples[start_sample + n] + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        float energy = s1 * s1 + s2 * s2 - coeff * s1 * s2;
        return std::max(0.0f, energy);
    };

    for (int sym_idx = 0; sym_idx < params.total_symbols; ++sym_idx) {
        size_t start_sample = offset + sym_idx * samples_per_sym;

        float max_energy = -1.0f;
        uint8_t best_tone = 0;

        for (int tone = 0; tone < params.num_tones; ++tone) {
            float energy = compute_tone_energy(start_sample, tone);
            if (energy > max_energy) {
                max_energy = energy;
                best_tone = static_cast<uint8_t>(tone);
            }
        }

        seq.tones[sym_idx] = best_tone;
    }

    return true;
}

bool demodulate_audio(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, ToneSequence& seq) {
    return demodulate_audio_offset(audio_samples, 0, base_freq_hz, sample_rate, proto, seq);
}

bool demodulate_audio_soft(const std::vector<float>& audio_samples, size_t offset, float base_freq_hz, float sample_rate, Protocol proto, std::vector<float>& llrs, int min_sync_matches) {
    auto params = get_protocol_params(proto);
    size_t samples_per_sym = static_cast<size_t>(std::round(params.symbol_period * sample_rate));
    if (samples_per_sym == 0) return false;

    size_t frame_samples = samples_per_sym * params.total_symbols;
    if (offset + frame_samples > audio_samples.size()) {
        return false;
    }

    // Measure audio energy to reject flat-line/pure-silence buffers
    double total_energy = 0.0;
    for (size_t i = 0; i < frame_samples; ++i) {
        float s = audio_samples[offset + i];
        total_energy += static_cast<double>(s * s);
    }
    if (total_energy < 1e-5) {
        return false;
    }

    int eff_min_sync = (min_sync_matches > 0) ? min_sync_matches
                      : ((proto == Protocol::LQ8 || proto == Protocol::LQ16) ? 5 : 6);

    llrs.clear();
    llrs.reserve(LDPC_CODEWORD_BITS);

    float dt = 1.0f / sample_rate;

    // Precalculate Goertzel IIR filter coefficients once
    float tone_coeff[16];
    for (int tone = 0; tone < params.num_tones; ++tone) {
        float tone_freq = base_freq_hz + static_cast<float>(tone) * params.tone_spacing;
        float omega = static_cast<float>(TWO_PI) * tone_freq * dt;
        tone_coeff[tone] = 2.0f * std::cos(omega);
    }

    if (proto == Protocol::LQ8 || proto == Protocol::LQ16) {
        int sync_matches_std = 0;
        const int sync_starts[3] = {0, 36, 72};
        int block_matches[3] = {0, 0, 0};

        for (int b = 0; b < 3; ++b) {
            for (int k = 0; k < 7; ++k) {
                int sym_idx = sync_starts[b] + k;
                const float* audio_ptr = &audio_samples[offset + sym_idx * samples_per_sym];

                float s1[8] = {0.0f}, s2[8] = {0.0f};
                for (size_t n = 0; n < samples_per_sym; ++n) {
                    float x = audio_ptr[n];
                    for (int t = 0; t < 8; ++t) {
                        float s0 = x + tone_coeff[t] * s1[t] - s2[t];
                        s2[t] = s1[t];
                        s1[t] = s0;
                    }
                }

                float max_energy = -1.0f;
                int best_tone = 0;
                for (int t = 0; t < 8; ++t) {
                    float energy = s1[t] * s1[t] + s2[t] * s2[t] - tone_coeff[t] * s1[t] * s2[t];
                    if (energy > max_energy) {
                        max_energy = energy;
                        best_tone = t;
                    }
                }
                if (best_tone == COSTAS_ARRAY_8[k]) {
                    ++sync_matches_std;
                    ++block_matches[b];
                }
            }
        }
        int multi_block_sync = (block_matches[0] >= 2 ? 1 : 0) +
                               (block_matches[1] >= 2 ? 1 : 0) +
                               (block_matches[2] >= 2 ? 1 : 0);
        if (sync_matches_std < eff_min_sync && !(multi_block_sync >= 2 && sync_matches_std >= 4)) {
            return false;
        }

        int data_block_1_start = 7;
        int data_block_2_start = 43;

        for (int block = 0; block < 2; ++block) {
            int start_sym = (block == 0) ? data_block_1_start : data_block_2_start;
            for (int s = 0; s < 29; ++s) {
                int sym_idx = start_sym + s;
                const float* audio_ptr = &audio_samples[offset + sym_idx * samples_per_sym];

                float s1[8] = {0.0f}, s2[8] = {0.0f};
                for (size_t n = 0; n < samples_per_sym; ++n) {
                    float x = audio_ptr[n];
                    for (int t = 0; t < 8; ++t) {
                        float s0 = x + tone_coeff[t] * s1[t] - s2[t];
                        s2[t] = s1[t];
                        s1[t] = s0;
                    }
                }

                float tone_mag[8];
                for (int t = 0; t < 8; ++t) {
                    float energy = std::max(0.0f, s1[t] * s1[t] + s2[t] * s2[t] - tone_coeff[t] * s1[t] * s2[t]);
                    tone_mag[t] = static_cast<float>(std::sqrt(energy));
                }

                float m0 = tone_mag[0];
                float m1 = tone_mag[1];
                float m2 = tone_mag[2];
                float m3 = tone_mag[3];
                float m4 = tone_mag[4];
                float m5 = tone_mag[5];
                float m6 = tone_mag[6];
                float m7 = tone_mag[7];

                float max_01 = std::max(m0, m1);
                float max_23 = std::max(m2, m3);
                float max_45 = std::max(m4, m5);
                float max_67 = std::max(m6, m7);

                float max0_b0 = std::max(max_01, max_23);
                float max1_b0 = std::max(max_45, max_67);

                float max0_b1 = std::max(max_01, std::max(m5, m6));
                float max1_b1 = std::max(max_23, std::max(m4, m7));

                float max0_b2 = std::max(std::max(m0, m3), max_45);
                float max1_b2 = std::max(std::max(m1, m2), max_67);

                constexpr float SCALE = 0.25f;
                llrs.push_back((max0_b0 - max1_b0) * SCALE);
                llrs.push_back((max0_b1 - max1_b1) * SCALE);
                llrs.push_back((max0_b2 - max1_b2) * SCALE);
            }
        }

        // Adaptive LLR normalization to reference variance 24.0
        float sum_llr = 0.0f;
        float sum_llr2 = 0.0f;
        for (float val : llrs) {
            sum_llr += val;
            sum_llr2 += val * val;
        }
        float var = (sum_llr2 - (sum_llr * sum_llr) / 174.0f) / 174.0f;
        if (var > 1e-4f) {
            float norm_factor = std::sqrt(24.0f / var);
            norm_factor = std::clamp(norm_factor, 0.1f, 10.0f);
            for (float& val : llrs) {
                val *= norm_factor;
            }
        }
        return true;
    } else {
        int sync_matches_std = 0;
        const int sync_starts[4] = {1, 34, 67, 100};
        const std::array<uint8_t, 4>* sync_patterns[4] = {&COSTAS_SYNC1_4, &COSTAS_SYNC2_4, &COSTAS_SYNC3_4, &COSTAS_SYNC4_4};
        int block_matches[4] = {0, 0, 0, 0};

        for (int b = 0; b < 4; ++b) {
            for (int k = 0; k < 4; ++k) {
                int sym_idx = sync_starts[b] + k;
                const float* audio_ptr = &audio_samples[offset + sym_idx * samples_per_sym];

                float s1[4] = {0.0f}, s2[4] = {0.0f};
                for (size_t n = 0; n < samples_per_sym; ++n) {
                    float x = audio_ptr[n];
                    for (int t = 0; t < 4; ++t) {
                        float s0 = x + tone_coeff[t] * s1[t] - s2[t];
                        s2[t] = s1[t];
                        s1[t] = s0;
                    }
                }

                float max_energy = -1.0f;
                int best_tone = 0;
                for (int t = 0; t < 4; ++t) {
                    float energy = s1[t] * s1[t] + s2[t] * s2[t] - tone_coeff[t] * s1[t] * s2[t];
                    if (energy > max_energy) {
                        max_energy = energy;
                        best_tone = t;
                    }
                }
                if (best_tone == (*sync_patterns[b])[k]) {
                    ++sync_matches_std;
                    ++block_matches[b];
                }
            }
        }
        int multi_block_sync = 0;
        for (int b = 0; b < 4; ++b) {
            if (block_matches[b] >= 2) ++multi_block_sync;
        }
        if (sync_matches_std < eff_min_sync && !(multi_block_sync >= 2 && sync_matches_std >= 4)) {
            return false;
        }

        const int data_starts[3] = {5, 38, 71};

        for (int block = 0; block < 3; ++block) {
            int start_sym = data_starts[block];
            for (int s = 0; s < 29; ++s) {
                int sym_idx = start_sym + s;
                const float* audio_ptr = &audio_samples[offset + sym_idx * samples_per_sym];

                float s1[4] = {0.0f}, s2[4] = {0.0f};
                for (size_t n = 0; n < samples_per_sym; ++n) {
                    float x = audio_ptr[n];
                    for (int t = 0; t < 4; ++t) {
                        float s0 = x + tone_coeff[t] * s1[t] - s2[t];
                        s2[t] = s1[t];
                        s1[t] = s0;
                    }
                }

                float tone_mag[4];
                for (int t = 0; t < 4; ++t) {
                    float energy = std::max(0.0f, s1[t] * s1[t] + s2[t] * s2[t] - tone_coeff[t] * s1[t] * s2[t]);
                    tone_mag[t] = static_cast<float>(std::sqrt(energy));
                }

                float m0 = tone_mag[0];
                float m1 = tone_mag[1];
                float m2 = tone_mag[2];
                float m3 = tone_mag[3];

                float max0_b0 = std::max(m0, m1);
                float max1_b0 = std::max(m2, m3);

                float max0_b1 = std::max(m0, m3);
                float max1_b1 = std::max(m1, m2);

                constexpr float SCALE = 0.25f;
                llrs.push_back((max0_b0 - max1_b0) * SCALE);
                llrs.push_back((max0_b1 - max1_b1) * SCALE);
            }
        }

        // Adaptive LLR normalization to reference variance 24.0
        float sum_llr = 0.0f;
        float sum_llr2 = 0.0f;
        for (float val : llrs) {
            sum_llr += val;
            sum_llr2 += val * val;
        }
        float var = (sum_llr2 - (sum_llr * sum_llr) / 174.0f) / 174.0f;
        if (var > 1e-4f) {
            float norm_factor = std::sqrt(24.0f / var);
            norm_factor = std::clamp(norm_factor, 0.1f, 10.0f);
            for (float& val : llrs) {
                val *= norm_factor;
            }
        }
        return true;
    }
}

bool decode_soft_llrs(const std::vector<float>& llrs, Message& msg, Protocol proto, int max_ldpc_iters) {
    if (llrs.size() < LDPC_CODEWORD_BITS) {
        return false;
    }

    float max_llr = 0.0f;
    for (float l : llrs) {
        max_llr = std::max(max_llr, std::abs(l));
    }
    if (max_llr < 1e-5f) {
        return false;
    }

    uint8_t decoded_91[LDPC_INPUT_BYTES];
    if (ldpc_decode(llrs.data(), decoded_91, max_ldpc_iters) < 0) {
        return false;
    }

    // Reject all-zero codeword (LDPC nullspace / unseeded CRC-14 nullspace)
    bool all_zero = true;
    for (int i = 0; i < LDPC_INPUT_BYTES; ++i) {
        if (decoded_91[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (all_zero) {
        return false;
    }

    if (!verify_crc14(decoded_91)) {
        return false;
    }

    uint8_t payload[PAYLOAD_BYTES];
    extract_payload(decoded_91, payload);

    if (proto == Protocol::LQ4 || proto == Protocol::LQ2) {
        for (size_t i = 0; i < PAYLOAD_BYTES; ++i) {
            payload[i] ^= FT4_XOR_SEQUENCE[i];
        }
    }

    if (decode_message(payload, msg)) {
        msg.raw_payload.assign(payload, payload + PAYLOAD_BYTES);
        return true;
    }
    return false;
}

bool message_to_audio(const Message& msg, Protocol proto, float base_freq_hz, float sample_rate, std::vector<float>& audio_samples) {
    ToneSequence seq;
    if (!encode_tones(msg, proto, seq)) {
        return false;
    }
    generate_audio(seq, base_freq_hz, sample_rate, audio_samples);
    return true;
}

bool audio_to_message(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, Message& msg, int num_threads, bool is_deep) {
    auto messages = audio_to_messages(audio_samples, base_freq_hz, sample_rate, proto, num_threads, is_deep);
    if (!messages.empty()) {
        msg = messages.front();
        return true;
    }
    return false;
}

// =============================================================================
// Multi-signal decoder
//
//   1. Candidate search on a power spectrogram (half-tone frequency bins, quarter-symbol time steps):
//      soft Costas score = sync-tone power against the mean power of the other tones.
//   2. Per candidate: band-limited complex baseband (one FFT of the whole buffer, one small inverse FFT
//      per candidate, 32 samples per symbol), then a fine time / frequency search on the sync symbols and
//      over the whole frame.
//   3. Complex tone correlations per symbol; carrier calibration: turn per symbol and timing from the sync
//      blocks, then all sync symbols coherent together (sidelobes judged with the data symbols) for the
//      carrier phase of the frame.
//   4. Soft bits: coherent (carrier phase known) symbol by symbol, and noncoherent over groups of symbols.
//   5. Sum-product LDPC on each; in deep mode ordered-statistics decoding on the coherent soft bits
//      (CRC-14 checked, distance-gated).
//   6. Deep mode: the decoded signals are masked in the spectrogram and the search runs again.
// =============================================================================

namespace {

using cfloat = std::complex<float>;

// Plain complex product (std::complex's operator* also handles inf/NaN, which costs a library call per product).
inline cfloat cmul(cfloat a, cfloat b) {
    return cfloat(a.real() * b.real() - a.imag() * b.imag(), a.real() * b.imag() + a.imag() * b.real());
}

inline cfloat unit(double phase) {
    return cfloat(static_cast<float>(std::cos(phase)), static_cast<float>(std::sin(phase)));
}

// Decoder settings, measured on AWGN at the protocols' threshold SNRs.
constexpr int DEEP_MAX_CANDIDATES = 200;   // candidates decoded per pass
constexpr int FAST_MAX_CANDIDATES = 60;
constexpr float DEEP_MIN_SCORE = 1.1f;     // sync score against the noise floor of the score
constexpr float FAST_MIN_SCORE = 1.3f;
constexpr int PEAKS_PER_BIN = 4;           // start times kept per frequency bin
constexpr int FREQ_STEPS = 8;              // fine frequency search: +/- half a tone spacing in 2 * 8 steps
constexpr double TIMING_RANGE = 3.0;       // sync-block timing search, baseband samples (of ~32 per symbol)
constexpr int BLOCK_W_STEPS = 25;          // sync-block turn search: +/- 0.5 rad per symbol ...
constexpr double BLOCK_W_STEP = 0.02;      // ... in 0.02 rad steps
constexpr int GLOBAL_W_STEPS = 150;        // whole-frame turn search: +/- 0.3 rad per symbol ...
constexpr double GLOBAL_W_STEP = 0.002;    // ... in 0.002 rad steps
constexpr int W_CANDIDATES = 5;            // whole-frame turn peaks judged with the data symbols
constexpr float LLR_GAIN = 1.5f;           // applied to the model LLRs (the amplitude estimate is noisy)
constexpr float LLR_CLAMP = 12.0f;
constexpr float LLR_MEDIAN_CAP = 4.0f;      // typical |llr| kept at most this (see condition_llrs)
constexpr int OSD_ORDER = 2;
constexpr int OSD_PAIR_SPAN = 40;
constexpr float OSD_MAX_DISTANCE = 0.04f;  // OSD codeword: disagreement / total |llr|
constexpr int OSD_MAX_SILENT_BITS = 8;     // bits with |llr| < 2 % of the mean allowed before OSD (AWGN: 0..6)

// ---- Mixed-radix FFT (any length; fast for lengths made of 2, 3 and 5) ----

struct FftPlan {
    int n = 0;
    std::vector<int> factors;
    std::vector<cfloat> twiddles;   // exp(-2 pi i k / n)
};

FftPlan make_fft_plan(int n) {
    FftPlan plan;
    plan.n = n;
    int m = n;
    for (int p : {4, 2, 3, 5}) {
        while (m % p == 0 && m > 1) { plan.factors.push_back(p); m /= p; }
    }
    for (int p = 7; m > 1; p += 2) {
        while (m % p == 0) { plan.factors.push_back(p); m /= p; }
    }
    if (plan.factors.empty()) plan.factors.push_back(1);
    plan.twiddles.resize(static_cast<size_t>(n));
    for (int k = 0; k < n; ++k) {
        double a = -TWO_PI * static_cast<double>(k) / static_cast<double>(n);
        plan.twiddles[static_cast<size_t>(k)] = cfloat(static_cast<float>(std::cos(a)), static_cast<float>(std::sin(a)));
    }
    return plan;
}

void fft_pass(const cfloat* in, cfloat* out, int n, int in_stride, const int* factors, const FftPlan& plan, int tw_stride) {
    const int p = factors[0];
    const int m = n / p;
    if (m == 1) {
        for (int q = 0; q < p; ++q) out[q] = in[q * in_stride];
    } else {
        for (int q = 0; q < p; ++q) {
            fft_pass(in + q * in_stride, out + q * m, m, in_stride * p, factors + 1, plan, tw_stride * p);
        }
    }
    if (p == 1) return;
    const cfloat* tw = plan.twiddles.data();
    const int big_n = plan.n;
    if (p == 2) {
        for (int k = 0; k < m; ++k) {
            cfloat a = out[k];
            cfloat b = cmul(out[m + k], tw[k * tw_stride]);
            out[k] = a + b;
            out[m + k] = a - b;
        }
        return;
    }
    if (p == 4) {
        for (int k = 0; k < m; ++k) {
            cfloat a0 = out[k];
            cfloat a1 = cmul(out[m + k], tw[k * tw_stride]);
            cfloat a2 = cmul(out[2 * m + k], tw[2 * k * tw_stride]);
            cfloat a3 = cmul(out[3 * m + k], tw[3 * k * tw_stride]);
            cfloat s02 = a0 + a2, d02 = a0 - a2, s13 = a1 + a3, d13 = a1 - a3;
            cfloat d13_mj(d13.imag(), -d13.real());   // d13 * (-i)
            out[k] = s02 + s13;
            out[m + k] = d02 + d13_mj;
            out[2 * m + k] = s02 - s13;
            out[3 * m + k] = d02 - d13_mj;
        }
        return;
    }
    cfloat tmp_small[16];
    std::vector<cfloat> tmp_big;
    cfloat* tmp = tmp_small;
    if (p > 16) { tmp_big.resize(static_cast<size_t>(p)); tmp = tmp_big.data(); }
    for (int k = 0; k < m; ++k) {
        for (int q = 0; q < p; ++q) tmp[q] = cmul(out[q * m + k], tw[q * k * tw_stride]);
        for (int r = 0; r < p; ++r) {
            cfloat acc = tmp[0];
            for (int q = 1; q < p; ++q) {
                acc += cmul(tmp[q], tw[(static_cast<long long>(q) * r * m * tw_stride) % big_n]);
            }
            out[r * m + k] = acc;
        }
    }
}

void fft_forward(const FftPlan& plan, const cfloat* in, cfloat* out) {
    fft_pass(in, out, plan.n, 1, plan.factors.data(), plan, 1);
}

void fft_inverse(const FftPlan& plan, std::vector<cfloat>& buf, std::vector<cfloat>& out) {
    for (auto& v : buf) v = std::conj(v);
    out.resize(buf.size());
    fft_forward(plan, buf.data(), out.data());
    for (auto& v : out) v = std::conj(v);
}

int next_fast_size(int n) {
    for (int m = std::max(n, 1);; ++m) {
        int r = m;
        for (int p : {2, 3, 5}) while (r % p == 0) r /= p;
        if (r == 1) return m;
    }
}

// ---- Frame layout ----

struct FrameLayout {
    int num_tones = 8;
    int bits_per_symbol = 3;
    int total_symbols = 79;
    int center_tone = 3;                    // tone placed near 0 Hz in the baseband
    std::vector<std::pair<int, int>> sync;  // (symbol index, tone)
    std::vector<int> data_symbols;          // symbol indices in codeword bit order
    const uint8_t* gray_inv = nullptr;      // tone -> bit value
};

FrameLayout make_frame_layout(Protocol proto) {
    FrameLayout L;
    if (proto == Protocol::LQ8 || proto == Protocol::LQ16) {
        L.num_tones = 8; L.bits_per_symbol = 3; L.total_symbols = 79; L.center_tone = 3;
        for (int start : COSTAS_OFFSETS_8) {
            for (int k = 0; k < 7; ++k) L.sync.push_back({start + k, COSTAS_ARRAY_8[k]});
        }
        for (int s = 7; s < 36; ++s) L.data_symbols.push_back(s);
        for (int s = 43; s < 72; ++s) L.data_symbols.push_back(s);
        L.gray_inv = GRAY_INV_MAP_8.data();
    } else {
        L.num_tones = 4; L.bits_per_symbol = 2; L.total_symbols = 105; L.center_tone = 1;
        const int starts[4] = {1, 34, 67, 100};
        const std::array<uint8_t, 4>* pats[4] = {&COSTAS_SYNC1_4, &COSTAS_SYNC2_4, &COSTAS_SYNC3_4, &COSTAS_SYNC4_4};
        for (int b = 0; b < 4; ++b) {
            for (int k = 0; k < 4; ++k) L.sync.push_back({starts[b] + k, (*pats[b])[k]});
        }
        for (int s = 5; s < 34; ++s) L.data_symbols.push_back(s);
        for (int s = 38; s < 67; ++s) L.data_symbols.push_back(s);
        for (int s = 71; s < 100; ++s) L.data_symbols.push_back(s);
        L.gray_inv = GRAY_INV_MAP_4.data();
    }
    return L;
}

// ---- Shared state of one decode call ----

struct DecodeContext {
    Protocol proto;
    ProtocolParams params;
    FrameLayout layout;
    float sample_rate = 12000.0f;
    bool deep = false;

    // Candidate spectrogram
    int sym_len = 0;          // samples per symbol (rounded)
    int time_osr = 4;         // spectrogram steps per symbol
    int freq_osr = 2;         // spectrogram bins per tone spacing
    int hop = 0;              // samples per spectrogram step
    int spec_fft = 0;
    int num_steps = 0;
    int bin_lo = 0, bin_hi = 0;   // candidate tone-0 bins searched
    int spec_bins = 0;            // bins kept per step (from 0)
    std::vector<float> power;     // num_steps x spec_bins

    // Whole-buffer spectrum for baseband extraction
    int big_n = 0;
    std::vector<cfloat> spectrum;
    int base_n = 0;               // baseband length
    FftPlan base_plan;
    double base_rate = 0.0;       // baseband sample rate
    double base_sps = 0.0;        // baseband samples per symbol
    int base_sym = 0;             // samples integrated per symbol
    std::vector<cfloat> tone_base;   // tone t at (t - center_tone) * spacing, base_sym samples each
};

struct Candidate {
    float score;
    int bin;    // tone-0 spectrogram bin
    int step;   // spectrogram step of symbol 0
};

struct CandidateResult {
    bool ok = false;
    Message msg;
    float freq_hz = 0.0f;
    double start_sample = 0.0;
};

bool finish_decode(const uint8_t decoded_91[LDPC_INPUT_BYTES], Protocol proto, Message& msg) {
    bool all_zero = true;
    for (int i = 0; i < LDPC_INPUT_BYTES; ++i) {
        if (decoded_91[i] != 0) { all_zero = false; break; }
    }
    if (all_zero || !verify_crc14(decoded_91)) return false;
    uint8_t payload[PAYLOAD_BYTES];
    extract_payload(decoded_91, payload);
    if (proto == Protocol::LQ4 || proto == Protocol::LQ2) {
        for (size_t i = 0; i < PAYLOAD_BYTES; ++i) payload[i] ^= FT4_XOR_SEQUENCE[i];
    }
    if (!decode_message(payload, msg)) return false;
    msg.raw_payload.assign(payload, payload + PAYLOAD_BYTES);
    return true;
}

void build_spectrogram(DecodeContext& ctx, const std::vector<float>& audio) {
    const int L = ctx.sym_len;
    ctx.spec_fft = next_fast_size(L * ctx.freq_osr);
    FftPlan plan = make_fft_plan(ctx.spec_fft);
    ctx.hop = std::max(1, L / ctx.time_osr);
    long long avail = static_cast<long long>(audio.size()) - L;
    ctx.num_steps = avail < 0 ? 0 : static_cast<int>(avail / ctx.hop) + 1;
    ctx.power.assign(static_cast<size_t>(ctx.num_steps) * ctx.spec_bins, 0.0f);
    std::vector<cfloat> in(static_cast<size_t>(ctx.spec_fft)), out(static_cast<size_t>(ctx.spec_fft));
    // Two real frames per complex FFT (one in the real part, one in the imaginary part).
    const int n = ctx.spec_fft;
    for (int t = 0; t < ctx.num_steps; t += 2) {
        const bool pair = t + 1 < ctx.num_steps;
        const size_t a = static_cast<size_t>(t) * ctx.hop;
        const size_t b = a + static_cast<size_t>(ctx.hop);
        for (int i = 0; i < L; ++i) in[i] = cfloat(audio[a + i], pair ? audio[b + i] : 0.0f);
        for (int i = L; i < n; ++i) in[i] = cfloat(0.0f, 0.0f);
        fft_forward(plan, in.data(), out.data());
        float* row_a = &ctx.power[static_cast<size_t>(t) * ctx.spec_bins];
        float* row_b = pair ? &ctx.power[static_cast<size_t>(t + 1) * ctx.spec_bins] : nullptr;
        for (int k = 0; k < ctx.spec_bins; ++k) {
            const cfloat zk = out[k];
            const cfloat zc = std::conj(out[(n - k) % n]);
            row_a[k] = 0.25f * std::norm(zk + zc);
            if (row_b) row_b[k] = 0.25f * std::norm(zk - zc);
        }
    }
}

float tone_bin_offset(const DecodeContext& ctx, int tone) {
    return static_cast<float>(tone) * ctx.params.tone_spacing * static_cast<float>(ctx.spec_fft) / ctx.sample_rate;
}

// Soft Costas score: power on the sync tones against the mean power of the other tones of the same symbols.
float sync_score(const DecodeContext& ctx, int bin, int step) {
    const int nt = ctx.layout.num_tones;
    int off[16];
    for (int t = 0; t < nt; ++t) off[t] = static_cast<int>(std::lround(tone_bin_offset(ctx, t)));
    if (bin < 0 || bin + off[nt - 1] >= ctx.spec_bins) return 0.0f;
    double sig = 0.0, all = 0.0;
    for (const auto& st : ctx.layout.sync) {
        int s = step + st.first * ctx.time_osr;
        if (s < 0 || s >= ctx.num_steps) continue;
        const float* row = &ctx.power[static_cast<size_t>(s) * ctx.spec_bins + bin];
        for (int t = 0; t < nt; ++t) all += row[off[t]];
        sig += row[off[st.second]];
    }
    const double rest = (all - sig) / static_cast<double>(nt - 1);
    return rest > 0.0 ? static_cast<float>(sig / rest) : 0.0f;
}

std::vector<Candidate> find_candidates(const DecodeContext& ctx, int max_candidates, float min_score,
                                       const std::vector<Candidate>& exclude) {
    std::vector<Candidate> out;
    const int frame_steps = (ctx.layout.total_symbols - 1) * ctx.time_osr + 1;
    // Starts from a little before the buffer (partially received frames) to where the frame still fits.
    const int step_lo = -ctx.time_osr * 2;
    const int step_hi = std::max(0, ctx.num_steps - frame_steps + ctx.time_osr * 2);
    const int min_sep = 2 * ctx.time_osr;

    // Per frequency bin, the strongest few start times (each at least two symbols from the others).
    const int nbins = ctx.bin_hi - ctx.bin_lo + 1;
    std::vector<std::vector<Candidate>> peaks(static_cast<size_t>(nbins));
    std::vector<float> best_scores;
    std::vector<float> scores(static_cast<size_t>(step_hi - step_lo + 1));
    std::vector<char> used(scores.size());
    for (int b = ctx.bin_lo; b <= ctx.bin_hi; ++b) {
        for (int s = step_lo; s <= step_hi; ++s) scores[s - step_lo] = sync_score(ctx, b, s);
        std::fill(used.begin(), used.end(), 0);
        auto& pk = peaks[static_cast<size_t>(b - ctx.bin_lo)];
        for (int n = 0; n < PEAKS_PER_BIN; ++n) {
            int best = -1;
            for (size_t i = 0; i < scores.size(); ++i) {
                if (!used[i] && scores[i] > 0.0f && (best < 0 || scores[i] > scores[static_cast<size_t>(best)])) best = static_cast<int>(i);
            }
            if (best < 0) break;
            pk.push_back({scores[static_cast<size_t>(best)], b, best + step_lo});
            for (int i = std::max(0, best - min_sep + 1); i < std::min(static_cast<int>(scores.size()), best + min_sep); ++i) used[i] = 1;
        }
        best_scores.push_back(pk.empty() ? 0.0f : pk[0].score);
    }
    if (best_scores.empty()) return out;

    // Normalise by a low percentile of the per-bin best scores (the noise floor of the score).
    std::vector<float> sorted = best_scores;
    size_t k = sorted.size() * 2 / 5;
    std::nth_element(sorted.begin(), sorted.begin() + static_cast<long>(k), sorted.end());
    float floor_score = std::max(sorted[k], 1e-6f);

    // Keep local maxima across neighbouring bins with a similar start time.
    std::vector<Candidate> kept;
    for (int bi = 0; bi < nbins; ++bi) {
        for (const auto& c : peaks[static_cast<size_t>(bi)]) {
            const float sc = c.score / floor_score;
            if (sc < min_score) continue;
            bool dominated = false;
            for (int nb = std::max(0, bi - 1); nb <= std::min(nbins - 1, bi + 1) && !dominated; ++nb) {
                if (nb == bi) continue;
                for (const auto& o : peaks[static_cast<size_t>(nb)]) {
                    if (std::abs(o.step - c.step) <= ctx.time_osr && (o.score > c.score || (o.score == c.score && o.bin < c.bin))) {
                        dominated = true;
                        break;
                    }
                }
            }
            if (dominated) continue;
            bool excluded = false;
            for (const auto& e : exclude) {
                if (std::abs(e.bin - c.bin) <= 1 && std::abs(e.step - c.step) <= ctx.time_osr) { excluded = true; break; }
            }
            if (!excluded) kept.push_back({sc, c.bin, c.step});
        }
    }
    std::stable_sort(kept.begin(), kept.end(), [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
    if (static_cast<int>(kept.size()) > max_candidates) kept.resize(static_cast<size_t>(max_candidates));
    return kept;
}

void build_spectrum(DecodeContext& ctx, const std::vector<float>& audio) {
    // Baseband: about 32 samples per symbol. The whole-buffer FFT length is chosen so that the baseband
    // length is FFT friendly too.
    // With a whole number of audio samples per baseband sample, symbols start on exact baseband samples.
    const int sps_target = 32;
    const double decim = static_cast<double>(ctx.sym_len) / sps_target;
    ctx.base_n = next_fast_size(static_cast<int>(std::ceil(static_cast<double>(audio.size()) / decim)) + 1);
    if (ctx.sym_len % sps_target == 0) {
        ctx.big_n = ctx.base_n * (ctx.sym_len / sps_target);
    } else {
        ctx.big_n = next_fast_size(static_cast<int>(std::ceil(ctx.base_n * decim)));
    }
    ctx.base_rate = static_cast<double>(ctx.base_n) * ctx.sample_rate / static_cast<double>(ctx.big_n);
    ctx.base_sps = ctx.base_rate * ctx.params.symbol_period;
    ctx.base_sym = static_cast<int>(std::lround(ctx.base_sps));
    ctx.base_plan = make_fft_plan(ctx.base_n);
    ctx.base_sym = std::min(ctx.base_sym, 128);
    ctx.tone_base.resize(static_cast<size_t>(ctx.layout.num_tones) * ctx.base_sym);
    for (int t = 0; t < ctx.layout.num_tones; ++t) {
        const double w = -TWO_PI * (t - ctx.layout.center_tone) * static_cast<double>(ctx.params.tone_spacing) / ctx.base_rate;
        for (int i = 0; i < ctx.base_sym; ++i) ctx.tone_base[static_cast<size_t>(t) * ctx.base_sym + i] = unit(w * i);
    }

    FftPlan plan = make_fft_plan(ctx.big_n);
    std::vector<cfloat> in(static_cast<size_t>(ctx.big_n), cfloat(0.0f, 0.0f));
    for (size_t i = 0; i < audio.size(); ++i) in[i] = cfloat(audio[i], 0.0f);
    ctx.spectrum.resize(static_cast<size_t>(ctx.big_n));
    fft_forward(plan, in.data(), ctx.spectrum.data());
}

// Complex baseband around tone `center_tone` of a signal whose tone 0 is at `f0` Hz.
// Returns the frequency (Hz) that sits at 0 Hz in the baseband.
double extract_baseband(const DecodeContext& ctx, double f0, std::vector<cfloat>& z) {
    const double bin_hz = ctx.sample_rate / static_cast<double>(ctx.big_n);
    const double spacing = ctx.params.tone_spacing;
    const double fc = f0 + ctx.layout.center_tone * spacing;
    const long ic = std::lround(fc / bin_hz);
    // Pass band: the tones plus one spacing on each side, with a raised-cosine edge of one spacing.
    const double half_pass = (ctx.layout.num_tones / 2.0 + 1.0) * spacing;
    const double edge = spacing;
    const long half = static_cast<long>(std::ceil((half_pass + edge) / bin_hz));
    std::vector<cfloat> buf(static_cast<size_t>(ctx.base_n), cfloat(0.0f, 0.0f));
    const float scale = 1.0f / static_cast<float>(ctx.big_n);
    for (long d = -half; d <= half; ++d) {
        long idx = ic + d;
        if (idx <= 0 || idx >= ctx.big_n / 2) continue;
        double w = 1.0;
        const double middle = ((ctx.layout.num_tones - 1) / 2.0 - ctx.layout.center_tone) * spacing;
        const double from_center = std::fabs(static_cast<double>(d) * bin_hz - middle);
        if (from_center > half_pass) {
            double x = (from_center - half_pass) / edge;
            w = x >= 1.0 ? 0.0 : 0.5 * (1.0 + std::cos(PI_F * x));
        }
        if (w <= 0.0) continue;
        long slot = ((d % ctx.base_n) + ctx.base_n) % ctx.base_n;
        buf[static_cast<size_t>(slot)] = ctx.spectrum[static_cast<size_t>(idx)] * (scale * static_cast<float>(w));
    }
    fft_inverse(ctx.base_plan, buf, z);
    return static_cast<double>(ic) * bin_hz;
}

// Tone reference vectors for one baseband symbol: tone t at (t - center_tone) * spacing + df.
void make_tone_refs(const DecodeContext& ctx, double df, std::vector<cfloat>& refs) {
    const int n = ctx.base_sym;
    const int nt = ctx.layout.num_tones;
    refs.resize(static_cast<size_t>(nt) * n);
    cfloat rot[128];
    const double w = -TWO_PI * df / ctx.base_rate;
    for (int i = 0; i < n && i < 128; ++i) rot[i] = unit(w * i);
    for (int t = 0; t < nt; ++t) {
        const cfloat* base = &ctx.tone_base[static_cast<size_t>(t) * n];
        for (int i = 0; i < n; ++i) refs[static_cast<size_t>(t) * n + i] = cmul(base[i], rot[i]);
    }
}

inline cfloat symbol_tone(const DecodeContext& ctx, const std::vector<cfloat>& z, long start, const cfloat* ref) {
    const int n = ctx.base_sym;
    float re = 0.0f, im = 0.0f;
    long lo = std::max(0L, -start);
    long hi = std::min(static_cast<long>(n), static_cast<long>(z.size()) - start);
    const cfloat* zp = z.data() + start;
    for (long i = lo; i < hi; ++i) {
        re += zp[i].real() * ref[i].real() - zp[i].imag() * ref[i].imag();
        im += zp[i].real() * ref[i].imag() + zp[i].imag() * ref[i].real();
    }
    return cfloat(re, im);
}

inline long symbol_start(const DecodeContext& ctx, long tau, int sym) {
    return tau + std::lround(static_cast<double>(sym) * ctx.base_sps);
}

double sync_power(const DecodeContext& ctx, const std::vector<cfloat>& z, long tau, const std::vector<cfloat>& refs) {
    double p = 0.0;
    for (const auto& st : ctx.layout.sync) {
        cfloat c = symbol_tone(ctx, z, symbol_start(ctx, tau, st.first), &refs[static_cast<size_t>(st.second) * ctx.base_sym]);
        p += std::norm(c);
    }
    return p;
}

// log I0(x), the modified Bessel function of order 0 (noncoherent detection of a tone of unknown phase).
inline double log_bessel_i0(double x) {
    if (x < 3.75) {
        double t = (x / 3.75) * (x / 3.75);
        return std::log(1.0 + t * (3.5156229 + t * (3.0899424 + t * (1.2067492 + t * (0.2659732 + t * (0.0360768 + t * 0.0045813))))));
    }
    double t = 3.75 / x;
    double p = 0.39894228 + t * (0.01328592 + t * (0.00225319 + t * (-0.00157565 + t * (0.00916281 + t * (-0.02057706 +
               t * (0.02635537 + t * (-0.01647633 + t * 0.00392377)))))));
    return x - 0.5 * std::log(x) + std::log(p);
}

// Log-likelihood ratios of `group` consecutive data symbols taken together, for a tone of amplitude `amp` in complex
// noise of power `noise` per tone correlation: each tone combination has likelihood I0(2 amp |sum| / noise), summed
// over the combinations with the bit at 0 and at 1.
void group_llrs(const FrameLayout& L, const std::vector<cfloat>& c, int group, double amp, double noise, float* out) {
    const int nt = L.num_tones;
    const int nb = L.bits_per_symbol;
    const int nd = static_cast<int>(L.data_symbols.size());
    const double k = 2.0 * amp / noise;
    int bit = 0;
    int d = 0;
    std::vector<double> lv;
    while (d < nd) {
        int g = 1;
        while (g < group && d + g < nd && L.data_symbols[d + g] == L.data_symbols[d + g - 1] + 1) ++g;
        const int gbits = g * nb;
        int combos = 1;
        for (int j = 0; j < g; ++j) combos *= nt;
        lv.resize(static_cast<size_t>(combos));
        double vmax = -1e300;
        for (int idx = 0; idx < combos; ++idx) {
            int rem = idx;
            cfloat sum(0.0f, 0.0f);
            for (int j = g - 1; j >= 0; --j) {
                sum += c[static_cast<size_t>(d + j) * nt + rem % nt];
                rem /= nt;
            }
            lv[idx] = log_bessel_i0(k * std::abs(sum));
            vmax = std::max(vmax, lv[idx]);
        }
        double p0[12], p1[12];
        for (int b = 0; b < gbits; ++b) { p0[b] = 0.0; p1[b] = 0.0; }
        for (int idx = 0; idx < combos; ++idx) {
            double w = std::exp(lv[idx] - vmax);
            int rem = idx;
            for (int j = g - 1; j >= 0; --j) {
                int val = L.gray_inv[rem % nt];
                rem /= nt;
                for (int b = 0; b < nb; ++b) {
                    if ((val >> (nb - 1 - b)) & 1) p1[j * nb + b] += w; else p0[j * nb + b] += w;
                }
            }
        }
        for (int b = 0; b < gbits && bit < LDPC_CODEWORD_BITS; ++b) {
            out[bit++] = static_cast<float>(std::log(std::max(p0[b], 1e-300)) - std::log(std::max(p1[b], 1e-300)));
        }
        d += g;
    }
}

// Coherent log-likelihood ratios: with the carrier phase known (taken out of `c`), a tone of amplitude `amp` in
// complex noise of power `noise` has likelihood exp(2 amp Re(c) / noise).
void coherent_llrs(const FrameLayout& L, const std::vector<cfloat>& c, double amp, double noise, float* out) {
    const int nt = L.num_tones;
    const int nb = L.bits_per_symbol;
    const double k = 2.0 * amp / noise;
    int bit = 0;
    for (size_t d = 0; d < L.data_symbols.size(); ++d) {
        double v[16];
        double vmax = -1e300;
        for (int t = 0; t < nt; ++t) {
            v[t] = k * c[d * nt + t].real();
            vmax = std::max(vmax, v[t]);
        }
        double p0[4] = {0.0, 0.0, 0.0, 0.0}, p1[4] = {0.0, 0.0, 0.0, 0.0};
        for (int t = 0; t < nt; ++t) {
            double w = std::exp(v[t] - vmax);
            int val = L.gray_inv[t];
            for (int b = 0; b < nb; ++b) {
                if ((val >> (nb - 1 - b)) & 1) p1[b] += w; else p0[b] += w;
            }
        }
        for (int b = 0; b < nb && bit < LDPC_CODEWORD_BITS; ++b) {
            out[bit++] = static_cast<float>(std::log(std::max(p0[b], 1e-300)) - std::log(std::max(p1[b], 1e-300)));
        }
    }
}

// Applies the LLR gain, then keeps the typical (median) magnitude at most LLR_MEDIAN_CAP before clipping. At high SNR
// the model LLRs are all huge; without this, clipping would give a faded or missing stretch of the frame the same
// confidence as the rest.
void condition_llrs(float* llr) {
    float mags[LDPC_CODEWORD_BITS];
    for (int i = 0; i < LDPC_CODEWORD_BITS; ++i) mags[i] = std::fabs(llr[i]) * LLR_GAIN;
    std::nth_element(mags, mags + LDPC_CODEWORD_BITS / 2, mags + LDPC_CODEWORD_BITS);
    const float median = mags[LDPC_CODEWORD_BITS / 2];
    const float scale = LLR_GAIN * (median > LLR_MEDIAN_CAP ? LLR_MEDIAN_CAP / median : 1.0f);
    for (int i = 0; i < LDPC_CODEWORD_BITS; ++i) llr[i] = std::clamp(llr[i] * scale, -LLR_CLAMP, LLR_CLAMP);
}

// Fine time / frequency search on the sync symbols, then over the whole frame. Returns the baseband start sample;
// `best_k` receives the frequency step (in units of `fstep` from `df0`).
long fine_sync(const DecodeContext& ctx, const std::vector<cfloat>& z, double df0, double fstep, long tau0, int& best_k) {
    const FrameLayout& L = ctx.layout;
    std::vector<cfloat> refs;
    make_tone_refs(ctx, df0, refs);
    const long tspan = std::max(2L, std::lround(ctx.base_sps / ctx.time_osr)) + 2;
    long best_tau = tau0;
    double best_p = -1.0;
    for (long tau = tau0 - tspan; tau <= tau0 + tspan; ++tau) {
        double p = sync_power(ctx, z, tau, refs);
        if (p > best_p) { best_p = p; best_tau = tau; }
    }
    best_k = 0;
    for (int k = -FREQ_STEPS; k <= FREQ_STEPS; ++k) {
        make_tone_refs(ctx, df0 + k * fstep, refs);
        double p = sync_power(ctx, z, best_tau, refs);
        if (p > best_p) { best_p = p; best_k = k; }
    }
    for (int round = 0; round < 2; ++round) {
        const long ct = best_tau;
        const int ck = best_k;
        for (int k = ck - 1; k <= ck + 1; ++k) {
            make_tone_refs(ctx, df0 + k * fstep, refs);
            for (long tau = ct - 2; tau <= ct + 2; ++tau) {
                double p = sync_power(ctx, z, tau, refs);
                if (p > best_p) { best_p = p; best_tau = tau; best_k = k; }
            }
        }
        if (best_tau == ct && best_k == ck) break;
    }

    // Whole frame: the known tone of every sync symbol plus the strongest tone of every data symbol, four times as
    // many symbols as the sync blocks alone.
    auto frame_energy = [&](long tau, const std::vector<cfloat>& r) {
        double e = 0.0;
        for (const auto& st : L.sync) {
            e += std::norm(symbol_tone(ctx, z, symbol_start(ctx, tau, st.first), &r[static_cast<size_t>(st.second) * ctx.base_sym]));
        }
        for (int sym : L.data_symbols) {
            const long st = symbol_start(ctx, tau, sym);
            double m = 0.0;
            for (int t = 0; t < L.num_tones; ++t) {
                m = std::max(m, static_cast<double>(std::norm(symbol_tone(ctx, z, st, &r[static_cast<size_t>(t) * ctx.base_sym]))));
            }
            e += m;
        }
        return e;
    };
    make_tone_refs(ctx, df0 + best_k * fstep, refs);
    double best_e = frame_energy(best_tau, refs);
    for (int round = 0; round < 3; ++round) {
        const long ct = best_tau;
        const int ck = best_k;
        for (int k = ck - 1; k <= ck + 1; ++k) {
            make_tone_refs(ctx, df0 + k * fstep, refs);
            for (long tau = ct - 1; tau <= ct + 1; ++tau) {
                if (k == ck && tau == ct) continue;
                double e = frame_energy(tau, refs);
                if (e > best_e) { best_e = e; best_tau = tau; best_k = k; }
            }
        }
        if (best_tau == ct && best_k == ck) break;
    }
    return best_tau;
}

CandidateResult decode_candidate(const DecodeContext& ctx, const Candidate& cand) {
    CandidateResult res;
    const FrameLayout& L = ctx.layout;
    const int nt = L.num_tones;
    const int nsym = L.total_symbols;
    const int nsync = static_cast<int>(L.sync.size());
    const double spacing = ctx.params.tone_spacing;
    const double f0 = cand.bin * static_cast<double>(ctx.sample_rate) / ctx.spec_fft;
    std::vector<cfloat> z;
    const double f_base = extract_baseband(ctx, f0, z);
    // Offset of the candidate's center tone from the baseband's 0 Hz.
    const double df0 = (f0 + L.center_tone * spacing) - f_base;
    const double audio_per_base = static_cast<double>(ctx.big_n) / static_cast<double>(ctx.base_n);
    const long tau0 = std::lround(static_cast<double>(cand.step) * ctx.hop / audio_per_base);

    const double fstep = spacing / (2.0 * FREQ_STEPS);
    int best_k = 0;
    long best_tau = fine_sync(ctx, z, df0, fstep, tau0, best_k);
    const double best_df = df0 + best_k * fstep;
    std::vector<cfloat> refs;
    make_tone_refs(ctx, best_df, refs);

    // Complex tone correlations of every symbol, with the carrier's turn per symbol (2 pi df T) taken out: tones an
    // integer number of cycles per symbol apart keep a continuous-phase signal on one phase from symbol to symbol.
    std::vector<cfloat> cs(static_cast<size_t>(nsym) * nt);
    const double turn = TWO_PI * best_df * ctx.params.symbol_period;
    auto correlate_all = [&]() {
        for (int k = 0; k < nsym; ++k) {
            const long st = symbol_start(ctx, best_tau, k);
            const cfloat rot = unit(-turn * k);
            for (int t = 0; t < nt; ++t) {
                cs[static_cast<size_t>(k) * nt + t] = cmul(symbol_tone(ctx, z, st, &refs[static_cast<size_t>(t) * ctx.base_sym]), rot);
            }
        }
    };
    correlate_all();
    auto sync_value = [&](int i) { return cs[static_cast<size_t>(L.sync[i].first) * nt + L.sync[i].second]; };
    // Phase of tone t after a timing error of e baseband samples, relative to the center tone.
    auto timing_phase = [&](int tone, double e) { return -TWO_PI * (tone - L.center_tone) * e / ctx.base_sps; };

    // Phase calibration on the sync blocks, each block coherent and the blocks added in power: the residual turn per
    // symbol (w) and the timing error (e). An error of half a sample or more moves the symbol windows and the search
    // runs again.
    std::vector<int> block_of(static_cast<size_t>(nsync));
    for (int i = 0, blk = 0; i < nsync; ++i) {
        if (i > 0 && L.sync[i].first != L.sync[i - 1].first + 1) ++blk;
        block_of[static_cast<size_t>(i)] = blk;
    }
    auto make_rotations = [&](const std::vector<double>& ws, std::vector<cfloat>& table) {
        table.resize(ws.size() * static_cast<size_t>(nsync));
        for (size_t j = 0; j < ws.size(); ++j) {
            for (int i = 0; i < nsync; ++i) table[j * nsync + i] = unit(-ws[j] * L.sync[i].first);
        }
    };
    std::vector<double> coarse_w, fine_w_list;
    for (int iw = -BLOCK_W_STEPS; iw <= BLOCK_W_STEPS; ++iw) coarse_w.push_back(iw * BLOCK_W_STEP);
    std::vector<cfloat> coarse_rot, fine_rot, sv(static_cast<size_t>(nsync));
    make_rotations(coarse_w, coarse_rot);
    double best_e = 0.0, best_w = 0.0;
    for (int pass = 0; pass < 3; ++pass) {
        double best_val = -1.0;
        auto search = [&](double e_lo, double e_hi, double e_step, const std::vector<double>& ws, const std::vector<cfloat>& rot) {
            for (double e = e_lo; e <= e_hi + 1e-9; e += e_step) {
                for (int i = 0; i < nsync; ++i) sv[i] = cmul(sync_value(i), unit(timing_phase(L.sync[i].second, e)));
                for (size_t j = 0; j < ws.size(); ++j) {
                    const cfloat* r = &rot[j * nsync];
                    double tot = 0.0;
                    cfloat acc(0.0f, 0.0f);
                    int blk = 0;
                    for (int i = 0; i < nsync; ++i) {
                        if (block_of[i] != blk) { tot += std::norm(acc); acc = cfloat(0.0f, 0.0f); blk = block_of[i]; }
                        acc += cmul(sv[i], r[i]);
                    }
                    tot += std::norm(acc);
                    if (tot > best_val) { best_val = tot; best_e = e; best_w = ws[j]; }
                }
            }
        };
        const double e_range = pass == 0 ? TIMING_RANGE : 0.6;
        search(-e_range, e_range, 0.1, coarse_w, coarse_rot);
        const double ce = best_e, cw = best_w;
        fine_w_list.clear();
        for (int k = -4; k <= 4; ++k) fine_w_list.push_back(cw + k * BLOCK_W_STEP / 5.0);
        make_rotations(fine_w_list, fine_rot);
        search(ce - 0.1, ce + 0.1, 0.02, fine_w_list, fine_rot);
        if (std::fabs(best_e) < 0.5 || pass == 2) break;
        // The windows start `best_e` samples late: move them back by the whole samples and correlate again.
        best_tau -= std::lround(best_e);
        correlate_all();
    }
    for (int k = 0; k < nsym; ++k) {
        for (int t = 0; t < nt; ++t) {
            cfloat& v = cs[static_cast<size_t>(k) * nt + t];
            v = cmul(v, unit(-best_w * k + timing_phase(t, best_e)));
        }
    }

    // All sync symbols coherent together. The blocks leave sidelobes in w (every 2 pi / block spacing) almost as high
    // as the true peak, so the strongest few are judged again with the data symbols: with the right turn and phase,
    // the strongest tone of each data symbol lines up with the carrier. Then the timing once more, and the carrier
    // phase of the frame.
    double fine_w = 0.0, fine_e = 0.0;
    cfloat carrier(1.0f, 0.0f);
    {
        auto coherent_sum = [&](double w, double e) {
            cfloat acc(0.0f, 0.0f);
            for (int i = 0; i < nsync; ++i) acc += cmul(sync_value(i), unit(-w * L.sync[i].first + timing_phase(L.sync[i].second, e)));
            return acc;
        };
        std::vector<double> sync_val(static_cast<size_t>(2 * GLOBAL_W_STEPS + 1));
        std::vector<cfloat> y(static_cast<size_t>(nsync)), step(static_cast<size_t>(nsync));
        for (int i = 0; i < nsync; ++i) {
            y[i] = cmul(sync_value(i), unit(GLOBAL_W_STEPS * GLOBAL_W_STEP * L.sync[i].first));
            step[i] = unit(-GLOBAL_W_STEP * L.sync[i].first);
        }
        for (int iw = -GLOBAL_W_STEPS; iw <= GLOBAL_W_STEPS; ++iw) {
            cfloat acc(0.0f, 0.0f);
            for (int i = 0; i < nsync; ++i) {
                acc += y[i];
                y[i] = cmul(y[i], step[i]);
            }
            sync_val[static_cast<size_t>(iw + GLOBAL_W_STEPS)] = std::norm(acc);
        }
        std::vector<std::pair<double, int>> maxima;
        for (int i = 0; i <= 2 * GLOBAL_W_STEPS; ++i) {
            const bool left = i == 0 || sync_val[static_cast<size_t>(i)] >= sync_val[static_cast<size_t>(i - 1)];
            const bool right = i == 2 * GLOBAL_W_STEPS || sync_val[static_cast<size_t>(i)] > sync_val[static_cast<size_t>(i + 1)];
            if (left && right) maxima.push_back({sync_val[static_cast<size_t>(i)], i - GLOBAL_W_STEPS});
        }
        std::sort(maxima.begin(), maxima.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        if (maxima.size() > static_cast<size_t>(W_CANDIDATES)) maxima.resize(static_cast<size_t>(W_CANDIDATES));
        double best_val = -1e300;
        for (const auto& mx : maxima) {
            const double w = mx.second * GLOBAL_W_STEP;
            const cfloat acc = coherent_sum(w, 0.0);
            const float am = std::abs(acc);
            if (am <= 0.0f) continue;
            const cfloat ph0 = std::conj(acc) / am;
            double data = 0.0;
            for (int sym : L.data_symbols) {
                const cfloat rot = cmul(ph0, unit(-w * sym));
                float m = -1e30f;
                for (int t = 0; t < nt; ++t) m = std::max(m, cmul(cs[static_cast<size_t>(sym) * nt + t], rot).real());
                data += m;
            }
            if (am + data > best_val) { best_val = am + data; fine_w = w; }
        }
        carrier = coherent_sum(fine_w, 0.0);
        best_val = std::norm(carrier);
        const double cw = fine_w;
        for (int ie = -10; ie <= 10; ++ie) {
            for (int iw = -4; iw <= 4; ++iw) {
                const double w = cw + iw * 0.0005, e = ie * 0.05;
                const cfloat acc = coherent_sum(w, e);
                if (std::norm(acc) > best_val) { best_val = std::norm(acc); fine_w = w; fine_e = e; carrier = acc; }
            }
        }
        const float cmag = std::abs(carrier);
        carrier = cmag > 0.0f ? std::conj(carrier) / cmag : cfloat(1.0f, 0.0f);
    }

    // Data symbols in codeword order: as they are (noncoherent readings) and with the carrier phase taken out.
    const size_t ndata = L.data_symbols.size();
    std::vector<cfloat> cd(ndata * nt), cc(ndata * nt);
    for (size_t d = 0; d < ndata; ++d) {
        const int sym = L.data_symbols[d];
        for (int t = 0; t < nt; ++t) {
            cd[d * nt + t] = cs[static_cast<size_t>(sym) * nt + t];
            cc[d * nt + t] = cmul(cd[d * nt + t], cmul(carrier, unit(-fine_w * sym + timing_phase(t, fine_e))));
        }
    }

    // Signal amplitude and noise per tone correlation, from the sync symbols (known tones).
    double sig_p = 0.0, noise_p = 0.0;
    for (const auto& st : L.sync) {
        for (int t = 0; t < nt; ++t) {
            const double p = std::norm(cs[static_cast<size_t>(st.first) * nt + t]);
            if (t == st.second) sig_p += p; else noise_p += p;
        }
    }
    noise_p /= static_cast<double>(nsync * (nt - 1));
    sig_p /= static_cast<double>(nsync);
    if (!(noise_p > 0.0)) return res;
    const double amp = std::sqrt(std::max(sig_p - noise_p, 0.05 * noise_p));

    // Two readings: coherent symbol by symbol, then noncoherent over groups of symbols (3 for 8 tones, 4 for 4 tones,
    // robust when the carrier phase wanders). Belief propagation on each; in deep mode, ordered statistics on the
    // coherent reading (ordered by the belief-propagation trajectory, then by the channel values), accepted only when
    // CRC-14 passes and the codeword is close to the soft values.
    const int group = (nt == 8) ? 3 : 4;
    const int bp_iters = ctx.deep ? 50 : 30;
    uint8_t decoded_91[LDPC_INPUT_BYTES];
    float coherent[LDPC_CODEWORD_BITS], trajectory[LDPC_CODEWORD_BITS], grouped[LDPC_CODEWORD_BITS];
    coherent_llrs(L, cc, amp, noise_p, coherent);
    condition_llrs(coherent);
    bool ok = ldpc_decode_sum_product(coherent, decoded_91, bp_iters, trajectory) >= 0 && finish_decode(decoded_91, ctx.proto, res.msg);
    if (!ok) {
        group_llrs(L, cd, group, amp * group, noise_p * group, grouped);
        condition_llrs(grouped);
        ok = ldpc_decode_sum_product(grouped, decoded_91, bp_iters) >= 0 && finish_decode(decoded_91, ctx.proto, res.msg);
    }
    // Ordered statistics needs soft values on (nearly) every bit: a frame that is partly silence (a candidate that
    // only overlaps a signal) leaves bits with no information, which the search would fit freely.
    bool informative = true;
    if (!ok && ctx.deep) {
        double mean_mag = 0.0;
        for (float v : coherent) mean_mag += std::fabs(v);
        mean_mag /= LDPC_CODEWORD_BITS;
        int silent = 0;
        for (float v : coherent) silent += std::fabs(v) < 0.02 * mean_mag ? 1 : 0;
        informative = mean_mag > 0.0 && silent <= OSD_MAX_SILENT_BITS;
    }
    if (!ok && ctx.deep && informative) {
        for (const float* in : {static_cast<const float*>(trajectory), static_cast<const float*>(coherent)}) {
            float distance = 1.0f;
            if (ldpc_decode_osd(in, decoded_91, OSD_ORDER, OSD_PAIR_SPAN, &distance) == 0 && distance <= OSD_MAX_DISTANCE &&
                finish_decode(decoded_91, ctx.proto, res.msg)) {
                ok = true;
                break;
            }
        }
    }
    if (!ok) return res;
    res.ok = true;
    res.freq_hz = static_cast<float>(f_base - L.center_tone * spacing + best_df);
    res.start_sample = static_cast<double>(best_tau) * audio_per_base;
    return res;
}

std::vector<CandidateResult> decode_candidates(const DecodeContext& ctx, const std::vector<Candidate>& cands, int threads) {
    std::vector<CandidateResult> results(cands.size());
    int workers = std::clamp(threads, 1, 64);
    workers = std::min<int>(workers, static_cast<int>(cands.size()));
    if (workers <= 1) {
        for (size_t i = 0; i < cands.size(); ++i) results[i] = decode_candidate(ctx, cands[i]);
        return results;
    }
    std::atomic<size_t> next{0};
    std::vector<std::thread> pool;
    pool.reserve(static_cast<size_t>(workers));
    for (int w = 0; w < workers; ++w) {
        pool.emplace_back([&]() {
            for (;;) {
                size_t i = next.fetch_add(1);
                if (i >= cands.size()) break;
                results[i] = decode_candidate(ctx, cands[i]);
            }
        });
    }
    for (auto& th : pool) th.join();
    return results;
}

// Lowers the spectrogram cells of a decoded signal so the next search sees what was underneath.
void mask_decoded(DecodeContext& ctx, const Message& msg, float freq_hz, double start_sample) {
    ToneSequence seq;
    if (!encode_tones(msg, ctx.proto, seq)) return;
    const double bin_per_hz = static_cast<double>(ctx.spec_fft) / ctx.sample_rate;
    const int step0 = static_cast<int>(std::lround(start_sample / ctx.hop));
    for (size_t sym = 0; sym < seq.tones.size(); ++sym) {
        int bin = static_cast<int>(std::lround((freq_hz + seq.tones[sym] * ctx.params.tone_spacing) * bin_per_hz));
        for (int s = 0; s < ctx.time_osr; ++s) {
            int step = step0 + static_cast<int>(sym) * ctx.time_osr + s;
            if (step < 0 || step >= ctx.num_steps) continue;
            float* row = &ctx.power[static_cast<size_t>(step) * ctx.spec_bins];
            for (int d = -1; d <= 1; ++d) {
                int b = bin + d;
                if (b < 0 || b >= ctx.spec_bins) continue;
                row[b] *= (d == 0) ? 0.1f : 0.25f;
            }
        }
    }
}

} // anonymous namespace

std::vector<Message> audio_to_messages(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, int num_threads, bool is_deep) {
    std::vector<Message> decoded_messages;
    if (sample_rate <= 0.0f) return decoded_messages;
    auto params = get_protocol_params(proto);

    size_t samples_per_sym = static_cast<size_t>(std::round(params.symbol_period * sample_rate));
    if (samples_per_sym == 0) return decoded_messages;
    size_t frame_samples = samples_per_sym * params.total_symbols;
    if (audio_samples.size() < frame_samples) return decoded_messages;

    double energy = 0.0;
    for (float s : audio_samples) energy += static_cast<double>(s) * s;
    if (!(energy > 1e-5)) return decoded_messages;   // silence (or NaN)

    DecodeContext ctx;
    ctx.proto = proto;
    ctx.params = params;
    ctx.layout = make_frame_layout(proto);
    ctx.sample_rate = sample_rate;
    ctx.deep = is_deep;
    ctx.sym_len = static_cast<int>(samples_per_sym);

    // Tone-0 search range, as before: 300 .. 2500 Hz, or +/- 60 Hz around the requested frequency.
    float freq_min = (base_freq_hz > 0.0f) ? std::max(200.0f, base_freq_hz - 60.0f) : 300.0f;
    float freq_max = (base_freq_hz > 0.0f) ? (base_freq_hz + 60.0f) : 2500.0f;
    ctx.spec_fft = next_fast_size(ctx.sym_len * ctx.freq_osr);
    const float bin_hz = sample_rate / static_cast<float>(ctx.spec_fft);
    const float top_hz = std::min(freq_max + params.occupied_bw + 2.0f * params.tone_spacing, sample_rate * 0.5f);
    ctx.spec_bins = std::min(ctx.spec_fft / 2, static_cast<int>(std::ceil(top_hz / bin_hz)) + 1);
    ctx.bin_lo = std::max(1, static_cast<int>(std::floor(freq_min / bin_hz)));
    ctx.bin_hi = std::min(static_cast<int>(std::ceil(freq_max / bin_hz)),
                          ctx.spec_bins - 1 - static_cast<int>(std::ceil(tone_bin_offset(ctx, ctx.layout.num_tones - 1))));
    if (ctx.bin_hi < ctx.bin_lo) return decoded_messages;

    build_spectrogram(ctx, audio_samples);
    if (ctx.num_steps <= 0) return decoded_messages;
    build_spectrum(ctx, audio_samples);

    const int max_candidates = is_deep ? DEEP_MAX_CANDIDATES : FAST_MAX_CANDIDATES;
    const float min_score = is_deep ? DEEP_MIN_SCORE : FAST_MIN_SCORE;

    std::vector<Candidate> tried;
    const int passes = is_deep ? 2 : 1;
    for (int pass = 0; pass < passes; ++pass) {
        std::vector<Candidate> cands = find_candidates(ctx, max_candidates, min_score, tried);
        if (cands.empty()) break;
        auto results = decode_candidates(ctx, cands, num_threads);
        tried.insert(tried.end(), cands.begin(), cands.end());
        int new_hits = 0;
        for (const auto& r : results) {
            if (!r.ok) continue;
            if (std::find(decoded_messages.begin(), decoded_messages.end(), r.msg) != decoded_messages.end()) continue;
            decoded_messages.push_back(r.msg);
            mask_decoded(ctx, r.msg, r.freq_hz, r.start_sample);
            ++new_hits;
        }
        if (new_hits == 0) break;
    }
    return decoded_messages;
}

#pragma pack(push, 1)
struct WavHeader {
    char riff_id[4] = {'R', 'I', 'F', 'F'};
    uint32_t riff_size = 0;
    char wave_id[4] = {'W', 'A', 'V', 'E'};
    char fmt_id[4]  = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t audio_format = 1; // PCM
    uint16_t num_channels = 1; // Mono
    uint32_t sample_rate = 12000;
    uint32_t byte_rate = 24000;
    uint16_t block_align = 2;
    uint16_t bits_per_sample = 16;
    char data_id[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size = 0;
};
#pragma pack(pop)

bool save_wav_file(const std::string& filename, const std::vector<float>& audio_samples, float sample_rate) {
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) return false;

    uint32_t num_samples = static_cast<uint32_t>(audio_samples.size());
    uint32_t data_size = num_samples * sizeof(int16_t);

    WavHeader hdr;
    hdr.sample_rate = static_cast<uint32_t>(sample_rate);
    hdr.byte_rate = hdr.sample_rate * 2;
    hdr.data_size = data_size;
    hdr.riff_size = data_size + sizeof(WavHeader) - 8;

    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

    for (float s : audio_samples) {
        float clamped = std::clamp(s, -1.0f, 1.0f);
        int16_t pcm = static_cast<int16_t>(clamped * 32767.0f);
        out.write(reinterpret_cast<const char*>(&pcm), sizeof(pcm));
    }

    return out.good();
}

bool load_wav_file(const std::string& filename, std::vector<float>& audio_samples, float& sample_rate) {
    audio_samples.clear();
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) return false;

    WavHeader hdr;
    in.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!in.good()) return false;

    if (std::memcmp(hdr.riff_id, "RIFF", 4) != 0 ||
        std::memcmp(hdr.wave_id, "WAVE", 4) != 0) {
        return false;
    }

    sample_rate = static_cast<float>(hdr.sample_rate);
    size_t num_samples = hdr.data_size / sizeof(int16_t);
    audio_samples.resize(num_samples);

    for (size_t i = 0; i < num_samples; ++i) {
        int16_t pcm = 0;
        in.read(reinterpret_cast<char*>(&pcm), sizeof(pcm));
        audio_samples[i] = static_cast<float>(pcm) / 32767.0f;
    }

    return in.good() || in.eof();
}

} // namespace lq
