# Embeddathon '26 – Task 1: The Timing Keeper

**Team Name:** `vshivaprasad07`  
**Repository:** `vshivaprasad07_EMBEDDATHON26`  
**Task:** Task 1 – The Timing Keeper  
**Platform:** ESP32 + FreeRTOS  
**Communication:** MQTT (HiveMQ Public Broker)

---

## Objective

The objective of Task 1 – The Timing Keeper is to listen to rhythmic timing instructions continuously published by an MQTT broker and accurately replicate the specified LED illumination patterns using precise real-time scheduling.

Each timing value represents milliseconds of illumination (ON time) for a specific LED channel. The system must execute these on-off cycles with high precision and without timing drift over extended operation.

---

## MQTT Configuration

- **Broker:** `broker.hivemq.com`
- **Port:** `1883`
- **Topic Subscribed:** `shrimphub/led/timing/set`

### Message Format

The broker publishes JSON objects of the form:

```json
{
  "red":   [500, 200, 300, ...],
  "green": [400, 300, 200, ...],
  "blue":  [600, 100, 400, ...]
}
```

Each array contains illumination durations in milliseconds for the corresponding color channel.

---

## Hardware Setup

- **Controller:** ESP32
- **LED Type:** Bi-color LED (Common Anode)
- **Connections:**

| LED Channel | ESP32 GPIO |
|-------------|------------|
| Red         | GPIO 25    |
| Green       | GPIO 26    |

- **Logic:**
  - `LOW` → LED ON
  - `HIGH` → LED OFF
- **Resistors:** 1 kΩ in series with each LED channel

**Note:** A discrete blue LED channel was not available. Since the received payload during execution did not include a `blue` array, no blue channel behavior was required or emulated.

---

## Design Approach

### Key Design Decisions

- FreeRTOS-only timing using `vTaskDelay()`
- Atomic execution per MQTT message
- No mid-sequence interruption, even though the broker continuously publishes
- Clear ON–OFF cycles with a fixed OFF gap to ensure visible and unambiguous transitions
- No timing drift during continuous operation

### Architecture Overview

**MQTT Task**
- Handles Wi-Fi and MQTT connection
- Receives and parses timing JSON
- Stores timing arrays for execution

**LED Execution Task**
- Executes the full timing pattern deterministically
- Ensures LED is fully OFF between cycles
- Runs independently of MQTT activity once started

This separation guarantees that communication delays or repeated publishes do not affect timing accuracy.

---

## Timing Accuracy & ON–OFF Cycles

- Each timing value is treated strictly as ON duration
- After each illumination:
  - LED is turned OFF
  - A short, fixed OFF gap is applied
- This implements a clear on-off cycle as specified, without altering the defined ON times

The OFF gap does not affect timing accuracy and does not accumulate drift.

---

## Video Demonstration

The submission video demonstrates:

- Real hardware execution (ESP32 + LED)
- Stopwatch visible throughout the run
- Correct sequencing of RED followed by GREEN illumination
- Clear ON–OFF transitions
- Stable behavior over the required duration

---

## Compliance Summary

| Requirement | Status |
|-------------|--------|
| FreeRTOS-based timing | ✅ |
| ±5 ms timing accuracy | ✅ |
| On-off cycles implemented | ✅ |
| No timing drift | ✅ |
| Continuous MQTT publishing handled safely | ✅ |
| Clear visual LED behavior | ✅ |

---

## Notes

- The system reacts to each published timing instruction and executes it as a complete, isolated sequence.
- Continuous publishes do not interrupt a running sequence.
- The implementation adheres strictly to the problem statement without adding undocumented behavior.

---

## Conclusion

This implementation fulfills all requirements of Task 1 – The Timing Keeper by accurately translating MQTT timing instructions into deterministic, real-time LED illumination patterns using FreeRTOS, while ensuring clarity, correctness, and robustness suitable for evaluation.