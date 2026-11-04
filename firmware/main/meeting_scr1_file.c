#include "meeting_scr1_file.h"
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

static int read_at(void *ctx, uint64_t offset, uint8_t *out, size_t length)
{
    FILE *file = ctx;
    if (offset > LONG_MAX || length > INT_MAX || fseek(file, (long)offset, SEEK_SET) != 0) return -1;
    size_t got = fread(out, 1, length, file);
    if (got != length) return -1;
    return (int)got;
}

typedef struct { FILE *file; meeting_cancel_fn cancelled; void *ctx; } scan_context;
static int read_at_cancel(void *ctx,uint64_t offset,uint8_t *out,size_t length)
{
    scan_context *s=ctx;
    if(s->cancelled && s->cancelled(s->ctx))return -1;
    return read_at(s->file,offset,out,length);
}

bool meeting_scr1_scan_file(FILE *file, meeting_scr1_scan_result *out)
{
    return meeting_scr1_scan_file_cancel(file,out,NULL,NULL);
}

bool meeting_scr1_scan_file_cancel(FILE *file, meeting_scr1_scan_result *out, meeting_cancel_fn cancelled, void *ctx)
{
    struct stat st;
    if (!file || !out || fstat(fileno(file), &st) != 0 || st.st_size < 0) return false;
    scan_context scan={.file=file,.cancelled=cancelled,.ctx=ctx};
    meeting_scr1_scan_result result;
    if(!meeting_scr1_scan(read_at_cancel, &scan, (uint64_t)st.st_size, &result) ||
       (cancelled && cancelled(ctx)))return false;
    *out=result;return true;
}

bool meeting_scr1_read_projected(FILE *file, const meeting_scr1_scan_result *scan,
                                 uint64_t offset, uint8_t *out, size_t length)
{
    if (!file || !scan || (!out && length) || length > INT_MAX || offset > scan->valid_end ||
        length > scan->valid_end - offset) return false;
    if (length && read_at(file, offset, out, length) != (int)length) return false;
    return meeting_scr1_project(offset, out, length, scan->valid_end, scan->frames);
}
