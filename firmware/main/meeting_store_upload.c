#include "meeting_store_upload.h"
#include "sc_store.h"
#include "esp_log.h"

static const char *TAG="meeting_upload";

meeting_upload_result meeting_store_upload(uint32_t file_id, bool has_time,
                                           int64_t recorded_at, uint8_t time_trust,
                                           const meeting_online_config *config)
{
    if (!config) return MEETING_UPLOAD_FAILED;
    FILE *source = sc_store_upload_acquire(file_id);
    if (!source) {ESP_LOGW(TAG,"file %lu: acquire failed",(unsigned long)file_id);return MEETING_UPLOAD_FAILED;}
    meeting_snapshot snapshot;
    meeting_upload_result result = MEETING_UPLOAD_FAILED;
    meeting_manifest manifest;
    esp_err_t prepared=sc_store_prepare_snapshot(file_id, &snapshot,config->https.is_cancelled,config->https.cancel_ctx);
    if (prepared == ESP_OK) {
        sc_confirmation_status status=sc_store_check_confirmation(source,&snapshot,has_time,recorded_at,time_trust,
                                                                  config->https.is_cancelled,config->https.cancel_ctx);
        if (status==SC_CONFIRMATION_PRESENT) result=MEETING_UPLOAD_CONFIRMED;
        else if (status==SC_CONFIRMATION_ABSENT) {
            result = meeting_online_session(source, &snapshot, has_time, recorded_at, time_trust, config, &manifest);
            if(result==MEETING_UPLOAD_FAILED)ESP_LOGW(TAG,"file %lu: remote session failed",(unsigned long)file_id);
            if (result == MEETING_UPLOAD_CONFIRMED && sc_store_confirm_upload(source, &snapshot, &manifest) != ESP_OK)
                result = MEETING_UPLOAD_FAILED;
        } else if(config->https.is_cancelled && config->https.is_cancelled(config->https.cancel_ctx)) result=MEETING_UPLOAD_PAUSED;
        else ESP_LOGW(TAG,"file %lu: confirmation metadata invalid",(unsigned long)file_id);
    } else if(config->https.is_cancelled && config->https.is_cancelled(config->https.cancel_ctx)) result=MEETING_UPLOAD_PAUSED;
    else if(prepared==ESP_ERR_NOT_FOUND)ESP_LOGW(TAG,"file %lu: meeting identity missing; retained for maintenance",(unsigned long)file_id);
    else ESP_LOGW(TAG,"file %lu: snapshot failed: %s",(unsigned long)file_id,esp_err_to_name(prepared));
    if (sc_store_upload_release(source) != ESP_OK) return MEETING_UPLOAD_FAILED;
    return result;
}
