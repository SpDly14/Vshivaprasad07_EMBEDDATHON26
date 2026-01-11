#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

QueueHandle_t commandQueue;
TaskHandle_t t_Input;
TaskHandle_t t_Display;
TaskHandle_t t_Blink;

volatile int currentDelay = 1000;

struct Command {
  char text[20];
  int blinkRate;
};

void HeartTask(void *pvParameters) {
  pinMode(2, OUTPUT);
  for (;;) {
    digitalWrite(2, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(2, LOW);
    vTaskDelay(currentDelay / portTICK_PERIOD_MS);
  }
}

void InputTask(void *pvParameters) {
  Serial.begin(115200);
  for (;;) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      StaticJsonDocument<200> doc;
      if (deserializeJson(doc, input) == DeserializationError::Ok) {
        Command cmd;
        strlcpy(cmd.text, doc["msg"], sizeof(cmd.text));
        cmd.blinkRate = doc["delay"];
        xQueueSend(commandQueue, &cmd, portMAX_DELAY);
        Serial.println("Command sent to queue!");
      } else {
        Serial.println("JSON Error");
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void DisplayTask(void *pvParameters) {
  Command receivedCmd;
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("Waiting...");
  display.display();
  for (;;) {
    if (xQueueReceive(commandQueue, &receivedCmd, portMAX_DELAY) == pdTRUE) {
      currentDelay = receivedCmd.blinkRate;
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println(receivedCmd.text);
      display.display();
      Serial.println("Screen Updated.");
    }
  }
}

void setup() {
  commandQueue = xQueueCreate(5, sizeof(Command));
  xTaskCreatePinnedToCore(InputTask, "Input Task", 4096, NULL, 2, &t_Input, 0);
  xTaskCreatePinnedToCore(DisplayTask, "Display Task", 4096, NULL, 1, &t_Display, 1);
  xTaskCreatePinnedToCore(HeartTask, "Heart Task", 2048, NULL, 3, &t_Blink, 1);
}

void loop() {}
