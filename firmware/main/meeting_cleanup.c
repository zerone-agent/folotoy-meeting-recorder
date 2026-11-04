#include "meeting_cleanup.h"
#include <string.h>

static bool same(const meeting_confirmation *a,const meeting_confirmation *b)
{
    uint8_t x[MEETING_CONFIRMATION_SIZE],y[MEETING_CONFIRMATION_SIZE];
    return meeting_confirmation_encode(a,x)&&meeting_confirmation_encode(b,y)&&!memcmp(x,y,sizeof(x));
}
meeting_cleanup_result meeting_cleanup_run(uint32_t file_id,const meeting_snapshot *snapshot,
    const meeting_confirmation_io *receipts,const meeting_confirmation_io *intents,const meeting_cleanup_io *io)
{
    if(!file_id||!snapshot||!receipts||!intents||!io||!io->cancelled||!io->audio_exists||!io->verify_audio||
       !io->close_readers||!io->remove_audio||!io->remove_index||!io->verify_absent_and_space)return MEETING_CLEANUP_ERROR;
    if(io->cancelled(io->ctx))return MEETING_CLEANUP_PAUSED;
    meeting_confirmation receipt,intent;
    if(meeting_confirmation_load(receipts,&receipt)!=MEETING_CONFIRMATION_OK||receipt.file_id!=file_id||
       !meeting_confirmation_matches_snapshot(&receipt,snapshot)||!meeting_confirmation_commit(receipts,&receipt))return MEETING_CLEANUP_ERROR;
    meeting_confirmation_status status=meeting_confirmation_load(intents,&intent);
    if(status!=MEETING_CONFIRMATION_OK&&status!=MEETING_CONFIRMATION_EMPTY)return MEETING_CLEANUP_ERROR;
    if(status==MEETING_CONFIRMATION_OK&&!same(&receipt,&intent))return MEETING_CLEANUP_ERROR;
    int exists=io->audio_exists(io->ctx);
    if(exists<0||exists>1||(status==MEETING_CONFIRMATION_EMPTY&&exists!=0))return MEETING_CLEANUP_ERROR;
    if(exists==0&&!io->verify_audio(io->ctx,&receipt,snapshot))
        return io->cancelled(io->ctx)?MEETING_CLEANUP_PAUSED:MEETING_CLEANUP_ERROR;
    if(io->cancelled(io->ctx))return MEETING_CLEANUP_PAUSED;
    if(!meeting_confirmation_commit(intents,&receipt))return MEETING_CLEANUP_ERROR;
    if(io->cancelled(io->ctx))return MEETING_CLEANUP_PAUSED;
    if(!io->close_readers(io->ctx))return MEETING_CLEANUP_ERROR;
    if(io->cancelled(io->ctx))return MEETING_CLEANUP_PAUSED;
    if(!io->remove_audio(io->ctx))return MEETING_CLEANUP_ERROR;
    if(io->cancelled(io->ctx))return MEETING_CLEANUP_PAUSED;
    if(!io->remove_index(io->ctx)||!io->verify_absent_and_space(io->ctx))return MEETING_CLEANUP_ERROR;
    return MEETING_CLEANUP_DONE;
}
