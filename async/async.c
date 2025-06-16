/*
 * Copyright 2018, 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "rtos_apps/log.h"
#include "rtos_apps/async.h"

#include "rtos_abstraction_layer.h"

#define ASYNC_NUM_MSG       16

struct async_msg {
    void (*func)(void *data);
    void *data;
};

struct rtos_apps_async {
    uint8_t queue_buffer[ASYNC_NUM_MSG * sizeof(struct async_msg)];
    rtos_mqueue_t queue;

    void (*func)(void *data);
    void *data;
    unsigned int period_ms;
    rtos_thread_t thread;
};

static void async_call_process(struct rtos_apps_async *async, unsigned int wait_ms)
{
    unsigned int elapsed, timeout;
    unsigned int last, now;
    struct async_msg msg;

    timeout = RTOS_TICKS_TO_UINT(RTOS_MS_TO_TICKS(wait_ms));
    last = rtos_get_current_time();

    while (true) {
        if (rtos_mqueue_receive(&async->queue, &msg, RTOS_UINT_TO_TICKS(timeout)) == 0) {
            msg.func(msg.data);

            now = rtos_get_current_time();
            elapsed = now - last;

            if (elapsed < timeout) {
                timeout -= elapsed;
                last = now;
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

static void async_task(void *pvParameters)
{
    struct rtos_apps_async *async = pvParameters;

    while (true) {
        async_call_process(async, async->period_ms);

        if (async->func)
            async->func(async->data);
    }
}

int rtos_apps_async_call(struct rtos_apps_async *async, void (*func)(void *data), void *data)
{
    struct async_msg msg;

    msg.func = func;
    msg.data = data;

    return rtos_mqueue_send(&async->queue, &msg, RTOS_NO_WAIT);
}

struct rtos_apps_async *rtos_apps_async_init(const struct rtos_apps_async_config *cfg)
{
    struct rtos_apps_async *async;

    async = rtos_malloc(sizeof(struct rtos_apps_async));
    if (!async) {
        log_err("rtos_malloc() failed\n");
        goto err_malloc;
    }

    memset(async, 0, sizeof(struct rtos_apps_async));

    async->func = cfg->func;
    async->data = cfg->data;
    async->period_ms = cfg->period_ms;

    if (rtos_mqueue_init(&async->queue, ASYNC_NUM_MSG, sizeof(struct async_msg),
                                      async->queue_buffer) < 0) {
        log_err("rtos_mqueue_init() failed\n");
        goto err_mqueue;
    }

    if (rtos_thread_create(&async->thread, cfg->priority, cfg->affinity,
        cfg->stack_size, cfg->name, &async_task, async) < 0) {
        log_err("rtos_thread_create(%s) failed\n", cfg->name);
        goto err_thread;
    }

    return async;

err_thread:
err_mqueue:
    rtos_free(async);

err_malloc:
    return NULL;
}

void rtos_apps_async_exit(struct rtos_apps_async *async)
{
    rtos_thread_abort(&async->thread);
    rtos_free(async);
}
