#pragma once
#include <stdio.h>
/* Atomically create a new recording file without ever truncating an existing
 * path. Caller must preserve the path on any subsequent failure. */
FILE *meeting_create_recording_file(const char *path);
