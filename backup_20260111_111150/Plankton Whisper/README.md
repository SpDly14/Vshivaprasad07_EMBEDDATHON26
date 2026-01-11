# The Plankton Whisper – FreeRTOS ESP32 Challenge

**Team Name:** Vshivaprasad07  
**Simulation Link (Wokwi):** [https://wokwi.com/projects/452673392606439425](https://wokwi.com/projects/452673392606439425)

---

## 📌 Overview

This project implements "The Plankton Whisper" challenge using FreeRTOS on ESP32. The system reacts to JSON commands sent via Serial by:

- Displaying a message on an OLED screen
- Adjusting the LED heartbeat speed dynamically
- Ensuring the heartbeat task never stops, even during display updates

The design strictly follows task separation, queue-based communication, and non-blocking RTOS principles.

---

## 🧠 System Architecture

The system is divided into three independent FreeRTOS tasks communicating through a queue.

### Task 1: The Ear (Input Task)

- Listens for JSON input via Serial
- Expected format:
  ```json
  {"msg":"Safe","delay":1000}
  ```
- Extracts message text and delay value
- Sends the data as a structured command to a FreeRTOS queue

### Task 2: The Face (OLED Task)

- Waits indefinitely for queue data
- Clears the OLED display
- Prints the received message in large text
- Updates a global variable to control heartbeat speed

### Task 3: The Heart (LED Task)

- Blinks an LED connected to GPIO 2
- ON time: 100 ms
- OFF time: Controlled by `currentDelay`
- Runs continuously and independently without blocking

---

## 🔧 Hardware Configuration

| Component      | Connection        |
|----------------|-------------------|
| ESP32 Board    | ESP32 Devkit V1   |
| OLED Display   | SSD1306 (I2C)     |
| OLED SDA       | GPIO 21           |
| OLED SCL       | GPIO 22           |
| LED            | GPIO 2            |

---

## 🧵 RTOS Design Highlights

- FreeRTOS queue used for inter-task communication
- Heartbeat task given highest priority
- Display task blocks safely using `portMAX_DELAY`
- No busy-waiting or blocking delays
- Safe operation on dual-core ESP32
- Skeleton code structure strictly followed

---

## 🧪 Test Commands

Use the Serial Monitor at **115200 baud** and send:

### Panic Mode
```json
{"msg":"DANGER!","delay":100}
```

### Calm Mode
```json
{"msg":"Safe Reef","delay":2000}
```

### Standard Mode
```json
{"msg":"Hello","delay":500}
```

---

## ✅ Expected Behavior

- OLED updates immediately with new message
- LED heartbeat speed changes according to delay value
- Heartbeat never pauses during screen updates
- System remains responsive to continuous commands

---

## 📂 Files Included

- `main.cpp` – Complete FreeRTOS implementation
- `demo_video.mp4` – System demonstration video
- Wokwi simulation link for verification

---

## 🚀 Getting Started

1. Open the [Wokwi simulation link](https://wokwi.com/projects/452673392606439425)
2. Start the simulation
3. Open the Serial Monitor
4. Send JSON commands to test the system
5. Observe the OLED display and LED behavior