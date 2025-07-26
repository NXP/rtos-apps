/*
 * Copyright 2019-2021, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_apps/log.h"
#include "rtos_apps/stats.h"

/** Initialize a stats structure.
 * @s: 			Pointer to structure to be initialized
 * @log2_size:	Set size to be reached before statistics are computed, expressed as a power of 2
 * @name:		character field used by stats_print
 * @func:		pointer to the function to be called when stats are computed
 *
 */
void stats_init(struct stats *s, unsigned int log2_size, const char *name, void (*func)(struct stats *s))
{
    s->log2_size = log2_size;
    s->name = name;
    s->func = func;

    s->abs_min = 0x7fffffff;
    s->abs_max = -0x7fffffff;

    stats_reset(s);
}

void stats_reset(struct stats *s)
{
    s->current_count = 0;
    s->current_min = 0x7fffffff;
    s->current_mean = 0;
    s->current_max = -0x7fffffff;
    s->current_ms = 0;
}

/** Example function to be passed to stats_init
 * Usage:
 * stats_init(s, log2_size, "your string", print_stats);
 */
void stats_print(struct stats *s)
{
    log_info("stats(%p) %s min %d mean %d max %d rms^2 %llu stddev^2 %llu absmin %d absmax %d\n\r", s, s->name, s->min,
             s->mean, s->max, s->ms, s->variance, s->abs_min, s->abs_max);
}

/** Update stats with a given sample.
 * @s: handler for the stats being monitored
 * @val: sample to be added
 *
 * This function adds a sample to the set.
 * If the number of accumulated samples is equal to the requested size of the set, the following indicators will be computed:
 *    . minimum observed value,
 *    . maximum observed value,
 *    . mean value,
 *    . square of the RMS (i.e. mean of the squares)
 *    . square of the standard deviation (i.e. variance)
 */
void stats_update(struct stats *s, int32_t val)
{
    s->current_count++;

    s->current_mean += val;
    s->current_ms += (int64_t)val * val;

    if (val < s->current_min) {
        s->current_min = val;
        if (val < s->abs_min)
            s->abs_min = val;
    }

    if (val > s->current_max) {
        s->current_max = val;
        if (val > s->abs_max)
            s->abs_max = val;
    }

    if (s->current_count == (1U << s->log2_size)) {
        s->ms = s->current_ms >> s->log2_size;
        s->variance = s->ms - ((s->current_mean * s->current_mean) >> (2 * s->log2_size));
        s->mean = s->current_mean >> s->log2_size;

        s->min = s->current_min;
        s->max = s->current_max;

        if (s->func)
            s->func(s);

        stats_reset(s);
    }
}

/** Compute current stats event if set size hasn't been reached yet.
 * @s: handler for the stats being monitored
 *
 * This function computes current statistics for the stats:
 *    . minimum observed value,
 *    . maximum observed value,
 *    . mean value,
 *    . square of the RMS (i.e. mean of the squares)
 *    . square of the standard deviation (i.e. variance)
 */
void stats_compute(struct stats *s)
{
    if (s->current_count) {
        s->ms = s->current_ms / s->current_count;
        s->variance = s->ms - (s->current_mean * s->current_mean) / ((int64_t)s->current_count * s->current_count);
        s->mean = s->current_mean / s->current_count;
    } else {
        s->mean = 0;
        s->ms = 0;
        s->variance = 0;
    }

    s->min = s->current_min;
    s->max = s->current_max;
}

int hist_init(struct hist *hist, unsigned int n_slots, unsigned slot_size)
{
    /* One extra slot for last bucket.
     */
    if ((n_slots + 1U) > RTOS_APPS_STATS_MAX_SLOTS)
        return -1;

    hist->n_slots = n_slots;
    hist->slot_size = slot_size;

    return 0;
}

void hist_update(struct hist *hist, unsigned int value)
{
    unsigned int slot = value / hist->slot_size;

    if (slot > hist->n_slots)
        slot = hist->n_slots;

    hist->slots[slot]++;
}

void hist_reset(struct hist *hist)
{
    int i;

    for (i = 0; i <= hist->n_slots; i++)
        hist->slots[i] = 0;
}

void hist_print(struct hist *hist)
{
    int i;

    log_info("n_slot %u slot_size %u \n", hist->n_slots, hist->slot_size);

    log_raw_info(RTOS_APPS_LOG_FMT, "INFO", RTOS_APPS_LOG_ARGS);
    for (i = 0; i < (hist->n_slots + 1U); i++) {
        log_raw_info("%u ", hist->slots[i]);
    }
    log_raw_info("\r\n");
}
