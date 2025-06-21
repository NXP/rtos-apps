/*
 * Copyright 2020-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SERIAL_IODEVICE_H_
#define _SERIAL_IODEVICE_H_

#include "cyclic_task.h"
#include "board.h"

#if BUILD_SERIAL == 1
int serial_iodevice_init(struct cyclic_task *c_task);
#else
static inline int serial_iodevice_init(struct cyclic_task *c_task) { return -1;}
#endif

#endif /* _SERIAL_IODEVICE_H_ */
