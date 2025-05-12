/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_AUDIO_ELEMENT_H_
#define _RTOS_APPS_AUDIO_ELEMENT_H_

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

#define AUDIO_ELEMENT_MAX_INPUTS  64
#define AUDIO_ELEMENT_MAX_OUTPUTS 64

enum {
    AUDIO_ELEMENT_DTMF_SOURCE = 0,
    AUDIO_ELEMENT_ROUTING,
    AUDIO_ELEMENT_SAI_SINK,
    AUDIO_ELEMENT_SAI_SOURCE,
    AUDIO_ELEMENT_SINE_SOURCE,
    AUDIO_ELEMENT_PLL,
#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    AUDIO_ELEMENT_AVTP_SOURCE, /* AVB audio stream listener */
    AUDIO_ELEMENT_AVTP_SINK,   /* AVB audio stream talker */
#endif
    AUDIO_ELEMENT_MAX,
};

/* Configuration */
struct audio_element_config {
    unsigned int type;

    unsigned int inputs;
    unsigned int input[AUDIO_ELEMENT_MAX_INPUTS]; /* indexes to buffer structures */

    unsigned int outputs;
    unsigned int output[AUDIO_ELEMENT_MAX_OUTPUTS]; /* indexes to buffer structures */

    unsigned int period;
    unsigned int sample_rate;

    union {
        struct dtmf_element_config dtmf;
        struct pll_element_config pll;
        struct routing_element_config routing;
        struct sai_sink_element_config sai_sink;
        struct sai_source_element_config sai_source;
        struct sine_element_config sine;
#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
        struct avtp_source_element_config avtp_source;
        struct avtp_sink_element_config avtp_sink;
#endif
    } u;
};

#endif /* _RTOS_APPS_AUDIO_ELEMENT_H_ */
