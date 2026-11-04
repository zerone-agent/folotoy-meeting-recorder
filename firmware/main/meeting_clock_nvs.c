#include "meeting_clock_nvs.h"
#include <limits.h>
#include <string.h>
#include <sys/time.h>

static uint32_t crc32(const uint8_t *p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static void encode(int64_t floor,uint8_t data[16])
{
    memcpy(data,"MCT1",4);for(unsigned i=0;i<8;i++)data[4+i]=(uint8_t)((uint64_t)floor>>(8*i));
    uint32_t c=crc32(data,12);for(unsigned i=0;i<4;i++)data[12+i]=(uint8_t)(c>>(8*i));
}
static int read_floor(void *ctx,int64_t *floor)
{
    if(!ctx||!floor)return -1;
    uint8_t data[16];size_t len=sizeof(data);esp_err_t err=nvs_get_blob(*(nvs_handle_t *)ctx,"floor",data,&len);
    if(err==ESP_ERR_NVS_NOT_FOUND)return 1;
    if(err!=ESP_OK||len!=sizeof(data)||memcmp(data,"MCT1",4))return -1;
    uint32_t crc=0;uint64_t value=0;for(unsigned i=0;i<4;i++)crc|=(uint32_t)data[12+i]<<(8*i);
    for(unsigned i=0;i<8;i++)value|=(uint64_t)data[4+i]<<(8*i);
    if(crc!=crc32(data,12)||value>INT64_MAX)return -1;
    *floor=(int64_t)value;return 0;
}
static bool commit_floor(void *ctx,int64_t floor)
{
    if(!ctx||floor<0)return false;
    int64_t old=0;int status=read_floor(ctx,&old);
    if(status<0||(status==0&&floor<old))return false;
    uint8_t data[16];encode(floor,data);nvs_handle_t h=*(nvs_handle_t *)ctx;
    if(nvs_set_blob(h,"floor",data,sizeof(data))!=ESP_OK||nvs_commit(h)!=ESP_OK)return false;
    int64_t check=0;return read_floor(ctx,&check)==0&&check==floor;
}
static bool set_wall_time(void *ctx,int64_t epoch)
{
    (void)ctx;struct timeval tv={.tv_sec=(time_t)epoch,.tv_usec=0};
    return (int64_t)tv.tv_sec==epoch&&settimeofday(&tv,NULL)==0;
}
bool meeting_clock_nvs_bind(nvs_handle_t *handle,meeting_clock_io *io)
{
    if(!handle||!io)return false;
    *io=(meeting_clock_io){.ctx=handle,.read_floor=read_floor,.commit_floor=commit_floor,.set_wall_time=set_wall_time};return true;
}
