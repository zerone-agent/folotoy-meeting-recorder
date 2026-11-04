#define time fake_time
#include "../../main/meeting_network.c"
#undef time
#include <assert.h>
#include <string.h>

static int wifi_running,ip,sta_running,spawn_ok=1,mutex_deleted,passes,connects,notifications,stop_failure,stop_during_pass,recover_after_delay;
static int try_calls,fail_try;
static int clock_sync_calls,clock_sync_success,clock_sync_record;
static unsigned delete_failures;static meeting_network_state state_before_wait;
static int64_t mono_us=1000000;static time_t wall=1800000000;
static void (*task_fn)(void *);
time_t fake_time(time_t *p){if(p)*p=wall;return wall;}
int64_t esp_timer_get_time(void){return mono_us;}
SemaphoreHandle_t xSemaphoreCreateMutex(void){return (void *)1;}
BaseType_t xSemaphoreTake(SemaphoreHandle_t h,unsigned wait){(void)h;if(wait==0&&++try_calls==fail_try)return 0;return pdTRUE;}
BaseType_t xSemaphoreGive(SemaphoreHandle_t h){(void)h;return pdTRUE;}
void vSemaphoreDelete(SemaphoreHandle_t h){(void)h;mutex_deleted++;}
BaseType_t xTaskCreate(void (*fn)(void *),const char *name,unsigned size,void *arg,unsigned priority,TaskHandle_t *out)
{(void)name;assert(size==CONFIG_MEETING_NETWORK_TASK_STACK_SIZE);(void)arg;(void)priority;if(!spawn_ok)return 0;task_fn=fn;if(out)*out=(void *)2;return pdPASS;}
void vTaskDelay(unsigned ticks){(void)ticks;state_before_wait=meeting_network_get_state();
 if(atomic_load(&s_stop_requested)){if(recover_after_delay){assert(s_config.device_token[0]);stop_failure=0;recover_after_delay=0;}return;}
 atomic_store(&s_enabled,false);}
