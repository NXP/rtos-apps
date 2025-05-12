/*
 * Copyright 2022, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_AUDIO_ELEMENT_AVTP_SOURCE_H_
#define _RTOS_APPS_AUDIO_ELEMENT_AVTP_SOURCE_H_

#include "genavb/control_clock_domain.h"

#define AVTP_RX_STREAM_N  2

struct avtp_source_element_config {
    unsigned int stream_n; /* number of streams */

    struct avtp_source_stream_config {
        unsigned int flags;
    } stream[AVTP_RX_STREAM_N];

    genavb_clock_domain_t clock_domain;
};

#endif /* _RTOS_APPS_AUDIO_ELEMENT_AVTP_SOURCE_H_ */
