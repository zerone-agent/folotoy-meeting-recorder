#include "meeting_retirement_nvs.h"
#include <stdio.h>
#include <string.h>
static bool make_key(uint32_t id,char prefix,int slot,char key[12])
{
    if(!id)return false;
    int n=slot<0?snprintf(key,12,"%c%08lx",prefix,(unsigned long)id):snprintf(key,12,"%c%08lx%d",prefix,(unsigned long)id,slot);
    return n==(slot<0?9:10);
}
static int read_marker(void *ctx,uint8_t out[MEETING_RETIRED_SIZE])
{
    meeting_retirement_nvs *s=ctx;char key[12];size_t len=MEETING_RETIRED_SIZE;
    if(!s||!out||!make_key(s->file_id,'d',-1,key))return -1;
    esp_err_t err=nvs_get_blob(s->markers,key,out,&len);if(err==ESP_ERR_NVS_NOT_FOUND)return 1;
    if(err==ESP_ERR_NVS_INVALID_LENGTH||(err==ESP_OK&&len!=MEETING_RETIRED_SIZE)){memset(out,0,MEETING_RETIRED_SIZE);return 0;}
    return err==ESP_OK?0:-1;
}
int meeting_retirement_nvs_read(nvs_handle_t handle,uint32_t file_id,meeting_retired *out)
{
    if(!out||!file_id)return -1;
    meeting_retirement_nvs state={.markers=handle,.file_id=file_id};uint8_t data[MEETING_RETIRED_SIZE];
    int status=read_marker(&state,data);
    if(status!=0)return status;
    return meeting_retired_decode(data,out)&&out->file_id==file_id?0:-1;
}
static bool write_marker(void *ctx,const uint8_t marker[MEETING_RETIRED_SIZE])
{
    meeting_retirement_nvs *s=ctx;char key[12];
    return s&&marker&&make_key(s->file_id,'d',-1,key)&&
           nvs_set_blob(s->markers,key,marker,MEETING_RETIRED_SIZE)==ESP_OK&&nvs_commit(s->markers)==ESP_OK;
}
static bool cancelled(void *ctx){meeting_retirement_nvs *s=ctx;return s->cancelled(s->ctx);}
static bool proof(void *ctx,const meeting_retired *id){meeting_retirement_nvs *s=ctx;return s->verify_completed(s->ctx,id);}
static bool absent(void *ctx){meeting_retirement_nvs *s=ctx;return s->files_absent(s->ctx);}
static bool erase_part(void *ctx,meeting_retire_part part)
{
    meeting_retirement_nvs *s=ctx;char key[12];nvs_handle_t handle;char prefix;int slot=-1;
    switch(part){
    case MEETING_RETIRE_SNAPSHOT_0:handle=s->snapshots;prefix='m';slot=0;break;
    case MEETING_RETIRE_SNAPSHOT_1:handle=s->snapshots;prefix='m';slot=1;break;
    case MEETING_RETIRE_RECEIPT:handle=s->receipts;prefix='c';break;
    case MEETING_RETIRE_RECORD:handle=s->records;prefix='r';break;
    case MEETING_RETIRE_INTENT:handle=s->intents;prefix='c';break;
    case MEETING_RETIRE_SEAL:handle=s->records;prefix='s';break;
    default:return false;
    }
    if(!make_key(s->file_id,prefix,slot,key))return false;
    esp_err_t err=nvs_erase_key(handle,key);
    if(err!=ESP_OK&&err!=ESP_ERR_NVS_NOT_FOUND)return false;
    if(nvs_commit(handle)!=ESP_OK)return false;
    size_t len=0;return nvs_get_blob(handle,key,NULL,&len)==ESP_ERR_NVS_NOT_FOUND;
}
bool meeting_retirement_nvs_bind(meeting_retirement_nvs *s,meeting_retirement_io *io)
{
    if(!s||!io||!s->file_id||!s->cancelled||!s->verify_completed||!s->files_absent)return false;
    *io=(meeting_retirement_io){.ctx=s,.cancelled=cancelled,.read_marker=read_marker,.write_marker_sync=write_marker,
        .verify_completed=proof,.files_absent=absent,.erase_part_sync=erase_part};return true;
}
