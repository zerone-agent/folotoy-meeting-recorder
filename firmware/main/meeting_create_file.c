#include "meeting_create_file.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
FILE *meeting_create_recording_file(const char *path)
{
    if (!path || !*path) { errno=EINVAL; return NULL; }
    int fd=open(path, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd<0) return NULL;
    FILE *file=fdopen(fd,"w+b");
    if (!file) { int saved=errno; close(fd); errno=saved; }
    return file;
}
