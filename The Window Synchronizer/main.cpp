/*
 * ESP32 MQTT Window Synchronizer with FreeRTOS Dual-Core
 * Based on Python simulator - automatically responds within tolerance
 * 
 * Core 0: MQTT handling and window monitoring
 * Core 1: Button monitoring, LED control, and auto-response
 * 
 * Hardware Setup (Right side of ESP32 only):
 * - Button: GPIO 13 (with internal pull-up) - Optional for manual override
 * - Bi-color LED (Common Anode):
 *   - Common Anode: 3.3V
 *   - Green cathode: GPIO 12 (via 220Ω resistor)
 *   - Red cathode: GPIO 14 (via 220Ω resistor)
 * 
 * LED Indicators:
 * - Red ON: Waiting for window
 * - Green ON: Window is open (auto-responding...)
 * - Yellow (both): Successful sync flash
 */

#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials - UPDATE THESE
const char* WIFI_SSID = "SpDly";
const char* WIFI_PASSWORD = "12345678";

// MQTT Configuration
const char* BROKER_HOST = "broker.mqttdashboard.com";
const int BROKER_PORT = 1883;
const char* WINDOW_TOPIC = "edrft_window"; 
const char* LISTENER_TOPIC = "cagedmonkey/listener";
const char* CLIENT_ID = "ESP32_Auto_Sync_v2";

const int BUTTON_PIN = 13; 
const int LED_GREEN = 12;      // Green cathode (window open)
const int LED_RED = 14;        // Red cathode (waiting)

// Timing constants
const unsigned long DEBOUNCE_MS = 20;
const unsigned long SYNC_TOLERANCE_MS = 50;  // ±50ms
const unsigned long AUTO_RESPONSE_DELAY_MS = 25;  // Respond 25ms after window opens
const int REQUIRED_SYNCS = 3;

// Inter-core communication
volatile bool windowOpen = false;
volatile unsigned long windowOpenTime = 0;
volatile unsigned long windowCloseTime = 0;
volatile int syncCount = 0;
volatile bool autoRespondTriggered = false;
volatile bool waitingForCode = false;

// Semaphores and mutexes
SemaphoreHandle_t windowStateMutex;
SemaphoreHandle_t syncCountMutex;

// Task handles
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t buttonTaskHandle = NULL;

// MQTT client
WiFiClient espClient;
PubSubClient mqtt(espClient);

// LED control functions (Common Anode - LOW = ON, HIGH = OFF)
void setLED(bool green, bool red) {
  digitalWrite(LED_GREEN, green ? LOW : HIGH);
  digitalWrite(LED_RED, red ? LOW : HIGH);
}

void flashYellow(int count, int duration) {
  for (int i = 0; i < count; i++) {
    setLED(true, true);  // Both ON = Yellow
    vTaskDelay(pdMS_TO_TICKS(duration));
    setLED(false, false);  // Both OFF
    vTaskDelay(pdMS_TO_TICKS(duration));
  }
}

void updateLEDState() {
  if (xSemaphoreTake(windowStateMutex, portMAX_DELAY)) {
    if (windowOpen) {
      setLED(true, false);  // Green only - window open
    } else {
      setLED(false, true);  // Red only - waiting
    }
    xSemaphoreGive(windowStateMutex);
  }
}

unsigned long getTimestampMs() {
  return millis();
}

