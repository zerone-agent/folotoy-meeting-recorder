#include "meeting_commission.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include <string.h>

static int nibble(char c)
{
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}
bool meeting_commission_mac_matches(const char *configured,const uint8_t actual[6])
{
    if(!configured||!actual||strlen(configured)!=12)return false;
    unsigned diff=0;
    for(unsigned i=0;i<6;i++){
        int a=nibble(configured[2*i]),b=nibble(configured[2*i+1]);
        if(a<0||b<0)return false;
        diff|=(unsigned)(((a<<4)|b)^actual[i]);
    }
    return diff==0;
}
bool meeting_commission_allowed(void)
{
    uint8_t mac[6];
    return esp_efuse_mac_get_default(mac)==ESP_OK&&
           meeting_commission_mac_matches(CONFIG_MEETING_SUPERVISED_DEVICE_MAC,mac);
}
