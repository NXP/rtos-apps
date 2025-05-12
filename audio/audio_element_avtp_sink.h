/*
 * Copyright 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _AUDIO_ELEMENT_AVTP_SINK_H_
#define _AUDIO_ELEMENT_AVTP_SINK_H_

#include "genavb/control_clock_domain.h"

#include "rtos_apps/audio/audio_element_avtp_sink.h"
#include "rtos_apps/audio/audio_pipeline_ctrl.h"

#include "audio_buffer.h"

#define AVTP_TX_CHANNEL_N 2

#define AUDIO_ELEMENT_AVTP_SINK_MAX 1

struct audio_element_config;
struct audio_element;

int avtp_sink_element_ctrl(struct audio_element *element, struct audio_cmd_element_avtp *cmd, unsigned int len,
                           void *ctrl_handle);
int avtp_sink_element_check_config(struct audio_element_config *config);
unsigned int avtp_sink_element_size(struct audio_element_config *config);
int avtp_sink_element_init(struct audio_element *element, struct audio_element_config *config,
                           struct audio_buffer *buffer);

#endif /* _AUDIO_ELEMENT_AVTP_SINK_H_ */
