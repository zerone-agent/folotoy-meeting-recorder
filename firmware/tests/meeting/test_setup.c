#include "meeting_setup.h"
#include "meeting_ca.h"
#include "meeting_clock_nvs.h"
#include "meeting_config_ble.h"
#include "meeting_config_store_nvs.h"
#include "meeting_network.h"
#include "meeting_provision_apply.h"
#include <assert.h>
#include <string.h>

static bool connected,has_data,valid=true,network,stop_fails,applied,acknowledged,network_fail,disconnect_on_publish;
static int stops,starts;
static int floor_status=1,apply_calls;
static bool commission_allowed=true;
bool meeting_commission_allowed(void){return commission_allowed;}
static int64_t now;
int64_t esp_timer_get_time(void){return now;}
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h){(void)name;(void)mode;*h=1;return ESP_OK;}
void nvs_close(nvs_handle_t h){(void)h;}
static int read_floor(void *ctx,int64_t *floor){(void)ctx;*floor=1800000000;return floor_status;}
bool meeting_clock_nvs_bind(nvs_handle_t *h,meeting_clock_io *io){io->ctx=h;io->read_floor=read_floor;return true;}
bool meeting_config_store_nvs_bind(nvs_handle_t *h,meeting_config_store_io *io){io->ctx=h;return true;}
bool meeting_network_enabled(void){return network;}
esp_err_t meeting_network_activate(const meeting_config *c,const meeting_clock *clock,const meeting_clock_io *io)
{(void)c;(void)clock;(void)io;assert(!connected);if(network_fail)return ESP_FAIL;network=true;return ESP_OK;}
bool meeting_config_ble_init(meeting_config_ble_inbox *i){(void)i;return true;}
void meeting_config_ble_destroy(meeting_config_ble_inbox *i){assert(i);assert(!connected);}
void meeting_config_ble_prepare(meeting_config_ble_inbox *i){(void)i;acknowledged=false;}
void meeting_config_ble_reset(void *ctx){(void)ctx;}
esp_err_t meeting_config_ble_handle(uint32_t s,const uint8_t *r,size_t n,uint8_t **o,ssize_t *l,void *ctx)
{(void)s;(void)r;(void)n;(void)o;(void)l;(void)ctx;return ESP_OK;}
bool meeting_config_ble_take(meeting_config_ble_inbox *i,uint8_t *out,size_t cap,size_t *n,meeting_pairing_token *t)
{(void)i;assert(cap>0);if(!has_data)return false;has_data=false;*out=42;*n=1;t->generation=1;t->session=2;return true;}
bool meeting_config_ble_publish(meeting_config_ble_inbox *i,meeting_pairing_token t){(void)i;if(disconnect_on_publish)valid=false;return meeting_pairing_ble_session_valid(t);}
bool meeting_config_ble_acknowledged(meeting_config_ble_inbox *i,meeting_pairing_token t){(void)i;(void)t;return acknowledged;}
esp_err_t meeting_pairing_ble_start(char code[13],meeting_pairing_handler handler,meeting_pairing_reset reset,void *ctx)
{(void)handler;(void)reset;(void)ctx;assert(!connected);connected=true;starts++;memcpy(code,"123456789012",13);return ESP_OK;}
esp_err_t meeting_pairing_ble_stop(void){stops++;if(stop_fails)return ESP_FAIL;connected=false;return ESP_OK;}
bool meeting_pairing_ble_session_valid(meeting_pairing_token t){return connected&&valid&&t.generation==1&&t.session==2;}
bool meeting_ca_validate_pem(const char *pem,uint8_t hash[32],void *ctx){(void)pem;(void)hash;(void)ctx;return true;}
bool meeting_config_parse(const uint8_t *j,size_t n,meeting_ca_validate f,void *ctx,meeting_config *c)
{(void)f;(void)ctx;if(n!=1||*j!=42)return false;memset(c,0,sizeof(*c));c->utc=1800000000;c->ca_sha256[0]=99;return true;}
void meeting_config_clear(meeting_config *c){memset(c,0,sizeof(*c));}
void meeting_clock_reset(meeting_clock *c){memset(c,0,sizeof(*c));}
meeting_provision_result meeting_provision_apply(const uint8_t *j,size_t n,const uint8_t hash[32],int64_t utc,bool confirmed,
 meeting_session_valid session,void *ctx,bool initial,uint64_t received,uint64_t uptime,const meeting_clock_io *cio,meeting_clock *clock,
 const meeting_config_store_io *store,meeting_ca_validate ca,void *cactx,uint8_t scratch[MEETING_CONFIG_RECORD_MAX],meeting_config *out)
{(void)j;(void)n;(void)received;(void)uptime;(void)cio;(void)store;(void)ca;(void)cactx;(void)scratch;(void)out;
 assert(confirmed&&initial==(floor_status==1)&&session(ctx)&&hash[0]==99&&utc==1800000000);apply_calls++;clock->trusted=true;
 return applied?MEETING_PROVISION_APPLIED:MEETING_PROVISION_CLOCK_FAILED;}
