/*
 * Copyright 2021-2022, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef USER_BUTTON_H_
#define USER_BUTTON_H_

#include "rtos_apps/tsn/user_button.h"

#include "rtos_abstraction_layer.h"

#if ENABLE_USER_BUTTON == 1
/*
 * Register an event queue for user button input events
*/
int user_button_add_event_queue(rtos_mqueue_t *evt_queue);
#else
static inline int user_button_add_event_queue(rtos_mqueue_t *evt_queue)
{
    return 0;
}

#endif

#endif /* USER_BUTTON_H_ */
