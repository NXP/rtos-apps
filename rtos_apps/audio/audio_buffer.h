/*
 * Copyright 2022-2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_AUDIO_BUFFER_H_
#define _RTOS_APPS_AUDIO_BUFFER_H_

#include "audio_format.h"

#define AUDIO_BUFFER_FLAG_SHARED      (1 << 0)
#define AUDIO_BUFFER_FLAG_SHARED_USER (1 << 1)

/* Configuration */
struct audio_buffer_config {
    unsigned int storage; /* storage array index */
    unsigned int flags;
    unsigned int shared_id;
};

struct audio_buffer_storage_config {
    audio_sample_t *base; /* if NULL allocate on init */
    unsigned int periods; /* used to determine the size */
};

#endif /* _RTOS_APPS_AUDIO_BUFFER_H_ */
