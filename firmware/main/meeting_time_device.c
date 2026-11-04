#include "meeting_time_device.h"
#include "meeting_time_http.h"
#include "esp_random.h"
#include "esp_timer.h"

typedef struct {meeting_time_http http;} device_context;
static bool random_nonce(void *ctx,uint8_t nonce[32]){(void)ctx;esp_fill_random(nonce,32);return true;}
static bool uptime(void *ctx,uint64_t *out){(void)ctx;int64_t now=esp_timer_get_time();if(now<0)return false;*out=(uint64_t)now/1000;return true;}
static bool cancelled(void *ctx){device_context *c=ctx;return c->http.cancelled(c->http.cancel_ctx);}
static bool fetch(void *ctx,const uint8_t nonce[32],char *response,size_t capacity,size_t *length,int *status){
 device_context *c=ctx;return meeting_time_http_fetch(&c->http,nonce,response,capacity,length,status);
}
meeting_time_client_result meeting_time_device_sync(meeting_time_challenge *challenge,
 const char *https_origin,const char *http_origin,const meeting_time_anchor *anchor,
 bool (*cancel)(void *),void *cancel_ctx,const meeting_clock_io *clock_io,meeting_clock *clock){
 if(!cancel)return MEETING_TIME_CLIENT_FAILED;
 device_context ctx={.http={https_origin,http_origin,cancel,cancel_ctx}};
 meeting_time_client_io io={&ctx,random_nonce,uptime,cancelled,fetch};
 return meeting_time_client_run(challenge,https_origin,anchor,&io,clock_io,clock);
}
