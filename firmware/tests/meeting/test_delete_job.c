#include "meeting_delete_job.h"
#include "meeting_runtime.h"
#include "freertos/task.h"
#include <assert.h>
static void (*fn)(void *);static void *arg;static int create_ok=1,reviewed,begun,deleted,callbacks;
static meeting_delete_job_result last;
BaseType_t xTaskCreate(void (*f)(void *),const char *name,unsigned size,void *a,unsigned priority,TaskHandle_t *out){(void)name;(void)priority;assert(size==8192&&!out);if(!create_ok)return 0;fn=f;arg=a;return pdPASS;}
void vTaskDelete(TaskHandle_t h){assert(!h);deleted++;}
meeting_delete_visibility meeting_runtime_review_delete(uint32_t id){assert(id==7);reviewed++;return MEETING_DELETE_UNKNOWN;}
esp_err_t meeting_runtime_begin_delete(uint32_t id,meeting_delete_visibility v){assert(id==7&&v==MEETING_DELETE_UNKNOWN);begun++;return ESP_ERR_INVALID_STATE;}
static void done(const meeting_delete_job_result *r,void *ctx){assert(ctx==(void *)1);last=*r;callbacks++;}
int main(void){
 assert(meeting_delete_job_start(0,7,MEETING_DELETE_JOB_REVIEW,MEETING_DELETE_UNKNOWN,done,(void *)1)==ESP_ERR_INVALID_ARG);
 create_ok=0;assert(meeting_delete_job_start(1,7,MEETING_DELETE_JOB_REVIEW,MEETING_DELETE_UNKNOWN,done,(void *)1)==ESP_ERR_NO_MEM);create_ok=1;
 assert(meeting_delete_job_start(1,7,MEETING_DELETE_JOB_REVIEW,MEETING_DELETE_UNKNOWN,done,(void *)1)==ESP_OK);
 assert(meeting_delete_job_start(2,7,MEETING_DELETE_JOB_BEGIN,MEETING_DELETE_UNKNOWN,done,(void *)1)==ESP_ERR_INVALID_STATE);
 fn(arg);assert(reviewed==1&&callbacks==1&&deleted==1&&last.token==1&&last.visibility==MEETING_DELETE_UNKNOWN);
 assert(meeting_delete_job_start(2,7,MEETING_DELETE_JOB_BEGIN,MEETING_DELETE_UNKNOWN,done,(void *)1)==ESP_OK);fn(arg);
 assert(begun==1&&callbacks==2&&deleted==2&&last.token==2&&last.result==ESP_ERR_INVALID_STATE);return 0;
}
