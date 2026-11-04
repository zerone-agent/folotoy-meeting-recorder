#include "meeting_receipt_json.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static const char *valid="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"committed\":true,\"manifest\":{\"count\":2,\"bytes\":100,\"digest\":\"0707070707070707070707070707070707070707070707070707070707070707\",\"reason\":\"interrupted\",\"recordedAt\":null,\"timeTrust\":\"uncalibrated\"}}";
int main(void){meeting_manifest m={.count=2,.bytes=100,.reason=3};memcpy(m.id,"12345678-1234-4123-8123-123456789abc",37);memset(m.digest,7,32);assert(meeting_receipt_json_matches(valid,strlen(valid),200,&m));assert(!meeting_receipt_json_matches(valid,strlen(valid),500,&m));assert(!meeting_receipt_json_matches(valid,strlen(valid)-1,200,&m));const char *bad="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"committed\":true,\"committed\":false,\"manifest\":{}}";assert(!meeting_receipt_json_matches(bad,strlen(bad),200,&m));bad="{\"id\":\"12345678-1234-4123-8123-123456789abc\\u0000x\",\"committed\":true,\"manifest\":{}}";assert(!meeting_receipt_json_matches(bad,strlen(bad),200,&m));
 const char *error="{\"error\":\"integrity\"}";assert(meeting_receipt_json_integrity_error(error,strlen(error),409));
 const char *cancelled="{\"state\":\"cancelled\"}";
 assert(meeting_cancel_json_parse(cancelled,strlen(cancelled),200,&m)==MEETING_CANCEL_CANCELLED);
 assert(meeting_cancel_json_parse(cancelled,strlen(cancelled),409,&m)==MEETING_CANCEL_UNKNOWN);
 const char *conflict="{\"state\":\"cancelled\",\"committed\":true}";
 assert(meeting_cancel_json_parse(conflict,strlen(conflict),200,&m)==MEETING_CANCEL_UNKNOWN);
 const char *duplicate_state="{\"state\":\"cancelled\",\"state\":\"committed\"}";
 assert(meeting_cancel_json_parse(duplicate_state,strlen(duplicate_state),200,&m)==MEETING_CANCEL_UNKNOWN);
 char committed[512];snprintf(committed,sizeof(committed),"{\"state\":\"committed\",%s",valid+1);
 assert(meeting_cancel_json_parse(committed,strlen(committed),200,&m)==MEETING_CANCEL_COMMITTED);
 assert(meeting_cancel_json_parse(committed,strlen(committed),200,NULL)==MEETING_CANCEL_UNKNOWN);
 meeting_manifest wrong=m;wrong.digest[0]^=1;
 assert(meeting_cancel_json_parse(committed,strlen(committed),200,&wrong)==MEETING_CANCEL_UNKNOWN);
 const char *duplicate_error="{\"error\":\"integrity\",\"error\":\"conflict\"}";
 assert(!meeting_receipt_json_integrity_error(duplicate_error,strlen(duplicate_error),409));
 assert(!meeting_receipt_json_integrity_error(error,strlen(error),200));
 meeting_manifest changed=m;changed.reason=0;assert(!meeting_receipt_json_matches(valid,strlen(valid),200,&changed));
 changed=m;changed.digest[0]^=1;assert(!meeting_receipt_json_matches(valid,strlen(valid),200,&changed));
 changed=m;changed.recorded_at_present=true;changed.recorded_at=1800000000;assert(!meeting_receipt_json_matches(valid,strlen(valid),200,&changed));
 changed=m;changed.id[0]='2';assert(!meeting_receipt_json_matches(valid,strlen(valid),200,&changed));
 changed=m;changed.bytes++;assert(!meeting_receipt_json_matches(valid,strlen(valid),200,&changed));
 changed=m;changed.time_trust=1;assert(!meeting_receipt_json_matches(valid,strlen(valid),200,&changed));
 const char *duplicate_bytes="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"committed\":true,\"manifest\":{\"count\":2,\"bytes\":100,\"bytes\":101,\"digest\":\"0707070707070707070707070707070707070707070707070707070707070707\",\"reason\":\"interrupted\",\"recordedAt\":null,\"timeTrust\":\"uncalibrated\"}}";
 assert(!meeting_receipt_json_matches(duplicate_bytes,strlen(duplicate_bytes),200,&m));
 const char *repair="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"overall\":\"failed\",\"expectedCount\":2,\"missingCount\":0,\"damagedSequences\":[1]}";
 uint8_t bitmap[1];assert(meeting_repair_json_parse(repair,strlen(repair),200,&m,bitmap,sizeof bitmap)&&bitmap[0]==2);
 const char *missing="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"overall\":\"failed\",\"expectedCount\":2,\"missingCount\":1,\"damagedSequences\":[1]}";
 assert(!meeting_repair_json_parse(missing,strlen(missing),200,&m,bitmap,sizeof bitmap)&&!bitmap[0]);
 const char *duplicate="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"overall\":\"failed\",\"expectedCount\":2,\"missingCount\":0,\"damagedSequences\":[1,1]}";
 assert(!meeting_repair_json_parse(duplicate,strlen(duplicate),200,&m,bitmap,sizeof bitmap));
 const char *unknown="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"overall\":\"failed\",\"expectedCount\":2,\"missingCount\":0,\"damagedSequences\":[]}";
 assert(!meeting_repair_json_parse(unknown,strlen(unknown),200,&m,bitmap,sizeof bitmap));
 const char *window="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"first\":0,\"chunks\":[{\"seq\":1,\"size\":3,\"digest\":\"0707070707070707070707070707070707070707070707070707070707070707\",\"valid\":true}]}";
 meeting_remote_chunk chunks[16];assert(meeting_chunk_window_json_parse(window,strlen(window),200,m.id,0,chunks));
 assert(!chunks[0].present&&chunks[1].present&&chunks[1].valid&&chunks[1].size==3&&chunks[1].digest[0]==7);
 assert(!meeting_chunk_window_json_parse(window,strlen(window),200,m.id,16,chunks));
 const char *legacy="{\"id\":\"12345678-1234-4123-8123-123456789abc\",\"chunks\":[{\"seq\":1,\"size\":3,\"digest\":\"0707070707070707070707070707070707070707070707070707070707070707\",\"valid\":true}]}";
 assert(meeting_legacy_chunk_window_json_parse(legacy,strlen(legacy),200,m.id,0,chunks)&&chunks[1].present&&chunks[1].valid);
 assert(!meeting_legacy_chunk_window_json_parse(legacy,strlen(legacy),200,"other",0,chunks));return 0;}