// WiFi connection
void setupWiFi() {
  Serial.println("\n╔════════════════════════════════════════════════════╗");
  Serial.println("║     ESP32 Window Synchronizer - AUTO MODE v2.0    ║");
  Serial.println("║       Automatically responds within 50ms           ║");
  Serial.println("╚════════════════════════════════════════════════════╝\n");
  
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }
  
  Serial.println("\n✓ WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void sendSyncMessage(unsigned long buttonPressTime, unsigned long winOpenTime) {
  unsigned long timeDiff = abs((long)(buttonPressTime - winOpenTime));
  
  Serial.println("\n╔════════════════════════════════════════════════════╗");
  Serial.println("║          ✓✓✓ SUCCESSFUL SYNC! ✓✓✓                 ║");
  Serial.println("╚════════════════════════════════════════════════════╝");
  Serial.print("Button press time: ");
  Serial.print(buttonPressTime);
  Serial.println(" ms");
  Serial.print("Window open time:  ");
  Serial.print(winOpenTime);
  Serial.println(" ms");
  Serial.print("Time difference:   ");
  Serial.print(timeDiff);
  Serial.println(" ms");
  Serial.print("Tolerance:         ±");
  Serial.print(SYNC_TOLERANCE_MS);
  Serial.println(" ms");
  
  // Create raw JSON response
  char jsonBuffer[128];
  snprintf(jsonBuffer, sizeof(jsonBuffer), 
           "{\"status\":\"synced\",\"timestamp_ms\":%lu}", 
           buttonPressTime);
  
  // Publish sync message
  bool published = mqtt.publish(LISTENER_TOPIC, jsonBuffer);
  
  if (published) {
    Serial.println("\n✓ Sync message published successfully!");
    
    // Dump raw response
    Serial.println("\n--- RAW RESPONSE SENT ---");
    Serial.print("Topic: ");
    Serial.println(LISTENER_TOPIC);
    Serial.print("Payload: ");
    Serial.println(jsonBuffer);
    Serial.print("Payload (hex): ");
    for (int i = 0; jsonBuffer[i] != '\0'; i++) {
      Serial.printf("%02X ", (byte)jsonBuffer[i]);
    }
    Serial.println();
    Serial.println("--- END RAW RESPONSE ---\n");
    
    if (xSemaphoreTake(syncCountMutex, portMAX_DELAY)) {
      syncCount++;
      int currentCount = syncCount;
      xSemaphoreGive(syncCountMutex);
      
      Serial.print("📊 Total successful syncs: ");
      Serial.print(currentCount);
      Serial.print("/");
      Serial.println(REQUIRED_SYNCS);
      
      // Flash yellow LED
      flashYellow(3, 200);
      
      if (currentCount >= REQUIRED_SYNCS) {
        Serial.println("\n════════════════════════════════════════════════════");
        Serial.println("║  🎉 MISSION COMPLETE! 🎉                          ║");
        Serial.println("║  3 SUCCESSFUL SYNCS ACHIEVED!                     ║");
        Serial.println("════════════════════════════════════════════════════\n");
        Serial.println("Waiting for your Task 4 challenge code...\n");
        waitingForCode = true;
      }
    }
  } else {
    Serial.println("✗ Failed to publish sync message");
  }
  
  Serial.println("────────────────────────────────────────────────────\n");
}

// Auto-respond when window opens
void autoRespond(unsigned long winOpenTime) {
  // Wait for auto response delay
  vTaskDelay(pdMS_TO_TICKS(AUTO_RESPONSE_DELAY_MS));
  
  unsigned long buttonPressTime = getTimestampMs();
  
  Serial.println("\n🤖 AUTO BUTTON PRESS");
  Serial.print("Button pressed at: ");
  Serial.print(buttonPressTime);
  Serial.println(" ms");
  
  unsigned long timeDiff = buttonPressTime - winOpenTime;
  Serial.print("✓ Perfect timing! (");
  Serial.print(timeDiff);
  Serial.println(" ms)");
  
  sendSyncMessage(buttonPressTime, winOpenTime);
  
  if (xSemaphoreTake(windowStateMutex, portMAX_DELAY)) {
    autoRespondTriggered = false;
    xSemaphoreGive(windowStateMutex);
  }
}

