#include "meeting_delete_offline.h"
#include "meeting_deletion_store.h"
#include "meeting_runtime.h"
#include "freertos/task.h"
#include <assert.h>
static meeting_deletion_next_status next_state;static int create_ok=1,creates,runs,delays,deletes,concurrent_start,fail_pass;
static void (*saved)(void *);
meeting_deletion_next_status meeting_deletion_store_next(uint32_t after,uint32_t *id){assert(!after&&id);*id=7;return next_state;}
BaseType_t xTaskCreate(void (*fn)(void *),const char *name,unsigned size,void *arg,unsigned priority,TaskHandle_t *out){(void)name;(void)arg;(void)priority;assert(size==8192&&!out);creates++;saved=fn;if(concurrent_start)assert(meeting_delete_offline_start()==ESP_ERR_INVALID_STATE);return create_ok?pdPASS:0;}
void vTaskDelay(unsigned ticks){assert(ticks==5000);delays++;}
void vTaskDelete(TaskHandle_t task){assert(!task);deletes++;}
meeting_delete_pass_result meeting_runtime_resume_deletions(const meeting_online_config *config){assert(!config);runs++;if(fail_pass){fail_pass=0;return (meeting_delete_pass_result){.failed=1};}return (meeting_delete_pass_result){.paused=runs==1};}
int main(void){
 next_state=MEETING_DELETION_NEXT_END;assert(meeting_delete_offline_start()==ESP_OK&&!creates);
 next_state=MEETING_DELETION_NEXT_ERROR;assert(meeting_delete_offline_start()==ESP_FAIL&&!creates);
 next_state=MEETING_DELETION_NEXT_FOUND;create_ok=0;concurrent_start=1;assert(meeting_delete_offline_start()==ESP_ERR_NO_MEM&&creates==1);
 create_ok=1;concurrent_start=0;assert(meeting_delete_offline_retry()==ESP_OK&&creates==2&&saved);
 assert(meeting_delete_offline_start()==ESP_OK&&creates==2);saved(NULL);
 assert(runs==3&&delays==1&&deletes==1);
 fail_pass=1;assert(meeting_delete_offline_start()==ESP_OK);saved(NULL);
 assert(runs==4&&deletes==2);assert(meeting_delete_offline_retry()==ESP_OK);saved(NULL);
 assert(runs==5&&deletes==3);return 0;
}
