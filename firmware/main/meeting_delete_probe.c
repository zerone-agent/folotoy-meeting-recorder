#include "sdkconfig.h"
#if CONFIG_MEETING_DEVICE_DELETE_PROBE
#include "meeting_commission.h"
#include "meeting_delete_offline.h"
#include "meeting_deletion_store.h"
#include "meeting_network.h"
#include "meeting_runtime.h"
#include "sc_store.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static void probe(void *unused){
 (void)unused;const uint32_t id=CONFIG_MEETING_DEVICE_DELETE_PROBE_FILE_ID;
 meeting_deletion_store store;meeting_deletion_io io;meeting_deletion saved;unsigned slot;
 if(!meeting_deletion_store_open(&store,id,&io)){ESP_LOGE("delete_probe","intent open failed");goto end;}
 meeting_deletion_status status=meeting_deletion_load(&io,&saved,&slot);meeting_deletion_store_close(&store);
 if(status==MEETING_DELETION_OK){
  if(!saved.legacy_local_only){ESP_LOGE("delete_probe","unexpected existing intent");goto end;}
  (void)meeting_delete_offline_start();
  for(unsigned i=0;i<100;i++){sc_store_info_t file;if(!sc_store_get(id,&file)){
   ESP_LOGI("delete_probe","RESET_RECOVERY=PASS");goto end;}vTaskDelay(pdMS_TO_TICKS(100));}
  ESP_LOGE("delete_probe","recovery did not finish");goto end;
 }
 if(status!=MEETING_DELETION_EMPTY){ESP_LOGE("delete_probe","intent damaged");goto end;}
 sc_store_info_t file;bool present;int64_t epoch;uint8_t trust;
 if(!sc_store_get(id,&file)||sc_store_record_time(id,&present,&epoch,&trust)){
  ESP_LOGE("delete_probe","file absent or has identity");goto end;
 }
 if(!meeting_network_stop_request()){ESP_LOGE("delete_probe","network stop request failed");goto end;}
 for(unsigned i=0;i<100&&!meeting_network_stop_complete();i++)vTaskDelay(pdMS_TO_TICKS(100));
 if(!meeting_network_stop_complete()){ESP_LOGE("delete_probe","network stop timed out");goto end;}
 if(meeting_runtime_begin_delete(id,MEETING_DELETE_UNKNOWN)!=ESP_OK){ESP_LOGE("delete_probe","intent commit failed");goto end;}
 ESP_LOGI("delete_probe","INTENT_COMMITTED; restarting for recovery");
 vTaskDelay(pdMS_TO_TICKS(200));esp_restart();
end:vTaskDelete(NULL);
}
bool meeting_delete_probe_start(void){return meeting_commission_allowed()&&xTaskCreate(probe,"delete_probe",8192,NULL,4,NULL)==pdPASS;}
#endif
