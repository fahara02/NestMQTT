#ifndef MQTT_ASYNC_TASK_H_
#define MQTT_ASYNC_TASK_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <Arduino.h>

extern SemaphoreHandle_t _xSemaphore;
extern TaskHandle_t _taskHandle;
extern EspClass ESP;

#define MQTT_SEMAPHORE_TAKE() xSemaphoreTake(_xSemaphore, portMAX_DELAY)
#define MQTT_SEMAPHORE_GIVE() xSemaphoreGive(_xSemaphore)
#define MQTT_GET_FREE_MEMORY()                                                 \
  std::max(ESP.getMaxAllocHeap(), ESP.getMaxAllocPsram())

#define MQTT_YIELD() vTaskDelay(1)

#endif // MQTT_ASYNC_TASK_H_
