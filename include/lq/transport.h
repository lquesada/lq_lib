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

#ifndef LQ_TRANSPORT_H
#define LQ_TRANSPORT_H

#include <cstdint>
#include <vector>
#include "lq/types.h"
#include "lq/constants.h"
#include "lq/message.h"

namespace lq {

/**
 * Encapsulates a sequence of channel tone symbols for transmission or demodulation.
 */
struct ToneSequence {
    Protocol protocol = Protocol::LQ8;
    std::vector<uint8_t> tones; ///< Tone indices: 0..7 (LQ8, LQ16) or 0..3 (LQ4, LQ2)
    float symbol_period = 0.160f;
    float tone_spacing = 6.25f;
    float tx_duration = 12.64f;

    size_t size() const { return tones.size(); }
    bool empty() const { return tones.empty(); }
    uint8_t operator[](size_t idx) const { return tones[idx]; }
    uint8_t& operator[](size_t idx) { return tones[idx]; }
};

/**
 * Direct Payload Transmission Pipeline (mirrors ft8_encode / ft4_encode):
 * 77-bit Payload (10 bytes) -> (+FT4 XOR) -> +CRC14 (82-bit zero-extended) -> LDPC(174,91) -> Channel Tones.
 * Supports Protocol::LQ8 (79 tones), Protocol::LQ4 (105 tones), Protocol::LQ2 (105 tones), and Protocol::LQ16 (79 tones).
 */
bool encode_payload(const uint8_t payload[PAYLOAD_BYTES], Protocol proto, ToneSequence& seq);

/**
 * Direct C-array style tone generation helper for seamless ft8_lib compatibility:
 * @param[in] payload - 10-byte array containing 77-bit packed message payload.
 * @param[out] tones - array receiving channel tones (79 bytes for LQ8/LQ16, 105 bytes for LQ4/LQ2).
 * @param[in] proto - protocol profile (LQ8, LQ4, LQ2, LQ16). Defaults to Protocol::LQ8.
 * Returns true on success.
 */
bool encode_payload(const uint8_t* payload, uint8_t* tones, Protocol proto = Protocol::LQ8);

/**
 * Full Transmission Pipeline:
 * Structured Message -> 77-bit Payload -> +CRC14 -> LDPC(174,91) -> Channel Tones
 * Supports Protocol::LQ8 (79 symbols), Protocol::LQ4 (105 symbols), Protocol::LQ2 (105 symbols), and Protocol::LQ16 (79 symbols).
 * Returns true on success.
 */
bool encode_tones(const Message& msg, Protocol proto, ToneSequence& seq);

/**
 * Direct Reception Pipeline from ToneSequence to 77-bit Payload:
 * Channel Tones -> Extract Data Symbols -> LDPC Decode -> CRC14 Check -> (+FT4 XOR) -> 77-bit Payload.
 * Returns true on success.
 */
bool decode_tones(const ToneSequence& seq, uint8_t payload[PAYLOAD_BYTES]);

/**
 * Direct C-array style tone demodulation helper for seamless ft8_lib compatibility:
 * @param[in] tones - array of channel tones (79 bytes for LQ8/LQ16, 105 bytes for LQ4/LQ2).
 * @param[out] payload - 10-byte array receiving 77-bit packed message payload.
 * @param[in] proto - protocol profile (LQ8, LQ4, LQ2, LQ16). Defaults to Protocol::LQ8.
 * Returns true on success.
 */
bool decode_payload(const uint8_t* tones, uint8_t* payload, Protocol proto = Protocol::LQ8);

/**
 * Full Reception Pipeline:
 * Channel Tones -> Extract Data Symbols -> LDPC Decode -> CRC14 Check -> 77-bit Payload -> Structured Message.
 * Returns true on success.
 */
bool decode_tones(const ToneSequence& seq, Message& msg);

/**
 * Lower-level helper: map 174-bit LDPC codeword to channel tones.
 */
bool codeword_to_tones(const uint8_t codeword[LDPC_CODEWORD_BYTES], Protocol proto, ToneSequence& seq);

/**
 * Lower-level helper: extract 174-bit LDPC codeword from channel tones (hard decision).
 */
bool tones_to_codeword(const ToneSequence& seq, uint8_t codeword[LDPC_CODEWORD_BYTES]);

/**
 * Synthesize Gaussian Frequency Shift Keying (GFSK) audio samples.

 * Uses Gaussian filter pulse shaping (BT = symbol_bt) and raised-cosine ramp at boundaries.
 * @param[in] symbols - array of tone symbols (0..num_tones-1).
 * @param[in] num_symbols - number of symbols (e.g. 79 for LQ8/LQ16, 105 for LQ4/LQ2).
 * @param[in] f0 - base audio carrier frequency in Hz.
 * @param[in] symbol_bt - Gaussian bandwidth-time product (2.0 for LQ8/LQ16, 1.0 for LQ4/LQ2).
 * @param[in] symbol_period - symbol duration in seconds (e.g. 0.160 for LQ8, 0.048 for LQ4, 0.024 for LQ2, 0.320 for LQ16).
 * @param[in] sample_rate - audio sample rate in Hz (e.g. 12000.0).
 * @param[out] signal - output float buffer receiving audio samples.
 */
void synth_gfsk(const uint8_t* symbols, int num_symbols, float f0, float symbol_bt, float symbol_period, float sample_rate, float* signal);

/**
 * Generate baseband / passband audio waveform from a ToneSequence.
 * Uses continuous-phase frequency shift keying (CPFSK / GFSK).
 * `base_freq_hz`: carrier base frequency (e.g. 1000.0 Hz or 1500.0 Hz).
 * `sample_rate`: audio sampling rate (e.g. 12000.0 Hz or 48000.0 Hz).
 * `audio_samples`: vector receiving normalised float audio samples in [-1.0, 1.0].
 */
void generate_audio(const ToneSequence& seq, float base_freq_hz, float sample_rate, std::vector<float>& audio_samples);

/**
 * Demodulate raw audio waveform into a ToneSequence using non-coherent tone matched filters.
 * `audio_samples`: input float audio samples.
 * `base_freq_hz`: expected base carrier frequency.
 * `sample_rate`: audio sampling rate in Hz.
 * `proto`: target protocol profile (LQ8, LQ4, LQ2, LQ16).
 * `seq`: output detected tone sequence.
 * Returns true if the audio buffer contains enough samples for a complete frame.
 */
bool demodulate_audio(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, ToneSequence& seq);

/**
 * Extract soft Log-Likelihood Ratios (LLRs) for 174 LDPC codeword bits from raw audio.
 * Computes non-coherent tone filter energies and applies Gray-coded soft symbol mapping.
 */
bool demodulate_audio_soft(const std::vector<float>& audio_samples, size_t offset, float base_freq_hz, float sample_rate, Protocol proto, std::vector<float>& llrs, int min_sync_matches = -1);

/**
 * Decode 174 soft LLRs directly via Normalized Min-Sum LDPC belief propagation.
 * Checks CRC-14 parity and unpacks 77-bit payload into Message.
 */
bool decode_soft_llrs(const std::vector<float>& llrs, Message& msg, Protocol proto = Protocol::LQ8, int max_ldpc_iters = 25);


/**
 * Compute Costas synchronization correlation score from a 2D STFT magnitude waterfall matrix.
 * @param[in] mag - 2D waterfall magnitude byte array (num_blocks x block_stride).
 * @param[in] num_blocks - total number of time blocks in waterfall.
 * @param[in] block_stride - number of frequency bins per time block.
 * @param[in] time_offset - candidate start time block index.
 * @param[in] freq_offset - candidate base frequency bin index.
 * @param[in] proto - protocol profile (LQ8, LQ4, LQ2, LQ16).
 * Returns integer sync correlation score (higher is stronger).
 */
int lq_sync_score(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, Protocol proto);

/**
 * Perform 3-point parabolic interpolation on adjacent waterfall frequency bins to refine carrier frequency to sub-bin precision.
 * @param[in] mag - 2D waterfall magnitude byte array.
 * @param[in] num_blocks - total number of time blocks.
 * @param[in] block_stride - frequency bins per time block.
 * @param[in] time_offset - candidate start time block index.
 * @param[in] freq_offset - candidate base frequency bin index.
 * @param[in] freq_sub - sub-bin resolution offset.
 * @param[in] freq_osr - frequency oversampling ratio.
 * @param[in] symbol_period - symbol duration in seconds.
 * @param[in] proto - protocol profile.
 * @param[out] out_score - refined sync score.
 * Returns refined base frequency in Hz.
 */
float lq_refine_frequency(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, int freq_sub, int freq_osr, float symbol_period, Protocol proto, int* out_score = nullptr);

/**
 * Estimate Signal-to-Noise Ratio (SNR) in dB relative to standard 2500 Hz reference bandwidth from waterfall power.
 * @param[in] mag2 - 2D waterfall squared magnitude / power array (num_blocks x block_stride).
 * @param[in] num_blocks - total number of time blocks.
 * @param[in] block_stride - frequency bins per time block.
 * @param[in] time_offset - candidate start time block index.
 * @param[in] freq_offset - candidate base frequency bin index.
 * @param[in] proto - protocol profile.
 * Returns estimated SNR in dB.
 */
float lq_guess_snr(const float* mag2, int num_blocks, int block_stride, int time_offset, int freq_offset, Protocol proto);

/**
 * Extract 174 LDPC soft Log-Likelihood Ratios (LLRs) directly from waterfall magnitude frames.
 * @param[in] mag - 2D waterfall magnitude byte array.
 * @param[in] num_blocks - total number of time blocks.
 * @param[in] block_stride - frequency bins per time block.
 * @param[in] time_offset - candidate start time block index.
 * @param[in] freq_offset - candidate base frequency bin index.
 * @param[in] proto - protocol profile.
 * @param[out] out_llrs_174 - array of 174 floats receiving LLR values (positive = bit 0, negative = bit 1).
 * Returns true on success.
 */
bool lq_extract_llrs_from_waterfall(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, Protocol proto, float out_llrs_174[LDPC_CODEWORD_BITS]);

/**
 * Subtract / attenuate detected signal from waterfall magnitude buffer for multi-pass decoding of weak overlapping signals.
 * @param[in,out] mag - 2D waterfall magnitude byte array.
 * @param[in] max_mag_size - total size of mag buffer in bytes.
 * @param[in] block_size - FFT block size in samples.
 * @param[in] block_stride - frequency bins per time block.
 * @param[in] payload - 10-byte array containing 77-bit message payload.
 * @param[in] freq_hz - detected base frequency in Hz.
 * @param[in] time_sec - detected start time in seconds.
 * @param[in] sample_rate - audio sampling rate in Hz.
 * @param[in] proto - protocol profile.
 */
void lq_subtract_signal_from_waterfall(uint8_t* mag, int max_mag_size, int block_size, int block_stride, const uint8_t* payload, float freq_hz, float time_sec, float sample_rate, Protocol proto);

/**
 * Complete End-to-End Audio Transmission Helper:
 * Message -> Payload -> CRC-14 -> LDPC(174,91) -> Tone Sequence -> Audio Waveform.
 */
bool message_to_audio(const Message& msg, Protocol proto, float base_freq_hz, float sample_rate, std::vector<float>& audio_samples);

/**
 * Complete End-to-End Audio Reception Helper:
 * Audio Waveform -> Multi-Candidate Time/Frequency Synchronization -> Soft LLR Demodulation -> LDPC Decoding -> CRC-14 Verification -> Message.
 * If `base_freq_hz <= 0.0f`, searches the entire audio passband (250 Hz - 2600 Hz).
 * If `base_freq_hz > 0.0f`, performs frequency-offset tolerant search around `base_freq_hz` (+/- 60 Hz).
 * `num_threads`: Number of worker threads for parallel candidate decoding (1 = sequential fast-path).
 */
bool audio_to_message(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, Message& msg, int num_threads = 1, bool is_deep = false);

/**
 * Multi-Signal Audio Waterfall Decoder:
 * Scans the entire audio buffer across time and frequency, decoding all active transmissions.
 * `num_threads`: Number of worker threads for parallel candidate decoding (1 = sequential fast-path).
 * `is_deep`: Fast vs Deep decoding strategy (true = half-bin refinement, relaxed sync threshold, 100 LDPC iterations, and multi-pass subtraction).
 */
std::vector<Message> audio_to_messages(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, int num_threads = 1, bool is_deep = false);

/**
 * Save a float audio buffer as a standard 16-bit PCM WAV file.
 */
bool save_wav_file(const std::string& filename, const std::vector<float>& audio_samples, float sample_rate);

/**
 * Load a 16-bit PCM WAV file into a normalised float audio buffer.
 */
bool load_wav_file(const std::string& filename, std::vector<float>& audio_samples, float& sample_rate);

} // namespace lq

#endif // LQ_TRANSPORT_H
