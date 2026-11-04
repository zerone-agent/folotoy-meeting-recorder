#include "meeting_delete_job.h"
#include "meeting_runtime.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdatomic.h>
#include <stdlib.h>
typedef struct {meeting_delete_job_result result;meeting_delete_job_done done;void *ctx;} job;
static atomic_bool running;
static void run(void *arg){
 job *j=arg;
 if(j->result.kind==MEETING_DELETE_JOB_REVIEW){
  j->result.visibility=meeting_runtime_review_delete(j->result.file_id);j->result.result=ESP_OK;
 }else j->result.result=meeting_runtime_begin_delete(j->result.file_id,j->result.visibility);
 meeting_delete_job_result result=j->result;meeting_delete_job_done done=j->done;void *ctx=j->ctx;
 free(j);atomic_store(&running,false);done(&result,ctx);vTaskDelete(NULL);
}
esp_err_t meeting_delete_job_start(uint32_t token,uint32_t id,meeting_delete_job_kind kind,
                                   meeting_delete_visibility reviewed,meeting_delete_job_done done,void *ctx){
 if(!token||!id||!done||(unsigned)kind>MEETING_DELETE_JOB_BEGIN||(unsigned)reviewed>MEETING_DELETE_UNKNOWN)return ESP_ERR_INVALID_ARG;
 bool expected=false;if(!atomic_compare_exchange_strong(&running,&expected,true))return ESP_ERR_INVALID_STATE;
 job *j=calloc(1,sizeof(*j));if(!j){atomic_store(&running,false);return ESP_ERR_NO_MEM;}
 j->result=(meeting_delete_job_result){.token=token,.file_id=id,.kind=kind,.visibility=reviewed};j->done=done;j->ctx=ctx;
 if(xTaskCreate(run,"delete_job",8192,j,4,NULL)!=pdPASS){free(j);atomic_store(&running,false);return ESP_ERR_NO_MEM;}
 return ESP_OK;
}
