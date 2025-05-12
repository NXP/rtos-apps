/*
 * Copyright 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_AUDIO_ELEMENT_AVTP_SINK_H_
#define _RTOS_APPS_AUDIO_ELEMENT_AVTP_SINK_H_

#include "genavb/control_clock_domain.h"

#define AVTP_TX_STREAM_N  2

struct avtp_sink_element_config {
    unsigned int stream_n; /* number of streams */

    struct avtp_sink_stream_config {
        /* avtp config unused for now */
    } stream[AVTP_TX_STREAM_N];

    genavb_clock_domain_t clock_domain;
};

#endif /* _RTOS_APPS_AUDIO_ELEMENT_AVTP_SINK_H_ */
