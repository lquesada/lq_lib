# The LQ Digital Mode Family — Golden Test Vectors Suite

This directory contains the official **Golden Test Vectors Suite** for the **LQ Digital Mode Family** (`liblq`).

Third-party implementations in **Rust**, **Python**, **Go**, **Java/Kotlin (Android)**, **JavaScript/TypeScript (Web)**, **C#/.NET**, and **embedded DSP firmware** can import these golden vectors to verify full binary compliance with the LQ specification across both the message/PDU layer and the RF physical channel layer.

---

## 1. Directory Structure & Files

The golden vectors are organized into two specialized suites plus combined reference files:

| File | Format | Description |
|---|---|---|
| [`golden_vectors_encoding.json`](golden_vectors_encoding.json) | JSON | **Message Encoding & Decoding Suite**: 196 vectors focusing purely on text parsing, canonical formatting, 77-bit Huffman payload serialization, CRC-14 error detection, and LDPC(174,91) codewords. |
| [`golden_vectors_physical.json`](golden_vectors_physical.json) | JSON | **Physical Layer & RF Channel Suite**: 196 vectors detailing symbol-by-symbol RF timelines, exact timing ($t_{\text{start}}$, $t_{\text{end}}$, duration), Costas synchronization arrays, ramp guard symbols, tone assignments, and baseband frequency offsets. |
| [`golden_vectors_physical.txt`](golden_vectors_physical.txt) | Text | Clean, human-readable tabular representation of the physical layer symbol timelines. |
| [`golden_vectors.json`](golden_vectors.json) | JSON | Combined comprehensive suite covering all 13 message formats across all 4 modes (`LQ8`, `LQ16`, `LQ4`, `LQ2`). |
| [`golden_vectors_paper.json`](golden_vectors_paper.json) | JSON | Subset containing the 80 exact worked examples published in the official specification paper. |
| [`golden_vectors.txt`](golden_vectors.txt) | Text | Clean, human-readable summary of the combined vectors. |
| [`README.md`](README.md) | Markdown | Developer documentation and vector schema definitions. |

---

## 2. Test Vector JSON Schemas

### 2.1 Encoding & Decoding Vector Schema (`golden_vectors_encoding.json`)

Used to test message parsing, packing, 77-bit payload bitfields, CRC-14, and LDPC(174,91) FEC encoding:

```json
{
  "id": "GVE_CQ_STD_01_LQ8",
  "description": "Standard CQ with 4-char Maidenhead grid in LQ8",
  "mode": "LQ8",
  "message_type": "CQ_STD",
  "type_id": 1,
  "plaintext": "CQ HB9IPH JN47",
  "canonical_text": "CQ HB9IPH JN47",
  "is_paper_example": true,
  "payload_77bit_bin": "00000000000111101111100000101011001110000000000000000000000100010010001011000",
  "payload_10byte_hex": "00 1E F8 2B 38 00 00 11 22 C0",
  "crc14_val": 365,
  "crc14_hex": "0x016D",
  "crc14_bin": "00000101101101",
  "ldpc_input_91bit_bin": "0000000000011110111110000010101100111000000000000000000000010001001000101100000000101101101",
  "codeword_174bit_bin": "000000000001111011111000001010110011100000000000000000000001000100100010110000000010110110110100111111000100011010001110111001100001110110001101000111001101111000100001110001",
  "codeword_22byte_hex": "00 1E F8 2B 38 00 00 11 22 C0 2D B4 FC 46 8E E6 1D 8D 1C DE 21 C4"
}
```

### 2.2 Physical Layer Vector Schema (`golden_vectors_physical.json`)

Used by SDR apps, transceiver frontends, and DSP audio pipelines to test timing, sync blocks, and tone synthesis:

