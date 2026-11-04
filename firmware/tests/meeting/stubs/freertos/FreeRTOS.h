#pragma once
#include <stdint.h>
#define pdTRUE 1
#define pdPASS 1
#define portMAX_DELAY UINT32_MAX
#define pdMS_TO_TICKS(v) (v)
typedef int BaseType_t;
typedef unsigned TickType_t;
