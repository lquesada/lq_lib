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

#ifndef LQ_C_API_H
#define LQ_C_API_H

#include <stdint.h>
#include <stddef.h>

#ifndef LQ_API
#ifdef _WIN32
#  if defined(LQ_BUILD_SHARED)
#    define LQ_API __declspec(dllexport)
#  elif defined(LQ_USE_SHARED)
#    define LQ_API __declspec(dllimport)
#  else
#    define LQ_API
#  endif
#else
#  define LQ_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Mode identifiers
typedef enum {
    LQ_MODE_LQ8        = 0,
    LQ_MODE_LQ4        = 1,
    LQ_MODE_LQ2        = 2,
    LQ_MODE_LQ16       = 3
} lq_mode_t;

// Message types — Canonical Types (1..16) and Compatibility Aliases
typedef enum {
    LQ_MSG_UNKNOWN          = 0,

    // Canonical 16 Message Types (1..16)
    LQ_MSG_CQ_STD           = 1,
    LQ_MSG_CQ_NONSTD_1      = 2,
    LQ_MSG_CQ_NONSTD_2      = 3,
    LQ_MSG_CQ_NONSTD_3      = 4,
    LQ_MSG_CALL_STD_NOSUF   = 5,
    LQ_MSG_CALL_STD_SUF     = 6,
    LQ_MSG_CALL_NONSTD      = 7,
    LQ_MSG_REPORT73_STD     = 8,
    LQ_MSG_M73_STD          = 9,
    LQ_MSG_M73_NONSTD       = 10,
    LQ_MSG_MULTI_REPORT73   = 11,
    LQ_MSG_MULTI_73         = 12,
    LQ_MSG_FREE_TEXT        = 13,
    LQ_MSG_RESERVED_A       = 14,
    LQ_MSG_RESERVED_B       = 15,
    LQ_MSG_RESERVED_C       = 16,

    // Legacy transitional aliases (retained for backward compatibility)
    LQ_MSG_CALL_STD         = 5,  // @deprecated Use LQ_MSG_CALL_STD_NOSUF
    LQ_MSG_CALL_STD_2SUF    = 6,  // @deprecated Use LQ_MSG_CALL_STD_SUF
    LQ_MSG_CALL_NONSTD_1    = 7,  // @deprecated Use LQ_MSG_CALL_NONSTD
    LQ_MSG_REPLY73_STD      = 8,  // @deprecated Use LQ_MSG_REPORT73_STD
    LQ_MSG_73_STD           = 9,  // @deprecated Use LQ_MSG_M73_STD
    LQ_MSG_73_NONSTD        = 10, // @deprecated Use LQ_MSG_M73_NONSTD
    LQ_MSG_MULTI_REPLY73    = 11, // @deprecated Use LQ_MSG_MULTI_REPORT73
    LQ_MSG_REPORT73_NONSTD  = 11, // @deprecated Use LQ_MSG_MULTI_REPORT73
    LQ_MSG_REPLY73_NONSTD   = 11, // @deprecated Use LQ_MSG_MULTI_REPORT73
    LQ_MSG_REPLY73_NONSTD_1 = 11, // @deprecated Use LQ_MSG_MULTI_REPORT73
    LQ_MSG_RESERVED         = 14, // @deprecated Use LQ_MSG_RESERVED_A
    LQ_MSG_RESERVED_4       = 14  // @deprecated Use LQ_MSG_RESERVED_A
} lq_msg_type_t;

// Multi-reply target structure
typedef struct {
    char call[16];
    uint32_t hash_24;
    int rst_db;
} lq_c_multi_target_t;

// C-compatible Message Structure
typedef struct {
    int type;                       // lq_msg_type_t
    char call_from[16];             // Sender / caller callsign
    char call_to[16];               // Target callsign (empty for CQ)
    uint8_t suffix_from;            // Suffix: 0 = None, 1 = /P
    uint8_t suffix_to;              // Suffix: 0 = None, 1 = /P
    char grid[8];                   // 4-character Maidenhead locator (e.g. "JN47")
    int rst_db;                     // Signal report in dB (-26..+5)
    char modifier[16];              // CQ modifier (e.g. "DX", "POTA")
    char text[80];                  // Free-text string or formatted message
    uint32_t hash_from_24;          // 24-bit hash of sender callsign (primary)
    uint32_t hash_to_24;            // 24-bit hash of target callsign (primary)
    uint32_t hash_from_20;          // 20-bit hash of sender callsign (H24 >> 4)
    uint32_t hash_to_20;            // 20-bit hash of target callsign (H24 >> 4)
    uint32_t hash_from_16;          // 16-bit hash of sender callsign (multi-reply)
    uint32_t hash_from_23;          // @deprecated Legacy 23-bit hash
    uint32_t hash_to_23;            // @deprecated Legacy 23-bit hash
    uint32_t hash_from_14;          // @deprecated Legacy 14-bit hash
    uint32_t hash_from_12;          // @deprecated Legacy 12-bit hash
    uint32_t hash_to_12;            // @deprecated Legacy 12-bit hash
    int num_multi_targets;          // Number of targets in multi-reply (0..2)
    lq_c_multi_target_t multi_targets[2]; // Multi-reply target array
    int is_valid;                   // 1 if valid, 0 if invalid
    int is_cq;                      // 1 if CQ
    int is_call;                    // 1 if CALL
    int is_reply73;                 // 1 if REPORT+73 (or legacy REPLY73)
    int is_report73;                // 1 if REPORT+73
    int is_73;                      // 1 if 73 STD
    int is_multi_reply73;           // 1 if MULTI_REPORT73 (or legacy MULTI_REPLY73)
    int is_multi_report73;          // 1 if MULTI_REPORT73
    int is_multi_73;                // 1 if MULTI_73
    int is_free_text;               // 1 if FREE_TEXT
    uint8_t payload[10];            // 77-bit packed message payload (10 bytes)
} lq_c_message_t;

