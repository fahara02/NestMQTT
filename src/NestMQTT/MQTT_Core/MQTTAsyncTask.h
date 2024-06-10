#ifndef MQTT_ASYNC_TASK_H_
#define MQTT_ASYNC_TASK_H_
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <Arduino.h>

namespace MQTTCore {

#define MQTT_SEMAPHORE_TAKE() xSemaphoreTake(_xSemaphore, portMAX_DELAY)
#define MQTT_SEMAPHORE_GIVE() xSemaphoreGive(_xSemaphore)
#define MQTT_YIELD() vTaskDelay(1)

} // namespace MQTTCore

#endif