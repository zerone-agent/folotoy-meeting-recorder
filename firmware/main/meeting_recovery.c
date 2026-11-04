#include "meeting_recovery.h"
#include "meeting_snapshot_file.h"
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

meeting_recovery_status meeting_recovery_prepare(FILE *source, const meeting_snapshot_io *io,
                                                 const uint8_t meeting_id[16], meeting_snapshot *out)
{
    return meeting_recovery_prepare_cancel(source,io,meeting_id,out,NULL,NULL);
}
meeting_recovery_status meeting_recovery_prepare_cancel(FILE *source, const meeting_snapshot_io *io,
                                                 const uint8_t meeting_id[16], meeting_snapshot *out,
                                                 meeting_cancel_fn cancelled, void *ctx)
{
    if (!source || !io || !meeting_id || !out) return MEETING_RECOVERY_STORAGE_ERROR;
    meeting_snapshot current; unsigned slot;
    meeting_snapshot_status status = meeting_snapshot_load(io, &current, &slot);
    if (status == MEETING_SNAPSHOT_OK) {
        if (memcmp(current.meeting_id, meeting_id, 16) != 0) return MEETING_RECOVERY_ID_CONFLICT;
        if (current.reason != 3 || !meeting_snapshot_matches_file_cancel(source, &current,cancelled,ctx)) return MEETING_RECOVERY_SOURCE_ERROR;
        *out = current;
        return MEETING_RECOVERY_OK;
    }
    if (status != MEETING_SNAPSHOT_EMPTY) return MEETING_RECOVERY_STORAGE_ERROR;
    unsigned char header[24];
    if (fseek(source, 0, SEEK_SET) != 0 || fread(header, 1, sizeof(header), source) != sizeof(header) ||
        header[20] != 0xff || header[21] != 0xff || header[22] != 0xff || header[23] != 0xff)
        return MEETING_RECOVERY_SOURCE_ERROR;
    meeting_scr1_scan_result scan;
    if (!meeting_scr1_scan_file_cancel(source, &scan,cancelled,ctx)) return MEETING_RECOVERY_SOURCE_ERROR;
    meeting_snapshot candidate = {.generation=1, .valid_end=scan.valid_end, .frames=scan.frames, .reason=3};
    memcpy(candidate.meeting_id, meeting_id, 16);
    if (!meeting_snapshot_source_hash_cancel(source, candidate.source_sha256,cancelled,ctx) ||
        !meeting_snapshot_matches_file_cancel(source, &candidate,cancelled,ctx) || (cancelled && cancelled(ctx))) return MEETING_RECOVERY_SOURCE_ERROR;
    if (!meeting_snapshot_commit(io, &candidate)) return MEETING_RECOVERY_STORAGE_ERROR;
    /* Re-read after commit: a successful storage API call is not enough. */
    if (meeting_snapshot_load(io, &current, &slot) != MEETING_SNAPSHOT_OK ||
        memcmp(current.meeting_id, meeting_id, 16) != 0 ||
        !meeting_snapshot_matches_file_cancel(source, &current,cancelled,ctx)) return MEETING_RECOVERY_STORAGE_ERROR;
    *out = current;
    return MEETING_RECOVERY_OK;
}

meeting_recovery_status meeting_closed_prepare(FILE *source, const meeting_snapshot_io *io,
                                               const uint8_t meeting_id[16], uint8_t reason,
                                               meeting_snapshot *out)
{
    return meeting_closed_prepare_cancel(source,io,meeting_id,reason,out,NULL,NULL);
}

