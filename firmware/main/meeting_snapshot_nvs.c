#include "meeting_snapshot_nvs.h"
#include <stdio.h>
#include <string.h>

static bool key_for(const meeting_snapshot_nvs *s, unsigned slot, char key[12])
{
    if (!s || !s->file_id || slot > 1) return false;
    return snprintf(key, 12, "m%08lx%u", (unsigned long)s->file_id, slot) == 10;
}
static int slot_read(void *ctx, unsigned slot, uint8_t out[MEETING_SNAPSHOT_SIZE])
{
    meeting_snapshot_nvs *s = ctx; char key[12]; size_t size = MEETING_SNAPSHOT_SIZE;
    if (!out || !key_for(s, slot, key)) return -1;
    esp_err_t err = nvs_get_blob(s->handle, key, out, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) return 1;
    /* An incorrectly sized blob is present but invalid, not a blank slot. */
    if (err == ESP_ERR_NVS_INVALID_LENGTH || (err == ESP_OK && size != MEETING_SNAPSHOT_SIZE)) {
        memset(out, 0, MEETING_SNAPSHOT_SIZE); return 0;
    }
    return err == ESP_OK ? 0 : -1;
}
static bool slot_write_sync(void *ctx, unsigned slot, const uint8_t data[MEETING_SNAPSHOT_SIZE])
{
    meeting_snapshot_nvs *s = ctx; char key[12];
    if (!data || !key_for(s, slot, key)) return false;
    return nvs_set_blob(s->handle, key, data, MEETING_SNAPSHOT_SIZE) == ESP_OK && nvs_commit(s->handle) == ESP_OK;
}
bool meeting_snapshot_nvs_bind(meeting_snapshot_nvs *storage, meeting_snapshot_io *io)
{
    if (!storage || !io || !storage->file_id) return false;
    *io = (meeting_snapshot_io){.ctx=storage,.read=slot_read,.write_sync=slot_write_sync};
    return true;
}