```json
{
  "id": "GVP_CQ_STD_01_LQ8",
  "description": "Standard CQ with 4-char Maidenhead grid in LQ8",
  "mode": "LQ8",
  "message_type": "CQ_STD",
  "type_id": 1,
  "plaintext": "CQ HB9IPH JN47",
  "canonical_text": "CQ HB9IPH JN47",
  "is_paper_example": true,
  "phy_parameters": {
    "slot_duration_s": 15.00,
    "tx_duration_s": 12.64,
    "guard_duration_s": 2.36,
    "symbol_period_s": 0.1600,
    "symbol_period_ms": 160.0,
    "tone_spacing_hz": 6.250000,
    "bandwidth_hz": 50.000000,
    "baud_rate": 6.250000,
    "modulation": "8-GFSK",
    "total_symbols": 79,
    "sync_symbols": 21,
    "data_symbols": 58,
    "ramp_symbols": 0
  },
  "costas_sync": {
    "pattern": [2, 5, 6, 1, 3, 0, 4],
    "sync_blocks": [
      {"block": 1, "start_symbol": 0, "end_symbol": 6, "symbols_count": 7},
      {"block": 2, "start_symbol": 36, "end_symbol": 42, "symbols_count": 7},
      {"block": 3, "start_symbol": 72, "end_symbol": 78, "symbols_count": 7}
    ]
  },
  "tones_count": 79,
  "tones": [2, 5, 6, 1, 3, 0, 4, 0, 0, 0, 1, 7, 2, ...],
  "symbol_timeline": [
    {
      "symbol_idx": 0,
      "start_time_s": 0.0000,
      "end_time_s": 0.1600,
      "duration_s": 0.1600,
      "tone": 2,
      "freq_offset_hz": 12.50,
      "type": "SYNC",
      "annotation": "Costas Sync 1 [0]"
    },
    {
      "symbol_idx": 1,
      "start_time_s": 0.1600,
      "end_time_s": 0.3200,
      "duration_s": 0.1600,
      "tone": 5,
      "freq_offset_hz": 31.25,
      "type": "SYNC",
      "annotation": "Costas Sync 1 [1]"
    },
    ...
  ]
}
```

---

## 3. Recommended Compliance Test Pipeline

Third-party implementations should execute the two compliance pipelines:

### Pipeline A: Message & Codec Compliance (`golden_vectors_encoding.json`)
1. **Text to Payload**: Parse `plaintext` into the 77-bit bitfield. Assert binary equality with `payload_77bit_bin` and hex equality with `payload_10byte_hex`.
2. **Payload to Text**: Unpack `payload_10byte_hex` back into text. Assert equality with `canonical_text`.
3. **CRC-14 Checksum**: Compute the 14-bit CRC over the 77-bit payload zero-extended to 82 bits using $G_{\text{CRC14}}(x) = \texttt{0x2757}$. Assert equality with `crc14_val`.
4. **LDPC(174,91) Encoding**: Encode the 91-bit block ($77\text{ payload} + 14\text{ CRC}$) via Gallager generator matrix $G_{83\times 91}$. Assert equality with `codeword_174bit_bin` and `codeword_22byte_hex`.

### Pipeline B: Physical Layer & RF Channel Compliance (`golden_vectors_physical.json`)
1. **Timing & Symbol Parameters**: Assert `slot_duration_s`, `tx_duration_s`, `symbol_period_s`, `tone_spacing_hz`, and `bandwidth_hz`.
2. **Costas Sync Alignment**:
   - **8-GFSK Modes (`LQ8`, `LQ16`)**: Verify 7-symbol Costas array $C_7 = [2, 5, 6, 1, 3, 0, 4]$ at symbol indices `0..6`, `36..42`, and `72..78`.
   - **4-GFSK Modes (`LQ4`, `LQ2`)**: Verify 4-symbol Costas arrays ($S_1=[0,2,3,1]$, $S_2=[1,3,2,0]$, $S_3=[2,0,1,3]$, $S_4=[3,1,0,2]$) at symbol indices `1..4`, `34..37`, `67..70`, and `100..103`.
3. **Ramp Symbols**: Verify 4-GFSK ramp guard symbols at index `0` ("Ramp Up") and index `104` ("Ramp Down") with tone index `0`.
4. **Timeline Contiguity**: Assert that every symbol starts exactly when the previous symbol ends ($t_{\text{start}}[i] = t_{\text{end}}[i-1] = i \cdot T_s$) and has duration $T_s$.
5. **Frequency Offsets**: Assert that baseband tone frequency offsets match $\Delta f_{\text{tone}} = \text{tone} \cdot \Delta f$.
6. **Reverse Demodulation**: Feed `tones` into the demodulator and confirm successful LDPC decoding and payload reconstruction matching `canonical_text`.

---

## 4. Regenerating & Verifying Vectors

To regenerate all golden vector suites:
```bash
cmake --build build --target generate_golden_vectors
build/generate_golden_vectors golden_vectors
```

To run the standalone CLI verification tool:
```bash
cmake --build build --target verify_golden_vectors
build/verify_golden_vectors
```

To run the full automated test suite with GoogleTest:
```bash
./run_tests.sh
```

---

## 5. License

MIT License — Copyright (c) 2026 Luis Quesada (HB9IPH). See [`LICENSE`](../LICENSE) for details.
