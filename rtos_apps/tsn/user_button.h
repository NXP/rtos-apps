/*
 * Copyright 2021-2022, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_TSN_USER_BUTTON_H_
#define _RTOS_APPS_TSN_USER_BUTTON_H_

#include <stdbool.h>

enum event_button {
    BUTTON_PRESSED,
};

#if ENABLE_USER_BUTTON == 1
/*
 * Init wakeup GPIO handling task
*/
int init_gpio_handling_task(void);

/*
 * Trigger a user button input through software
*/
void user_button(bool from_isr);

#else
static inline int init_gpio_handling_task(void)
{
    return 0;
}

static inline void user_button(bool from_isr)
{
}
#endif

#endif /* _RTOS_APPS_TSN_USER_BUTTON_H_ */
