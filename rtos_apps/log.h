/*
 * Copyright 2022, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _RTOS_APPS_LOG_H_
#define _RTOS_APPS_LOG_H_

#include "rtos_abstraction_layer.h"

/** Log levels definition */
typedef enum {
    LOG_CRIT,
    LOG_ERR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,

    LOG_LEVEL_MAX
} rtos_apps_log_level_t;

/** Current log level configuration */
extern rtos_apps_log_level_t rtos_apps_log_level_config;

#if defined(CONFIG_RTOS_APPS_LOG_TIMESTAMP)

#include <stdint.h>
#include <inttypes.h>

extern uint64_t rtos_apps_log_timestamp;

static inline void rtos_apps_log_timestamp_update(uint64_t timestamp)
{
    rtos_apps_log_timestamp = timestamp;
}

#define RTOS_APPS_LOG_FMT  "%-4.4s %11.11" PRIu64 " %-22.22s: "
#define RTOS_APPS_LOG_ARGS rtos_apps_log_timestamp, __func__

#else

#define RTOS_APPS_LOG_FMT  "%-4.4s %-22.22s: "
#define RTOS_APPS_LOG_ARGS __func__

#endif

/** Logging macros definitions
 *
 *  Usage:
 *
 *      log(INFO, "---\n");
 *  or
 * 	    log_info("---\n");
 */
#define log(LEVEL, format, ...)                                                                                        \
    do {                                                                                                               \
        if (rtos_apps_log_level_config >= LOG_##LEVEL)                                                                 \
            rtos_printf(RTOS_APPS_LOG_FMT format "\r", #LEVEL, RTOS_APPS_LOG_ARGS, ##__VA_ARGS__);                     \
    } while (0)

#define log_crit(...)  log(CRIT, __VA_ARGS__)
#define log_err(...)   log(ERR, __VA_ARGS__)
#define log_warn(...)  log(WARN, __VA_ARGS__)
#define log_info(...)  log(INFO, __VA_ARGS__)
#define log_debug(...) log(DEBUG, __VA_ARGS__)

#define log_raw(LEVEL, ...)                                                                                            \
    do {                                                                                                               \
        if (rtos_apps_log_level_config >= LOG_##LEVEL)                                                                 \
            rtos_printf(__VA_ARGS__);                                                                                  \
    } while (0)

#define log_raw_info(...) log_raw(INFO, __VA_ARGS__)

/** Set current log level configuration */
static inline void rtos_apps_log_level_config_set(rtos_apps_log_level_t rtos_apps_log_level)
{
    if (rtos_apps_log_level < LOG_LEVEL_MAX)
        rtos_apps_log_level_config = rtos_apps_log_level;
}

#endif /* _RTOS_APPS_LOG_H_ */
