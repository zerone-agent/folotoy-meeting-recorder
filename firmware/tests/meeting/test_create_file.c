#define _POSIX_C_SOURCE 200809L
#include "meeting_create_file.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
int main(void){char name[]="/tmp/meeting-create-XXXXXX";int fd=mkstemp(name);assert(fd>=0);const char text[]="existing recording";assert(write(fd,text,sizeof text)==sizeof text);assert(close(fd)==0);assert(meeting_create_recording_file(name)==NULL);FILE *f=fopen(name,"rb");assert(f);char readback[sizeof text];assert(fread(readback,1,sizeof readback,f)==sizeof readback&&!memcmp(readback,text,sizeof text));fclose(f);assert(unlink(name)==0);f=meeting_create_recording_file(name);assert(f);assert(fwrite(text,1,sizeof text,f)==sizeof text);assert(fclose(f)==0);assert(meeting_create_recording_file(name)==NULL);assert(unlink(name)==0);return 0;}
