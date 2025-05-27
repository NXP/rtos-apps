/*
 * Copyright 2022, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_AUDIO_ENTRY_H_
#define _RTOS_APPS_AUDIO_ENTRY_H_

#include <stdint.h>

struct rtos_apps_audio_config {
    unsigned int thread_count;

    unsigned int data_priority;
    unsigned int data_stack_size;

    unsigned int ctrl_priority;
    unsigned int ctrl_stack_size;

    void *ctrl_handle;
};

int rtos_apps_audio_init(const struct rtos_apps_audio_config *config);

#endif /* _RTOS_APPS_AUDIO_ENTRY_H_ */
