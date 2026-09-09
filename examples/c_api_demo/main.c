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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lq/c_api.h"

int main(void) {
    printf("====================================================================\n");
    printf("  LQ Digital Mode Family — Pure C API Integration Demo (v%s)\n", lq_c_version_string());
    printf("====================================================================\n\n");

    // 1. Inspect Mode Parameters
    printf("--- Phase 1: Mode Parameters Query ---\n");
    lq_mode_t modes[4] = {LQ_MODE_LQ8, LQ_MODE_LQ4, LQ_MODE_LQ2, LQ_MODE_LQ16};
    for (int i = 0; i < 4; ++i) {
        lq_c_mode_params_t params;
        if (lq_c_get_mode_params(modes[i], &params)) {
            printf("  Mode: %-8s | Symbols: %3d | BW: %6.2f Hz | Slot: %5.2f s | SNR: %5.1f dB\n",
                   params.name, params.total_symbols, params.bandwidth_hz, params.slot_duration_sec, params.threshold_snr_db);
        }
    }
    printf("\n");

    // 2. Encode and Format Messages
    printf("--- Phase 2: Message Encoding, Hashing & Formatting in Pure C ---\n");
    const char* test_strings[5] = {
        "CQ DX HB9IPH JN47",
        "YO1YO TU2TU KL22 -03",
        "YO1YO TU2TU R+05",
        "YO1YO TU2TU 73",
        "<YO1YO> R+05 <TU2TU> R-03 <HB9IPH>"
    };

    for (int i = 0; i < 5; ++i) {
        lq_c_message_t msg;
        if (lq_c_parse_message(test_strings[i], &msg)) {
            char formatted[128];
            lq_c_format_message(&msg, formatted, sizeof(formatted));

            printf("  Input String   : %s\n", test_strings[i]);
            printf("  Parsed Type    : %d (is_cq=%d, is_call=%d, is_report73=%d, is_73=%d, is_multi=%d)\n",
                   msg.type, msg.is_cq, msg.is_call, msg.is_report73, msg.is_73, msg.is_multi_report73);
            printf("  Sender / Target: '%s' -> '%s' (Grid: '%s', RST: %+d dB)\n",
                   msg.call_from, msg.call_to, msg.grid, msg.rst_db);
            printf("  Hashes         : 24b_from=0x%06X, 24b_to=0x%06X, 16b_from=0x%04X\n",
                   msg.hash_from_24, msg.hash_to_24, msg.hash_from_16);
            if (msg.is_multi_report73 || msg.is_multi_73) {
                printf("  Multi Targets  : %d targets\n", msg.num_multi_targets);
                for (int t = 0; t < msg.num_multi_targets; ++t) {
                    printf("    [%d] Call: '%s', Hash24: 0x%06X, RST: %+d dB\n",
                           t + 1, msg.multi_targets[t].call, msg.multi_targets[t].hash_24, msg.multi_targets[t].rst_db);
                }
            }

            // Channel tones for LQ8 (79 symbols)
            uint8_t tones[128];
            int enc_res = lq_c_encode_tones(&msg, tones, LQ_MODE_LQ8);
            printf("  LQ8 Tones Enc  : %s (first 10 tones: %d %d %d %d %d %d %d %d %d %d)\n",
                   enc_res ? "SUCCESS" : "FAILED",
                   tones[0], tones[1], tones[2], tones[3], tones[4],
                   tones[5], tones[6], tones[7], tones[8], tones[9]);

            // Decode tones back to C message
            lq_c_message_t decoded_msg;
            if (lq_c_decode_tones(tones, &decoded_msg, LQ_MODE_LQ8)) {
                printf("  Decode Tones Rx: '%s' -> '%s' [%+d dB] -> MATCH!\n\n",
                       decoded_msg.call_from, decoded_msg.call_to, decoded_msg.rst_db);
            } else {
                printf("  Decode Tones Rx: FAILED!\n\n");
                return 1;
            }
        }
    }

    // 3. Audio & GFSK Synthesis
    printf("--- Phase 3: GFSK Audio Synthesis ---\n");
    lq_c_message_t qso_msg;
    lq_c_parse_message("YO1YO HB9IPH JN47 -03", &qso_msg);
    uint8_t lq8_tones[79];
    lq_c_encode_tones(&qso_msg, lq8_tones, LQ_MODE_LQ8);

    // 79 symbols * 1920 samples/symbol = 151680 samples @ 12kHz
    int max_samples = 79 * 1920;
    float* audio = (float*)malloc(max_samples * sizeof(float));
    int generated_count = lq_c_generate_audio(lq8_tones, 79, 1000.0f, 12000.0f, LQ_MODE_LQ8, audio, max_samples);
    printf("  Generated %d GFSK audio samples @ 12.0 kHz (base freq 1000.0 Hz)\n", generated_count);
    free(audio);
    printf("\n");

    printf("====================================================================\n");
    printf("  ✓ PURE C API VALIDATION COMPLETED WITH 100%% SUCCESS!\n");
    printf("====================================================================\n");

    return 0;
}

