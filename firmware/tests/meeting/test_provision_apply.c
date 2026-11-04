#include "meeting_provision_apply.h"
#include <assert.h>
#include <string.h>
static const uint8_t json[]="{\"version\":1,\"ssid\":\"wifi\",\"password\":\"12345678\",\"origin\":\"https://example.com\",\"deviceToken\":\"abcdefghijklmnopqrstuvwxyz012345\",\"caPem\":\"dummy\",\"caSha256\":\"0101010101010101010101010101010101010101010101010101010101010101\",\"utc\":1800000000,\"timezoneMinutes\":480}";
typedef struct {uint8_t blob[MEETING_CONFIG_RECORD_MAX];size_t len;int64_t floor;int floor_exists,fail_store,valid_calls,disconnect_at,clock_set;} fixture;
static bool ca(const char *pem,uint8_t hash[32],void *ctx){(void)ctx;if(strcmp(pem,"dummy"))return false;memset(hash,1,32);return true;}
static bool session(void *ctx){fixture *f=ctx;f->valid_calls++;return !f->disconnect_at||f->valid_calls<f->disconnect_at;}
static int read_config(void *ctx,uint8_t *out,size_t cap,size_t *size){fixture *f=ctx;if(!f->len)return 1;assert(cap>=f->len);memcpy(out,f->blob,f->len);*size=f->len;return 0;}
static bool write_config(void *ctx,const uint8_t *data,size_t size){fixture *f=ctx;if(f->fail_store)return false;memcpy(f->blob,data,size);f->len=size;return true;}
static int read_floor(void *ctx,int64_t *floor){fixture *f=ctx;if(!f->floor_exists)return 1;*floor=f->floor;return 0;}
static bool write_floor(void *ctx,int64_t floor){fixture *f=ctx;f->floor=floor;f->floor_exists=1;return true;}
static bool set_time(void *ctx,int64_t epoch){fixture *f=ctx;(void)epoch;f->clock_set++;return true;}
int main(void)
{
    fixture f={0};meeting_config_store_io store={&f,read_config,write_config};meeting_clock_io clock_io={&f,read_floor,write_floor,set_time};
    uint8_t scratch[MEETING_CONFIG_RECORD_MAX],hash[32];memset(hash,1,sizeof hash);meeting_config out;meeting_clock clock;
    for(int step=1;step<=5;step++){
        fixture interrupted={.disconnect_at=step};meeting_config_store_io interrupted_store={&interrupted,read_config,write_config};
        meeting_clock_io interrupted_clock={&interrupted,read_floor,write_floor,set_time};
        assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,true,session,&interrupted,true,10,10,&interrupted_clock,&clock,&interrupted_store,ca,NULL,scratch,&out)==MEETING_PROVISION_SESSION_LOST);
        assert(!clock.trusted&&!out.ssid[0]);
        if(step<=3)assert(!interrupted.floor_exists&&!interrupted.clock_set&&!interrupted.len);
    }
    f.floor_exists=1;f.floor=1800000001;
    assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,true,session,&f,false,10,10,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_CLOCK_FAILED);
    assert(!f.len&&!f.clock_set&&!clock.trusted);f.floor_exists=0;
    assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,false,session,&f,true,10,10,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_REJECTED);
    assert(!f.len&&!f.clock_set);
    hash[0]=2;assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,true,session,&f,true,10,10,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_REJECTED);assert(!f.len&&!f.clock_set);hash[0]=1;
    assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,true,session,&f,false,10,10,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_CLOCK_FAILED);assert(!f.len&&!clock.trusted);
    f.fail_store=1;assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,true,session,&f,true,10,10,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_STORE_FAILED);assert(!clock.trusted&&!f.len);
    f.fail_store=0;f.valid_calls=0;f.disconnect_at=5;
    assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,true,session,&f,false,10,10,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_SESSION_LOST);
    assert(!clock.trusted&&f.len); /* durable config alone never starts upload */
    f.valid_calls=0;f.disconnect_at=0;
    assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,true,session,&f,false,10,10,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_APPLIED);
    assert(clock.trusted&&!strcmp(out.ssid,"wifi"));
    uint8_t changed[sizeof(json)];memcpy(changed,json,sizeof(json));char *ssid=strstr((char *)changed,"wifi");assert(ssid);memcpy(ssid,"test",4);
    char *utc=strstr((char *)changed,"1800000000");assert(utc);memcpy(utc,"1800001000",10);
    int64_t floor_before=f.floor;int set_before=f.clock_set;uint8_t saved[MEETING_CONFIG_RECORD_MAX];size_t saved_len=f.len;memcpy(saved,f.blob,saved_len);
    assert(meeting_provision_apply(changed,sizeof(changed)-1,hash,1800001000,true,session,&f,false,10,10,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_STORE_FAILED);
    assert(f.floor==floor_before&&f.clock_set==set_before&&f.len==saved_len&&!memcmp(f.blob,saved,saved_len));

    assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,true,session,&f,false,10,3010,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_APPLIED);
    assert(clock.epoch_at_sync==1800000003);
    assert(meeting_provision_apply(json,sizeof(json)-1,hash,1800000000,true,session,&f,false,10,120011,&clock_io,&clock,&store,ca,NULL,scratch,&out)==MEETING_PROVISION_REJECTED);
    return 0;
}
