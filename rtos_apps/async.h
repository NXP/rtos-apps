/*
 * Copyright 2018, 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_ASYNC_H_
#define _RTOS_APPS_ASYNC_H_

#include "rtos_abstraction_layer.h"

struct rtos_apps_async_config {
    const char *name;
    unsigned int stack_size;
    int priority;
    int affinity;

    void (*func)(void *data);
    void *data;
    unsigned int period_ms;
};

struct rtos_apps_async;

int rtos_apps_async_call(struct rtos_apps_async *async, void (*func)(void *data), void *data);
struct rtos_apps_async *rtos_apps_async_init(const struct rtos_apps_async_config *cfg);
void rtos_apps_async_exit(struct rtos_apps_async *async);

#endif /* _RTOS_APPS_ASYNC_H_ */