// Check if string contains keyword (case-insensitive)
bool containsKeyword(const char* str, const char* keyword) {
  int strLen = strlen(str);
  int keyLen = strlen(keyword);
  
  for (int i = 0; i <= strLen - keyLen; i++) {
    bool match = true;
    for (int j = 0; j < keyLen; j++) {
      char c1 = tolower(str[i + j]);
      char c2 = tolower(keyword[j]);
      if (c1 != c2) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

// MQTT callback - dumps raw data and detects window state
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string
  char payloadStr[length + 1];
  memcpy(payloadStr, payload, length);
  payloadStr[length] = '\0';
  
  // Dump raw data
  Serial.println("\n--- RAW MQTT DATA RECEIVED ---");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Length: ");
  Serial.println(length);
  Serial.print("Payload (hex): ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.printf("%02X ", payload[i]);
  }
  Serial.println();
  Serial.print("Payload (string): ");
  Serial.println(payloadStr);
  Serial.println("--- END RAW DATA ---\n");
  
  // Check if this is Task 4 code (after 3 syncs)
  if (waitingForCode && strcmp(topic, LISTENER_TOPIC) == 0) {
    Serial.println("╔════════════════════════════════════════════════════╗");
    Serial.println("║     🎁 SPECIAL MESSAGE RECEIVED! 🎁               ║");
    Serial.println("╚════════════════════════════════════════════════════╝");
    Serial.print("TASK 4 CODE: ");
    Serial.println(payloadStr);
    Serial.println("────────────────────────────────────────────────────\n");
    Serial.println(">>> Save this code for Task 4! <<<\n");
    return;
  }
  
  // Process window topic
  if (strcmp(topic, WINDOW_TOPIC) == 0) {
    bool isOpen = false;
    bool isClosed = false;
    
    // Check for window open keywords
    if (containsKeyword(payloadStr, "bloom") || 
        containsKeyword(payloadStr, "open") ||
        containsKeyword(payloadStr, "corals bloom")) {
      isOpen = true;
      Serial.println("[DEBUG] Detected WINDOW OPEN keyword in message");
    }
    // Check for window close keywords
    else if (containsKeyword(payloadStr, "close") ||
             containsKeyword(payloadStr, "krill") ||
             containsKeyword(payloadStr, "reefing krills")) {
      isClosed = true;
      Serial.println("[DEBUG] Detected WINDOW CLOSED keyword in message");
    }
    
    if (xSemaphoreTake(windowStateMutex, portMAX_DELAY)) {
      if (isOpen && !windowOpen) {
        windowOpen = true;
        windowOpenTime = getTimestampMs();
        autoRespondTriggered = true;
        
        Serial.println("\n╔════════════════════════════════════════════════════╗");
        Serial.println("║          >>> WINDOW OPENED <<<                     ║");
        Serial.println("╚════════════════════════════════════════════════════╝");
        Serial.print("Window opened at: ");
        Serial.print(windowOpenTime);
        Serial.println(" ms");
        Serial.print("● LED: GREEN - Auto-responding in ");
        Serial.print(AUTO_RESPONSE_DELAY_MS);
        Serial.println("ms...");
        Serial.println("────────────────────────────────────────────────────");
        
        // Trigger auto-response in separate context
        xSemaphoreGive(windowStateMutex);
        
        // Schedule auto-response
        unsigned long capturedTime = windowOpenTime;
        xTaskCreate(
          [](void* param) {
            unsigned long* pTime = (unsigned long*)param;
            autoRespond(*pTime);
            vTaskDelete(NULL);
          },
          "AutoRespond",
          4096,
          &capturedTime,
          1,
          NULL
        );
        return;
        
      } else if (isClosed && windowOpen) {
        windowCloseTime = getTimestampMs();
        Serial.println("\n╔════════════════════════════════════════════════════╗");
        Serial.println("║          <<< WINDOW CLOSED >>>                     ║");
        Serial.println("╚════════════════════════════════════════════════════╝");
        Serial.println("● LED: RED - Waiting for next window...");
        Serial.println("────────────────────────────────────────────────────\n");
        windowOpen = false;
        autoRespondTriggered = false;
      }
      xSemaphoreGive(windowStateMutex);
    }
  }
}

// MQTT connection
void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    
    if (mqtt.connect(CLIENT_ID)) {
      Serial.println("connected");
      
      // Subscribe to window topic
      mqtt.subscribe(WINDOW_TOPIC);
      Serial.print("✓ Subscribed to: ");
      Serial.println(WINDOW_TOPIC);
      
      // Subscribe to listener topic for Task 4 code
      mqtt.subscribe(LISTENER_TOPIC);
      Serial.print("✓ Subscribed to: ");
      Serial.print(LISTENER_TOPIC);
      Serial.println(" (for Task 4 code)");
      
      Serial.println("\n● LED: RED - Waiting for window...\n");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retrying in 5s");
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

// MQTT Task - Runs on Core 0
void mqttTask(void* parameter) {
  Serial.print("MQTT Task running on core: ");
  Serial.println(xPortGetCoreID());
  
  setupWiFi();
  mqtt.setServer(BROKER_HOST, BROKER_PORT);
  mqtt.setCallback(mqttCallback);
  connectMQTT();
  
  Serial.println("\n📡 MQTT Configuration:");
  Serial.print("   Broker: ");
  Serial.print(BROKER_HOST);
  Serial.print(":");
  Serial.println(BROKER_PORT);
  Serial.print("   Window Topic: ");
  Serial.println(WINDOW_TOPIC);
  Serial.print("   Listener Topic: ");
  Serial.println(LISTENER_TOPIC);
  Serial.println("\n⚙️ Settings:");
  Serial.println("   Mode: AUTOMATIC");
  Serial.print("   Response delay: ");
  Serial.print(AUTO_RESPONSE_DELAY_MS);
  Serial.println(" ms");
  Serial.print("   Tolerance: ±");
  Serial.print(SYNC_TOLERANCE_MS);
  Serial.println(" ms");
  Serial.print("   Required syncs: ");
  Serial.println(REQUIRED_SYNCS);
  Serial.println("\n────────────────────────────────────────────────────\n");
  
  for (;;) {
    if (!mqtt.connected()) {
      connectMQTT();
    }
    mqtt.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Button Task - Runs on Core 1 (optional manual override)
void buttonTask(void* parameter) {
  Serial.print("Button Task running on core: ");
  Serial.println(xPortGetCoreID());
  
  bool lastButtonState = HIGH;
  unsigned long lastDebounceTime = 0;
  bool buttonPressed = false;
  
  for (;;) {
    // Update LED state
    updateLEDState();
    
    // Read button (optional manual override)
    int reading = digitalRead(BUTTON_PIN);
    
    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
      if (reading != buttonPressed) {
        buttonPressed = reading;
        
        if (buttonPressed == LOW) {  // Manual button press
          unsigned long pressTime = getTimestampMs();
          
          Serial.println("\n🔘 MANUAL BUTTON PRESSED");
          Serial.print("Pressed at: ");
          Serial.print(pressTime);
          Serial.println(" ms");
          
          bool isWindowOpen;
          unsigned long winOpenTime;
          
          if (xSemaphoreTake(windowStateMutex, portMAX_DELAY)) {
            isWindowOpen = windowOpen;
            winOpenTime = windowOpenTime;
            xSemaphoreGive(windowStateMutex);
          }
          
          if (isWindowOpen) {
            unsigned long timeDiff = abs((long)(pressTime - winOpenTime));
            if (timeDiff <= SYNC_TOLERANCE_MS) {
              Serial.print("✓ Within tolerance! (");
              Serial.print(timeDiff);
              Serial.println(" ms)");
              sendSyncMessage(pressTime, winOpenTime);
            } else {
              Serial.print("✗ Too late! Time difference: ");
              Serial.print(timeDiff);
              Serial.print(" ms (tolerance: ±");
              Serial.print(SYNC_TOLERANCE_MS);
              Serial.println(" ms)");
            }
          } else {
            Serial.println("✗ No window open! Wait for GREEN LED.");
          }
        }
      }
    }
    
    lastButtonState = reading;
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  // Configure GPIO
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  // Initialize LEDs (Red ON = waiting)
  setLED(false, true);
  
  // Create mutexes
  windowStateMutex = xSemaphoreCreateMutex();
  syncCountMutex = xSemaphoreCreateMutex();
  
  // Create tasks on specific cores
  xTaskCreatePinnedToCore(
    mqttTask,
    "MQTT_Task",
    10240,
    NULL,
    2,
    &mqttTaskHandle,
    0  // Core 0
  );
  
  xTaskCreatePinnedToCore(
    buttonTask,
    "Button_Task",
    4096,
    NULL,
    1,
    &buttonTaskHandle,
    1  // Core 1
  );
  
  Serial.println("Tasks created:");
  Serial.println("  Core 0: MQTT monitoring & auto-response");
  Serial.println("  Core 1: Button (manual override) & LED control\n");
}

void loop() {
  // Empty - all work done in tasks
  vTaskDelay(portMAX_DELAY);
}