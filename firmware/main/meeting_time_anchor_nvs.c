#include "meeting_time_anchor_nvs.h"
static int read_anchor(void *ctx,uint8_t out[MEETING_TIME_ANCHOR_SIZE]){
 if(!ctx||!out)return -1;
 size_t n=MEETING_TIME_ANCHOR_SIZE;esp_err_t e=nvs_get_blob(*(nvs_handle_t *)ctx,"anchor",out,&n);
 if(e==ESP_ERR_NVS_NOT_FOUND)return 1;
 return e==ESP_OK&&n==MEETING_TIME_ANCHOR_SIZE?0:-1;
}
static bool write_anchor(void *ctx,const uint8_t data[MEETING_TIME_ANCHOR_SIZE]){
 return ctx&&data&&nvs_set_blob(*(nvs_handle_t *)ctx,"anchor",data,MEETING_TIME_ANCHOR_SIZE)==ESP_OK&&nvs_commit(*(nvs_handle_t *)ctx)==ESP_OK;
}
bool meeting_time_anchor_nvs_bind(nvs_handle_t *handle,meeting_time_anchor_io *out){if(!handle||!out)return false;*out=(meeting_time_anchor_io){handle,read_anchor,write_anchor};return true;}
