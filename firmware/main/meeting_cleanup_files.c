#include "meeting_cleanup_files.h"
#include "meeting_upload.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static int file_status(const char *path)
{
    struct stat st;if(stat(path,&st)==0)return S_ISREG(st.st_mode)?0:-1;
    return errno==ENOENT?1:-1;
}
static bool cancelled(void *ctx){meeting_cleanup_files *f=ctx;return f->cancelled(f->ctx);}
static int audio_exists(void *ctx){meeting_cleanup_files *f=ctx;return file_status(f->audio_path);}
static bool verify_audio(void *ctx,const meeting_confirmation *receipt,const meeting_snapshot *snapshot)
{
    meeting_cleanup_files *f=ctx;FILE *source=fopen(f->audio_path,"rb");if(!source)return false;
    meeting_manifest actual;
    bool ok=meeting_upload_manifest_cancel(source,snapshot,receipt->manifest.recorded_at_present,
        receipt->manifest.recorded_at,receipt->manifest.time_trust,&actual,f->cancelled,f->ctx)&&
        meeting_receipt_matches(&receipt->manifest,&actual,true);
    if(fclose(source)!=0)ok=false;
    return ok;
}
static bool close_readers(void *ctx){meeting_cleanup_files *f=ctx;return f->close_readers(f->ctx);}
static bool remove_file(const char *path)
{
    int state=file_status(path);if(state==1)return true;if(state!=0)return false;
    return unlink(path)==0&&file_status(path)==1;
}
static bool remove_audio(void *ctx){meeting_cleanup_files *f=ctx;return remove_file(f->audio_path);}
static bool remove_index(void *ctx){meeting_cleanup_files *f=ctx;return remove_file(f->index_path);}
static bool verify_absent(void *ctx)
{
    meeting_cleanup_files *f=ctx;
    return file_status(f->audio_path)==1&&file_status(f->index_path)==1&&f->read_space(f->ctx);
}
bool meeting_cleanup_files_bind(meeting_cleanup_files *files,meeting_cleanup_io *io)
{
    if(!files||!io||!files->audio_path||!files->index_path||!files->cancelled||!files->close_readers||!files->read_space)return false;
    *io=(meeting_cleanup_io){.ctx=files,.cancelled=cancelled,.audio_exists=audio_exists,.verify_audio=verify_audio,
        .close_readers=close_readers,.remove_audio=remove_audio,.remove_index=remove_index,.verify_absent_and_space=verify_absent};
    return true;
}
