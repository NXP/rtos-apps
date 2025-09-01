/*
 * Copyright 2018, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_STATS_H_
#define _RTOS_APPS_STATS_H_

#include <stdint.h>

struct rtos_apps_stats {
    uint32_t log2_size;
    uint32_t current_count;

    int32_t current_min;
    int32_t current_max;
    int64_t current_mean;
    uint64_t current_ms;

    /* Stats snapshot */
    int32_t min;
    int32_t max;
    int32_t mean;
    uint64_t ms;
    uint64_t variance;

    /* absolute min/max (never reset) */
    int32_t abs_min;
    int32_t abs_max;

    const char *name;
    void (*func)(struct rtos_apps_stats *s);
};

#define RTOS_APPS_STATS_MAX_SLOTS 101U

struct rtos_apps_hist {
    uint32_t slots[RTOS_APPS_STATS_MAX_SLOTS];
    unsigned int n_slots;
    unsigned int slot_size;
};

void rtos_apps_stats_init(struct rtos_apps_stats *s, unsigned int log2_size, const char *name, void (*func)(struct rtos_apps_stats *s));
void rtos_apps_stats_reset(struct rtos_apps_stats *s);
void rtos_apps_stats_print(struct rtos_apps_stats *s);
void rtos_apps_stats_update(struct rtos_apps_stats *s, int32_t val);
void rtos_apps_stats_compute(struct rtos_apps_stats *s);

int rtos_apps_hist_init(struct rtos_apps_hist *hist, unsigned int n_slots, unsigned slot_size);
void rtos_apps_hist_update(struct rtos_apps_hist *hist, unsigned int value);
void rtos_apps_hist_reset(struct rtos_apps_hist *hist);
void rtos_apps_hist_print(struct rtos_apps_hist *hist);

#endif /* _RTOS_APPS_STATS_H_ */
