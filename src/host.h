/*
 * FILE:    host.h
 * AUTHORS: Martin Benes     <martinbenesh@gmail.com>
 *          Lukas Hejtmanek  <xhejtman@ics.muni.cz>
 *          Petr Holub       <hopet@ics.muni.cz>
 *          Milos Liska      <xliska@fi.muni.cz>
 *          Jiri Matela      <matela@ics.muni.cz>
 *          Dalibor Matura   <255899@mail.muni.cz>
 *          Ian Wesley-Smith <iwsmith@cct.lsu.edu>
 *
 * Copyright (c) 2005-2010 CESNET z.s.p.o.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 
 *      This product includes software developed by CESNET z.s.p.o.
 * 
 * 4. Neither the name of the CESNET nor the names of its contributors may be
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
 *
 */
#ifndef __host_h
#define __host_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "config_unix.h"
#include "config_win32.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct rtp;
struct state_uv;
struct video_frame;

extern int uv_argc;
extern char **uv_argv;

extern long packet_rate;

extern volatile bool should_exit_receiver;

/* TODO: remove these variables (should be safe) */
extern unsigned int hd_size_x;
extern unsigned int hd_size_y;
extern unsigned int hd_color_spc;
extern unsigned int hd_color_bpp;

extern unsigned int bitdepth;

extern unsigned int progressive;

extern void (*exit_uv)(int status);

extern unsigned int audio_capture_channels;

#define MAX_CUDA_DEVICES 4
extern unsigned int cuda_devices[];
extern unsigned int cuda_devices_count;

extern const char *sage_receiver;

// for aggregate.c
struct vidcap;
struct display;
int initialize_video_display(const char *requested_display,
                                                char *fmt, unsigned int flags,
                                                struct display **);
int initialize_video_capture(const char *requested_capture,
                                               char *fmt, unsigned int flags,
                                               struct vidcap **);
struct vcodec_state;
void destroy_decoder(struct vcodec_state *video_decoder_state);

void *ultragrid_rtp_receiver_thread(void *arg);
void destroy_rtp_devices(struct rtp ** network_devices);
struct rtp **change_tx_port(struct state_uv *, int port);

// if not NULL, data should be exported
extern char *export_dir;
extern char *sage_network_device;

#ifdef __cplusplus
}
#endif

#endif
