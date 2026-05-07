/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_COMMON_H_
#define _RTOS_APPS_COMMON_H_

#include "stdint.h"

int str2mac(const char *str, uint8_t *mac);
int mac2str(uint8_t *mac, char *str, unsigned int len);

#endif /* _RTOS_APPS_COMMON_H_ */
