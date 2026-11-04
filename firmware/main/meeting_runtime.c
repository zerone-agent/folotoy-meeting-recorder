#include "meeting_runtime.h"
#include "sc_store.h"
#include "meeting_deletion_store.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdatomic.h>

static meeting_arbitration arbitration;
/* Long LittleFS scans and hashing run in task context; allow the idle task to
 * run periodically so they do not starve the task watchdog. */
static atomic_uint cooperative_checks;
static void cooperate(void)
{
    if((atomic_fetch_add(&cooperative_checks,1u)&255u)==255u)vTaskDelay(1);
}
typedef struct { const meeting_https_config *original; bool (*has_ip)(void *); void *network_ctx; } cancel_context;
static bool cancelled(void *ctx)
{
    cancel_context *c=ctx;
    cooperate();
    return meeting_arbitration_upload_cancelled(&arbitration) ||
           (c->has_ip && !c->has_ip(c->network_ctx)) || c->original->is_cancelled(c->original->cancel_ctx);
}
static bool time_valid(void *ctx)
{
    cancel_context *c=ctx;return c->original->time_valid(c->original->cancel_ctx);
}
void meeting_runtime_init(void){meeting_arbitration_init(&arbitration);}
bool meeting_runtime_record_request(void){return meeting_arbitration_record_request(&arbitration);}
bool meeting_runtime_record_acquire(void){return meeting_arbitration_record_acquire(&arbitration);}
bool meeting_runtime_record_cancel(void){return meeting_arbitration_record_cancel(&arbitration);}
bool meeting_runtime_record_end(void){return meeting_arbitration_record_end(&arbitration);}
bool meeting_runtime_maintenance_acquire(void){return meeting_arbitration_maintenance_acquire(&arbitration);}
bool meeting_runtime_maintenance_release(void){return meeting_arbitration_maintenance_release(&arbitration);}
static bool cleanup_cancelled(void *ctx){(void)ctx;cooperate();return meeting_arbitration_upload_cancelled(&arbitration);}
meeting_delete_visibility meeting_runtime_review_delete(uint32_t file_id)
{
    if(!file_id||!meeting_arbitration_upload_begin(&arbitration))return MEETING_DELETE_UNKNOWN;
    meeting_delete_visibility result=sc_store_delete_visibility(file_id,cleanup_cancelled,NULL);
    if(meeting_arbitration_upload_cancelled(&arbitration))result=MEETING_DELETE_UNKNOWN;
    meeting_arbitration_upload_end(&arbitration);return result;
}
esp_err_t meeting_runtime_begin_delete(uint32_t file_id,meeting_delete_visibility reviewed)
{
    if(!file_id||!meeting_arbitration_upload_begin(&arbitration))return ESP_ERR_INVALID_STATE;
    esp_err_t result=sc_store_begin_delete_checked(file_id,reviewed,cleanup_cancelled,NULL);
    meeting_arbitration_upload_end(&arbitration);return result;
}
meeting_cleanup_result meeting_runtime_resume_cleanup(void)
{
    if(!meeting_arbitration_upload_begin(&arbitration))return MEETING_CLEANUP_PAUSED;
    meeting_cleanup_result result=sc_store_resume_cleanups(cleanup_cancelled,NULL);
    meeting_arbitration_upload_end(&arbitration);return result;
}
meeting_delete_pass_result meeting_runtime_resume_deletions(const meeting_online_config *config)
{
    meeting_delete_pass_result result={0};uint32_t last=0,id;
    for(;;){
        meeting_deletion_next_status next=meeting_deletion_store_next(last,&id);
        if(next==MEETING_DELETION_NEXT_END)break;
        if(next!=MEETING_DELETION_NEXT_FOUND){result.failed++;break;}
        if(!meeting_arbitration_upload_begin(&arbitration)){result.paused=true;break;}
        result.attempted++;
        esp_err_t local=sc_store_finish_delete_local(id,cleanup_cancelled,NULL);
        if(local!=ESP_OK&&!meeting_arbitration_upload_cancelled(&arbitration)){
            if(sc_store_prepare_delete_proof(id,cleanup_cancelled,NULL)==ESP_OK)
                local=sc_store_finish_delete_local(id,cleanup_cancelled,NULL);
        }
        if(meeting_arbitration_upload_cancelled(&arbitration))result.paused=true;
        else if(local!=ESP_OK)result.failed++;
        else if(config&&config->has_ip&&config->https.is_cancelled&&config->https.time_valid&&
                config->has_ip(config->network_ctx)&&config->https.time_valid(config->https.cancel_ctx)){
            cancel_context ctx={.original=&config->https,.has_ip=config->has_ip,.network_ctx=config->network_ctx};meeting_https_config https=config->https;
            https.cancel_ctx=&ctx;https.is_cancelled=cancelled;https.time_valid=time_valid;
            meeting_upload_transport transport;
            if(meeting_https_bind(&https,&transport)&&sc_store_cancel_delete_remote(id,&transport)==ESP_OK)result.completed++;
            else if(meeting_arbitration_upload_cancelled(&arbitration))result.paused=true;
            else result.failed++;
            meeting_https_close(&https);
        }
        meeting_arbitration_upload_end(&arbitration);
        if(result.paused)break;
        last=id;
    }
    return result;
}
meeting_upload_result meeting_runtime_upload(uint32_t file_id,bool has_time,int64_t recorded_at,
                                             uint8_t time_trust,const meeting_online_config *config)
{
    if(!config||!config->https.is_cancelled||!config->https.time_valid)return MEETING_UPLOAD_FAILED;
    if(!meeting_arbitration_upload_begin(&arbitration))return MEETING_UPLOAD_PAUSED;
    cancel_context ctx={.original=&config->https,.has_ip=config->has_ip,.network_ctx=config->network_ctx};meeting_online_config local=*config;
    local.https.cancel_ctx=&ctx;local.https.is_cancelled=cancelled;local.https.time_valid=time_valid;
    meeting_upload_result result=meeting_store_upload(file_id,has_time,recorded_at,time_trust,&local);
    if(result==MEETING_UPLOAD_CONFIRMED){
        meeting_cleanup_result cleaned=sc_store_cleanup_confirmed(file_id,cancelled,&ctx);
        if(cleaned==MEETING_CLEANUP_PAUSED)result=MEETING_UPLOAD_PAUSED;
        else if(cleaned!=MEETING_CLEANUP_DONE)result=MEETING_UPLOAD_FAILED;
    }
    meeting_arbitration_upload_end(&arbitration);
    return result;
}
