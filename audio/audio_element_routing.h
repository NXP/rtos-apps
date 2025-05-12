/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _AUDIO_ELEMENT_ROUTING_H_
#define _AUDIO_ELEMENT_ROUTING_H_

#include "rtos_apps/audio/audio_element_routing.h"
#include "rtos_apps/audio/audio_pipeline_ctrl.h"

#include "audio_buffer.h"

struct audio_element_config;
struct audio_element;

int routing_element_ctrl(struct audio_element *element, struct audio_cmd_element_routing *cmd, unsigned int len,
                         void *ctrl_handle);
int routing_element_check_config(struct audio_element_config *config);
unsigned int routing_element_size(struct audio_element_config *config);
int routing_element_init(struct audio_element *element, struct audio_element_config *config,
                         struct audio_buffer *buffer);

#endif /* _AUDIO_ELEMENT_ROUTING_H_ */
