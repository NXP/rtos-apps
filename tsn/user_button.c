/*
 * Copyright 2021, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_apps/tsn/user_button.h"

#ifdef CONFIG_RTOS_APPS_USER_BUTTON
#include <stdbool.h>
#include <string.h>

#include "fsl_common.h"
#include "rtos_abstraction_layer.h"
#include "rtos_apps/log.h"

#define USER_BUTTON_TASK_STACK_SIZE (RTOS_MINIMAL_STACK_SIZE + 128)
#define USER_BUTTON_TASK_PRIORITY   2

#define MAX_EVENT_QUEUES 2

struct rtos_apps_user_button {
    rtos_sem_t sem;
    rtos_mutex_t mutex;
    rtos_thread_t thread;
    struct {
        rtos_mqueue_t *handle;
        bool used;
    } queue[MAX_EVENT_QUEUES];
    void (*init)(void);
    void (*exit)(void);
    uint32_t (*active)(void);
    void (*clear)(void);
    IRQn_Type irq_n;
};

static void user_button_task(void *data)
{
    struct rtos_apps_user_button *button = data;

    for (;;) {
        if (!rtos_sem_take(&button->sem, RTOS_WAIT_FOREVER)) {
            /*
             * ISR may be triggered multiple times even though the ISR bit has been cleared.
             * Hence, the ISR bit must always be checked
             */
            if (button->active()) {
                log_raw_info("\nA user_button press detected\n");
                rtos_apps_user_button_event(button);
                button->clear();
            }

            EnableIRQ(button->irq_n);
        }
    }
}

void rtos_apps_user_button_irq(struct rtos_apps_user_button *button)
{
    bool yield = false;

    DisableIRQ(button->irq_n);

    rtos_sem_give_from_isr(&button->sem, &yield);
    rtos_yield_from_isr(yield);
}

void rtos_apps_user_button_unregister_queue(struct rtos_apps_user_button *button, rtos_mqueue_t *queue)
{
    int i;

    rtos_mutex_lock(&button->mutex, RTOS_WAIT_FOREVER);

    for (i = 0; i < MAX_EVENT_QUEUES; i++) {
        if (!button->queue[i].used)
            continue;

        if (button->queue[i].handle == queue) {
            button->queue[i].handle = NULL;
            button->queue[i].used = false;
            goto out;
        }
    }

out:
    rtos_mutex_unlock(&button->mutex);
}

int rtos_apps_user_button_register_queue(struct rtos_apps_user_button *button, rtos_mqueue_t *queue)
{
    int i;

    rtos_mutex_lock(&button->mutex, RTOS_WAIT_FOREVER);

    for (i = 0; i < MAX_EVENT_QUEUES; i++) {
        if (button->queue[i].used)
            continue;

        button->queue[i].handle = queue;
        button->queue[i].used = true;
        goto out;
    }

    rtos_mutex_unlock(&button->mutex);

    return -1;

out:
    rtos_mutex_unlock(&button->mutex);

    return 0;
}

void rtos_apps_user_button_event(struct rtos_apps_user_button *button)
{
    enum event_button event = BUTTON_PRESSED;
    unsigned int i;

    rtos_mutex_lock(&button->mutex, RTOS_WAIT_FOREVER);

    for (i = 0; i < MAX_EVENT_QUEUES; i++) {
        if (!button->queue[i].used)
            continue;

        rtos_mqueue_send(button->queue[i].handle, &event, RTOS_NO_WAIT);
    }

    rtos_mutex_unlock(&button->mutex);
}

struct rtos_apps_user_button *rtos_apps_user_button_init(struct rtos_apps_user_button_config *cfg)
{
    struct rtos_apps_user_button *button;

    button = rtos_malloc(sizeof(struct rtos_apps_user_button));
    if (!button) {
        log_err("rtos_malloc() failed\n");
        goto err_malloc;
    }

    memset(button, 0, sizeof(struct rtos_apps_user_button));

    button->init = cfg->init;
    button->exit = cfg->exit;
    button->active = cfg->active;
    button->clear = cfg->clear;
    button->irq_n = (IRQn_Type)cfg->irq_n;

    if (rtos_mutex_init(&button->mutex) < 0)  {
        log_err("rtos_mutex_init() failed\n");
        goto err_mutex;
    }

    if (rtos_sem_init(&button->sem, 0) < 0) {
        log_err("rtos_sem_init() failed\n");
        goto err_sem;
    }

    if (rtos_thread_create(&button->thread, USER_BUTTON_TASK_PRIORITY, 0, USER_BUTTON_TASK_STACK_SIZE,
         "user button task", &user_button_task, button) < 0) {
        log_err("rtos_thread_create() failed\n");
        goto err_thread;
    }

    /* Init button GPIO */
    button->init();

    return button;

err_thread:
    rtos_sem_destroy(&button->sem);

err_sem:
err_mutex:
    rtos_free(button);

err_malloc:
    return NULL;
}

void rtos_apps_user_button_exit(struct rtos_apps_user_button *button)
{
    button->exit();

    rtos_thread_abort(&button->thread);
    rtos_sem_destroy(&button->sem);
    rtos_free(button);
}
#else
void rtos_apps_user_button_irq(struct rtos_apps_user_button *button) { return; }

int rtos_apps_user_button_register_queue(struct rtos_apps_user_button *button, rtos_mqueue_t *queue) { return 0; }

void rtos_apps_user_button_unregister_queue(struct rtos_apps_user_button *button, rtos_mqueue_t *queue) { return; }

void rtos_apps_user_button_event(struct rtos_apps_user_button *button) { return; }

struct rtos_apps_user_button *rtos_apps_user_button_init(struct rtos_apps_user_button_config *cfg) { return (struct rtos_apps_user_button *)0xdeadbeef; }

void rtos_apps_user_button_exit(struct rtos_apps_user_button *button) { return; }

#endif
