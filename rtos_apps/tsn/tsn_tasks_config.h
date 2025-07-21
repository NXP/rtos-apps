/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_TSN_TASKS_CONFIG_H_
#define _RTOS_APPS_TSN_TASKS_CONFIG_H_

#include "genavb/clock.h"
#include "genavb/socket.h"

#define MAX_PEERS     2

#define MAX_RX_SOCKET 2
#define MAX_TX_SOCKET 1

#define PACKET_SIZE                80
#define APP_PERIOD_DEFAULT         100000
#define APP_PERIOD_MIN             100000

#define APP_OFFSET_DEFAULT         0
#define APP_OFFSET_MAX             1

#define APP_NETWORK_BUDGET_DEFAULT 15000

#define APP_PERIOD_SERIAL_DEFAULT  2000000

/* Supported APP_MODEs */
#define MOTOR_NETWORK 0 /* Enables the controller to control 1 or 2 motors remotely */
#define MOTOR_LOCAL   1 /* Not supported */
#define NETWORK_ONLY  2 /* Enables to observe time-sensitive traffic between controller and 1 or 2 IO devices */
#define SERIAL        3

typedef enum control_strategies {
    SYNCHRONIZED,
    FOLLOW,
    HOLD_INDEX,
    INTERLACED,
    STOP,
    IDENTIFICATION
} control_strategies_t;

enum task_id {
    CONTROLLER_0,
    IO_DEVICE_0,
    IO_DEVICE_1,
    MAX_TASKS_ID
};

struct tsn_task_params {
    unsigned int priority;
    unsigned short stack_depth;

    genavb_clock_id_t clk_id;
    unsigned int task_period_ns;
    unsigned int task_period_offset_ns; //modulo 1 second
    unsigned int transfer_time_ns;
    uint8_t stream_priority;
    unsigned int tx_time_offset_ns;
    bool tx_time_enabled;
    unsigned int port_id;
    unsigned int num_packets;
    bool zero_copy;
    unsigned int rx_tc_mask;

    int num_rx_socket;
    int rx_buf_size;
    struct genavb_socket_rx_params rx_params[MAX_RX_SOCKET];

    int num_tx_socket;
    int tx_buf_size;
    struct genavb_socket_tx_params tx_params[MAX_TX_SOCKET];
    struct rtos_apps_async *async;
};

struct alarm_task_config {
    struct tsn_task_params params;
    int type;
    int id;
    int stream_id;
    unsigned int length;
};

struct socket_config {
    int peer_id;
    int stream_id;
};

struct cyclic_task_config {
    struct tsn_task_params params;
    int type;
    int id;
    int num_peers;
    struct socket_config rx_socket[MAX_PEERS];
    struct socket_config tx_socket;

    /* private */
    void (*log_update_time)(genavb_clock_id_t clk_id);
};

struct cyclic_task_config *tsn_conf_get_cyclic_task(int index);

#endif /* _RTOS_APPS_TSN_TASKS_CONFIG_H_ */
