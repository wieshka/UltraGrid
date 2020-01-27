/**
 * @file   video_display/vrg.cpp
 * @author Martin Pulec     <pulec@cesnet.cz>
 */
/*
 * Copyright (c) 2020 CESNET, z. s. p. o.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of CESNET nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "config_win32.h"

#include <chrono>

// VrgInputFormat::RGBA conflicts with codec_t::RGBA
#define RGBA VR_RGBA
#include <vrgstream.h>
#undef RGBA

#include "debug.h"
#include "lib_common.h"
#include "utils/misc.h"
#include "video.h"
#include "video_display.h"

#define MOD_NAME "[VRG] "
#define MAGIC_VRG to_fourcc('V', 'R', 'G', ' ')

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

struct state_vrg {
        uint32_t magic;
        struct video_desc saved_desc;

        high_resolution_clock::time_point t0 = high_resolution_clock::now();
        long long int frames;
        long long int frames_last;
};

static void display_vrg_probe(struct device_info **available_cards, int *count, void (**deleter)(void *))
{
        *deleter = free;
        *available_cards = NULL;
        *count = 0;
}

static void *display_vrg_init(struct module *parent, const char *fmt, unsigned int flags)
{
        UNUSED(fmt);
        UNUSED(flags);
        UNUSED(parent);
        struct state_vrg *s;

        s = new state_vrg();
        if (s == NULL) {
                return NULL;
        }
        s->magic = MAGIC_VRG;

        return s;
}

static void display_vrg_run(void *arg)
{
        UNUSED(arg);
}

static void display_vrg_done(void *state)
{
        struct state_vrg *s = (struct state_vrg *) state;
        assert(s->magic == MAGIC_VRG);
        delete s;
}

static struct video_frame *display_vrg_getf(void *state)
{
        struct state_vrg *s = (struct state_vrg *) state;
        assert(s->magic == MAGIC_VRG);

        return vf_alloc_desc_data(s->saved_desc);
}

static int display_vrg_putf(void *state, struct video_frame *frame, int flags)
{
        struct state_vrg *s = (struct state_vrg *) state;
        assert(s->magic == MAGIC_VRG);

        if (flags == PUTF_DISCARD) {
                vf_free(frame);
                return 0;
        }

        enum VrgStreamApiError ret;
        high_resolution_clock::time_point t_start = high_resolution_clock::now();
        ret = vrgStreamSubmitFrame(s->frames, frame->tiles[0].data);
        if (ret != Ok) {
                LOG(LOG_LEVEL_ERROR) << MOD_NAME "Submit Frame failed: " << ret << "\n";
        }
        high_resolution_clock::time_point t_end = high_resolution_clock::now();
        LOG(LOG_LEVEL_DEBUG) << "[VRG] Frame submit took " <<
                duration_cast<microseconds>(t_start - t_end).count() / 1000000.0
                        << " seconds\n";

        // calling vrgStreamRenderFrame sometime freezes
        //struct RenderPacket render_packet;
        //ret = vrgStreamRenderFrame(s->frames, &render_packet);
        //if (ret != Ok) {
        //        LOG(LOG_LEVEL_ERROR) << MOD_NAME "Render Frame failed: " << ret << "\n";
        //}

        high_resolution_clock::time_point now = high_resolution_clock::now();
        double seconds = duration_cast<microseconds>(now - s->t0).count() / 1000000.0;
        if (seconds >= 5) {
                long long frames = s->frames - s->frames_last;
                LOG(LOG_LEVEL_INFO) << "[VRG] " << frames << " frames in "
                        << seconds << " seconds = " <<  frames / seconds << " FPS\n";
                s->t0 = now;
                s->frames_last = s->frames;
        }

        s->frames += 1;
        vf_free(frame);

        return 0;
}

static int display_vrg_get_property(void *state, int property, void *val, size_t *len)
{
        UNUSED(state);
        codec_t codecs[] = {
                I420,
                RGBA,
        };
        int rgb_shift[] = {0, 8, 16};

        switch (property) {
                case DISPLAY_PROPERTY_CODECS:
                        if(sizeof(codecs) > *len) {
                                return FALSE;
                        }
                        memcpy(val, codecs, sizeof(codecs));
                        *len = sizeof(codecs);
                        break;
                case DISPLAY_PROPERTY_RGB_SHIFT:
                        if(sizeof(rgb_shift) > *len) {
                                return FALSE;
                        }
                        memcpy(val, rgb_shift, sizeof(rgb_shift));
                        *len = sizeof(rgb_shift);
                        break;
                case DISPLAY_PROPERTY_BUF_PITCH:
                        *(int *) val = PITCH_DEFAULT;
                        *len = sizeof(int);
                        break;
                default:
                        return FALSE;
        }
        return TRUE;
}

static int display_vrg_reconfigure(void *state, struct video_desc desc)
{
        struct state_vrg *s = (struct state_vrg *) state;
        assert(s->magic == MAGIC_VRG);
        assert(desc.color_spec == RGBA || desc.color_spec == I420);

        s->saved_desc = desc;

        enum VrgStreamApiError ret = vrgStreamInit(desc.color_spec == RGBA ? VR_RGBA : YUV420);
        if (ret != Ok) {
                LOG(LOG_LEVEL_ERROR) << MOD_NAME "Initialization failed: " << ret << "\n";
                return FALSE;
        }

        return TRUE;
}

static void display_vrg_put_audio_frame(void *state, struct audio_frame *frame)
{
        UNUSED(state);
        UNUSED(frame);
}

static int display_vrg_reconfigure_audio(void *state, int quant_samples, int channels,
                int sample_rate)
{
        UNUSED(state);
        UNUSED(quant_samples);
        UNUSED(channels);
        UNUSED(sample_rate);

        return FALSE;
}

static const struct video_display_info display_vrg_info = {
        display_vrg_probe,
        display_vrg_init,
        display_vrg_run,
        display_vrg_done,
        display_vrg_getf,
        display_vrg_putf,
        display_vrg_reconfigure,
        display_vrg_get_property,
        display_vrg_put_audio_frame,
        display_vrg_reconfigure_audio,
};

REGISTER_MODULE(vrg, &display_vrg_info, LIBRARY_CLASS_VIDEO_DISPLAY, VIDEO_DISPLAY_ABI_VERSION);
