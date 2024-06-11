#include "ArduinoJson.h"
#include "MQTTClient.h"
#include "MQTTCore.h"
#include "MQTTTransmitter.h"
#include "sdkconfig.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>


void arduinoTask(void *pvParameter) {
  // Set WiFi to station mode and disconnect from an AP if it was previously
  // connected

  Serial.begin(115200);
  delay(100);

  while (1) {

    Serial.println("arduino core running");
    // Wait a bit before scanning again
    delay(5000);
  }
}

void setup() {
  // Set WiFi to station mode and disconnect from an AP if it was previously
  // connected
  initArduino();

  xTaskCreate(&arduinoTask, "arduino_task", configMINIMAL_STACK_SIZE, NULL, 5,
              NULL);
  Serial.begin(115200);
  delay(100);
}

void loop() {

  Serial.println("hello world ");
  // Wait a bit before scanning again
  delay(5000);
}
