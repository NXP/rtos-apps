/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _AUDIO_ELEMENT_H_
#define _AUDIO_ELEMENT_H_

#include "rtos_apps/audio/audio_element.h"
#include "rtos_apps/audio/audio_pipeline_ctrl.h"

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
#include "audio_element_avtp_sink.h"
#include "audio_element_avtp_source.h"
#endif
#include "audio_element_dtmf.h"
#include "audio_element_pll.h"
#include "audio_element_routing.h"
#include "audio_element_sai_sink.h"
#include "audio_element_sai_source.h"
#include "audio_element_sine.h"

extern const char *element_name[AUDIO_ELEMENT_MAX];

struct audio_element {
    void *data;

    unsigned int type;
    unsigned int sample_rate;
    unsigned int period;
    unsigned int element_id;

    int (*run)(struct audio_element *element);
    void (*reset)(struct audio_element *element);
    void (*exit)(struct audio_element *element);
    void (*dump)(struct audio_element *element);
    void (*stats)(struct audio_element *element);
};

int audio_element_ctrl(struct audio_element *element, struct audio_cmd_element *cmd, unsigned int len,
                       void *ctrl_handle);
void audio_element_exit(struct audio_element *element);
void audio_element_dump(struct audio_element *element);
void audio_element_stats(struct audio_element *element);
int audio_element_check_config(struct audio_element_config *config);
unsigned int audio_element_data_size(struct audio_element_config *config);
int audio_element_init(struct audio_element *element, struct audio_element_config *config, struct audio_buffer *buffer);

static inline int audio_element_run(struct audio_element *element)
{
    return element->run(element);
}

static inline void audio_element_reset(struct audio_element *element)
{
    element->reset(element);
}

#endif /* _AUDIO_ELEMENT_H_ */
