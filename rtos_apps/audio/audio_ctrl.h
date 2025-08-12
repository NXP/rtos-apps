/*
 * Copyright 2021-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _RTOS_APPS_AUDIO_CTRL_H_
#define _RTOS_APPS_AUDIO_CTRL_H_

#include <stdint.h>
#include <stdbool.h>

#include "audio_pipeline_ctrl.h"

enum {
    AUDIO_CMD_TYPE_RUN = 0x0100,
    AUDIO_CMD_TYPE_STOP,
    AUDIO_RESP_TYPE = 0x0110,

    AUDIO_CMD_TYPE_PIPELINE_DUMP = 0x200,
    AUDIO_RESP_TYPE_PIPELINE = 0x2ff,

    AUDIO_CMD_TYPE_ELEMENT_DUMP = 0x300,
    AUDIO_RESP_TYPE_ELEMENT = 0x3ff,

    AUDIO_CMD_TYPE_ELEMENT_ROUTING_CONNECT = 0x400,
    AUDIO_CMD_TYPE_ELEMENT_ROUTING_DISCONNECT = 0x401,
    AUDIO_RESP_TYPE_ELEMENT_ROUTING = 0x44f,

    AUDIO_CMD_TYPE_ELEMENT_PLL_ENABLE = 0x450,
    AUDIO_CMD_TYPE_ELEMENT_PLL_DISABLE = 0x451,
    AUDIO_CMD_TYPE_ELEMENT_PLL_ID = 0x452,
    AUDIO_RESP_TYPE_ELEMENT_PLL = 0x45f,

    AUDIO_CMD_TYPE_ELEMENT_AVTP_SOURCE_CONNECT = 0x480,
    AUDIO_CMD_TYPE_ELEMENT_AVTP_SOURCE_DISCONNECT = 0x481,
    AUDIO_CMD_TYPE_ELEMENT_AVTP_SOURCE_SET_HANDLE = 0x482,
    AUDIO_CMD_TYPE_ELEMENT_AVTP_SINK_CONNECT = 0x490,
    AUDIO_CMD_TYPE_ELEMENT_AVTP_SINK_DISCONNECT = 0x491,
    AUDIO_CMD_TYPE_ELEMENT_AVTP_SINK_SET_HANDLE = 0x492,
    AUDIO_RESP_TYPE_ELEMENT_AVTP = 0x49f,
};

enum {
    AUDIO_RESP_STATUS_SUCCESS = 0,
    AUDIO_RESP_STATUS_ERROR = 1,
};

/* Audio application commands */
struct audio_cmd_run {
    uint32_t type;
    uint32_t id;
    uint32_t frequency;
    uint32_t period;
    bool use_alternate_config;
    uint8_t addr[6];
};

struct audio_cmd_stop {
    uint32_t type;
};

struct audio_resp_audio {
    uint32_t type;
    uint32_t status;
};

struct audio_cmd {
    uint32_t type;
};

struct audio_resp {
    uint32_t type;
    uint32_t status;
};

struct audio_command {
    union {
         struct audio_cmd cmd;
         struct audio_cmd_run audio_run;
         struct audio_cmd_stop audio_stop;
         struct audio_cmd_pipeline audio_pipeline;
    } u;
};

struct audio_response {
    union {
        struct audio_resp resp;
        struct audio_resp_audio audio;
        struct audio_resp_element_routing routing;
    } u;
};

#endif /* _RTOS_APPS_AUDIO_CTRL_H_ */
