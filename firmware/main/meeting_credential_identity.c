#include "meeting_credential_identity.h"
#include <string.h>

bool meeting_reset_identity_from_config(const meeting_config *config,const char *event_id,meeting_reset_record *out)
{
    if(!config||!event_id||!out||!meeting_config_revocation_identity_valid(config))return false;
    size_t event_len=0;while(event_len<sizeof(out->event_id)&&event_id[event_len])event_len++;
    if(!event_len||event_len>=sizeof(out->event_id))return false;
    meeting_reset_record record={.state=MEETING_RESET_PENDING};
    memcpy(record.origin,config->origin,sizeof(record.origin));
    memcpy(record.device_id,config->device_id,sizeof(record.device_id));
    memcpy(record.credential_id,config->credential_id,sizeof(record.credential_id));
    memcpy(record.event_id,event_id,event_len);
    uint8_t encoded[MEETING_RESET_RECORD_SIZE];
    if(!meeting_reset_encode(&record,encoded))return false;
    *out=record;return true;
}
