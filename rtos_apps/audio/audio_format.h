/*
 * Copyright 2022, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _RTOS_APPS_AUDIO_FORMAT_H_
#define _RTOS_APPS_AUDIO_FORMAT_H_

/* Audio samples in double floating point format, and [-1.0, 1.0] range.
 * If the format is changed, most of the functions below need to be adjusted.
 * Using a format with less than 32bit (default hardware fifo width) would
 * require more extensives adjustments.
 */
typedef double audio_sample_t;

#endif /* _RTOS_APPS_AUDIO_FORMAT_H_ */
