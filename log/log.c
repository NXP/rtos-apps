/*
 * Copyright 2022, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_apps/log.h"

/** Current log level configuration */
rtos_apps_log_level_t rtos_apps_log_level_config = LOG_INFO;

#if defined(CONFIG_RTOS_APPS_LOG_TIMESTAMP)
uint64_t rtos_apps_log_timestamp = 0;
#endif