meeting_recovery_status meeting_sealed_prepare_cancel(FILE *source,const meeting_snapshot_io *io,
                                               const meeting_seal *seal,meeting_snapshot *out,
                                               meeting_cancel_fn cancelled,void *ctx)
{
    uint8_t encoded[MEETING_SEAL_SIZE];struct stat st;unsigned char header[24];meeting_scr1_scan_result scan;
    if(!source||!io||!out||!meeting_seal_encode(seal,encoded))return MEETING_RECOVERY_STORAGE_ERROR;
    if(fstat(fileno(source),&st)!=0||st.st_size<0||(uint64_t)st.st_size!=seal->bytes||
       fseek(source,0,SEEK_SET)!=0||fread(header,1,sizeof(header),source)!=sizeof(header)||
       ((uint32_t)header[8]|((uint32_t)header[9]<<8)|((uint32_t)header[10]<<16)|((uint32_t)header[11]<<24))!=seal->file_id||
       header[20]!=0xff||header[21]!=0xff||header[22]!=0xff||header[23]!=0xff||
       !meeting_scr1_scan_file_cancel(source,&scan,cancelled,ctx)||scan.incomplete_tail||
       scan.frames!=seal->frames||scan.valid_end!=seal->bytes)return MEETING_RECOVERY_SOURCE_ERROR;
    meeting_snapshot current;unsigned slot;meeting_snapshot_status status=meeting_snapshot_load(io,&current,&slot);
    if(status==MEETING_SNAPSHOT_OK){
        if(memcmp(current.meeting_id,seal->meeting_id,16))return MEETING_RECOVERY_ID_CONFLICT;
        if(current.reason!=seal->reason||current.frames!=seal->frames||current.valid_end!=seal->bytes||
           !meeting_snapshot_matches_file_cancel(source,&current,cancelled,ctx))return MEETING_RECOVERY_SOURCE_ERROR;
        *out=current;return MEETING_RECOVERY_OK;
    }
    if(status!=MEETING_SNAPSHOT_EMPTY)return MEETING_RECOVERY_STORAGE_ERROR;
    meeting_snapshot candidate={.generation=1,.valid_end=seal->bytes,.frames=seal->frames,.reason=seal->reason};
    memcpy(candidate.meeting_id,seal->meeting_id,16);
    if(!meeting_snapshot_source_hash_cancel(source,candidate.source_sha256,cancelled,ctx)||
       !meeting_snapshot_matches_file_cancel(source,&candidate,cancelled,ctx)||(cancelled&&cancelled(ctx)))return MEETING_RECOVERY_SOURCE_ERROR;
    if(!meeting_snapshot_commit(io,&candidate)||meeting_snapshot_load(io,&current,&slot)!=MEETING_SNAPSHOT_OK||
       current.reason!=seal->reason||memcmp(current.meeting_id,seal->meeting_id,16)||
       !meeting_snapshot_matches_file_cancel(source,&current,cancelled,ctx))return MEETING_RECOVERY_STORAGE_ERROR;
    *out=current;return MEETING_RECOVERY_OK;
}
meeting_recovery_status meeting_closed_prepare_cancel(FILE *source, const meeting_snapshot_io *io,
                                               const uint8_t meeting_id[16], uint8_t reason,
                                               meeting_snapshot *out, meeting_cancel_fn cancelled, void *ctx)
{
    if (!source || !io || !meeting_id || !out || reason > 3) return MEETING_RECOVERY_STORAGE_ERROR;
    meeting_snapshot current; unsigned slot;
    meeting_snapshot_status status = meeting_snapshot_load(io, &current, &slot);
    if (status == MEETING_SNAPSHOT_OK) {
        if (memcmp(current.meeting_id, meeting_id, 16) != 0) return MEETING_RECOVERY_ID_CONFLICT;
        if (current.reason != reason || !meeting_snapshot_matches_file_cancel(source, &current,cancelled,ctx)) return MEETING_RECOVERY_SOURCE_ERROR;
        *out = current; return MEETING_RECOVERY_OK;
    }
    if (status != MEETING_SNAPSHOT_EMPTY) return MEETING_RECOVERY_STORAGE_ERROR;
    unsigned char header[24];
    if (fseek(source, 0, SEEK_SET) != 0 || fread(header, 1, sizeof(header), source) != sizeof(header) ||
        (header[20] == 0xff && header[21] == 0xff && header[22] == 0xff && header[23] == 0xff)) return MEETING_RECOVERY_SOURCE_ERROR;
    meeting_scr1_scan_result scan;
    if (!meeting_scr1_scan_file_cancel(source, &scan,cancelled,ctx) || scan.incomplete_tail) return MEETING_RECOVERY_SOURCE_ERROR;
    meeting_snapshot candidate = {.generation=1, .valid_end=scan.valid_end, .frames=scan.frames, .reason=reason};
    memcpy(candidate.meeting_id, meeting_id, 16);
    if (!meeting_snapshot_source_hash_cancel(source, candidate.source_sha256,cancelled,ctx) || !meeting_snapshot_matches_file_cancel(source, &candidate,cancelled,ctx) || (cancelled && cancelled(ctx))) return MEETING_RECOVERY_SOURCE_ERROR;
    if (!meeting_snapshot_commit(io, &candidate)) return MEETING_RECOVERY_STORAGE_ERROR;
    if (meeting_snapshot_load(io, &current, &slot) != MEETING_SNAPSHOT_OK ||
        current.reason != reason || memcmp(current.meeting_id, meeting_id, 16) ||
        !meeting_snapshot_matches_file_cancel(source, &current,cancelled,ctx)) return MEETING_RECOVERY_STORAGE_ERROR;
    *out = current; return MEETING_RECOVERY_OK;
}
