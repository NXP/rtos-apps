/*
 * Copyright 2018-2021, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CYCLIC_TASK_H_
#define _CYCLIC_TASK_H_

#include "rtos_abstraction_layer.h"

#include "rtos_apps/tsn/tsn_entry.h"

#include "monitoring_stats.h"
#include "tsn_task.h"
#include "tsn_tasks_config.h"

struct socket_stats {
    bool pending;
    unsigned int valid_frames;
    unsigned int err_id;
    unsigned int err_ts;
    unsigned int err_underflow;
    int link_status;
    unsigned int traffic_latency_max;
    unsigned int traffic_latency_min;
    struct stats traffic_latency;
    struct hist traffic_latency_hist;
};

struct socket {
    int peer_id;
    struct socket_stats stats;
    struct socket_stats stats_snap;
    struct rtos_apps_async *async;
    struct net_socket *net_sock;
};

struct cyclic_task {
    struct tsn_task *task;
    struct tsn_task_params params;
    int id;
    int num_peers;
    struct socket rx_socket[MAX_PEERS];
    struct socket tx_socket;
    void (*net_rx_func)(void *ctx, int msg_id, int src_id, void *buf, int len);
    void (*loop_func)(void *ctx, int timer_status);
    void *ctx;
    void (*log_update_time)(genavb_clock_id_t clk_id);
    rtos_mqueue_t *queue_h;
};

int cyclic_task_init(struct cyclic_task *c_task, struct cyclic_task_config *cfg,
                     void (*net_rx_func)(void *ctx, int msg_id, int src_id, void *buf, int len),
                     void (*loop_func)(void *ctx, int timer_status), void *ctx);
void cyclic_task_exit(struct cyclic_task *c_task);
int cyclic_task_start(struct cyclic_task *c_task);
void cyclic_task_stop(struct cyclic_task *c_task);
int cyclic_net_transmit(struct cyclic_task *c_task, int msg_id, void *buf, int len);
void cyclic_task_get_monitoring(struct cyclic_task *task, struct monitoring_msg_cyclic_task *mon_cyclic_task,
                                uint32_t num_socket_monitored);
void cyclic_task_set_period(struct cyclic_task_config *cfg, struct rtos_apps_tsn_config *config);
void cyclic_task_set_tx_time(struct cyclic_task_config *cfg, unsigned int tx_time_offset_ns, bool tx_time_enabled);

#endif /* _CYCLIC_TASK_H_ */
