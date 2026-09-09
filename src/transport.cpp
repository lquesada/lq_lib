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
#include <thread>

namespace lq {

namespace {

constexpr double TWO_PI = 6.28318530717958647692;
constexpr float PI_F = 3.14159265358979323846f;

// In-place Radix-2 Cooley-Tukey FFT for fast spectrogram pre-screening
void fft_radix2(std::vector<std::complex<float>>& a) {
    size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * PI_F / static_cast<float>(len);
        std::complex<float> wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t j = 0; j < len / 2; ++j) {
                std::complex<float> u = a[i + j];
                std::complex<float> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

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
        return match >= 10;
    } else if (seq.protocol == Protocol::LQ4 || seq.protocol == Protocol::LQ2) {
        if (seq.size() < 105) return false;
        int match = 0;
        for (size_t i = 0; i < 4; ++i) {
            if (seq[1 + i] == COSTAS_SYNC1_4[i]) match++;
            if (seq[34 + i] == COSTAS_SYNC2_4[i]) match++;
            if (seq[67 + i] == COSTAS_SYNC3_4[i]) match++;
            if (seq[100 + i] == COSTAS_SYNC4_4[i]) match++;
        }
        return match >= 8;
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

    // 2. Decode LDPC codeword
    uint8_t decoded_91[LDPC_INPUT_BYTES];
    if (ldpc_decode_hard_bits(codeword, decoded_91) < 0) {
        return false; // LDPC decode failure
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
                      : ((proto == Protocol::LQ8 || proto == Protocol::LQ16) ? 7 : 10);

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
        int symbols_checked = 0;

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
                }
                ++symbols_checked;
                int remaining = 21 - symbols_checked;
                if (sync_matches_std + remaining < eff_min_sync) {
                    return false;
                }
            }
        }
        if (sync_matches_std < eff_min_sync) {
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
        return true;
    } else {
        int sync_matches_std = 0;
        const int sync_starts[4] = {1, 34, 67, 100};
        const std::array<uint8_t, 4>* sync_patterns[4] = {&COSTAS_SYNC1_4, &COSTAS_SYNC2_4, &COSTAS_SYNC3_4, &COSTAS_SYNC4_4};
        int symbols_checked = 0;

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
                }
                ++symbols_checked;
                int remaining = 16 - symbols_checked;
                if (sync_matches_std + remaining < eff_min_sync) {
                    return false;
                }
            }
        }
        if (sync_matches_std < eff_min_sync) {
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

std::vector<Message> audio_to_messages(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, int num_threads, bool is_deep) {
    std::vector<Message> decoded_messages;
    auto params = get_protocol_params(proto);

    size_t samples_per_sym = static_cast<size_t>(std::round(params.symbol_period * sample_rate));
    if (samples_per_sym == 0) return decoded_messages;
    size_t frame_samples = samples_per_sym * params.total_symbols;

    if (audio_samples.size() < frame_samples) {
        return decoded_messages;
    }

    int max_ldpc_iters = is_deep ? 100 : 25;

    float freq_min = (base_freq_hz > 0.0f) ? std::max(200.0f, base_freq_hz - 60.0f) : 300.0f;
    float freq_max = (base_freq_hz > 0.0f) ? (base_freq_hz + 60.0f) : 2500.0f;

    float freq_step;
    size_t time_step;
    int sync_threshold;

    if (!is_deep) {
        if (proto == Protocol::LQ8 || proto == Protocol::LQ16) {
            freq_step = 6.25f;
        } else {
            freq_step = 10.42f;
        }
        time_step = samples_per_sym / 2;
        sync_threshold = 7;
    } else {
        if (proto == Protocol::LQ8 || proto == Protocol::LQ16) {
            freq_step = 3.125f;
        } else {
            freq_step = 5.21f;
        }
        time_step = samples_per_sym / 2;
        sync_threshold = 5;
    }
    if (time_step == 0) time_step = 1;

    size_t max_time_offset = audio_samples.size() - frame_samples;

    struct CandidateTrack {
        float f;
        std::vector<size_t> time_offsets;
        int max_score = 0;
    };

    struct DecodedHit {
        Message msg;
        float freq;
        size_t t_offset;
    };

    size_t n_fft = 2048;
    while (n_fft < samples_per_sym) n_fft <<= 1;

    size_t num_time_steps = (audio_samples.size() - samples_per_sym) / time_step + 1;
    size_t spec_stride = n_fft / 2 + 1;
    std::vector<float> spec(num_time_steps * spec_stride, 0.0f);

    float bin_factor = static_cast<float>(n_fft) / sample_rate;
    const int sync_starts_8[3] = {0, 36, 72};
    const int sync_starts_4[4] = {1, 34, 67, 100};
    const std::array<uint8_t, 4>* sync_patterns_4[4] = {&COSTAS_SYNC1_4, &COSTAS_SYNC2_4, &COSTAS_SYNC3_4, &COSTAS_SYNC4_4};

    float eff_freq_step = (proto == Protocol::LQ16 && freq_step > params.tone_spacing) ? params.tone_spacing : freq_step;
    size_t sym_mult = std::max<size_t>(1, samples_per_sym / time_step);

    auto scan_tracks = [&](const std::vector<float>& current_spec, int threshold) -> std::vector<CandidateTrack> {
        std::vector<CandidateTrack> result_tracks;

        auto test_freq = [&](float f) {
            int tone_bins[16];
            for (int tn = 0; tn < params.num_tones; ++tn) {
                tone_bins[tn] = std::clamp(static_cast<int>(std::round((f + static_cast<float>(tn) * params.tone_spacing) * bin_factor)), 0, static_cast<int>(n_fft / 2));
            }

            CandidateTrack track;
            track.f = f;

            for (size_t t_offset = 0; t_offset <= max_time_offset; t_offset += time_step) {
                int matches = 0;
                size_t t_step_base = t_offset / time_step;

                if (proto == Protocol::LQ8 || proto == Protocol::LQ16) {
                    for (int b = 0; b < 3; ++b) {
                        for (int k = 0; k < 7; ++k) {
                            size_t step_idx = t_step_base + static_cast<size_t>(sync_starts_8[b] + k) * sym_mult;
                            if (step_idx >= num_time_steps) continue;
                            uint8_t costas_tone = COSTAS_ARRAY_8[k];
                            const float* row = &current_spec[step_idx * spec_stride];
                            float max_e = -1.0f;
                            int best_tn = 0;
                            for (int tn = 0; tn < 8; ++tn) {
                                float e = row[tone_bins[tn]];
                                if (e > max_e) {
                                    max_e = e;
                                    best_tn = tn;
                                }
                            }
                            if (best_tn == costas_tone) {
                                ++matches;
                            }
                        }
                    }
                    if (matches >= threshold) {
                        track.time_offsets.push_back(t_offset);
                        track.max_score = std::max(track.max_score, matches);
                    }
                } else {
                    for (int b = 0; b < 4; ++b) {
                        for (int k = 0; k < 4; ++k) {
                            size_t step_idx = t_step_base + static_cast<size_t>(sync_starts_4[b] + k) * sym_mult;
                            if (step_idx >= num_time_steps) continue;
                            uint8_t costas_tone = (*sync_patterns_4[b])[k];
                            const float* row = &current_spec[step_idx * spec_stride];
                            float max_e = -1.0f;
                            int best_tn = 0;
                            for (int tn = 0; tn < 4; ++tn) {
                                float e = row[tone_bins[tn]];
                                if (e > max_e) {
                                    max_e = e;
                                    best_tn = tn;
                                }
                            }
                            if (best_tn == costas_tone) {
                                ++matches;
                            }
                        }
                    }
                    if (matches >= threshold) {
                        track.time_offsets.push_back(t_offset);
                        track.max_score = std::max(track.max_score, matches);
                    }
                }
            }

            if (!track.time_offsets.empty()) {
                result_tracks.push_back(std::move(track));
            }
        };

        if (base_freq_hz > 0.0f) {
            int k_max = static_cast<int>(std::ceil(60.0f / eff_freq_step));
            for (int k = -k_max; k <= k_max; ++k) {
                float f = base_freq_hz + static_cast<float>(k) * eff_freq_step;
                if (f >= freq_min && f <= freq_max) {
                    test_freq(f);
                }
            }
        } else {
            for (float f = freq_min; f <= freq_max; f += eff_freq_step) {
                test_freq(f);
            }
        }

        // Sort candidate tracks by max_score descending so strongest signals are decoded first
        std::sort(result_tracks.begin(), result_tracks.end(), [](const CandidateTrack& a, const CandidateTrack& b) {
            return a.max_score > b.max_score;
        });

        if (result_tracks.size() > 64) {
            result_tracks.resize(64);
        }

        return result_tracks;
    };

    // Defensive clamping on thread count (1 to 64)
    int effective_threads = std::clamp(num_threads, 1, 64);

    std::vector<CandidateTrack> tracks;

    if (base_freq_hz <= 0.0f || is_deep) {
        // Fast FFT-based Costas sync correlation pre-screener
        if (effective_threads > 1 && num_time_steps > 1) {
            size_t n_workers = std::min(static_cast<size_t>(effective_threads), num_time_steps);
            size_t chunk = (num_time_steps + n_workers - 1) / n_workers;
            std::vector<std::thread> stft_threads;
            stft_threads.reserve(n_workers);

            for (size_t w = 0; w < n_workers; ++w) {
                size_t t_start = w * chunk;
                size_t t_end = std::min(t_start + chunk, num_time_steps);

                stft_threads.emplace_back([&, t_start, t_end]() {
                    std::vector<std::complex<float>> local_fft(n_fft);
                    for (size_t t = t_start; t < t_end; ++t) {
                        size_t start = t * time_step;
                        for (size_t i = 0; i < samples_per_sym; ++i) {
                            local_fft[i] = std::complex<float>(audio_samples[start + i], 0.0f);
                        }
                        for (size_t i = samples_per_sym; i < n_fft; ++i) {
                            local_fft[i] = std::complex<float>(0.0f, 0.0f);
                        }
                        fft_radix2(local_fft);
                        float* row = &spec[t * spec_stride];
                        for (size_t b = 0; b <= n_fft / 2; ++b) {
                            float r = local_fft[b].real();
                            float im = local_fft[b].imag();
                            row[b] = r * r + im * im;
                        }
                    }
                });
            }
            for (auto& th : stft_threads) {
                if (th.joinable()) th.join();
            }
        } else {
            std::vector<std::complex<float>> fft_buf(n_fft);
            for (size_t t = 0; t < num_time_steps; ++t) {
                size_t start = t * time_step;
                for (size_t i = 0; i < samples_per_sym; ++i) {
                    fft_buf[i] = std::complex<float>(audio_samples[start + i], 0.0f);
                }
                for (size_t i = samples_per_sym; i < n_fft; ++i) {
                    fft_buf[i] = std::complex<float>(0.0f, 0.0f);
                }
                fft_radix2(fft_buf);
                float* row = &spec[t * spec_stride];
                for (size_t b = 0; b <= n_fft / 2; ++b) {
                    float r = fft_buf[b].real();
                    float im = fft_buf[b].imag();
                    row[b] = r * r + im * im;
                }
            }
        }

        tracks = scan_tracks(spec, sync_threshold);
    } else {
        int k_max = static_cast<int>(std::ceil(60.0f / eff_freq_step));
        for (int k = -k_max; k <= k_max; ++k) {
            float f = base_freq_hz + static_cast<float>(k) * eff_freq_step;
            if (f < freq_min || f > freq_max) continue;
            CandidateTrack track;
            track.f = f;
            for (size_t t_offset = 0; t_offset <= max_time_offset; t_offset += time_step) {
                track.time_offsets.push_back(t_offset);
            }
            if (!track.time_offsets.empty()) {
                tracks.push_back(std::move(track));
            }
        }
    }

    auto decode_tracks = [&](const std::vector<CandidateTrack>& candidate_tracks,
                             std::vector<DecodedHit>& hits) {
        if (candidate_tracks.empty()) return;

        // Single-thread fast path: executes directly on calling thread without thread spawning overhead
        if (effective_threads <= 1 || candidate_tracks.size() <= 1) {
            for (const auto& track : candidate_tracks) {
                for (size_t t_offset : track.time_offsets) {
                    bool near_existing_hit = false;
                    for (const auto& h : hits) {
                        if (std::abs(track.f - h.freq) < params.tone_spacing * 0.75f &&
                            std::abs(static_cast<long long>(t_offset) - static_cast<long long>(h.t_offset)) <= static_cast<long long>(time_step)) {
                            near_existing_hit = true;
                            break;
                        }
                    }
                    if (near_existing_hit) continue;

                    std::vector<float> llrs;
                    if (demodulate_audio_soft(audio_samples, t_offset, track.f, sample_rate, proto, llrs, sync_threshold)) {
                        Message msg;
                        if (decode_soft_llrs(llrs, msg, proto, max_ldpc_iters)) {
                            bool exists = false;
                            for (const auto& h : hits) {
                                if (h.msg == msg) { exists = true; break; }
                            }
                            if (!exists) {
                                hits.push_back({msg, track.f, t_offset});
                            }
                            break;
                        }
                    }
                }
            }
            return;
        }

        // Multi-threaded path: partition tracks across worker threads
        size_t num_workers = std::min(static_cast<size_t>(effective_threads), candidate_tracks.size());
        size_t chunk_size = (candidate_tracks.size() + num_workers - 1) / num_workers;

        std::vector<std::vector<DecodedHit>> worker_results(num_workers);
        std::vector<std::thread> workers;
        workers.reserve(num_workers);

        for (size_t w = 0; w < num_workers; ++w) {
            size_t start_track = w * chunk_size;
            size_t end_track = std::min(start_track + chunk_size, candidate_tracks.size());

            workers.emplace_back([&, w, start_track, end_track]() {
                for (size_t i = start_track; i < end_track; ++i) {
                    const auto& track = candidate_tracks[i];
                    for (size_t t_offset : track.time_offsets) {
                        bool near_existing_hit = false;
                        for (const auto& h : worker_results[w]) {
                            if (std::abs(track.f - h.freq) < params.tone_spacing * 0.75f &&
                                std::abs(static_cast<long long>(t_offset) - static_cast<long long>(h.t_offset)) <= static_cast<long long>(time_step)) {
                                near_existing_hit = true;
                                break;
                            }
                        }
                        if (near_existing_hit) continue;

                        std::vector<float> llrs;
                        if (demodulate_audio_soft(audio_samples, t_offset, track.f, sample_rate, proto, llrs, sync_threshold)) {
                            Message msg;
                            if (decode_soft_llrs(llrs, msg, proto, max_ldpc_iters)) {
                                bool exists = false;
                                for (const auto& h : worker_results[w]) {
                                    if (h.msg == msg) { exists = true; break; }
                                }
                                if (!exists) {
                                    worker_results[w].push_back({msg, track.f, t_offset});
                                }
                                break;
                            }
                        }
                    }
                }
            });
        }

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        // Merge worker results in deterministic track order
        for (size_t w = 0; w < num_workers; ++w) {
            for (const auto& hit : worker_results[w]) {
                bool exists = false;
                for (const auto& h : hits) {
                    if (h.msg == hit.msg) { exists = true; break; }
                }
                if (!exists) {
                    hits.push_back(hit);
                }
            }
        }
    };

    std::vector<DecodedHit> first_pass_hits;
    decode_tracks(tracks, first_pass_hits);

    for (const auto& hit : first_pass_hits) {
        if (std::find(decoded_messages.begin(), decoded_messages.end(), hit.msg) == decoded_messages.end()) {
            decoded_messages.push_back(hit.msg);
        }
    }

    // Multi-pass signal subtraction in deep mode
    if (is_deep && !first_pass_hits.empty() && !spec.empty()) {
        size_t steps_per_sym = std::max<size_t>(1, samples_per_sym / time_step);

        for (const auto& hit : first_pass_hits) {
            ToneSequence seq;
            if (!encode_tones(hit.msg, proto, seq)) {
                continue;
            }

            for (size_t sym = 0; sym < seq.tones.size(); ++sym) {
                uint8_t tone = seq.tones[sym];
                float tone_freq = hit.freq + static_cast<float>(tone) * params.tone_spacing;
                int tone_bin = std::clamp(static_cast<int>(std::round(tone_freq * bin_factor)), 0, static_cast<int>(n_fft / 2));
                size_t step_start = (hit.t_offset + sym * samples_per_sym) / time_step;

                for (size_t s = 0; s < steps_per_sym; ++s) {
                    size_t step_idx = step_start + s;
                    if (step_idx < num_time_steps) {
                        float* row = &spec[step_idx * spec_stride];
                        row[tone_bin] *= 0.1f;
                        if (tone_bin > 0) {
                            row[tone_bin - 1] *= 0.25f;
                        }
                        if (tone_bin + 1 <= static_cast<int>(n_fft / 2)) {
                            row[tone_bin + 1] *= 0.25f;
                        }
                    }
                }
            }
        }

        std::vector<CandidateTrack> secondary_tracks = scan_tracks(spec, 5);

        std::vector<DecodedHit> second_pass_hits;
        decode_tracks(secondary_tracks, second_pass_hits);

        for (const auto& hit : second_pass_hits) {
            if (std::find(decoded_messages.begin(), decoded_messages.end(), hit.msg) == decoded_messages.end()) {
                decoded_messages.push_back(hit.msg);
            }
        }
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
