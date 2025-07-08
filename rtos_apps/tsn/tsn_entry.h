/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_TSN_ENTRY_H_
#define _RTOS_APPS_TSN_ENTRY_H_

#include <stdbool.h>

struct rtos_apps_tsn_config {
    unsigned int mode;
    unsigned int role;
    unsigned int num_io_devices;
    float motor_offset;
    unsigned int control_strategy;
    unsigned int cmd_client;
    unsigned int period_ns;
    unsigned int priority;
    unsigned int tx_time_offset_ns;
    bool tx_time_enabled;
    unsigned int port_id;
    unsigned int packets;
    bool zero_copy;
    unsigned int rx_tc_mask;
};

int rtos_apps_tsn_init(struct rtos_apps_tsn_config *config);

#endif /* _RTOS_APPS_TSN_ENTRY_H_ */
