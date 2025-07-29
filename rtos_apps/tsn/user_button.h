/*
 * Copyright 2021-2022, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_TSN_USER_BUTTON_H_
#define _RTOS_APPS_TSN_USER_BUTTON_H_

#include <stdbool.h>

#include "rtos_abstraction_layer.h"

enum event_button {
    BUTTON_PRESSED,
};

struct rtos_apps_user_button_config {
    unsigned int irq_n;
    void (*init)(void);
    void (*exit)(void);
    uint32_t (*active)(void);
    void (*clear)(void);
};

struct rtos_apps_user_button;

void rtos_apps_user_button_irq(struct rtos_apps_user_button *button);

/*
 * Register an event queue for user button events
*/
int rtos_apps_user_button_register_queue(struct rtos_apps_user_button *button, rtos_mqueue_t *queue);

/*
 * Generate a user button event Through software
*/
void rtos_apps_user_button_event(struct rtos_apps_user_button *button);

struct rtos_apps_user_button *rtos_apps_user_button_init(struct rtos_apps_user_button_config *cfg);

void rtos_apps_user_button_exit(struct rtos_apps_user_button *button);

#endif /* _RTOS_APPS_TSN_USER_BUTTON_H_ */
