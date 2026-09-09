# The LQ Digital Mode Family: High-Efficiency Protocols for Weak-Signal Amateur Radio

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C++17 / C++20](https://img.shields.io/badge/Language-C%2B%2B17%20%2F%20C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Web Portal](https://img.shields.io/badge/Web%20Portal-lq8.org-cyan)](https://lq8.org)
[![Native Support](https://img.shields.io/badge/Supported%20By-qFT8-success)](https://qft8.com)
[![Paper](https://img.shields.io/badge/Specification-PDF-red)](paper/lq_digitalmode.pdf)

**LQ** is a family of next-generation amateur radio digital modes designed for high-efficiency weak-signal communication across HF, VHF, and UHF bands.

* **Author:** Luis Quesada (HB9IPH) — [https://luisquesada.com](https://luisquesada.com)
* **Official Web Portal:** [https://lq8.org](https://lq8.org)
* **Open-Source Repository:** [https://github.com/lquesada/lq_lib](https://github.com/lquesada/lq_lib)
* **Native Android Application:** [qFT8](https://qft8.com)
* **Specification Document:** [`paper/lq_digitalmode.pdf`](paper/lq_digitalmode.pdf)
* **Strategic Roadmap:** [`TODO.md`](TODO.md)

---

## ⚡ Core Innovation: 4-Message QSO via Huffman Coding

In conventional modes like FT8, completing a standard contact requires a sequence of **six transmissions** (90 seconds total) because fixed-width message-type overhead leaves insufficient payload room to exchange both signal reports and locators simultaneously.

**The LQ protocol family eliminates this inefficiency by applying variable-length Huffman prefix coding to the message-type field.** 

By assigning a **1-bit codeword (`1`)** to the most data-dense `CALL std` message, LQ reclaims payload capacity to fit **two standard callsigns (56 bits), a grid locator (15 bits), and an SNR signal report (5 bits) into a single 77-bit transmission** with zero waste ($1 + 28 + 28 + 15 + 5 = 77$).

### Standard QSO Sequence Comparison

| Step | Standard FT8 (6 Transmissions / 90s) | **LQ Family (4 Transmissions / 60s)** |
|:---:|---|---|
| **1** | `CQ YO1YO JN47` | `CQ YO1YO JN47` |
| **2** | `YO1YO TU2TU KL22` | `YO1YO TU2TU KL22 -03` *(Caller answers with locator & report in 1-bit frame)* |
| **3** | `TU2TU YO1YO +05` | `TU2TU YO1YO R+05` *(DX returns report & first 73)* |
| **4** | `YO1YO TU2TU R-03` | `YO1YO TU2TU 73` *(Caller transmits concluding 73 acknowledgment)* |
| **5** | `TU2TU YO1YO RR73` | *— (Eliminated)* |
| **6** | `YO1YO TU2TU 73` | *— (Eliminated)* |

### 🚀 High-Throughput Pileup Confirmation (`MULTI-REPORT+73`)

For contest and DXpedition pileups where multiple stations answer simultaneously, LQ introduces **`MULTI-REPORT+73` (Type 11)**:
* **Single Transmission:** The DX station confirms **up to 2 stations simultaneously** in a single 15-second transmission ($3\text{b prefix} + 16\text{b DX hash} + 2 \times [24\text{b Target Hash} + 5\text{b SNR}] = 77\text{ bits}$).
* **Throughput Multiplier Analysis:**
  * **$\mathbf{2.33\times}$ over FT8 Fox & Hound:** Completing 2 QSOs in 3 slots (45s) vs 7 slots (105s) in pipelined dual-stream FT8 Fox & Hound yields a $\frac{105}{45} = \mathbf{2.33\times}$ speedup (>2.3× throughput, 160 vs ~69 QSOs/h).
  * **0.0 dB Power Penalty:** LQ8 operates as a single-carrier constant-envelope 50.0 Hz transmission (0.0 dB power splitting penalty, 0 IMD splatter), whereas FT8 multi-streaming splits power by $-3.0\text{ dB}$ (2 streams) to $-7.0\text{ dB}$ (5 streams) and requires PA backoff.
  * **Full 60s QSO Speedup:** Completes full 1-on-1 contacts in 4 transmissions (60s vs 75–90s in FT8: $\mathbf{1.50\times}$ over canonical 6-slot / $\mathbf{1.25\times}$ over 5-step RR73) with zero SNR penalty.
  * **Dual-Carrier Scalability:** Under dual-carrier operation (100 Hz bandwidth), pileup capacity scales to $\mathbf{320\text{ QSOs/h}}$ ($4.67\times$ over dual-stream FT8 Fox & Hound).
* **Full 24-bit Target Hashes:** 24-bit CRC-24 hashes with 16.78M bins prevent hash collisions.
* **Single-Station Duplication:** When replying to 1 station, Target 2 duplicates Target 1 (processed by decoders as a single station).

---

## 📊 Comprehensive Mode Comparison Matrix

| Parameter | FT8 | **LQ8** | FT4 | **LQ4** | **LQ2** | **LQ16** |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| **Modulation** | 8-GFSK | **8-GFSK** | 4-GFSK | **4-GFSK** | **4-GFSK** | **8-GFSK** |
| **Bandwidth** | 50.0 Hz | **50.0 Hz** | 83.3 Hz | **83.3 Hz** | **166.7 Hz** | **25.0 Hz** |
| **TX Duration** | 12.64 s | **12.64 s** | 5.04 s | **5.04 s** | **2.52 s** | **25.28 s** |
| **Time Slot** | 15.0 s | **15.0 s** | 7.5 s | **7.5 s** | **3.75 s** | **30.0 s** |
| **Sensitivity (SNR)** | **$-$21.0 dB** | **$-$21.0 dB** | $-$17.5 dB | **$-$17.5 dB** | **$-$14.0 dB** | **$-$24.0 dB** |
| **QSO Messages** | 6 | **4** | 6 | **4** | **4** | **4** |
| **Min. QSO Time** | 90 s | **60 s** | 45 s | **30 s** | **15 s** | **120 s** |
| **Max Contact Rate** | 0.67 QSO/min | **1.00 QSO/min** | 1.33 QSO/min | **2.00 QSO/min** | **4.00 QSO/min** | **0.50 QSO/min** |
| **Performance Edge** | Baseline | **1.5x throughput at FT8 sensitivity (+3.5 dB over FT4)** | Baseline | **1.5x throughput at FT4 sensitivity (+3.5 dB over FT2)** | **15s Meteor scatter burst** | **Deep DX down to -24 dB SNR** |

---

## 📻 Operating Frequencies, Band Planning & ADIF Logging

All protocols in the LQ Digital Mode family use dedicated **3.0 kHz USB audio channels** (dial frequency to dial + 3.0 kHz) across HF, VHF, and UHF bands, enabling multiple simultaneous QSOs on distinct audio subcarrier offsets without mutual interference.

For live, up-to-date recommended dial frequencies, IARU band-plan coordination notes, and standardized ADIF logging specifications for LoTW, QRZ.com, and eQSL, please refer to the official web portal:

👉 **[https://lq8.org#frequencies](https://lq8.org#frequencies)** &bull; **[https://lq8.org#adif](https://lq8.org#adif)**

---

## 🛠️ Key Technical Specifications

* **Forward Error Correction:** Systematic LDPC(174, 91) code with CRC-14 (`0x2757`) parity check. Normalized Min-Sum belief propagation decoding with 9.1 dB coding gain.
* **Standard Callsigns:** 28-bit mixed-radix encoding ($37 \times 36 \times 10 \times 27^3$), supporting all standard ITU prefix structures and native 1-bit dedicated portable suffix (`/P`, none).
* **Non-Standard Callsigns:** 38-character alphabet base-38 big-integer packing supporting up to 9 characters plus standard suffix (or 9 characters including non-standard suffix) in structured QSOs (48 bits) and up to 13 characters plus standard suffix in CQ frames (69 bits).
* **Callsign Hash:** 24-bit CRC-24/Q hash (`0x864CFB`, mask `0xFFFFFF`), providing 16.78M collision-resistant bins, with a 16-bit DX hash for multi-reply transmissions.
* **Maidenhead Grid Locators:** 15-bit packing for all 32,400 Maidenhead grid squares plus blank sentinel.
* **CQ Modifiers:** 20-bit open encoder supporting 3-digit decimal designators (`000`--`999`) and arbitrary 1-to-4 character Base-32 alphanumeric tokens (`DX`, `POTA`, `SOTA`, `WWFF`, `RTTY`, `EU`, `NA`, etc.).
* **Signal Reports (SNR):** 5-bit report covering $-26$\,dB to $+5$\,dB in 1\,dB steps ($N_5 = \text{SNR}_{\text{dB}} + 26 \in [0, 31]$).
* **Free-Text Varicode:** 104-character alphabet with Shannon entropy $H(X) = 4.22$\,bits/char, mean code length $\bar{L} = 4.25$\,bits/char ($99.4\%$ efficiency), carrying $\sim$17.2 readable text characters per standalone frame (and up to 20+ for abbreviations) with reserved special character `⚡` (for higher-layer protocols built on top of LQ) and safe zero `[FILL]` padding.

---

## 📁 Repository Directory Structure

```
LQ8/
├── CMakeLists.txt              # Root CMake build configuration
├── build.sh                    # Top-level library & tests build script
├── clean.sh                    # Top-level deep clean script
├── LICENSE                     # MIT License
├── README.md                   # Primary project overview and documentation
├── TODO.md                     # Strategic roadmap and adoption checklist
├── .gitignore                  # Git ignore rules
│
├── docs/                       # Official Web Portal (https://lq8.org)
│   ├── index.html              # Modern dark-theme interactive portal
│   ├── qft8.jpg                # qFT8 application screenshot
│   └── tones_waterfall_lq8.png # LQ8 physical channel waterfall spectrogram
│
├── include/lq/                 # Public C++ and C API Header Files
│   ├── lq.h                    # Master include header
│   ├── c_api.h                 # Pure C API bindings (C99/C11 compatible)
│   ├── easy.h                  # Simplified, high-level user API
│   ├── types.h                 # Core constants, enums (MessageType), structs
│   ├── message.h               # Message structures, bit-packing, formatting
│   ├── transport.h             # Transceiver transport & DSP waterfall sync primitives
│   ├── callsign.h              # ITU standard callsign mixed-radix codecs
│   ├── callsign_nonstd.h       # Base-38 compound and non-standard callsign codecs
│   ├── hash.h                  # 24, 22, 12, 10-bit callsign hash algorithms
│   ├── locator.h               # 15-bit Maidenhead grid locator codecs
│   ├── modifier.h              # 20-bit CQ modifier numeric & Base-32 codecs
│   ├── rst.h                   # 5-bit RST signal report codecs
│   ├── crc.h                   # CRC-14 parity and CRC-24/Q callsign hashing
│   ├── ldpc.h                  # LDPC(174, 91) encoder and Normalized Min-Sum decoder
│   ├── varicode.h              # Optimal 104-symbol Huffman Varicode codec
│   └── huffman.h               # Message-type prefix Huffman tree parser
│
├── src/                        # Core C++ & C Implementation Files
│   ├── c_api.cpp               # Pure C wrapper exports & object lifecycle
│   ├── message.cpp             # Message serialization, bit packing, text parsing
│   ├── callsign.cpp            # ITU standard callsign encoding
│   ├── callsign_nonstd.cpp     # Base-38 non-standard callsign encoding
│   ├── locator.cpp             # 15-bit Maidenhead grid locator encoding
│   ├── modifier.cpp            # CQ modifier numeric & Base-32 encoding
│   ├── rst.cpp                 # 5-bit RST signal report encoding
│   ├── crc.cpp                 # CRC-14 and CRC-24/Q algorithms
│   ├── hash.cpp                # 24, 22, 12, 10-bit callsign hash mapping
│   ├── ldpc.cpp                # Systematic generator and parity check matrices
│   ├── varicode.cpp            # Free-text Varicode encoder/decoder tables
│   ├── huffman.cpp             # Prefix code table implementation
│   ├── transport.cpp           # GFSK synthesis & 2D STFT waterfall synchronization
│   └── easy.cpp                # High-level one-line encode/decode helper API
│
├── tests/                      # Exhaustive GoogleTest Test Suites
│   ├── test_c_api.cpp          # Full verification of pure C API functions
│   ├── test_waterfall_dsp.cpp  # 2D Costas sync, parabolic refinement, LLR extraction
│   ├── test_parallel_decoder.cpp # Multi-core candidate parallelism & zero-seam tests
│   ├── test_comprehensive_matrix.cpp # Extensive edge-case, fuzzing, and round-trip matrix
│   ├── test_easy_api.cpp       # Verification of the high-level C++ easy API
│   ├── test_callsign.cpp       # ITU standard callsign mixed-radix tests
│   ├── test_callsign_nonstd.cpp# Base-38 compound and non-standard callsign tests
│   ├── test_hash.cpp           # Callsign hash collision and precision tests
│   ├── test_locator.cpp        # 15-bit Maidenhead grid locator test suite
│   ├── test_modifier.cpp       # 20-bit CQ modifier numeric & Base-32 tests
│   ├── test_rst.cpp            # 5-bit RST signal report tests
│   ├── test_varicode.cpp       # 104-character Varicode entropy and sentinel tests
│   ├── test_huffman.cpp        # Prefix-free Huffman message-type code tests
│   ├── test_message.cpp        # 13 message types bit packing & text round-trip tests
│   ├── test_crc.cpp            # CRC-14 bit-flip detection tests
│   ├── test_ldpc.cpp           # LDPC(174, 91) error correction & Min-Sum decoder tests
│   ├── test_transport.cpp      # Transceiver protocol state machine tests
│   ├── test_coverage_edge_cases.cpp # Dedicated coverage and boundary tests
│   ├── test_call_reply_combinations.cpp # All call/reply message pairing matrix
│   ├── test_golden_vectors.cpp # Automated golden vectors validation test suite
│   ├── data/                   # Comprehensive JSON test battery vectors
│   ├── java/                   # Cross-language Java test battery runner
│   └── test_ft8_lib/           # FT8 Reference Library Cross-Compatibility Suite
│       ├── CMakeLists.txt      # Dedicated CMake configuration for ft8_lib testing
│       ├── build.sh            # Test build & execution wrapper
│       ├── test_ft8_compat.cpp # Cross-layer verification against ft8_lib
│       └── ft8_lib/            # Cloned reference ft8_lib repository
│
├── golden_vectors/             # Official Multi-Mode Golden Test Vectors Dataset
│   ├── README.md               # Third-party developer integration guide & schema
│   ├── golden_vectors.json     # Comprehensive machine-readable dataset (196 vectors)
│   ├── golden_vectors_paper.json # Paper worked examples subset (76 vectors)
│   └── golden_vectors.txt      # Clean tabular human-readable test vectors
│
├── examples/                   # Demonstration & Reference Implementations
│   ├── c_api_demo/             # Pure C application integration demo
│   │   ├── main.c
│   │   └── build.sh
│   ├── tutorial/               # Step-by-step developer guide & API walkthrough
│   │   ├── main.cpp
│   │   └── build.sh
│   ├── demo/                   # Audio modem & TX/RX demonstration
│   │   ├── main.cpp
│   │   └── build.sh
│   ├── transceiver/            # Comprehensive automated transceiver test matrix
│   │   ├── main.cpp
│   │   └── build.sh
│   ├── matrix/                 # Exhaustive protocol & codec validation runner
│   │   ├── main.cpp
│   │   └── build.sh
│   └── android/                # Production Native Android Application (qFT8)
│       ├── app/                # Android app module with native JNI bridge
│       └── build.sh            # Gradle Android APK build wrapper
│
├── tools/                      # Ancillary Generation, Serialization & Diagnostic Tools
│   ├── verify_golden_vectors.cpp # Standalone CLI golden vectors compliance verifier
│   ├── generate_golden_vectors.cpp # Generates/refreshes golden vector datasets
│   ├── verify_paper_examples.cpp # Master 76-test paper bitfield assertions verifier
│   ├── generate_test_battery.cpp # Generates comprehensive JSON test battery
│   ├── generate_worked_examples.py # Generates verified worked example vectors
│   ├── generate_tone_diagrams.py # Generates scaled physical-layer waterfall plots
│   ├── generate_varicode.py    # Generates 104-symbol Huffman Varicode table & LaTeX/C++
│   ├── measure_coverage.py     # Code coverage parser & metric reporting
│   └── export_tones.cpp        # Exports tone symbols and durations to JSON
│
└── paper/                      # Complete Academic Specification Paper (LaTeX)
    ├── lq_digitalmode.tex      # Master LaTeX document
    ├── build.sh                # Automated PDF compilation script with pdflatex
    ├── figures/                # Scaled waterfall spectrograms & figures
    └── sections/               # Modular LaTeX sections (11 sections + worked examples)
```

---

## 💡 Developer Tutorial & Code Examples (`examples/tutorial`)

The repository includes a comprehensive, interactive tutorial in [`examples/tutorial`](examples/tutorial) demonstrating how to use the library to encode and decode across all protocol layers.

To compile and run the tutorial:
```bash
./examples/tutorial/build.sh
./examples/tutorial/lq_tutorial
```

### 1. Constructing & Formatting Messages
```cpp
#include "lq/lq.h"

// Construct a 3-message QSO contact request (Step 2: CALL std)
lq::Message msg;
msg.type    = lq::MessageType::CALL_STD;
msg.call_1  = "YO1YO";   // Station being called
msg.call_2  = "HB9IPH";  // Calling station
msg.locator = "JN47";   // Caller Maidenhead grid
msg.rst_db  = -3;        // Signal report in dB

// Format as standard text: "YO1YO HB9IPH JN47 -03"
std::string text = lq::format_message(msg);
```

### 2. Encoding to 77-Bit Binary & Hexadecimal
```cpp
uint8_t payload[lq::PAYLOAD_BYTES] = {0}; // 10 bytes (77 bits)
if (lq::encode_message(msg, payload)) {
    // Hex: F8 87 79 0B DF 05 67 44 8B B8
    // Bit structure: [1-bit Huffman] [28-bit Call1] [28-bit Call2] [15-bit Grid] [5-bit RST]
}
```

### 3. Channel Modulation & Tone Sequencing (LQ8, LQ4, LQ2)
```cpp
lq::ToneSequence seq;
lq::encode_tones(msg, lq::Protocol::LQ8, seq);
// seq.size()        == 79 symbols (8-GFSK, Costas synchronized)
// seq.tone_spacing  == 6.25 Hz
// seq.tx_duration   == 12.64 s
```

### 4. Continuous-Phase GFSK Audio Synthesis
```cpp
std::vector<float> audio;
lq::generate_audio(seq, 1500.0f, 12000.0f, audio); // 1500 Hz base carrier, 12 kHz sample rate
```

### 5. Decoding from Raw Hex / Binary Payloads
```cpp
uint8_t incoming_payload[10] = { 0xF8, 0x87, 0x79, 0x0B, 0xDF, 0x05, 0x67, 0x44, 0x8B, 0xB8 };
lq::Message decoded;
if (lq::decode_message(incoming_payload, decoded)) {
    std::cout << "Decoded: " << lq::format_message(decoded) << "\n";
    std::cout << "Target:  " << decoded.call_1 << "\n";
    std::cout << "Caller:  " << decoded.call_2 << "\n";
    std::cout << "Locator: " << decoded.locator << "\n";
    std::cout << "RST:     " << decoded.rst_db << " dB\n";
}
```

### 6. Demodulating from Tones & Audio Waveforms
```cpp
// Decode directly from tone sequences
lq::Message msg_from_tones;
lq::decode_tones(seq, msg_from_tones);

// Demodulate directly from raw acoustic float audio
lq::Message msg_from_audio;
lq::audio_to_message(audio, 1500.0f, 12000.0f, lq::Protocol::LQ8, msg_from_audio);
```

### 7. Fluent High-Level API (`lq::easy`)
```cpp
#include "lq/easy.h"

// One-line audio synthesis & decoding
auto audio_samples = lq::text_to_audio("CQ HB9IPH JN47", lq::Protocol::LQ8);
auto decoded_text  = lq::audio_to_text(audio_samples, 1500.0f, 12000.0f, lq::Protocol::LQ8);

// Stateful Transceiver
lq::Transceiver trx("HB9IPH", "JN47", lq::Protocol::LQ8);
auto tx_audio = trx.generate_cq("SOTA");
auto rx_msg   = trx.decode(tx_audio);
```

### 8. Pure C API & Cross-Language Integration (`lq/c_api.h`)

For embedded microcontrollers, C-only SDR applications (like `qFT8`), Python/Rust FFI bindings, and Android JNI engines, `lq_lib` exports a 100% pure C interface:

```c
#include "lq/c_api.h"

// 1. Parse a QSO message in C
lq_c_message_t msg;
lq_c_parse_message("YO1YO HB9IPH JN47 -03", &msg);

// 2. Generate channel tones for LQ8 (79 symbols)
uint8_t tones[79];
lq_c_encode_tones(&msg, tones, LQ_MODE_LQ8);

// 3. Synthesize GFSK float audio samples
float audio[79 * 1920];
int samples = lq_c_generate_audio(tones, 79, 1500.0f, 12000.0f, LQ_MODE_LQ8, audio, 79 * 1920);

// 4. Decode tones back to C message struct
lq_c_message_t rx_msg;
if (lq_c_decode_tones(tones, &rx_msg, LQ_MODE_LQ8)) {
    printf("Decoded in C: %s -> %s (Grid: %s, RST: %+d dB)\n",
           rx_msg.call_to, rx_msg.call_from, rx_msg.grid, rx_msg.rst_db);
}
```

### 9. 2D STFT Waterfall DSP & Synchronization Primitives

`lq_lib` provides high-performance physical-layer DSP primitives for software-defined radio pipelines:

* **Costas Sync Scoring (`lq_sync_score` / `lq_c_waterfall_sync_score`):** Computes normalized 2D Costas correlation across magnitude spectrogram frames for LQ16, LQ8, LQ4, and LQ2 modes.
* **Sub-Bin Frequency Refinement (`lq_refine_frequency` / `lq_c_waterfall_refine_frequency`):** Uses 3-point parabolic peak interpolation for sub-bin carrier frequency tracking with &plusmn;0.1 Hz accuracy.
* **SNR Estimation (`lq_guess_snr` / `lq_c_waterfall_guess_snr`):** Computes true SNR in dB relative to standard 2500 Hz SSB voice bandwidth.
* **Direct Soft LLR Extraction (`lq_extract_llrs_from_waterfall` / `lq_c_waterfall_extract_llrs`):** Extracts 174 LDPC log-likelihood ratios directly from magnitude waterfall bins without intermediate carrier demodulation.
* **Multi-Pass Signal Subtraction (`lq_subtract_signal_from_waterfall` / `lq_c_waterfall_subtract_signal`):** Subtracts decoded signal tone energy from the spectrogram matrix to uncover overlapping weak signals underneath strong transmitters.
* **GFSK Pulse Shaping (`synth_gfsk` / `lq_c_synth_gfsk`):** Gaussian pulse shaping with $BT=2.0$ for LQ16/LQ8, $BT=1.0$ for LQ4/LQ2, and raised-cosine boundary ramp.

### 10. Multi-Core Candidate Parallelism Architecture

Weak-signal demodulation, soft LLR extraction, LDPC(174,91) Normalized Min-Sum belief propagation, and CRC-14 verification consume over 90% of receiver CPU time. `lq_lib` provides native **multi-core candidate parallelism** across all decoding pipelines:

#### Architectural Guarantees:
1. **Zero Boundary Seams & Zero Dropped Signals:** Raw audio waveforms and waterfall frequency matrices are **never sliced** into time or frequency sub-bands. The candidate search space is evaluated continuously, generating a discrete candidate index array $[0, M)$.
2. **Read-Only Candidate Independence:** Each candidate represents a discrete $(f, t)$ coordinate. Soft demodulation, LLR calculation, and LDPC decoding are completely read-only and execute concurrently on worker threads without locks or data races.
3. **Single-Thread Fast-Path (`num_threads == 1`):** When `num_threads <= 1` (the default) or candidate count $M \le 1$, decoding runs on the calling thread directly with zero thread-spawning or synchronization overhead.
4. **Bit-for-Bit Determinism:** Candidate results are aggregated in deterministic index order and deduplicated on the main thread, guaranteeing that 1, 2, 4, 8, or 16 threads produce **100% bit-identical results**.

```cpp
// 1. High-Level C++ API with Multi-Threading
#include "lq/transport.h"
#include "lq/easy.h"

// Auto-tune threads for device cores (bounded to avoid thermal throttling)
int threads = std::max(1, std::min(8, static_cast<int>(std::thread::hardware_concurrency())));

// Multi-signal waterfall decode using 4 threads
auto messages = lq::audio_to_messages(audio_samples, 0.0f, 12000.0f, lq::Protocol::LQ8, threads);

// Fluent 1-line text decoding with 4 threads
auto text = lq::audio_to_text(audio_samples, 1500.0f, 12000.0f, lq::Protocol::LQ8, threads);

// Stateful Transceiver configured with worker threads
lq::Transceiver rx("HB9IPH", "JN47", lq::Protocol::LQ8, 1500.0f, 12000.0f, threads);
auto msg = rx.decode(audio_samples); // Uses configured thread pool
```

```c
// 2. Pure C ABI with Multi-Threading Support
#include "lq/c_api.h"

lq_c_message_t rx_msg;
int ok = lq_c_audio_to_message(audio_ptr, num_samples, 1500.0f, 12000.0f, LQ_MODE_LQ8, &rx_msg, 4);

lq_c_message_t rx_list[16];
int count = lq_c_audio_to_messages(audio_ptr, num_samples, 0.0f, 12000.0f, LQ_MODE_LQ8, rx_list, 16, 4);

// Extended C API with Fast vs Deep decode strategy (is_deep = 1)
int count_deep = lq_c_audio_to_messages_ext(audio_ptr, num_samples, 0.0f, 12000.0f, LQ_MODE_LQ8, rx_list, 16, 4, 1);
```

---

## 📚 Complete Module-by-Module API Reference

### 1. High-Level Quick-Start API (`lq/easy.h`)

#### Factory Message Builders
* `lq::Message make_cq(std::string_view callsign, std::string_view locator = "", std::string_view modifier = "")`  
  Constructs a standard or non-standard CQ discovery message.
* `lq::Message make_call(std::string_view target_call, std::string_view my_call, std::string_view locator, int rst_db = 0)`  
  Constructs a 1-bit Huffman `CALL std` or hashed contact initiation message.
* `lq::Message make_reply73(std::string_view target_call, std::string_view my_call, int rst_db = 0)`  
  Constructs a final report and 73 confirmation message.
* `lq::Message make_free_text(std::string_view text)`  
  Constructs a free-text message carrying up to $\sim$17.2 characters using extended 104-character Varicode.

#### Packing & Unpacking
* `std::optional<std::vector<uint8_t>> pack(const Message& msg)`  
  Packs a `Message` into a 10-byte (77-bit) payload vector. Returns `std::nullopt` on validation error.
* `std::optional<Message> unpack(const std::vector<uint8_t>& payload)`  
  Unpacks a 10-byte payload into a structured `Message`.
* `std::optional<Message> unpack(const uint8_t* payload_bytes, size_t len = PAYLOAD_BYTES)`  
  Unpacks raw byte pointer into a `Message`.

#### One-Line Audio Helpers
* `std::optional<ToneSequence> text_to_tones(std::string_view text, Protocol proto = Protocol::LQ8)`  
  Parses human-readable text and encodes directly to channel tone symbols.
* `std::optional<std::string> tones_to_text(const ToneSequence& seq)`  
  Decodes a tone sequence directly to human-readable text.
* `std::vector<float> text_to_audio(std::string_view text, Protocol proto = Protocol::LQ8, float base_freq_hz = 1500.0f, float sample_rate = 12000.0f)`  
  Synthesizes normalized float audio samples directly from a text string.
* `std::optional<std::string> audio_to_text(const std::vector<float>& audio_samples, float base_freq_hz = 1500.0f, float sample_rate = 12000.0f, Protocol proto = Protocol::LQ8)`  
  Demodulates float audio samples directly to human-readable text.

#### `lq::Transceiver` Helper Class
```cpp
class Transceiver {
public:
    Transceiver(std::string_view my_callsign = "HB9IPH",
                std::string_view my_grid = "JN47",
                Protocol proto = Protocol::LQ8,
                float default_freq_hz = 1500.0f,
                float default_sample_rate = 12000.0f);

    void set_callsign(std::string_view callsign);
    const std::string& get_callsign() const;
    void set_grid(std::string_view grid);
    const std::string& get_grid() const;
    void set_protocol(Protocol proto);
    Protocol get_protocol() const;
    void set_frequency(float freq_hz);
    float get_frequency() const;
    void set_sample_rate(float sample_rate);
    float get_sample_rate() const;

    std::vector<float> generate_cq(std::string_view modifier = "") const;
    std::vector<float> generate_call(std::string_view target_call, int rst_db = 0) const;
    std::vector<float> generate_reply73(std::string_view target_call, int rst_db = 0) const;
    std::vector<float> generate_free_text(std::string_view text) const;
    std::vector<float> generate_audio(const Message& msg) const;

    std::optional<Message> decode(const std::vector<float>& audio_samples) const;
    std::optional<std::string> decode_text(const std::vector<float>& audio_samples) const;
};
```

---

### 2. Core Message Structures & Codecs (`lq/message.h`)

* `bool encode_message(const Message& msg, uint8_t payload[PAYLOAD_BYTES])`  
  Encodes structured message into 10-byte (77-bit) payload.
* `bool decode_message(const uint8_t payload[PAYLOAD_BYTES], Message& msg)`  
  Decodes 10-byte payload into structured message.
* `std::string format_message(const Message& msg)`  
  Formats message as standard human-readable text.
* `bool parse_message(std::string_view text, Message& msg)`  
  Parses human-readable text into structured message.

---

### 3. Physical Layer & Transceiver Transport (`lq/transport.h`)

* `bool encode_payload(const uint8_t payload[PAYLOAD_BYTES], Protocol proto, ToneSequence& seq)`  
  Encodes 77-bit payload + CRC-14 + LDPC(174,91) $\to$ `ToneSequence`.
* `bool encode_payload(const uint8_t* payload, uint8_t* tones, Protocol proto = Protocol::LQ8)`  
  Direct C-array style tone generator for `ft8_lib` drop-in compatibility.
* `bool encode_tones(const Message& msg, Protocol proto, ToneSequence& seq)`  
  Full transmission pipeline from `Message` to `ToneSequence`.
* `bool decode_tones(const ToneSequence& seq, uint8_t payload[PAYLOAD_BYTES])`  
  Demodulates tones to 77-bit payload with CRC-14 verification.
* `bool decode_payload(const uint8_t* tones, uint8_t* payload, Protocol proto = Protocol::LQ8)`  
  Direct C-array style tone demodulator.
* `bool decode_tones(const ToneSequence& seq, Message& msg)`  
  Full reception pipeline from `ToneSequence` to `Message`.
* `void generate_audio(const ToneSequence& seq, float base_freq_hz, float sample_rate, std::vector<float>& audio_samples)`  
  Continuous-phase GFSK audio synthesis.
* `bool demodulate_audio(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, ToneSequence& seq)`  
  Non-coherent matched-filter tone demodulator.
* `bool demodulate_audio_soft(const std::vector<float>& audio_samples, size_t offset, float base_freq_hz, float sample_rate, Protocol proto, std::vector<float>& llrs)`  
  Soft Log-Likelihood Ratio (LLR) extractor.
* `bool decode_soft_llrs(const std::vector<float>& llrs, Message& msg, Protocol proto = Protocol::LQ8, int max_ldpc_iters = 25)`  
  Normalized Min-Sum LDPC belief propagation soft decoder with customizable iteration count.
* `bool message_to_audio(const Message& msg, Protocol proto, float base_freq_hz, float sample_rate, std::vector<float>& audio_samples)`  
  End-to-end message to audio pipeline.
* `bool audio_to_message(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, Message& msg, int num_threads = 1, bool is_deep = false)`  
  End-to-end audio to message demodulator and decoder.
* `std::vector<Message> audio_to_messages(const std::vector<float>& audio_samples, float base_freq_hz, float sample_rate, Protocol proto, int num_threads = 1, bool is_deep = false)`  
  Multi-signal passband scanner with Fast vs Deep decode strategy and multi-pass signal subtraction.
* `bool save_wav_file(const std::string& filename, const std::vector<float>& audio_samples, float sample_rate)`  
  Exports audio to 16-bit PCM WAV.
* `bool load_wav_file(const std::string& filename, std::vector<float>& audio_samples, float& sample_rate)`  
  Loads 16-bit PCM WAV into float buffer.

---

### 4. Callsigns, Locators, Modifiers & Reports

* **Callsigns (`lq/callsign.h`):**
  * `bool is_standard_callsign(std::string_view call)`
  * `bool normalise_standard_callsign(std::string_view call, std::string& normalised)`
  * `bool encode_callsign_std(std::string_view call, uint32_t& packed)` (28-bit mixed-radix)
  * `bool decode_callsign_std(uint32_t packed, std::string& call)`
* **Non-Standard Callsigns (`lq/callsign_nonstd.h`):**
  * `bool is_valid_nonstd_callsign(std::string_view call, int max_chars)`
  * `bool encode_callsign_nonstd(std::string_view call, int max_chars, BitBuffer& bb)` (38-char base-38 big-integer)
  * `bool decode_callsign_nonstd(BitBuffer& bb, int max_chars, std::string& call)`
* **Grid Locators (`lq/locator.h`):**
  * `bool is_valid_locator(std::string_view loc)`
  * `bool encode_locator_15(std::string_view loc, uint16_t& packed)` (15-bit Maidenhead)
  * `bool decode_locator_15(uint16_t packed, std::string& loc)`
* **CQ Modifiers (`lq/modifier.h`):**
  * `bool encode_modifier_20(std::string_view mod, uint32_t& packed)` (20-bit: 3-digit numeric 000..999 + Base-32 alphanumeric)
  * `bool decode_modifier_20(uint32_t packed, std::string& mod)`
* **Signal Reports (`lq/rst.h`):**
  * `uint8_t encode_rst_5(int rst_db)` (5-bit $-26$\,dB to $+5$\,dB)
  * `int decode_rst_5(uint8_t packed)`
  * `std::string format_rst(int rst_db)`
  * `bool parse_rst(std::string_view str, int& rst_db)`

---

### 5. Error Correction, CRC, Hashes & Varicode

* **Varicode (`lq/varicode.h`):**
  * `bool is_varicode_char(char c)`
  * `int encode_varicode(std::string_view text, BitBuffer& bb, int max_bits = 75)`
  * `int decode_varicode(BitBuffer& bb, std::string& out, int max_bits = 75)`
* **CRC (`lq/crc.h`):**
  * `uint16_t compute_crc14(const uint8_t* data, int num_bits = 77)` (Polynomial `0x2757`)
  * `uint16_t compute_payload_crc14(const uint8_t payload[10])`
  * `void append_crc14(const uint8_t payload[10], uint8_t out_91[12])`
  * `bool verify_crc14(const uint8_t data_91[12])`
  * `void extract_payload(const uint8_t data_91[12], uint8_t payload[10])`
* **Callsign Hash (`lq/hash.h`):**
  * `uint32_t hash_callsign_24(std::string_view call)` (CRC-24/Q polynomial `0x864CFB`)
* **LDPC (`lq/ldpc.h`):**
  * `void ldpc_encode(const uint8_t in_91[12], uint8_t out_174[22])`
  * `int ldpc_decode(const float llr[174], uint8_t out_91[12], int max_iters = 25)`
  * `int ldpc_decode_hard_bits(const uint8_t in_174[22], uint8_t out_91[12], int max_iters = 25)`
  * `bool ldpc_check_syndrome(const uint8_t codeword[22])`
* **Huffman Codes (`lq/huffman.h`):**
  * `const HuffmanCodeEntry* get_huffman_entry(MessageType type)`
  * `int encode_huffman_prefix(MessageType type, BitBuffer& bb)`
  * `MessageType decode_huffman_prefix(BitBuffer& bb)`

---

## 🚀 Building & Running

### Requirements
* C++17 or C++20 compliant compiler (GCC 9+, Clang 10+, MSVC 2019+)
* CMake 3.20+
* POSIX or Windows environment

### Build & Run All Automated Tests
```bash
# Runs all 23 CTest unit suites + all 5 standalone examples + FT8 cross-compatibility suite
./run_full_tests.sh
```

### Full System & Verification Build
```bash
# Builds library, runs all tests, compiles 8-page paper PDF, and measures code coverage
./build_all.sh
```

Or via CMake directly:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Run CTest Suites Directly
```bash
cd build
ctest --output-on-failure
```
All **23 test suites** verify 100% pass across codecs, error correction, and transceiver transport.

---

## 🧪 FT8 Library Cross-Compatibility Test Suite (`tests/test_ft8_lib/`)

The [`tests/test_ft8_lib/`](tests/test_ft8_lib) directory defines automated cross-compatibility tests comparing `lq_lib` directly against the reference [`ft8_lib`](https://github.com/lquesada/qFT8/blob/main/LIB_FT8LICENSE.md) implementation (Copyright © 2018 Kārlis Gobas, released under the MIT License).

> [!NOTE]
> `ft8_lib` is included in this repository under `tests/test_ft8_lib/` strictly as an external test reference to verify that the `lq_lib` physical and transport layer implementations are mathematically correct and 100% compatible with the standard FT8/FT4 transport layer. The `lq_lib` library itself is an independent, cleanroom reference implementation written from scratch in modern C++20 and **does not contain any code from `ft8_lib`**.

The compatibility test suite exercises:

* **CRC-14 & Zero-Extension Identity:** Validates bit-for-bit parity equivalence against `ftx_compute_crc` and `ftx_add_crc`.
* **LDPC(174,91) Generator & Parity Check Equivalence:** Bit-for-bit identity against `encode174` and Min-Sum/Belief Propagation decoder parity verification.
* **Tone Mapping Equivalence:** Exact tone sequence generation matches for 8-GFSK FT8/LQ8 (79 symbols) and 4-GFSK FT4/LQ4 (105 symbols).
* **Cross-Decoding:** 
  * `lq_lib` encoded tones decoded directly by `ft8_lib` demodulation/decoding pipelines.
  * `ft8_lib` encoded tones decoded directly by `lq_lib` demodulation/decoding pipelines.
* **Audio Waveform & Soft Demodulation:** Full acoustic modulation loopback with soft LLR belief-propagation decoding.

To build and run the FT8 compatibility suite:
```bash
cd tests/test_ft8_lib
./build.sh
```

---

## 🧪 Official Golden Test Vectors Suite (`golden_vectors/`)

The [`golden_vectors/`](golden_vectors/) directory provides the official, multi-layer golden test vector dataset and validation harness for the **LQ Digital Mode Family**. Any third-party transceiver implementation (in **Rust**, **Python**, **Go**, **Java / Kotlin / Android**, **JavaScript / TypeScript / Web**, **C# / .NET**, or **embedded DSP microcontrollers**) can import these vectors to guarantee bit-for-bit mathematical compliance with the LQ specification.

### 1. Dataset Files & Formats

| File | Format | Description |
|---|---|---|
| [`golden_vectors/golden_vectors.json`](golden_vectors/golden_vectors.json) | JSON | Machine-readable dataset containing **196 test vectors** covering all 13 message formats across all 4 modes (`LQ8`, `LQ16`, `LQ4`, `LQ2`). |
| [`golden_vectors/golden_vectors_paper.json`](golden_vectors/golden_vectors_paper.json) | JSON | Dedicated subset containing **76 vectors** corresponding 1:1 with all worked examples published in the academic specification paper. |
| [`golden_vectors/golden_vectors.txt`](golden_vectors/golden_vectors.txt) | Text | Clean tabular human-readable representation detailing all encoding layers per test vector. |
| [`golden_vectors/README.md`](golden_vectors/README.md) | Markdown | Developer integration guide, test pipeline specifications, and JSON schema documentation. |

### 2. Verifying Golden Vectors

```bash
# 1. Run the standalone CLI verification tool (validates all 196 vectors in ~15 ms):
build/verify_golden_vectors

# 2. Run the automated GoogleTest CTest target:
ctest -R test_golden_vectors --output-on-failure

# 3. Regenerate or refresh golden vector datasets:
build/generate_golden_vectors golden_vectors
```

### 3. Cross-Language Validation Recipe (Python / Rust / Java / Go / JS)

Third-party implementations can execute an automated 5-step compliance check against `golden_vectors.json`:

```python
import json

with open("golden_vectors/golden_vectors.json") as f:
    data = json.load(f)

for v in data["test_vectors"]:
    # 1. Text -> 77-bit Payload Packing
    payload_hex = my_lq_encode(v["plaintext"])
    assert payload_hex == v["layers"]["payload_10byte_hex"], f"Payload mismatch on {v['id']}"

    # 2. Payload -> Canonical Text Decoding
    text = my_lq_decode(payload_hex)
    assert text == v["canonical_text"], f"Text mismatch on {v['id']}"

    # 3. CRC-14 Checksum
    crc = my_calc_crc14(payload_hex)
    assert crc == v["layers"]["crc14_val"], f"CRC mismatch on {v['id']}"

    # 4. LDPC(174,91) Systematic FEC Encoding
    codeword_hex = my_ldpc_encode(payload_hex)
    assert codeword_hex == v["layers"]["codeword_22byte_hex"], f"LDPC mismatch on {v['id']}"

    # 5. Physical Modulation Tone Mapping (LQ8, LQ16, LQ4, LQ2)
    tones = my_synthesize_tones(payload_hex, v["mode"])
    assert tones == v["layers"]["tones"], f"Tone sequence mismatch on {v['id']}"

print("✓ 100% compliant with LQ Digital Mode Family specification!")
```

---

## 🧪 Comprehensive Test Battery & Multi-Language Validation Suite (`tests/data/` & `tests/java/`)

To ensure that independent application developers (building transceivers in **C++**, **Java / Kotlin / Android**, **Python**, **Rust**, or **C#**) implement bit-for-bit identical message encoding, hash calculations, and deterministic transceiver state machines, `lq_lib` provides an automated, comprehensive test battery and multi-language validation framework.

### 1. Unified Message Representation & Conversion APIs
Transceiver applications can seamlessly convert between structured message payloads, raw 77-bit binary strings, 10-byte hexadecimal vectors, and human-readable standard protocol text:

```cpp
#include "lq/message.h"

// Convert human-readable text directly to hex or 77-bit binary string
std::string hex_str = lq::text_to_hex("CQ POTA HB9IPH JN47");
std::string bin_str = lq::text_to_binary("CQ POTA HB9IPH JN47");

// Convert binary / hex back to standardized text (with hash resolution)
std::string text_from_hex = lq::hex_to_text(hex_str);
std::string text_from_bin = lq::binary_to_text(bin_str);

// Low-level raw payload conversions
uint8_t payload[10];
lq::hex_to_payload(hex_str, payload);
std::string roundtrip_hex = lq::payload_to_hex(payload);
std::string roundtrip_bin = lq::payload_to_binary(payload);
```

Matching zero-overhead C ABI functions are available in [`include/lq/c_api.h`](include/lq/c_api.h) (`lq_c_text_to_hex`, `lq_c_text_to_binary`, `lq_c_hex_to_text`, `lq_c_binary_to_text`, `lq_c_payload_to_hex`, `lq_c_payload_to_binary`, `lq_c_hex_to_payload`, `lq_c_binary_to_payload`).

### 2. Deterministic Intent & Transceiver Decision Engine (`include/lq/intent.h`)
The reference library includes a standard deterministic decision engine that processes received frames across a time slot and user operational intent according to standardized amateur radio protocol rules:

* **Calling CQ:** Formats Type 1–4 CQ messages with optional modifiers (`CQ POTA`, `CQ DX`, `CQ SOTA`, etc.).
* **Answering a CQ:** Measures incoming SNR and emits an optimized `CALL` message (`CALL <Target> <MyCall> <MyGrid> <SNR>`).
* **Single-Station Reply:** Emits a `REPORT+73` frame with signal report and initial 73 acknowledgment.
* **Concluding 73:** Caller emits concluding `73` confirmation frame (`73 <Target> <MyCall>`) completing the 4-step exchange.
* **Multi-Station Pileup Reply:** When responding to multiple callers simultaneously, emits a `MULTI-REPORT+73` (Type 11) frame confirming up to 2 callers in a single 12.64s transmission.
* **QSO Completion Tracking:** Detects incoming `REPORT+73`, `73`, `MULTI-REPORT+73`, and `MULTI-73` frames addressed to the operator, matching 24-bit hashes against the station's known callsign cache and logging confirmed QSOs.

### 3. Automated Test Battery Generator (`tools/generate_test_battery.cpp`)
The test generator exports exhaustive test vectors into a portable JSON battery file (`tests/data/test_battery.json`):
```bash
./build/generate_test_battery tests/data/test_battery.json
```
This file includes:
* **`codec_conversions`**: Hundreds of verified test vectors with exact standard text, 10-byte hex strings, and 77-bit binary sequences covering all 18 message formats.
* **`hash_verifications`**: Reference hash vectors for standard, non-standard, and compound callsigns across CRC-24/Q, 23-bit, 14-bit, 12-bit, and 10-bit hash algorithms.
* **`intent_scenarios`**: Deterministic test vectors specifying station configuration, received slot frames, user intent actions, expected transmit text/hex/binary, and resulting QSO completion state.

### 4. Zero-Dependency Java Validation Runner (`tests/java/`)
For Android and Java developers, [`tests/java/com/lq/LQTestBatteryRunner.java`](tests/java/com/lq/LQTestBatteryRunner.java) is a self-contained test harness with **zero external dependencies** that validates Java/Kotlin engines against the full test battery:

```bash
# Run the Java Test Battery
./tests/java/run_java_tests.sh
```

```
=================================================================
   LQ Digital Mode Family — Java Test Battery Validation Runner   
=================================================================
Reading Test Battery from: tests/data/test_battery.json

>>> Phase 1: Callsign Hash Algorithm Equivalence (CRC-24/Q, 23b, 14b, 12b, 10b)...
  ✓ Passed 27 hash calculation test vectors.

>>> Phase 2: Codec Hex and Binary Representation Roundtrips...
  ✓ Passed 442 hex/binary codec conversion vectors.

>>> Phase 3: Deterministic User Intent & Transceiver Decision Scenarios...
  ✓ Passed 7 transceiver intent & decision scenarios.

=================================================================
  ✓ ALL 476 JAVA BATTERY TESTS PASSED WITH ZERO ERRORS!
=================================================================
```


## 📊 Generating Waterfall Spectrogram Diagrams

To regenerate the LQ8 and multi-mode waterfall diagrams ([`paper/figures/tones_waterfall_lq8.pdf`](paper/figures/tones_waterfall_lq8.pdf), [`docs/tones_waterfall_lq8.png`](docs/tones_waterfall_lq8.png), and [`paper/figures/tones_waterfall_comparison.pdf`](paper/figures/tones_waterfall_comparison.pdf)):

```bash
# 1. Compile and run export_tones tool
mkdir -p build && g++ -std=c++20 -O3 -Iinclude src/*.cpp tools/export_tones.cpp -o build/export_tones
./build/export_tones build/tones_data.json

# 2. Run the Python generator script (requires matplotlib and numpy)
python3 tools/generate_tone_diagrams.py
```
This script computes the tone matrices with 1-second grid separators and automatically saves the output images to `paper/figures/tones_waterfall_lq8.pdf`, `paper/figures/tones_waterfall_comparison.pdf`, and `docs/tones_waterfall_lq8.png`.

---

## 📱 Supported Applications

### **qFT8 for Android**
Full mobile native transceiver application with built-in support for **LQ8, LQ4, LQ2, and LQ16**:
* Real-time waterfall display ($300 - 2700$\,Hz)
* USB OTG audio and Bluetooth CAT control
* Automated QSO state machine and logging
* Download at: [https://qft8.com](https://qft8.com)



---

## 📄 Academic Specification Paper (`paper/`)

The full specification paper is compiled via `./build.sh` in the `paper/` directory:
```bash
cd paper
./build.sh
```
The resulting PDF document (`paper/lq_digitalmode.pdf`) provides the complete mathematical and protocol architecture specification for software and SDR developers:
* **Title:** *The LQ Digital Mode Family: Protocol Architecture and Reference Specification for Weak-Signal Communications*
* **Author:** Luis Quesada (HB9IPH)

---

## 📚 Prior Art & Scientific Foundation

The LQ digital mode family builds upon foundational scientific research in weak-signal communications, information theory, and channel coding:
* **FT8 / FT4 Physical Transport & Signaling:** Franke, S. (K9AN), Somerville, B. (G4WJS), & Taylor, J. (K1JT) (2020). *"The FT4 and FT8 Communication Protocols"*, *QEX*, no. 2, pp. 7–17.
* **Low-Density Parity-Check (LDPC) Codes:** Gallager, R. G. (1962). *"Low-density parity-check codes"*, *IRE Transactions on Information Theory*, vol. 8, no. 1, pp. 21–28.
* **Costas Synchronization Sequences:** Costas, J. P. (1984). *"A study of a class of detection waveforms having nearly ideal range-Doppler ambiguity properties"*, *Proceedings of the IEEE*, vol. 72, no. 8, pp. 996–1009.
* **Optimal Variable-Length Prefix Codes:** Huffman, D. A. (1952). *"A Method for the Construction of Minimum-Redundancy Codes"*, *Proceedings of the IRE*, vol. 40, no. 9, pp. 1098–1101.

---

## 🤝 Acknowledgments

The original concept, protocol architecture, and reference implementations were developed by the author. Generative AI was used to assist with background research, code testing, and manuscript drafting. The author thanks the global amateur radio community for valuable feedback during early on-air testing.

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

## 🔗 Links & References

* **Official Web Portal:** [https://lq8.org](https://lq8.org)
* **Community Subreddit:** [https://www.reddit.com/r/LQ8](https://www.reddit.com/r/LQ8)
* **News & Updates (Mailing List):** [https://groups.google.com/g/lq8-news](https://groups.google.com/g/lq8-news)
* **Author Website:** [https://luisquesada.com](https://luisquesada.com)
* **Native Application (qFT8):** [https://qft8.com](https://qft8.com)
* **GitHub Repository:** [https://github.com/lquesada/lq_lib](https://github.com/lquesada/lq_lib)

