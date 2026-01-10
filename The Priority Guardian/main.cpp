#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/* ================= CONFIG ================= */

#define WIFI_SSID     "SpDly"
#define WIFI_PASSWORD "12345678"

#define BROKER_HOST   "broker.mqttdashboard.com"
#define BROKER_PORT   1883

#define TOPIC_BG      "krillparadise/data/stream"
#define TOPIC_DISTRESS "shivaprasadvshivaprasad07"
#define TOPIC_ACK     "manojkumar10b35vshivaprasad07"

/* ================= HARDWARE ================= */

#define DISTRESS_LED  32    // LED ON during distress
#define LED_ON        HIGH
#define LED_OFF       LOW

/* ================= GLOBALS ================= */

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

/* Queues */
QueueHandle_t bgQueue;
QueueHandle_t distressQueue;

/* Rolling average */
float ring[10];
uint8_t ringIndex = 0;
uint8_t ringCount = 0;

/* ================= WIFI ================= */

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

/* ================= MQTT CALLBACK ================= */

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  // Background stream
  if (strcmp(topic, TOPIC_BG) == 0) {
    float value = atof((char*)payload);
    xQueueSend(bgQueue, &value, 0);
    return;
  }

  // Distress signal
  if (strcmp(topic, TOPIC_DISTRESS) == 0) {
    uint32_t t = millis();
    xQueueSend(distressQueue, &t, 0);
    return;
  }

  // ---------- INFO / NEXT-TASK LOGGING ----------
  Serial.println("================================");
  Serial.print("[INFO] MQTT message received\n");
  Serial.print("[INFO] Topic   : ");
  Serial.println(topic);
  Serial.print("[INFO] Payload : ");

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("================================");
}


/* ================= MQTT ================= */

void connectMQTT() {
  while (!mqttClient.connected()) {
    mqttClient.connect("task2_priority_guardian");
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  mqttClient.subscribe(TOPIC_BG);
  mqttClient.subscribe(TOPIC_DISTRESS);
}

/* ================= TASKS ================= */

// Priority 2 – MQTT Dispatcher
void mqttTask(void* pv) {
  connectWiFi();
  mqttClient.setServer(BROKER_HOST, BROKER_PORT);
  mqttClient.setCallback(mqttCallback);
  connectMQTT();

  for (;;) {
    mqttClient.loop();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// Priority 1 – Background Chorus
void backgroundTask(void* pv) {
  float val;

  for (;;) {
    if (xQueueReceive(bgQueue, &val, portMAX_DELAY)) {

      ring[ringIndex] = val;
      ringIndex = (ringIndex + 1) % 10;
      if (ringCount < 10) ringCount++;

      float sum = 0;
      for (uint8_t i = 0; i < ringCount; i++) sum += ring[i];
      float avg = sum / ringCount;

      Serial.print("[BG] Value: ");
      Serial.print(val);
      Serial.print("  Avg: ");
      Serial.println(avg, 2);
    }
  }
}

// Priority 3 – Distress Handler
void distressTask(void* pv) {
  uint32_t rxTime;

  for (;;) {
    if (xQueueReceive(distressQueue, &rxTime, portMAX_DELAY)) {

      digitalWrite(DISTRESS_LED, LED_ON);

      uint32_t ackTime = millis();

      StaticJsonDocument<128> doc;
      doc["status"] = "ACK";
      doc["timestamp_ms"] = ackTime;

      char buffer[128];
      serializeJson(doc, buffer);
      mqttClient.publish(TOPIC_ACK, buffer);

      Serial.print("[DISTRESS] RX: ");
      Serial.print(rxTime);
      Serial.print("  ACK: ");
      Serial.print(ackTime);
      Serial.print("  Latency: ");
      Serial.print(ackTime - rxTime);
      Serial.println(" ms");

      digitalWrite(DISTRESS_LED, LED_OFF);
    }
  }
}

/* ================= SETUP ================= */

void setup() {
  Serial.begin(115200);

  pinMode(DISTRESS_LED, OUTPUT);
  digitalWrite(DISTRESS_LED, LED_OFF);

  bgQueue = xQueueCreate(20, sizeof(float));
  distressQueue = xQueueCreate(5, sizeof(uint32_t));

  xTaskCreatePinnedToCore(backgroundTask, "BG", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(mqttTask, "MQTT", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(distressTask, "DISTRESS", 4096, NULL, 3, NULL, 1);
}

void loop() {
}
