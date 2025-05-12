/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _AUDIO_PIPELINE_H_
#define _AUDIO_PIPELINE_H_

#include "rtos_apps/audio/audio_pipeline.h"

#include "audio_buffer.h"
#include "audio_element.h"

/* pipeline memory layout
 * stage[0]
 * stage[1]
 * ...
 * stage[n]
 * element[0]
 * element[1]
 * ...
 * element[m]
 * element[0]
 * element[1]
 * ...
 * element[l]
 * buffer[0]
 * buffer[1]
 * ...
 * buffer[k]
 * buffer storage
 */
struct audio_pipeline_stage {
    unsigned int elements;
    struct audio_element *element;
};

struct audio_pipeline {
    unsigned int stages;
    unsigned int buffers;

    struct audio_pipeline_stage *stage;

    struct audio_buffer *buffer;
};

int audio_pipeline_ctrl(struct audio_cmd_pipeline *cmd, unsigned int len, void *ctrl_handle);
struct audio_pipeline *audio_pipeline_init(struct audio_pipeline_config *config);
int audio_pipeline_run(struct audio_pipeline *pipeline);
void audio_pipeline_exit(struct audio_pipeline *pipeline);
void audio_pipeline_reset(struct audio_pipeline *pipeline);
void audio_pipeline_dump(struct audio_pipeline *pipeline);
void audio_pipeline_stats(struct audio_pipeline *pipeline);

#endif /* _AUDIO_PIPELINE_H_ */
