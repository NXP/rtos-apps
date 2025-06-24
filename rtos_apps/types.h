/*
 * Copyright 2018-2019, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_TYPES_H_
#define _RTOS_APPS_TYPES_H_

#define NSECS_PER_SEC   (1000000000)
#define NSECS_PER_SEC_F (1000000000.0)
#define NSECS_PER_MSEC  (1000000)
#define NSECS_PER_USEC  (1000)
#define SECS_PER_MIN    (60.0)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define offset_of(type, member)           ((unsigned long)&(((type *)0)->member))
#define container_of(entry, type, member) ((type *)((unsigned char *)(entry)-offset_of(type, member)))

#endif /* _RTOS_APPS_TYPES_H_*/
