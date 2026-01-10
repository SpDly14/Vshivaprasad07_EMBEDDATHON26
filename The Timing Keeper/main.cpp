cpp
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/* ================= CONFIG ================= */

#define WIFI_SSID     "SpDly"
#define WIFI_PASSWORD "12345678"
#define MQTT_BROKER   "broker.hivemq.com"
#define TIMING_TOPIC  "shrimphub/led/timing/set"

/* ================= HARDWARE ================= */

#define LED_RED     32
#define LED_GREEN   26

#define LED_ON   LOW     // common anode
#define LED_OFF  HIGH

#define OFF_GAP_MS 50    // visible OFF between cycles

/* ================= GLOBALS ================= */

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

volatile bool newSequenceAvailable = false;
volatile bool executing = false;

uint32_t redTimings[16];
uint32_t greenTimings[16];
uint8_t redCount = 0;
uint8_t greenCount = 0;

/* ================= WIFI ================= */

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

/* ================= MQTT CALLBACK ================= */

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (executing) return;

  StaticJsonDocument<1024> doc;
  if (deserializeJson(doc, payload, length)) return;

  redCount = 0;
  greenCount = 0;

  if (doc.containsKey("red")) {
    for (uint32_t d : doc["red"].as<JsonArray>()) {
      if (redCount < 16) redTimings[redCount++] = d;
    }
  }

  if (doc.containsKey("green")) {
    for (uint32_t d : doc["green"].as<JsonArray>()) {
      if (greenCount < 16) greenTimings[greenCount++] = d;
    }
  }

  newSequenceAvailable = true;
}

/* ================= MQTT ================= */

void connectMQTT() {
  while (!mqttClient.connected()) {
    mqttClient.connect("task1_timing_keeper");
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  mqttClient.subscribe(TIMING_TOPIC);
}

/* ================= TASKS ================= */

void mqttTask(void* pv) {
  connectWiFi();
  mqttClient.setServer(MQTT_BROKER, 1883);
  mqttClient.setCallback(mqttCallback);
  connectMQTT();

  for (;;) {
    mqttClient.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void ledTask(void* pv) {
  for (;;) {

    if (newSequenceAvailable) {
      executing = true;
      newSequenceAvailable = false;

      /* ---- RED SEQUENCE ---- */
      for (uint8_t i = 0; i < redCount; i++) {
        digitalWrite(LED_RED, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(redTimings[i]));
        digitalWrite(LED_RED, LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(OFF_GAP_MS));
      }

      /* ---- COLOR SEPARATION ---- */
      digitalWrite(LED_RED, LED_OFF);
      digitalWrite(LED_GREEN, LED_OFF);
      vTaskDelay(pdMS_TO_TICKS(OFF_GAP_MS));

      /* ---- GREEN SEQUENCE ---- */
      for (uint8_t i = 0; i < greenCount; i++) {
        digitalWrite(LED_GREEN, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(greenTimings[i]));
        digitalWrite(LED_GREEN, LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(OFF_GAP_MS));
      }

      digitalWrite(LED_RED, LED_OFF);
      digitalWrite(LED_GREEN, LED_OFF);

      executing = false;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/* ================= SETUP ================= */

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_RED, LED_OFF);
  digitalWrite(LED_GREEN, LED_OFF);

  xTaskCreatePinnedToCore(mqttTask, "MQTT_TASK", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(ledTask,  "LED_TASK",  2048, NULL, 1, NULL, 1);
}

void loop() {
  /* RTOS only */
}
