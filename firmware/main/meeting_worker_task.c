#include "meeting_worker_task.h"
#include "meeting_worker.h"
#include "sc_store.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static meeting_online_config s_config;
static sc_file_entry_t s_entries[SC_STORE_MAX_FILES];
static TaskHandle_t s_task;
static void worker(void *arg)
{
    (void)arg;
    for(;;){
        meeting_worker_result result=meeting_worker_pass(&s_config,s_entries,SC_STORE_MAX_FILES);
        /* Failed requests are retried no faster than this bound; a cancelled
         * pass gives recording time to take ownership before trying again. */
        vTaskDelay(pdMS_TO_TICKS(result.paused ? 5000 : 60000));
    }
}
esp_err_t meeting_worker_start(const meeting_online_config *config)
{
    if(!config||!config->has_ip||!config->https.is_cancelled||!config->https.time_valid||
       !config->https.origin||!config->https.device_token||!config->https.ca_pem||s_task)return ESP_ERR_INVALID_ARG;
    s_config=*config;
    if(xTaskCreate(worker,"meeting_upload",16384,NULL,4,&s_task)!=pdPASS){s_task=NULL;return ESP_ERR_NO_MEM;}
    return ESP_OK;
}
