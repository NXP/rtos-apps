/*
 * Copyright 2018-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TSN_TASKS_CONFIG_H_
#define _TSN_TASKS_CONFIG_H_

#include "rtos_abstraction_layer.h"

#include "genavb/net_types.h"

#define ETHERTYPE_MOTOROLA 0x818D
#define VLAN_ID            2

#define SCHED_TRAFFIC_OFFSET 35000

#define TASK_DEFAULT_STACK_SIZE   (RTOS_MINIMAL_STACK_SIZE + 256)
#define TASK_DEFAULT_PRIORITY     (RTOS_MAX_PRIORITY - 1)
#define TASK_DEFAULT_QUEUE_LENGTH (8)

#define NET_DELAY_OFFSET_DEFAULT        (APP_PERIOD_DEFAULT / 2)

#define NET_DELAY_OFFSET_SERIAL_DEFAULT (APP_PERIOD_SERIAL_DEFAULT / 2)

enum task_type {
    CYCLIC_CONTROLLER,
    CYCLIC_IO_DEVICE,
    ALARM_MONITOR,
    ALARM_IO_DEVICE,
};

struct tsn_stream {
    struct net_address address;
};

const struct tsn_stream *tsn_conf_get_stream(int index);
struct alarm_task_config *tsn_conf_get_alarm_task(int index);

#endif /* _TSN_TASKS_CONFIG_H_ */
