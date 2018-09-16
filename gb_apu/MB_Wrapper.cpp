#include "Multi_Buffer.h"
#include "MB_Wrapper.h"

extern "C" {
        Multi_Buffer* new_Stereo_Buffer() {
                return new Stereo_Buffer();
        }
        Multi_Buffer* new_Mono_Buffer() {
                return new Mono_Buffer();
        }

        void delete_Multi_Buffer(Multi_Buffer* mb) {
                delete mb;
        }

        blargg_err_t Multi_Buffer_set_channel_count(Multi_Buffer* mb,
                        int x) {
                return mb->set_channel_count(x);
        }


        Blip_Buffer* Stereo_Buffer_center(Multi_Buffer* mb) {
                return reinterpret_cast<Stereo_Buffer* >(mb)->center();
        }
        Blip_Buffer* Stereo_Buffer_left(Multi_Buffer* mb) {
                return reinterpret_cast<Stereo_Buffer* >(mb)->left();
        }
        Blip_Buffer* Stereo_Buffer_right(Multi_Buffer* mb) {
                return reinterpret_cast<Stereo_Buffer* >(mb)->right();
        }


        blargg_err_t Multi_Buffer_set_sample_rate(Multi_Buffer *mb,
                        long rate) {
                return mb->set_sample_rate(rate);
        }
        blargg_err_t Multi_Buffer_set_sample_rate_msec(Multi_Buffer *mb,
                        long rate, int msec) {
                return mb->set_sample_rate(rate, msec);
        }

        void Multi_Buffer_clock_rate(Multi_Buffer* mb, long rate) {
                mb->clock_rate(rate);
        }

        void Multi_Buffer_bass_freq(Multi_Buffer* mb, int bass) {
                mb->bass_freq(bass);
        }

        void Multi_Buffer_clear(Multi_Buffer* mb) {
                mb->clear();
        }

        long Multi_Buffer_sample_rate(const Multi_Buffer* mb) {
                return mb->sample_rate();
        }

        int Multi_Buffer_length(const Multi_Buffer* mb) {
                return mb->length();
        }

        void Multi_Buffer_end_frame(Multi_Buffer* mb, blip_time_t t,
                        bool added_stereo) {
                mb->end_frame(t, added_stereo);
        }

        int Multi_Buffer_samples_per_frame(Multi_Buffer* mb) {
                return mb->samples_per_frame();
        }

        unsigned int Multi_Buffer_channels_changed_count(
                        Multi_Buffer* mb) {
                return mb->channels_changed_count();
        }

        long Multi_Buffer_read_samples(Multi_Buffer* mb,
                        blip_sample_t* out, long count) {
                return mb->read_samples(out, count);
        }

        long Multi_Buffer_samples_avail(const Multi_Buffer* mb) {
                return mb->samples_avail();
        }
}
