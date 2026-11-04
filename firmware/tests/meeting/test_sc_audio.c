#include <assert.h>
#include <stdlib.h>
#include "../../main/sc_audio.c"
static int wake_result,format_result,sleep_result,sleep_count,sem_available,deleted,notified;
static int force_fallback,fail_realloc,realloc_calls;
const char *esp_err_to_name(esp_err_t e){(void)e;return "error";}
esp_err_t bsp_audio_init(void){return ESP_OK;}
esp_err_t bsp_audio_sleep(void){sleep_count++;return sleep_result;}
esp_err_t bsp_audio_wake(void){return wake_result;}
esp_err_t bsp_audio_set_format(uint32_t hz,uint8_t bits,uint8_t ch){(void)hz;(void)bits;(void)ch;return format_result;}
void bsp_audio_set_volume(uint8_t volume){(void)volume;}
esp_err_t bsp_audio_read(void *data,size_t size){(void)data;(void)size;return ESP_FAIL;}
void *heap_caps_malloc(size_t n,unsigned caps){(void)caps;if(force_fallback&&n==sizeof(OpusEncoder))return NULL;return malloc(n);}
void *heap_caps_realloc(void *p,size_t n,unsigned caps){(void)caps;realloc_calls++;return fail_realloc?NULL:realloc(p,n);}
size_t heap_caps_get_free_size(unsigned caps){(void)caps;return 100000;}
size_t heap_caps_get_largest_free_block(unsigned caps){(void)caps;return force_fallback?sizeof(OpusEncoder)-1:100000;}
int opus_encoder_get_size(int channels){(void)channels;return sizeof(OpusEncoder);}
int opus_encoder_init(OpusEncoder *e,int rate,int channels,int app){(void)e;(void)rate;(void)channels;(void)app;return OPUS_OK;}
int opus_encoder_ctl(OpusEncoder *e,int request,...){(void)e;(void)request;return 0;}
int opus_encode(OpusEncoder *e,const int16_t *pcm,int samples,uint8_t *data,int cap){(void)e;(void)pcm;(void)samples;(void)data;(void)cap;return 1;}
TaskHandle_t xTaskCreateStatic(void (*fn)(void *),const char *name,unsigned size,void *arg,unsigned prio,StackType_t *stack,StaticTask_t *tcb){(void)fn;(void)name;(void)size;(void)arg;(void)prio;(void)stack;(void)tcb;return (void *)2;}
void vTaskDelete(TaskHandle_t task){assert(task==(void *)2);deleted++;}
void xTaskNotifyGive(TaskHandle_t task){assert(task==(void *)2);notified++;}
unsigned ulTaskNotifyTake(BaseType_t clear,unsigned ticks){(void)clear;(void)ticks;return 0;}
unsigned uxTaskGetStackHighWaterMark(TaskHandle_t task){(void)task;return 100;}
SemaphoreHandle_t xSemaphoreCreateBinary(void){return (void *)1;}
BaseType_t xSemaphoreTake(SemaphoreHandle_t h,unsigned ticks){(void)h;(void)ticks;if(sem_available){sem_available=0;return pdTRUE;}return pdFALSE;}
BaseType_t xSemaphoreGive(SemaphoreHandle_t h){(void)h;sem_available=1;return pdTRUE;}
int main(void){
 assert(sc_audio_init(NULL,NULL)==ESP_OK);assert(sleep_count==1);
 /* Boot releases the idle encoder for TLS; recording must recreate it. */
 assert(s_enc&&sc_audio_release_encoder_idle()&&!s_enc&&deleted==1);
 wake_result=ESP_FAIL;assert(sc_audio_prepare()==ESP_FAIL);assert(!s_codec_prepared&&sleep_count==2);
 wake_result=ESP_OK;format_result=ESP_FAIL;assert(sc_audio_prepare()==ESP_FAIL);assert(!s_codec_prepared&&sleep_count==3);
 sleep_result=ESP_FAIL;assert(sc_audio_prepare()==ESP_FAIL&&s_codec_needs_reset);
 assert(sc_audio_prepare()==ESP_ERR_INVALID_STATE&&s_codec_needs_reset);
 assert(!sc_audio_release_encoder_idle());sleep_result=ESP_OK;
 format_result=ESP_OK;assert(sc_audio_prepare()==ESP_OK&&s_codec_prepared);
 assert(sc_audio_start()==ESP_OK&&notified==1&&!s_quiescent);
 assert(!sc_audio_stop());assert(!sc_audio_release_encoder_idle());
 assert(sc_audio_prepare()==ESP_ERR_INVALID_STATE); /* in-flight capture must not be restarted */
 s_codec_prepared=false;s_quiescent=true;sem_available=1;
 assert(sc_audio_stop()&&sc_audio_release_encoder_idle()&&deleted==2);
 assert(sc_audio_prepare()==ESP_OK&&s_codec_prepared);
 assert(sc_audio_release_encoder_idle()&&deleted==3);
 force_fallback=1;assert(sc_audio_prepare()==ESP_OK&&realloc_calls==1);
 assert(sc_audio_release_encoder_idle()&&deleted==4);
 fail_realloc=1;assert(sc_audio_prepare()==ESP_ERR_NO_MEM&&realloc_calls==2&&!s_enc);
 return 0;
}
