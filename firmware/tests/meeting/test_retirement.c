#include "meeting_retirement.h"
#include <assert.h>
#include <string.h>
typedef struct {uint8_t marker[MEETING_RETIRED_SIZE];bool present,files_gone,proof,fail_marker;int fail_part;bool parts[MEETING_RETIRE_PARTS];} fixture;
static bool cancelled(void *ctx){(void)ctx;return false;}
static int read_marker(void *ctx,uint8_t out[MEETING_RETIRED_SIZE]){fixture *f=ctx;if(!f->present)return 1;memcpy(out,f->marker,sizeof(f->marker));return 0;}
static bool write_marker(void *ctx,const uint8_t marker[MEETING_RETIRED_SIZE]){fixture *f=ctx;if(f->fail_marker)return false;memcpy(f->marker,marker,sizeof(f->marker));f->present=true;return true;}
static bool proof(void *ctx,const meeting_retired *id){(void)id;return ((fixture *)ctx)->proof;}
static bool files_absent(void *ctx){return ((fixture *)ctx)->files_gone;}
static bool erase(void *ctx,meeting_retire_part part){fixture *f=ctx;assert(f->present&&f->files_gone);if(f->fail_part==(int)part)return false;f->parts[part]=false;return true;}
int main(void)
{
    meeting_retired id={.file_id=7,.meeting_id={1}};
    fixture f={.files_gone=true,.proof=true,.fail_part=-1};meeting_retirement_io io={&f,cancelled,read_marker,write_marker,proof,files_absent,erase};
    assert(meeting_retirement_run(&id,false,&io)==MEETING_RETIRE_ERROR&&!f.present);
    f.proof=false;assert(meeting_retirement_run(&id,true,&io)==MEETING_RETIRE_ERROR&&!f.present);
    f.proof=true;f.fail_marker=true;assert(meeting_retirement_run(&id,true,&io)==MEETING_RETIRE_ERROR&&!f.present);
    for(int stop=0;stop<MEETING_RETIRE_PARTS;stop++){
        f=(fixture){.files_gone=true,.proof=true,.fail_part=stop};for(int i=0;i<MEETING_RETIRE_PARTS;i++)f.parts[i]=true;
        assert(meeting_retirement_run(&id,true,&io)==MEETING_RETIRE_ERROR&&f.present);
        f.fail_part=-1;f.proof=false; /* prior metadata may already be gone */
        assert(meeting_retirement_run(&id,false,&io)==MEETING_RETIRE_DONE);
        meeting_retired completed;assert(meeting_retired_decode(f.marker,&completed)&&completed.complete);
        for(int i=0;i<MEETING_RETIRE_PARTS;i++)assert(!f.parts[i]);
    }
    f.files_gone=false;assert(meeting_retirement_run(&id,false,&io)==MEETING_RETIRE_ERROR);
    f.files_gone=true;meeting_retired other=id;other.meeting_id[0]=2;
    assert(meeting_retirement_run(&other,false,&io)==MEETING_RETIRE_ERROR);
    f.marker[5]^=1;assert(meeting_retirement_run(&id,false,&io)==MEETING_RETIRE_ERROR);
    return 0;
}
