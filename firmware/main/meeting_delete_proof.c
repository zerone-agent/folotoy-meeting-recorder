#include "meeting_delete_proof.h"
#include <string.h>
bool meeting_delete_proof_prepare(uint32_t file_id,FILE *source,const meeting_snapshot *snapshot,
    bool has_time,int64_t recorded_at,uint8_t time_trust,const meeting_confirmation_io *store,
    meeting_cancel_fn cancelled,void *ctx)
{
    if(!file_id||!source||!snapshot||!store)return false;
    meeting_confirmation proof={.file_id=file_id};
    if(!meeting_upload_manifest_cancel(source,snapshot,has_time,recorded_at,time_trust,&proof.manifest,cancelled,ctx))return false;
    memcpy(proof.source_sha256,snapshot->source_sha256,sizeof(proof.source_sha256));
    return meeting_confirmation_matches_snapshot(&proof,snapshot)&&(!cancelled||!cancelled(ctx))&&meeting_confirmation_commit(store,&proof);
}
bool meeting_delete_proof_load(uint32_t file_id,const meeting_snapshot *snapshot,
    const meeting_confirmation_io *store,meeting_manifest *expected)
{
    meeting_confirmation proof;
    if(!file_id||!snapshot||!store||!expected||meeting_confirmation_load(store,&proof)!=MEETING_CONFIRMATION_OK||
       proof.file_id!=file_id||!meeting_confirmation_matches_snapshot(&proof,snapshot))return false;
    *expected=proof.manifest;return true;
}
