# Task 2: The Priority Guardian

## 📋 Task Overview

Dual-priority system handling rolling averages and urgent distress signals.

**Status:** ✅ Completed

---

## 🎯 Objectives

- [List specific objectives from the task description]
- [What needed to be accomplished]
- [Success criteria]

---

## 🔌 Hardware Setup

### Components Used
- ESP32 Development Board

### Pin Connections
```
[Component] -> ESP32 Pin
Example:
Red LED   -> GPIO 25 (via 220Ω resistor)
Green LED -> GPIO 26 (via 220Ω resistor)
Blue LED  -> GPIO 27 (via 220Ω resistor)
```

### Circuit Diagram
![Circuit Diagram](../docs/task2_circuit.png)

---

## 💻 Software Architecture

### Task Structure
```
[Describe your FreeRTOS task structure]
- Task priorities
- Task responsibilities
- Inter-task communication
```

### Key Functions
```cpp
// Main function descriptions
void taskFunction() {
    // Purpose and logic
}
```

### Data Flow
```
[Describe how data flows through your system]
MQTT → Processing → Action → Response
```

---

## 🚀 Implementation Details

### Approach
[Explain your implementation approach]

### Algorithm/Logic
[Describe the algorithm or logic used]

### Challenges Faced
1. **Challenge:** [Description]
   - **Solution:** [How you solved it]

2. **Challenge:** [Description]
   - **Solution:** [How you solved it]

---

## 📊 Results & Performance

### Metrics Achieved
| Metric | Target | Achieved | Status |
|--------|--------|----------|--------|
| [Metric 1] | [Value] | [Value] | ✅/❌ |
| [Metric 2] | [Value] | [Value] | ✅/❌ |

### Serial Output Sample
```
[Paste relevant serial output showing successful operation]
```

### Screenshots/Logs
- [Link to or embed relevant screenshots]
- [Link to log files]

---

## 🎥 Video Evidence

**Video Link:** [INSERT_VIDEO_LINK]

**Video Contents:**
- [ ] Functionality demonstrated
- [ ] Stopwatch visible (if required)
- [ ] Serial monitor showing relevant data
- [ ] LED indicators working correctly
- [ ] Timing requirements met
- [ ] [Any other specific requirements]

---

## 📁 Files in This Directory

- `*.ino` - Main Arduino sketch
- `config.h` - Configuration file (WiFi, MQTT credentials)
- `*.h` - Additional header files
- `logs.txt` - Execution logs and test results
- `README.md` - This file

---

## 🛠️ Build Instructions

### Prerequisites
```bash
# List any specific libraries or dependencies for this task
```

### Compilation
```bash
# Arduino IDE: Open .ino file and upload
# OR
# PlatformIO:
cd Task2_ThePriorityGuardian
pio run --target upload
pio device monitor
```

### Testing
1. [Step-by-step testing procedure]
2. [Expected results]
3. [How to verify success]

---

## 🔍 Code Walkthrough

### Main Loop
```cpp
void loop() {
    // [Explain what happens in the main loop]
}
```

### Key Code Sections
```cpp
// [Include and explain important code snippets]
```

---

## 📝 Notes & Observations

### What Worked Well
- [Things that went smoothly]

### What Could Be Improved
- [Areas for potential improvement]

### Learning Points
- [Key learnings from this task]

---

**Task Completion Date:** [DATE]  
**Time Spent:** [HOURS]  
**Iterations:** [NUMBER]
