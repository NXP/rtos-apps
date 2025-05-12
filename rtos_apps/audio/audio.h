/*
 * Copyright 2021-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_AUDIO_H_
#define _RTOS_APPS_AUDIO_H_

#define MAX_AUDIO_DATA_THREADS 2

struct play_pipeline_config {
    const struct audio_pipeline_config *cfg[MAX_AUDIO_DATA_THREADS];
};

#endif /* _RTOS_APPS_AUDIO_H_ */
