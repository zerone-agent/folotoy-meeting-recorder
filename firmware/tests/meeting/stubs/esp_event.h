#pragma once
#include "esp_err.h"
#include <stdint.h>
typedef const char *esp_event_base_t;typedef void *esp_event_handler_instance_t;
extern esp_event_base_t WIFI_EVENT,IP_EVENT;
#define WIFI_EVENT_STA_DISCONNECTED 1
#define IP_EVENT_STA_GOT_IP 2
typedef void (*event_handler_t)(void*,esp_event_base_t,int32_t,void*);
esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_handler_instance_register(esp_event_base_t,int32_t,event_handler_t,void*,esp_event_handler_instance_t*);
esp_err_t esp_event_handler_instance_unregister(esp_event_base_t,int32_t,esp_event_handler_instance_t);
