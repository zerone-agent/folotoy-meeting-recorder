#include "meeting_runtime.h"
#include "sc_store.h"
#include "meeting_deletion_store.h"
#include <assert.h>

static int calls, user_cancel, clock_ok=1, interrupt_cleanup,delete_enabled,delete_attempt,proof_attempt,remote_attempt,interrupt_delete,interrupt_review,network_up=1,drop_during_prepare,simulate_long_review,yields;
void vTaskDelay(unsigned ticks){assert(ticks==1);yields++;}
static bool cancel(void *ctx){(void)ctx;return user_cancel;}
static bool time_ok(void *ctx){(void)ctx;return clock_ok;}
static bool ip(void *ctx){(void)ctx;return network_up;}
meeting_deletion_next_status meeting_deletion_store_next(uint32_t after,uint32_t *id){if(delete_enabled&&!after){*id=8;return MEETING_DELETION_NEXT_FOUND;}return MEETING_DELETION_NEXT_END;}
esp_err_t sc_store_finish_delete_local(uint32_t id,meeting_cancel_fn cancelled,void *ctx){assert(id==8);delete_attempt++;if(interrupt_delete){assert(meeting_runtime_record_request());assert(cancelled(ctx));return ESP_FAIL;}return delete_attempt>1?ESP_OK:ESP_FAIL;}
esp_err_t sc_store_prepare_delete_proof(uint32_t id,meeting_cancel_fn cancelled,void *ctx){assert(id==8&&!cancelled(ctx));proof_attempt++;return ESP_OK;}
void meeting_https_close(meeting_https_config *c){(void)c;}
bool meeting_https_bind(meeting_https_config *config,meeting_upload_transport *out){(void)config;*out=(meeting_upload_transport){0};return true;}
esp_err_t sc_store_cancel_delete_remote(uint32_t id,const meeting_upload_transport *transport){assert(id==8&&transport);remote_attempt++;return ESP_OK;}
meeting_delete_visibility sc_store_delete_visibility(uint32_t id,meeting_cancel_fn cancelled,void *ctx){assert(id==8&&!cancelled(ctx));if(simulate_long_review)for(int i=0;i<1024;i++)assert(!cancelled(ctx));if(interrupt_review)assert(meeting_runtime_record_request());return MEETING_DELETE_CONFIRMED;}
esp_err_t sc_store_begin_delete_checked(uint32_t id,meeting_delete_visibility reviewed,meeting_cancel_fn cancelled,void *ctx){assert(id==8&&reviewed==MEETING_DELETE_CONFIRMED&&!cancelled(ctx));return ESP_OK;}
meeting_cleanup_result sc_store_cleanup_confirmed(uint32_t id,meeting_cancel_fn cancelled,void *ctx)
{(void)id;if(interrupt_cleanup){assert(meeting_runtime_record_request());assert(cancelled(ctx));assert(!meeting_runtime_record_acquire());return MEETING_CLEANUP_PAUSED;}return MEETING_CLEANUP_DONE;}
meeting_cleanup_result sc_store_resume_cleanups(meeting_cancel_fn cancelled,void *ctx)
{return cancelled(ctx)?MEETING_CLEANUP_PAUSED:MEETING_CLEANUP_DONE;}
meeting_upload_result meeting_store_upload(uint32_t id,bool has_time,int64_t recorded_at,uint8_t trust,
                                           const meeting_online_config *config)
{
    (void)has_time;(void)recorded_at;(void)trust;assert(id==4);calls++;
    assert(config->https.time_valid(config->https.cancel_ctx));
    assert(!config->https.is_cancelled(config->https.cancel_ctx));
    if(drop_during_prepare){network_up=0;assert(config->https.is_cancelled(config->https.cancel_ctx));return MEETING_UPLOAD_PAUSED;}
    if(interrupt_cleanup)return MEETING_UPLOAD_CONFIRMED;
    assert(meeting_runtime_record_request());
    assert(config->https.is_cancelled(config->https.cancel_ctx));
    assert(!meeting_runtime_record_acquire()); /* source is still owned */
    return MEETING_UPLOAD_PAUSED; /* fake store has released its source */
}
int main(void)
{
    meeting_online_config config={.https={.is_cancelled=cancel,.time_valid=time_ok}};
    meeting_runtime_init();
    assert(meeting_runtime_resume_cleanup()==MEETING_CLEANUP_DONE);
    assert(meeting_runtime_upload(4,false,0,0,&config)==MEETING_UPLOAD_PAUSED && calls==1);
    assert(meeting_runtime_record_acquire());
    assert(meeting_runtime_upload(4,false,0,0,&config)==MEETING_UPLOAD_PAUSED && calls==1);
    assert(meeting_runtime_record_end());
    assert(meeting_runtime_record_request());assert(meeting_runtime_record_cancel());
    assert(meeting_runtime_upload(4,false,0,0,&config)==MEETING_UPLOAD_PAUSED && calls==2);
    assert(meeting_runtime_record_cancel());
    assert(meeting_runtime_record_request());assert(meeting_runtime_record_acquire());assert(meeting_runtime_record_end());
    interrupt_cleanup=1;assert(meeting_runtime_upload(4,false,0,0,&config)==MEETING_UPLOAD_PAUSED);
    assert(meeting_runtime_record_acquire());assert(meeting_runtime_resume_cleanup()==MEETING_CLEANUP_PAUSED);
    assert(meeting_runtime_record_end());interrupt_cleanup=0;assert(meeting_runtime_resume_cleanup()==MEETING_CLEANUP_DONE);
    config.has_ip=ip;drop_during_prepare=1;assert(meeting_runtime_upload(4,false,0,0,&config)==MEETING_UPLOAD_PAUSED);
    network_up=1;drop_during_prepare=0;
    delete_enabled=1;config.has_ip=ip;
    meeting_delete_pass_result d=meeting_runtime_resume_deletions(&config);
    assert(d.attempted==1&&d.completed==1&&!d.failed&&!d.paused&&proof_attempt==1&&delete_attempt==2&&remote_attempt==1);
    interrupt_delete=1;d=meeting_runtime_resume_deletions(&config);assert(d.paused&&d.attempted==1&&remote_attempt==1);
    assert(meeting_runtime_record_acquire());assert(meeting_runtime_record_end());
    interrupt_delete=0;simulate_long_review=1;assert(meeting_runtime_review_delete(8)==MEETING_DELETE_CONFIRMED&&yields>=4);simulate_long_review=0;
    assert(meeting_runtime_begin_delete(8,MEETING_DELETE_CONFIRMED)==ESP_OK);
    interrupt_review=1;assert(meeting_runtime_review_delete(8)==MEETING_DELETE_UNKNOWN);
    assert(meeting_runtime_record_acquire());assert(meeting_runtime_review_delete(8)==MEETING_DELETE_UNKNOWN);
    assert(meeting_runtime_begin_delete(8,MEETING_DELETE_CONFIRMED)==ESP_ERR_INVALID_STATE);assert(meeting_runtime_record_end());
    return 0;
}
