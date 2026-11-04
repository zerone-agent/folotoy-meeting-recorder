#include "meeting_upload_session.h"
meeting_upload_result meeting_upload_session(FILE *source, const meeting_snapshot *snapshot,
                                              bool has_time, int64_t recorded_at, uint8_t time_trust,
                                              meeting_https_config *config, meeting_manifest *confirmed)
{
    meeting_upload_transport transport;
    if (!source || !snapshot || !meeting_https_bind(config, &transport)) return MEETING_UPLOAD_FAILED;
    if (transport.cancelled(transport.ctx)) return MEETING_UPLOAD_PAUSED;
    meeting_manifest manifest;
    if (!meeting_upload_manifest_cancel(source, snapshot, has_time, recorded_at, time_trust, &manifest,transport.cancelled,transport.ctx))
        return transport.cancelled(transport.ctx)?MEETING_UPLOAD_PAUSED:MEETING_UPLOAD_FAILED;
    if (transport.cancelled(transport.ctx)) return MEETING_UPLOAD_PAUSED;
    meeting_upload_result result=meeting_upload_run(source, snapshot, &manifest, &transport);
    meeting_https_close(config);
    if(result==MEETING_UPLOAD_CONFIRMED && confirmed)*confirmed=manifest;
    return result;
}
