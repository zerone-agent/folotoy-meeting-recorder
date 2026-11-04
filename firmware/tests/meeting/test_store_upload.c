#include "meeting_store_upload.h"
#include "sc_store.h"
#include <assert.h>

static int acquired, prepared, sessions, released;
static int acquire_ok = 1, prepare_ok = 1, release_ok = 1, confirm_ok = 1, confirmations;
static FILE *source;
static meeting_upload_result session_result = MEETING_UPLOAD_CONFIRMED;
static sc_confirmation_status existing=SC_CONFIRMATION_ABSENT;

FILE *sc_store_upload_acquire(uint32_t id) {
    assert(id == 7); acquired++; return acquire_ok ? source : NULL;
}
esp_err_t sc_store_prepare_snapshot(uint32_t id, meeting_snapshot *snapshot, meeting_cancel_fn cancelled, void *ctx) {
    (void)cancelled; (void)ctx;
    assert(id == 7 && snapshot); prepared++; return prepare_ok ? ESP_OK : 1;
}
esp_err_t sc_store_upload_release(FILE *file) {
    assert(file == source); released++; return release_ok ? ESP_OK : 1;
}
meeting_upload_result meeting_online_session(FILE *file, const meeting_snapshot *snapshot,
    bool has_time, int64_t recorded_at, uint8_t time_trust, const meeting_online_config *config, meeting_manifest *confirmed) {
    assert(confirmed);
    assert(file == source && snapshot && config && !has_time && recorded_at == 0 && time_trust == 0);
    sessions++; return session_result;
}
esp_err_t sc_store_confirm_upload(FILE *file, const meeting_snapshot *snapshot, const meeting_manifest *manifest) {
    assert(file == source && snapshot && manifest); confirmations++; return confirm_ok ? ESP_OK : 1;
}
sc_confirmation_status sc_store_check_confirmation(FILE *file,const meeting_snapshot *snapshot,
                                                    bool has_time,int64_t recorded_at,uint8_t time_trust,
                                                    meeting_cancel_fn cancelled,void *ctx) {
    (void)cancelled; (void)ctx;
    assert(file==source && snapshot && !has_time && recorded_at==0 && time_trust==0);return existing;
}
int main(void) {
    source = tmpfile(); assert(source);
    meeting_online_config config = {0};
    assert(meeting_store_upload(7, false, 0, 0, NULL) == MEETING_UPLOAD_FAILED);
    acquire_ok = 0;
    assert(meeting_store_upload(7, false, 0, 0, &config) == MEETING_UPLOAD_FAILED);
    assert(acquired == 1 && prepared == 0 && released == 0);
    acquire_ok = 1; prepare_ok = 0;
    assert(meeting_store_upload(7, false, 0, 0, &config) == MEETING_UPLOAD_FAILED);
    assert(prepared == 1 && sessions == 0 && released == 1);
    prepare_ok = 1; session_result = MEETING_UPLOAD_PAUSED;
    assert(meeting_store_upload(7, false, 0, 0, &config) == MEETING_UPLOAD_PAUSED);
    assert(sessions == 1 && released == 2);
    session_result = MEETING_UPLOAD_CONFIRMED; release_ok = 0;
    assert(meeting_store_upload(7, false, 0, 0, &config) == MEETING_UPLOAD_FAILED);
    assert(sessions == 2 && released == 3);
    release_ok = 1; confirm_ok = 0;
    assert(meeting_store_upload(7, false, 0, 0, &config) == MEETING_UPLOAD_FAILED);
    confirm_ok = 1;
    assert(meeting_store_upload(7, false, 0, 0, &config) == MEETING_UPLOAD_CONFIRMED);
    assert(sessions == 4 && released == 5 && confirmations == 3);
    existing=SC_CONFIRMATION_PRESENT;
    assert(meeting_store_upload(7, false, 0, 0, &config) == MEETING_UPLOAD_CONFIRMED);
    assert(sessions == 4 && released == 6 && confirmations == 3);
    existing=SC_CONFIRMATION_ERROR;
    assert(meeting_store_upload(7, false, 0, 0, &config) == MEETING_UPLOAD_FAILED);
    assert(sessions == 4 && released == 7 && confirmations == 3);
    existing=SC_CONFIRMATION_ABSENT;session_result=MEETING_UPLOAD_FAILED;
    assert(meeting_store_upload(7, false, 0, 0, &config) == MEETING_UPLOAD_FAILED);
    assert(sessions == 5 && released == 8 && confirmations == 3);
    fclose(source);
    return 0;
}
