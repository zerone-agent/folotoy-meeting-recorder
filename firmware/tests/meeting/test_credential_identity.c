#include "meeting_credential_identity.h"
#include <assert.h>
#include <string.h>
int main(void)
{
 meeting_config config={0};meeting_reset_record record;
 strcpy(config.origin,"https://example.com");strcpy(config.device_token,"abcdefghijklmnopqrstuvwxyz012345");
 assert(!meeting_reset_identity_from_config(&config,"event_1",&record)); /* legacy */
 strcpy(config.device_id,"device_1");strcpy(config.credential_id,"653bb1245e828fcda4fa53fcd5a3def5bd7654e651f54b4132b73d74e64435c4");
 assert(meeting_reset_identity_from_config(&config,"event_1",&record));
 assert(!strcmp(record.device_id,"device_1")&&!strcmp(record.event_id,"event_1")&&record.state==MEETING_RESET_PENDING);
 assert(!meeting_reset_identity_from_config(&config,"../invalid",&record));
 config.device_token[0]=' ';assert(!meeting_reset_identity_from_config(&config,"event_1",&record));config.device_token[0]='a';
 config.credential_id[0]='0';assert(!meeting_reset_identity_from_config(&config,"event_1",&record));
 memset(config.device_id,'x',sizeof(config.device_id));assert(!meeting_reset_identity_from_config(&config,"event_1",&record));
 return 0;
}
