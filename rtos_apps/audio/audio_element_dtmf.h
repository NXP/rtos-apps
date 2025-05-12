/*
 * Copyright 2022, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_AUDIO_ELEMENT_DTMF_H_
#define _RTOS_APPS_AUDIO_ELEMENT_DTMF_H_

struct dtmf_element_config {
    unsigned int us;
    unsigned int pause_us;
    unsigned int sequence_pause_us;

    double amplitude;

    char *sequence;
};

#endif /* _RTOS_APPS_AUDIO_ELEMENT_DTMF_H_ */
