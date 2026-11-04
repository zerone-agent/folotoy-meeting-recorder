#include "meeting_commission.h"
#include "esp_err.h"
#include <assert.h>
#include <string.h>
static int read_error;
esp_err_t esp_efuse_mac_get_default(uint8_t *mac)
{static const uint8_t id[6]={0,0x11,0x22,0x33,0x44,0x55};if(read_error)return ESP_FAIL;memcpy(mac,id,6);return ESP_OK;}
int main(void)
{
    const uint8_t id[6]={0,0x11,0x22,0x33,0x44,0x55};
    assert(meeting_commission_mac_matches("001122334455",id));
    assert(!meeting_commission_mac_matches("",id));
    assert(!meeting_commission_mac_matches("00112233445g",id));
    assert(!meeting_commission_mac_matches("001122334456",id));
    assert(!meeting_commission_mac_matches("00:11:22:33:44:55",id));
    assert(meeting_commission_allowed());read_error=1;assert(!meeting_commission_allowed());
    return 0;
}