// Mode metadata parameters
typedef struct {
    int mode_id;
    const char* name;
    int total_symbols;
    int num_tones;
    int bits_per_symbol;
    float symbol_period;
    float tone_spacing;
    float bandwidth_hz;
    float tx_duration_sec;
    float slot_duration_sec;
    float threshold_snr_db;
} lq_c_mode_params_t;

// Version information
const char* lq_c_version_string(void);
int lq_c_version_major(void);
int lq_c_version_minor(void);
int lq_c_version_patch(void);

// Mode parameters
int lq_c_get_mode_params(lq_mode_t mode, lq_c_mode_params_t* out_params);

// Hash functions
uint32_t lq_c_hash_callsign_24(const char* callsign);
uint32_t lq_c_hash_callsign_20(const char* callsign);
uint32_t lq_c_hash_callsign_23(const char* callsign);
uint32_t lq_c_hash_callsign_22(const char* callsign);
uint32_t lq_c_hash_callsign_16(const char* callsign);
uint32_t lq_c_hash_callsign_14(const char* callsign);
uint32_t lq_c_hash_callsign_12(const char* callsign);
uint32_t lq_c_hash_callsign_10(const char* callsign);

// Message Encoding & Decoding
int lq_c_encode_message(const lq_c_message_t* msg, uint8_t payload[10]);
int lq_c_decode_message(const uint8_t payload[10], lq_c_message_t* out_msg);
int lq_c_resolve_callsigns(lq_c_message_t* msg, const char* const* known_callsigns, size_t count);
int lq_c_decode_message_with_known_calls(const uint8_t payload[10], lq_c_message_t* out_msg, const char* const* known_callsigns, size_t count);
int lq_c_format_message(const lq_c_message_t* msg, char* out_str, size_t max_len);
int lq_c_message_to_text(const lq_c_message_t* msg, char* out_str, size_t max_len);
int lq_c_payload_to_text(const uint8_t payload[10], char* out_text, size_t max_len, const char* const* known_callsigns, size_t count);
int lq_c_parse_message(const char* text, lq_c_message_t* out_msg);

// Direct Conversions (Payload <-> Hex <-> Binary <-> Text)
int lq_c_payload_to_hex(const uint8_t payload[10], char* out_hex, size_t max_len, int space_separated);
int lq_c_payload_to_binary(const uint8_t payload[10], char* out_bin, size_t max_len);
int lq_c_hex_to_payload(const char* hex_str, uint8_t out_payload[10]);
int lq_c_binary_to_payload(const char* bin_str, uint8_t out_payload[10]);
int lq_c_hex_to_text(const char* hex_str, char* out_text, size_t max_len, const char* const* known_callsigns, size_t count);
int lq_c_binary_to_text(const char* bin_str, char* out_text, size_t max_len, const char* const* known_callsigns, size_t count);
int lq_c_text_to_hex(const char* text, char* out_hex, size_t max_len, int space_separated);
int lq_c_text_to_binary(const char* text, char* out_bin, size_t max_len);

// Transport Tone Encoding & Decoding
int lq_c_encode_payload(const uint8_t payload[10], uint8_t* out_tones, lq_mode_t mode);
int lq_c_decode_payload(const uint8_t* tones, uint8_t out_payload[10], lq_mode_t mode);
int lq_c_encode_tones(const lq_c_message_t* msg, uint8_t* out_tones, lq_mode_t mode);
int lq_c_decode_tones(const uint8_t* tones, lq_c_message_t* out_msg, lq_mode_t mode);

// Audio & GFSK Synthesis & Demodulation
void lq_c_synth_gfsk(const uint8_t* symbols, int num_symbols, float f0, float symbol_bt, float symbol_period, float sample_rate, float* signal_out);
int lq_c_generate_audio(const uint8_t* tones, int num_tones, float base_freq_hz, float sample_rate, lq_mode_t mode, float* audio_out, int max_samples);
int lq_c_audio_to_message(const float* audio, size_t num_samples, float base_freq, float sample_rate, lq_mode_t mode, lq_c_message_t* out_msg, int num_threads);
LQ_API int lq_c_audio_to_messages(const float* audio, size_t num_samples, float base_freq, float sample_rate, lq_mode_t mode, lq_c_message_t* out_msgs, size_t max_msgs, int num_threads);
LQ_API int lq_c_audio_to_messages_ext(const float* audio_samples, int num_samples, float base_freq, float sample_rate, lq_mode_t mode, lq_c_message_t* out_messages, int max_messages, int num_threads, int is_deep);

// Waterfall DSP & Candidate Synchronization
int lq_c_waterfall_sync_score(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, lq_mode_t mode);
float lq_c_waterfall_refine_frequency(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, int freq_sub, int freq_osr, float symbol_period, lq_mode_t mode, int* out_score);
float lq_c_waterfall_guess_snr(const float* mag2, int num_blocks, int block_stride, int time_offset, int freq_offset, lq_mode_t mode);
int lq_c_waterfall_extract_llrs(const uint8_t* mag, int num_blocks, int block_stride, int time_offset, int freq_offset, lq_mode_t mode, float out_llrs_174[174]);
void lq_c_waterfall_subtract_signal(uint8_t* mag, int max_mag_size, int block_size, int block_stride, const uint8_t payload[10], float freq_hz, float time_sec, float sample_rate, lq_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif // LQ_C_API_H