void xTaskNotifyGive(TaskHandle_t h){assert(h==(void *)2);notifications++;}
unsigned ulTaskNotifyTake(BaseType_t clear,unsigned ticks){(void)clear;vTaskDelay(ticks);return 0;}
void vTaskDelete(TaskHandle_t h){(void)h;}
bool sc_wifi_running(void){return wifi_running;}
bool meeting_sta_running(void){return sta_running;}
meeting_cleanup_result meeting_runtime_resume_cleanup(void){return MEETING_CLEANUP_DONE;}
meeting_delete_pass_result meeting_runtime_resume_deletions(const meeting_online_config *config){(void)config;return (meeting_delete_pass_result){.failed=delete_failures};}
bool meeting_sta_has_ip(void){return ip;}
meeting_time_client_result meeting_time_device_sync(meeting_time_challenge *challenge,const char *https,const char *http,
 const meeting_time_anchor *anchor,bool (*cancel)(void *),void *cancel_ctx,const meeting_clock_io *io,meeting_clock *clock){
 (void)challenge;(void)io;assert(anchor&&!strcmp(https,"https://example.com")&&!strcmp(http,"http://example.com"));
 clock_sync_calls++;if(clock_sync_record){meeting_network_record_request();assert(cancel(cancel_ctx));return MEETING_TIME_CLIENT_PAUSED;}
 if(!clock_sync_success)return MEETING_TIME_CLIENT_FAILED;
 *clock=(meeting_clock){.trusted=true,.epoch_at_sync=1800000000,.uptime_at_sync_ms=1000,.persisted_floor=1800000000};return MEETING_TIME_CLIENT_OK;
}
esp_err_t meeting_sta_start(const char *ssid,const char *password){assert(!strcmp(ssid,"wifi")&&!strcmp(password,"12345678"));sta_running=1;connects++;return ESP_OK;}
esp_err_t meeting_sta_reconnect(void){connects++;return ESP_OK;}
esp_err_t meeting_sta_stop(void){if(stop_failure)return ESP_FAIL;sta_running=0;return ESP_OK;}
void meeting_config_clear(meeting_config *c){memset(c,0,sizeof(*c));}
meeting_worker_result meeting_worker_pass(const meeting_online_config *config,sc_file_entry_t *entries,size_t capacity)
{
    assert(entries&&capacity==UPLOAD_LIST_BATCH&&config->https.time_valid(config->https.cancel_ctx));
    assert(meeting_network_get_state()==(ip?MEETING_NETWORK_SYNCING:MEETING_NETWORK_CONNECTING));
    meeting_network_record_saved();
    assert(!config->https.is_cancelled(config->https.cancel_ctx));passes++;
    if(stop_during_pass){assert(meeting_network_stop_request());assert(config->https.is_cancelled(config->https.cancel_ctx));
        assert(!meeting_network_stop_complete()&&!meeting_network_record_ready());}
    return (meeting_worker_result){0};
}
static bool commit_floor(void *ctx,int64_t floor){(void)ctx;(void)floor;return true;}
static int read_floor(void *ctx,int64_t *floor){(void)ctx;*floor=1800000000;return 0;}
static bool set_wall(void *ctx,int64_t epoch){(void)ctx;(void)epoch;return true;}
int main(void)
{
    meeting_config cfg={.ssid="wifi",.password="12345678",.origin="https://example.com",.device_token="token",.ca_pem="cert",.timezone_minutes=480};
    meeting_clock clock={.trusted=true,.epoch_at_sync=1800000000,.uptime_at_sync_ms=1000,.persisted_floor=1800000000};
    meeting_clock_io io={.commit_floor=commit_floor};
    wifi_running=1;assert(meeting_network_activate(&cfg,&clock,&io)==ESP_ERR_INVALID_STATE);wifi_running=0;
    wall=1800000200;assert(meeting_network_activate(&cfg,&clock,&io)==ESP_ERR_INVALID_STATE&&mutex_deleted==1);wall=1800000000;
    spawn_ok=0;assert(meeting_network_activate(&cfg,&clock,&io)==ESP_ERR_NO_MEM&&mutex_deleted==2&&!meeting_network_enabled());
    spawn_ok=1;assert(meeting_network_activate(&cfg,&clock,&io)==ESP_OK&&meeting_network_enabled());
    assert(meeting_network_get_state()==MEETING_NETWORK_CONNECTING);
    int16_t zone=0;try_calls=0;fail_try=1;assert(meeting_network_clock_view(&zone)==MEETING_CLOCK_CHECKING);
    try_calls=0;fail_try=2;assert(meeting_network_clock_view(&zone)==MEETING_CLOCK_CHECKING);
    try_calls=0;fail_try=0;assert(meeting_network_clock_view(&zone)==MEETING_CLOCK_TRUSTED&&zone==480);
    wall=1800000200;assert(meeting_network_clock_view(&zone)==MEETING_CLOCK_OFF);wall=1800000000;s_clock=clock;
    int64_t captured=0;assert(meeting_network_trusted_time(&captured)&&captured==1800000000);
    wall=1800000200;assert(!meeting_network_trusted_time(&captured));wall=1800000000;
    s_clock=clock;assert(meeting_network_trusted_time(&captured));
    assert(meeting_network_activate(&cfg,&clock,&io)==ESP_ERR_INVALID_STATE);
    task_fn(NULL);assert(connects==1&&passes==1&&!meeting_network_enabled());
    assert(meeting_network_get_state()==MEETING_NETWORK_OFF);
    assert(notifications==1);
    /* Failed deletion recovery must not be presented as a healthy idle state. */
    atomic_store(&s_enabled,true);ip=1;delete_failures=1;task_fn(NULL);
    assert(state_before_wait==MEETING_NETWORK_RETRY&&passes==2);
    ip=0;delete_failures=0;
    /* A pending recording must stop Wi-Fi before it reports readiness and
     * must never start an upload while the radio is being released. */
    atomic_store(&s_enabled,true);sta_running=1;meeting_network_record_request();
    assert(!meeting_network_record_ready());task_fn(NULL);
    assert(!sta_running&&atomic_load(&s_record_ready)&&passes==2);
    atomic_store(&s_enabled,true);sta_running=1;stop_failure=1;
    meeting_network_record_request();task_fn(NULL);
    assert(sta_running&&!atomic_load(&s_record_ready)&&passes==2);
    stop_failure=0;meeting_network_record_release();
    assert(!atomic_load(&s_record_requested));
    /* A stop requested inside an in-flight worker cancels it, retries radio
     * shutdown before wiping credentials, and only then reports complete. */
    atomic_store(&s_enabled,false);s_task=NULL;sta_running=0;ip=1;stop_during_pass=1;
    assert(meeting_network_activate(&cfg,&clock,&io)==ESP_OK);
    stop_failure=1;recover_after_delay=1;task_fn(NULL);
    assert(meeting_network_stop_complete()&&!meeting_network_enabled()&&!sta_running);
    assert(!meeting_network_trusted_time(&captured));
    assert(meeting_network_clock_view(&zone)==MEETING_CLOCK_OFF);
    assert(!s_config.device_token[0]&&!s_config.password[0]&&!s_clock.trusted&&s_task==NULL&&s_clock_lock==NULL);
    assert(meeting_network_get_state()==MEETING_NETWORK_OFF);
    int before=notifications;meeting_network_record_saved();assert(notifications==before);
    /* Restore must not upload until signed time is established. */
    meeting_time_anchor anchor={0};io.read_floor=read_floor;io.set_wall_time=set_wall;
    stop_during_pass=0;ip=1;sta_running=0;clock_sync_success=0;
    assert(meeting_network_activate_restored(&cfg,&anchor,"http://example.com",&io)==ESP_OK);
    int before_pass=passes;task_fn(NULL);assert(clock_sync_calls==1&&passes==before_pass);
    /* During bootstrap, a recording request wins before any upload starts. */
    atomic_store(&s_enabled,true);clock_sync_record=1;task_fn(NULL);
    assert(clock_sync_calls==2&&passes==before_pass&&!sta_running&&atomic_load(&s_record_ready));
    meeting_network_record_release();clock_sync_record=0;clock_sync_success=1;atomic_store(&s_enabled,true);task_fn(NULL);
    assert(clock_sync_calls==3&&passes==before_pass+1);
    return 0;
}
