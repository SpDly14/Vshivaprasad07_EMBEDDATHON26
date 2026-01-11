# Priority Guardian – FreeRTOS MQTT System (ESP32)

## Overview

**Priority Guardian** is a FreeRTOS-based ESP32 application that demonstrates **priority-aware task scheduling**, **MQTT-based communication**, and **inter-task coordination using queues**.

The system listens to two MQTT streams:

1. A **continuous background data stream** (low priority)
2. A **critical distress signal** (high priority)

When a distress signal is received, it **preempts background processing**, immediately sends an **ACK response**, measures latency, and briefly activates a hardware LED to indicate the emergency event.

This project is designed to clearly showcase **real-time responsiveness**, **task prioritization**, and **deterministic behavior** in embedded systems.

---

## System Architecture

### Task Priority Model

| Task Name        | Priority   | Core   | Purpose                                 |
| ---------------- | ---------- | ------ | --------------------------------------- |
| Distress Handler | 3 (High)   | Core 1 | Handles emergency signals and sends ACK |
| MQTT Dispatcher  | 2 (Medium) | Core 0 | Maintains WiFi + MQTT communication     |
| Background Task  | 1 (Low)    | Core 0 | Processes background sensor/data stream |

FreeRTOS ensures that when a **distress message arrives**, the **Distress Handler task preempts all others**.

---

## Hardware Requirements

* ESP32 (ESP32-WROOM / DevKit)
* 1 LED connected to GPIO **32** (Distress Indicator)
* Active WiFi connection

---

## Software Stack

* **ESP32 Arduino Core**
* **FreeRTOS** (built-in with ESP32 Arduino)
* **WiFi.h** – Network connectivity
* **PubSubClient** – MQTT communication
* **ArduinoJson** – ACK message formatting

---

## MQTT Configuration

### Broker

```
broker.mqttdashboard.com:1883
```

### Topics Used

| Purpose         | Topic                           |
| --------------- | ------------------------------- |
| Background data | `krillparadise/data/stream`     |
| Distress signal | `shivaprasadvshivaprasad07`     |
| ACK response    | `manojkumar10b35vshivaprasad07` |

---

## Functional Description

### 1. WiFi & MQTT Initialization

* ESP32 connects to WiFi on startup
* MQTT client connects to the broker
* Subscribes to background and distress topics

---

### 2. Background Task (Low Priority)

* Receives floating-point values via MQTT
* Stores values in a **ring buffer (size 10)**
* Computes a **rolling average**
* Prints real-time values and averages to Serial Monitor

This task is **non-critical** and can be safely interrupted.

---

### 3. Distress Handler Task (High Priority)

* Triggered instantly when a distress message arrives
* Actions performed:

  1. Turns ON distress LED
  2. Captures ACK timestamp
  3. Publishes JSON ACK message
  4. Calculates end-to-end latency
  5. Turns OFF distress LED

**This task demonstrates deterministic response under load.**

---

### 4. Inter-Task Communication

* **FreeRTOS Queues** are used instead of shared variables

| Queue           | Data Type  | Used By                |
| --------------- | ---------- | ---------------------- |
| `bgQueue`       | `float`    | MQTT → Background Task |
| `distressQueue` | `uint32_t` | MQTT → Distress Task   |

Queues guarantee thread safety and avoid race conditions.

---

## JSON ACK Format

```json
{
  "status": "ACK",
  "timestamp_ms": 123456
}
```

---

## LED Behavior

* **OFF** → Normal operation
* **ON (brief)** → Distress signal received and processed

---

## Key Design Highlights

* True **priority preemption** using FreeRTOS
* Zero busy-waiting (blocking queues only)
* Core-pinned tasks for predictable scheduling
* Real-time latency measurement
* Clean separation of concerns

---

## How to Run

1. Open the project in Arduino IDE / PlatformIO
2. Select ESP32 board
3. Update WiFi credentials if required
4. Upload to ESP32
5. Monitor Serial output at **115200 baud**
6. Publish test messages to the MQTT topics

---

## Expected Output

* Continuous background value + rolling average logs
* Immediate ACK and latency print on distress event
* LED flashes during distress handling

---

## Use Case Relevance

This project maps directly to real-world systems such as:

* Emergency alert systems
* Safety-critical IoT devices
* Maritime or industrial distress signaling
* RTOS priority validation demos

---

## Author

**Team Name:** Vshivaprasad07

---

## License

Open for academic, learning, and demonstration purposes.
