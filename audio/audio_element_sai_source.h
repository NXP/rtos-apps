/*
 * Copyright 2022, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _AUDIO_ELEMENT_SAI_SOURCE_H_
#define _AUDIO_ELEMENT_SAI_SOURCE_H_

#include "audio_buffer.h"

struct audio_element_config;
struct audio_element;

int sai_source_element_check_config(struct audio_element_config *config);
unsigned int sai_source_element_size(struct audio_element_config *config);
int sai_source_element_init(struct audio_element *element, struct audio_element_config *config,
                            struct audio_buffer *buffer);

#endif /* _AUDIO_ELEMENT_SAI_SOURCE_H_ */
