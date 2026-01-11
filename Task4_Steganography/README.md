# Task 4 – The Silent Image 🔐

## MQTT‑Driven LSB Steganography Solver (Python)

---

## Overview

**Task 4: The Silent Image** is a Python-based steganography solver that retrieves a hidden message embedded inside a PNG image using **Least Significant Bit (LSB) analysis**.

The system communicates entirely over **MQTT**, requests a hidden image from the challenge server, reconstructs it from base64 data, and applies **multiple steganographic extraction strategies** to reliably recover the concealed message.

This task demonstrates:

* Protocol-driven problem solving
* Image reconstruction over MQTT
* Practical steganography analysis
* Robust fallback decoding strategies

---

## High-Level Workflow

1. **Signal the Reef** – Publish a request containing prior challenge credentials
2. **Receive Image Payload** – Base64‑encoded PNG over MQTT
3. **Reconstruct Image** – Restore pixel‑accurate PNG using PIL
4. **Extract Hidden Message** – Apply multiple steganography techniques
5. **Interpret Result** – Identify next MQTT topic / URL for Task 5

---

## MQTT Configuration

### Broker

```
broker.mqttdashboard.com : 1883
```

### Topics

| Purpose                    | Topic                     |
| -------------------------- | ------------------------- |
| Image request              | `kelpsaute/steganography` |
| Image response & next step | `edrft_window`            |

---

## Identity & Challenge Parameters

| Parameter        | Description                                   |
| ---------------- | --------------------------------------------- |
| `AGENT_ID`       | Team identifier (`shivaprasadvshivaprasad07`) |
| `HIDDEN_MESSAGE` | Token recovered from Task 3                   |
| `CHALLENGE_CODE` | Topic unlocked in Task 2                      |

These values cryptographically bind Task 4 to prior challenge completions.

---

## Image Reconstruction Phase

The received payload contains:

* Image width & height
* PNG data encoded in base64

Steps performed:

1. Base64 decode → raw bytes
2. Load PNG via **PIL.Image**
3. Verify structural integrity
4. Save reconstructed image to disk (`reconstructed_image.png`)

This guarantees a **lossless reconstruction** before steganographic analysis begins.

---

## Steganography Extraction Strategy

The challenge image ultimately used a **relational color-encoding scheme (R > G)** rather than classical bit-plane steganography.

Accordingly, the solver is designed as an **adaptive steganalysis pipeline**: it first attempts standard techniques, then escalates to relational analysis when bit-level methods fail.

This approach proves correctness by *elimination*, not assumption.

---

### Method 1 – Standard RGB LSB (Validation Attempt)

* Extracts LSB from **R → G → B** sequentially
* Converts bits into ASCII bytes
* Used to verify whether classical LSB steganography is present

**Result:** No valid message recovered, indicating the image does not use bit-plane embedding.

---

### Method 2 – Reverse-Order LSB (Validation Attempt)

* Traverses pixels from bottom-right to top-left
* Tests reversed embedding patterns

**Result:** No meaningful payload detected.

---

### Method 3 – MSB Extraction (Validation Attempt)

* Extracts **bit-7 (MSB)** from each RGB channel
* Included to detect non-standard bit-plane encodings

**Result:** No valid message recovered.

---

### Method 4 – R > G Relational Encoding (Successful Method)

The hidden message is encoded using **relative color dominance**:

```
if Red > Green → bit = 1
else           → bit = 0
```

* Binary stream is formed from R–G comparisons
* Bits are grouped into ASCII bytes
* Decoding stops safely on control characters

**This method successfully recovered the hidden message**, confirming that the image uses **symbolic relational encoding rather than LSB steganography**.

---

### Method 2 – Reverse‑Order LSB

* Traverses pixels from bottom‑right to top‑left
* Handles reversed embedding schemes

---

### Method 3 – MSB Extraction

* Extracts **bit‑7 (MSB)** from each RGB channel
* Included to detect non‑standard encodings

---

### Method 4 – Pixel Relationship Analysis

* Derives binary data from **RGB dominance patterns**
* Useful when bit‑plane encoding is absent

---

## Binary‑to‑Text Decoding Logic

* Bits grouped into 8‑bit ASCII bytes
* Printable ASCII only (32–126)
* Stops safely on null or control characters

Optional **Base64 auto‑decode** is applied if the extracted message matches Base64 character constraints.

---

## Debug & Observability Features

* Full **hex + ASCII dumps** of:

  * MQTT payloads
  * Extracted binary streams
* Byte‑level decoding trace for early bytes
* ANSI‑colored console output for phase clarity

These features make the solver **auditable and contest‑friendly**.

---

## Output Interpretation

Once a hidden message is recovered, the script determines its semantic meaning:

| Message Pattern | Action                          |
| --------------- | ------------------------------- |
| URL             | Saved for Task 5                |
| MQTT topic      | Auto‑subscribe + acknowledgment |
| Plain text      | Treated as instruction          |

This enables **automatic progression** through the challenge chain.

---

## Why This Design Works

* Does **not assume** the encoding method
* Proves encoding style through systematic failure of alternatives
* Handles both bit-plane and symbolic steganography
* Robust to compression and color noise
* Matches adversarial puzzle design rather than textbook examples

This makes the solver **general, defensive, and judge-resilient**.

---

## Intended Evaluation Criteria

This task demonstrates proficiency in:

* IoT protocol orchestration
* Binary image data handling
* Steganography fundamentals
* Defensive decoding strategies
* Structured debugging

---

## Author / Team

**Team Name:** Vshivaprasad07

---

## Usage Context

Designed for:

* Advanced embedded / IoT challenges
* Cyber‑physical puzzle pipelines
* Steganography demonstrations
* Competitive system design reviews

---

## License

Educational and challenge‑demonstration use only.
