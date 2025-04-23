/*
 * Copyright 2022, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _AUDIO_PIPELINE_CTRL_H_
#define _AUDIO_PIPELINE_CTRL_H_

#if (CONFIG_GENAVB_ENABLE == 1)
#include "genavb/genavb.h"
#endif

#include <stdint.h>

/* Audio pipeline commands */
struct audio_cmd_pipeline_id {
    uint32_t id; /* pipeline id */
};

struct audio_cmd_element_id {
    uint32_t type; /* element type */
    uint32_t id;   /* element id, for the given type */
};

struct audio_cmd_element_common {
    uint32_t type; /* command type */
    struct audio_cmd_pipeline_id pipeline;
    struct audio_cmd_element_id element;
};

struct audio_resp_element_routing {
    uint32_t type; /* command type */
    uint32_t status;
};

struct audio_cmd_element_routing_disconnect {
    uint32_t type; /* command type */
    struct audio_cmd_pipeline_id pipeline;
    struct audio_cmd_element_id element;
    uint32_t output;
};

struct audio_cmd_element_routing_connect {
    uint32_t type; /* command type */
    struct audio_cmd_pipeline_id pipeline;
    struct audio_cmd_element_id element;
    uint32_t output;
    uint32_t input;
};

struct audio_cmd_element_routing {
    union {
        struct audio_cmd_element_common common;
        struct audio_cmd_element_routing_connect connect;
        struct audio_cmd_element_routing_disconnect disconnect;
    } u;
};

struct audio_cmd_element_pll {
    union {
        struct audio_cmd_element_common common;
    } u;
    uint32_t pll_id;
};

struct audio_cmd_element_dump {
    uint32_t type; /* command type */
    struct audio_cmd_pipeline_id pipeline;
    struct audio_cmd_element_id element;
};

struct audio_cmd_element_avtp_disconnect {
    uint32_t type; /* command type */
    struct audio_cmd_pipeline_id pipeline;
    struct audio_cmd_element_id element;
    uint32_t stream_index;
};

struct audio_cmd_element_avtp_connect {
    uint32_t type; /* command type */
    struct audio_cmd_pipeline_id pipeline;
    struct audio_cmd_element_id element;
    uint32_t stream_index;
#if (CONFIG_GENAVB_ENABLE == 1)
    struct genavb_stream_params stream_params;
#endif
};

struct audio_cmd_element_avtp {
    union {
        struct audio_cmd_element_common common;
        struct audio_cmd_element_avtp_connect connect;
        struct audio_cmd_element_avtp_disconnect disconnect;
    } u;
};

struct audio_resp_element {
    uint32_t type; /* command type */
    uint32_t status;
};

struct audio_cmd_element {
    union {
        struct audio_cmd_element_common common;
        struct audio_cmd_element_routing routing;
        struct audio_cmd_element_pll pll;
        struct audio_cmd_element_dump dump;
        struct audio_cmd_element_avtp avtp;
    } u;
};

struct audio_cmd_pipeline_common {
    uint32_t type; /* command type */
    struct audio_cmd_pipeline_id pipeline;
};

struct audio_cmd_pipeline_dump {
    uint32_t type; /* command type */
    struct audio_cmd_pipeline_id pipeline;
};

struct audio_resp_audio_pipeline {
    uint32_t type; /* command type */
    uint32_t status;
};

struct audio_cmd_pipeline {
    union {
        struct audio_cmd_pipeline_common common;
        struct audio_cmd_pipeline_dump audio_pipeline_dump;
        struct audio_cmd_element element;
    } u;
};

#endif /* _AUDIO_PIPELINE_CTRL_H_ */
