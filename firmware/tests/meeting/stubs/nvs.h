#pragma once
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
typedef int nvs_handle_t;
typedef struct fake_nvs_iterator *nvs_iterator_t;
typedef struct { char namespace_name[16]; char key[16]; int type; } nvs_entry_info_t;
#define ESP_OK 0
#define ESP_ERR_NVS_NOT_FOUND 1
#define ESP_ERR_NVS_INVALID_LENGTH 2
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define NVS_TYPE_ANY 0
#define NVS_TYPE_BLOB 1
esp_err_t nvs_open(const char*,int,nvs_handle_t*);
void nvs_close(nvs_handle_t);
esp_err_t nvs_get_blob(nvs_handle_t,const char*,void*,size_t*);
esp_err_t nvs_set_blob(nvs_handle_t,const char*,const void*,size_t);
esp_err_t nvs_commit(nvs_handle_t);
esp_err_t nvs_erase_key(nvs_handle_t,const char*);
esp_err_t nvs_get_u32(nvs_handle_t,const char*,uint32_t*);
esp_err_t nvs_set_u32(nvs_handle_t,const char*,uint32_t);
esp_err_t nvs_entry_find(const char*,const char*,int,nvs_iterator_t*);
esp_err_t nvs_entry_next(nvs_iterator_t*);
esp_err_t nvs_entry_info(nvs_iterator_t,nvs_entry_info_t*);
void nvs_release_iterator(nvs_iterator_t);
