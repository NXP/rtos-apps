/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _AUDIO_AVB_H_
#define _AUDIO_AVB_H_

struct pipeline_ctx *ctx;

typedef struct {
    struct genavb_stream_params stream_params;
    struct genavb_stream_handle *stream_handle;

    unsigned int batch_size_ns;
    unsigned int cur_batch_size;
} aar_crf_stream_t;

struct crf_stream {
    aar_crf_stream_t stream;
    unsigned int connected;
    int index;
};

struct avtp_avb_ctx {
    struct genavb_control_handle *ctrl_h;
    struct genavb_control_handle *controlled_h;
    struct genavb_control_handle *clk_h;
    struct genavb_handle *avb_handle;
    struct crf_stream crf_stream;
};

/* AVB specific callbacks */
int audio_avb_init(struct pipeline_ctx *ctx);
void audio_avb_exit(struct pipeline_ctx *ctx);
void audio_avb_ctrl(void *handle);

#endif /* _AUDIO_AVB_H_ */