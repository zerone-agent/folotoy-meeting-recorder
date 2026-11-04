#pragma once
#include <stdint.h>
typedef struct {int value;} OpusEncoder;
#define OPUS_OK 0
#define OPUS_APPLICATION_VOIP 1
#define OPUS_RESET_STATE 2
#define OPUS_SET_BITRATE(x) 3,(x)
#define OPUS_SET_COMPLEXITY(x) 4,(x)
#define OPUS_SET_VBR(x) 5,(x)
#define OPUS_SET_DTX(x) 6,(x)
#define OPUS_SET_SIGNAL(x) 7,(x)
#define OPUS_SET_MAX_BANDWIDTH(x) 8,(x)
#define OPUS_SIGNAL_VOICE 1
#define OPUS_BANDWIDTH_WIDEBAND 1
int opus_encoder_get_size(int channels);
int opus_encoder_init(OpusEncoder *encoder,int rate,int channels,int application);
int opus_encoder_ctl(OpusEncoder *encoder,int request,...);
int opus_encode(OpusEncoder *encoder,const int16_t *pcm,int samples,uint8_t *data,int capacity);
