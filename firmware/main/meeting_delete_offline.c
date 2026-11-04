#include "meeting_delete_offline.h"
#include "meeting_deletion_store.h"
#include "meeting_runtime.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdatomic.h>
static const char *TAG="meeting_delete";
/* 0 idle, 1 running, 2 another pass requested, 3 task creation in progress. */
static atomic_uint state;
static atomic_bool retry_needed;
static void recover(void *arg)
{
    (void)arg;
    while(atomic_load(&state)==3)vTaskDelay(pdMS_TO_TICKS(1));
    for(;;){
        meeting_delete_pass_result result;
        do {
        result=meeting_runtime_resume_deletions(NULL);
        if(result.paused)vTaskDelay(pdMS_TO_TICKS(5000));
        } while(result.paused);
        if(result.failed){atomic_store(&retry_needed,true);ESP_LOGW(TAG,"offline delete recovery needs attention: %u",result.failed);}
        unsigned expected=1;
        if(atomic_compare_exchange_strong(&state,&expected,0))break;
        /* Do not overwrite a new request made while switching to the next pass. */
        expected=2;
        (void)atomic_compare_exchange_strong(&state,&expected,1);
    }
    vTaskDelete(NULL);
}
esp_err_t meeting_delete_offline_start(void)
{
    uint32_t id;meeting_deletion_next_status status=meeting_deletion_store_next(0,&id);
    if(status==MEETING_DELETION_NEXT_END)return ESP_OK;
    if(status!=MEETING_DELETION_NEXT_FOUND){atomic_store(&retry_needed,true);return ESP_FAIL;}
    for(;;){
        unsigned expected=atomic_load(&state);
        if(expected==3){atomic_store(&retry_needed,true);return ESP_ERR_INVALID_STATE;}
        if(expected==2)return ESP_OK;
        if(expected==1){if(atomic_compare_exchange_weak(&state,&expected,2))return ESP_OK;continue;}
        if(expected==0&&atomic_compare_exchange_weak(&state,&expected,3))break;
    }
    if(xTaskCreate(recover,"delete_recover",8192,NULL,4,NULL)!=pdPASS){atomic_store(&state,0);atomic_store(&retry_needed,true);return ESP_ERR_NO_MEM;}
    atomic_store(&state,1);
    return ESP_OK;
}
esp_err_t meeting_delete_offline_retry(void)
{
    if(!atomic_exchange(&retry_needed,false))return ESP_OK;
    return meeting_delete_offline_start();
}
