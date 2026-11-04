#pragma once
#include "FreeRTOS.h"
typedef void *TaskHandle_t;
BaseType_t xTaskCreate(void (*fn)(void *),const char *name,unsigned size,void *arg,unsigned priority,TaskHandle_t *out);
void vTaskDelay(unsigned ticks);
void vTaskDelete(TaskHandle_t task);
void xTaskNotifyGive(TaskHandle_t task);
unsigned ulTaskNotifyTake(BaseType_t clear,unsigned ticks);
