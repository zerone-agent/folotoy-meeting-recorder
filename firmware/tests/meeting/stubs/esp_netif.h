#pragma once
#include "esp_err.h"
typedef struct {int x;} esp_netif_t;
esp_err_t esp_netif_init(void);esp_netif_t *esp_netif_create_default_wifi_sta(void);void esp_netif_destroy_default_wifi(esp_netif_t*);
