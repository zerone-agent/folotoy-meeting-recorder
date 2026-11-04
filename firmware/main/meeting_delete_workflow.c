#include "meeting_delete_workflow.h"
#include <stdint.h>

meeting_delete_result meeting_delete_resume(const meeting_deletion_io *storage,const meeting_delete_workflow_io *io)
{
    if(!storage||!io||!io->ensure_remote_proof||!io->remove_local||!io->cancel_remote)return MEETING_DELETE_ERROR;
    meeting_deletion state;unsigned slot;
    if(meeting_deletion_load(storage,&state,&slot)!=MEETING_DELETION_OK)return MEETING_DELETE_ERROR;
    if(!io->ensure_remote_proof(io->ctx,&state))return MEETING_DELETE_ERROR;
    if(!state.local_done){
        if(state.generation==UINT64_MAX||!io->remove_local(io->ctx,&state))return MEETING_DELETE_ERROR;
        state.generation++;state.local_done=true;
        if(!meeting_deletion_commit(storage,&state))return MEETING_DELETE_ERROR;
    }
    if(state.remote==MEETING_DELETE_REMOTE_PENDING){
        int result=io->cancel_remote(io->ctx,&state);
        if(result==0)return MEETING_DELETE_PENDING;
        if((result!=1&&result!=2)||state.generation==UINT64_MAX)return MEETING_DELETE_ERROR;
        state.generation++;state.remote=(meeting_delete_remote)result;
        if(!meeting_deletion_commit(storage,&state))return MEETING_DELETE_ERROR;
    }
    return MEETING_DELETE_DONE;
}
