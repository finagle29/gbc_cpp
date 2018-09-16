#ifndef __MB_WRAPPER_H
#define __MB_WRAPPER_H

#include "Blip_Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Multi_Buffer Multi_Buffer;

Multi_Buffer* new_Stereo_Buffer();
Multi_Buffer* new_Mono_Buffer();
void delete_Multi_Buffer(Multi_Buffer* mb);

blargg_err_t Multi_Buffer_set_channel_count(Multi_Buffer* mb, int);


struct channel_t {
        Blip_Buffer* center;
        Blip_Buffer* left;
        Blip_Buffer* right;
};

channel_t Multi_Buffer_Channel(Multi_Buffer* mb, int index);

blargg_err_t Multi_Buffer_set_sample_rate(Multi_Buffer *mb, long rate);
blargg_err_t Multi_Buffer_set_sample_rate_msec(Multi_Buffer *mb,
                long rate, int msec);
void Multi_Buffer_clock_rate(Multi_Buffer* mb, long);
void Multi_Buffer_bass_freq(Multi_Buffer* mb, int);
void Multi_Buffer_clear(Multi_Buffer* mb);
long Multi_Buffer_sample_rate(const Multi_Buffer* mb);

int Multi_Buffer_length(const Multi_Buffer* mb);
void Multi_Buffer_end_fram(Multi_Buffer* mb, blip_time_t, bool added_stereo);
int Multi_Buffer_samples_per_frame(Multi_Buffer* mb);
unsigned int Multi_Buffer_channels_changed_count(Multi_Buffer* mb);

long Multi_Buffer_read_samples(Multi_Buffer* mb, blip_sample_t*, long);
long Multi_Buffer_samples_avail(const Multi_Buffer* mb);

#ifdef __cplusplus
}
#endif
#endif