int main(void)
{
 meeting_setup_view view;assert(meeting_setup_start()==ESP_OK);meeting_setup_view_get(&view);
 assert(view.state==MEETING_SETUP_PAIRING&&!strcmp(view.code,"123456789012"));
 has_data=true;meeting_setup_poll();meeting_setup_view_get(&view);
 assert(view.state==MEETING_SETUP_REVIEW&&view.ca_sha256[0]==99&&view.utc==1800000000);
 floor_status=-1;assert(meeting_setup_confirm()==MEETING_SETUP_ERR_CLOCK&&!apply_calls);
 floor_status=1;
 commission_allowed=false;assert(meeting_setup_confirm()==MEETING_SETUP_ERR_CLOCK&&!apply_calls);
 commission_allowed=true;
 assert(meeting_setup_confirm()==ESP_OK&&!network&&connected&&!apply_calls);
 meeting_setup_view_get(&view);assert(view.state==MEETING_SETUP_FIRST_CLOCK);
 assert(meeting_setup_confirm()==MEETING_SETUP_ERR_CLOCK&&!network&&!connected&&apply_calls==1);
 floor_status=0;
 assert(meeting_setup_start()==ESP_OK);has_data=true;meeting_setup_poll();valid=false;meeting_setup_poll();
 meeting_setup_view_get(&view);assert(view.state==MEETING_SETUP_IDLE&&!connected);valid=true;
 assert(meeting_setup_start()==ESP_OK);stop_fails=true;assert(meeting_setup_cancel()==ESP_FAIL);
 meeting_setup_view_get(&view);assert(view.state==MEETING_SETUP_PAIRING);stop_fails=false;assert(meeting_setup_cancel()==ESP_OK);
 assert(meeting_setup_start()==ESP_OK);has_data=true;meeting_setup_poll();applied=true;network_fail=true;
 assert(meeting_setup_confirm()==ESP_OK);acknowledged=true;meeting_setup_poll();
 assert(!network&&!connected&&meeting_setup_take_result()==MEETING_SETUP_NETWORK_FAILED);
 network_fail=false;floor_status=1;
 assert(meeting_setup_start()==ESP_OK);has_data=true;meeting_setup_poll();
 assert(meeting_setup_confirm()==ESP_OK);meeting_setup_view_get(&view);assert(view.state==MEETING_SETUP_FIRST_CLOCK);
 assert(meeting_setup_confirm()==ESP_OK&&!network&&connected);
 meeting_setup_poll();assert(!network&&connected);
 acknowledged=true;valid=false;stop_fails=true;meeting_setup_poll();
 meeting_setup_view_get(&view);assert(view.state==MEETING_SETUP_SAVED&&!network&&connected);
 stop_fails=false;meeting_setup_poll();assert(network&&!connected&&stops>=5&&starts==5);
 assert(meeting_setup_take_result()==MEETING_SETUP_NETWORK_STARTED);
 assert(meeting_setup_take_result()==MEETING_SETUP_NO_RESULT);
 /* A committed configuration remains usable when the client disconnects
  * before receiving the acknowledgement. Stop failures must be retried. */
 network=false;valid=true;floor_status=0;
 assert(meeting_setup_start()==ESP_OK);has_data=true;meeting_setup_poll();
 assert(meeting_setup_confirm()==ESP_OK);valid=false;stop_fails=true;meeting_setup_poll();
 meeting_setup_view_get(&view);assert(view.state==MEETING_SETUP_SAVED&&!network);
 stop_fails=false;meeting_setup_poll();assert(network&&!connected);
 assert(meeting_setup_take_result()==MEETING_SETUP_NETWORK_STARTED);
 /* An idle client that never acknowledges cannot hold setup forever. */
 network=false;valid=true;assert(meeting_setup_start()==ESP_OK);has_data=true;meeting_setup_poll();
 assert(meeting_setup_confirm()==ESP_OK);now+=121LL*1000000LL;
 meeting_setup_poll();assert(network&&!connected);
 assert(meeting_setup_take_result()==MEETING_SETUP_NETWORK_STARTED);
 /* The session may drop after the durable apply but before publishing. */
 network=false;valid=true;disconnect_on_publish=true;
 assert(meeting_setup_start()==ESP_OK);has_data=true;meeting_setup_poll();
 assert(meeting_setup_confirm()==ESP_OK);meeting_setup_poll();assert(network&&!connected);
 assert(meeting_setup_take_result()==MEETING_SETUP_NETWORK_STARTED);
 return 0;
}
