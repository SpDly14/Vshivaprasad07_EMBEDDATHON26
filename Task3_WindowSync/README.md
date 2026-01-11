# ESP32 MQTT Window Synchronizer (FreeRTOS, Dual‑Core)

## Project Overview

This project implements an **automatic time‑window synchronization system** on an ESP32 using **FreeRTOS with explicit dual‑core task partitioning**.

The ESP32 listens for a *time‑critical “window open” event* over MQTT and **automatically responds within a tightly controlled timing tolerance**. The system is derived from a Python‑based simulator and faithfully reproduces its timing semantics on real hardware.

The design emphasizes:

* Deterministic response timing
* Clear task isolation across ESP32 cores
* Robust inter‑task synchronization
* Transparent debugging via raw MQTT dumps

---

## High‑Level Concept

A remote controller publishes messages indicating when a **response window opens and closes**.

When the window opens:

1. The ESP32 timestamps the event
2. Automatically schedules a response **25 ms later**
3. Publishes a synchronization message
4. Verifies timing is within **±50 ms tolerance**

After **3 successful synchronizations**, the system waits for a **special Task‑4 challenge code**, delivered via MQTT.

---

## Core Allocation Strategy

| Core       | Responsibility                                                   |
| ---------- | ---------------------------------------------------------------- |
| **Core 0** | WiFi + MQTT handling, window detection, auto‑response scheduling |
| **Core 1** | Button monitoring (manual override), LED state control           |

This separation ensures:

* Network jitter never blocks timing‑critical logic
* UI and GPIO handling remain responsive

---

## Hardware Configuration

### GPIO Assignments (Right‑side ESP32 pins)

| Component | GPIO  | Notes                                               |
| --------- | ----- | --------------------------------------------------- |
| Button    | 13    | Internal pull‑up enabled (optional manual override) |
| Green LED | 12    | Common‑anode bi‑color LED (LOW = ON)                |
| Red LED   | 14    | Common‑anode bi‑color LED (LOW = ON)                |
| LED Anode | 3.3 V | Via current‑limiting resistors (220 Ω)              |

---

## LED State Semantics

| LED State                | Meaning                             |
| ------------------------ | ----------------------------------- |
| **Red ON**               | Waiting for window                  |
| **Green ON**             | Window open (auto‑response pending) |
| **Yellow (Red + Green)** | Successful synchronization          |

---

## MQTT Configuration

### Broker

```
broker.mqttdashboard.com : 1883
```

### Topics

| Purpose                     | Topic                  |
| --------------------------- | ---------------------- |
| Window control              | `edrft_window`         |
| Sync response / Task‑4 code | `cagedmonkey/listener` |

---

## Timing Parameters

| Parameter           | Value  |
| ------------------- | ------ |
| Sync tolerance      | ±50 ms |
| Auto‑response delay | 25 ms  |
| Required syncs      | 3      |
| Button debounce     | 20 ms  |

These constants are **compile‑time deterministic** and not adjusted dynamically, ensuring predictable real‑time behavior.

---

## FreeRTOS Synchronization Design

### Shared State Protection

* `windowStateMutex` → protects window open/close state and timestamps
* `syncCountMutex` → protects successful sync counter

No shared variable is accessed without mutex protection, preventing race conditions across cores.

---

## Window Detection Logic

Incoming MQTT payloads are **raw‑dumped** and parsed using **keyword detection** (case‑insensitive):

### Window Open Triggers

* `bloom`
* `open`
* `corals bloom`

### Window Close Triggers

* `close`
* `krill`
* `reefing krills`

This design mirrors the Python simulator’s symbolic messaging model rather than relying on rigid JSON formats.

---

## Automatic Response Mechanism

When a window opens:

1. Timestamp captured (`windowOpenTime`)
2. Auto‑response task dynamically spawned
3. Task delays exactly **25 ms**
4. Timestamped sync message published

This approach avoids blocking the MQTT callback while preserving accurate timing.

---

## Sync Validation & Feedback

A synchronization is considered successful if:

```
|response_time − window_open_time| ≤ 50 ms
```

On success:

* Sync message is published
* Yellow LED flashes 3 times
* Sync counter increments

After 3 successes, the system enters **Task‑4 waiting mode**.

---

## JSON Response Format

```json
{
  "status": "synced",
  "timestamp_ms": 123456
}
```

Payloads are logged both as **ASCII** and **hex dumps** for verification.

---

## Manual Override (Optional)

A physical button allows manual triggering:

* Valid only while the window is open
* Subject to the same ±50 ms tolerance
* Useful for testing and demonstration

---

## Why This Design Works

* True **real‑time responsiveness** via FreeRTOS scheduling
* Network‑heavy tasks isolated from timing‑critical logic
* Deterministic delays (no `delay()` calls)
* Clean mapping from simulator → hardware
* Clear visual + serial observability

---

## Intended Evaluation Criteria

This project demonstrates:

* Dual‑core FreeRTOS mastery
* MQTT protocol handling under timing constraints
* Safe concurrency with mutexes
* Real‑world embedded synchronization patterns

---

## Author / Team

**Team Name:** Vshivaprasad07

---

## Usage Context

Designed for:

* Embedded systems challenges
* RTOS demonstrations
* Time‑sensitive IoT protocols
* Interview and portfolio review

---

## License

Educational and demonstration use.
