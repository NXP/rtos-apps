/*
 * Copyright 2018-2019, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ALARM_TASK_H_
#define _ALARM_TASK_H_

#include "rtos_abstraction_layer.h"

#include "tsn_task.h"
#include "tsn_tasks_config.h"

struct alarm_task {
    struct tsn_task *task;
    struct tsn_task_params params;
    unsigned int id;
    rtos_mqueue_t *queue_h;
    void (*net_rx_func)(void *ctx, unsigned int msg_id, unsigned int src_id, void *buf, unsigned int len);
    void *ctx;
};

int alarm_task_monitor_init(struct alarm_task *a_task, struct alarm_task_config *cfg,
                            void (*net_rx_func)(void *ctx, unsigned int msg_id, unsigned int src_id, void *buf, unsigned int len),
                            void *ctx);
void alarm_task_monitor_exit(struct alarm_task *a_task);
int alarm_task_io_init(struct alarm_task *a_task, struct alarm_task_config *cfg, void (*main_loop)(void *data), void *data);
void alarm_task_io_exit(struct alarm_task *a_task);
int alarm_net_transmit(struct alarm_task *a_task, unsigned int msg_id, void *buf, unsigned int len);

#endif /* _ALARM_TASK_H_ */
