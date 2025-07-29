/*
 * Copyright 2020-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SERIAL_IODEVICE_H_
#define _SERIAL_IODEVICE_H_

#include "rtos_apps/tsn/tsn_entry.h"

#include "cyclic_task.h"

int serial_iodevice_init(struct cyclic_task *c_task, struct rtos_apps_tsn_serial_iodevice_config *cfg);

#endif /* _SERIAL_IODEVICE_H_ */
