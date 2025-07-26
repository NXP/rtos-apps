/*
 * Copyright 2018-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "rtos_apps/async.h"
#include "rtos_apps/log.h"
#include "rtos_apps/types.h"

#include "cyclic_task.h"
#include "tsn_tasks_config.h"

#define CYCLIC_STAT_PERIOD_SEC 5
#define CYCLIC_EVENT_QUEUE_LENGTH 1

struct cyclic_event {
    unsigned int type;
    void *data;
};

enum cyclic_event_type {
    CYCLIC_EVENT_TYPE_TIMER = 0,
};

static void socket_stats_print(void *data)
{
    struct socket *sock = data;

    stats_compute(&sock->stats_snap.traffic_latency);

    log_info("cyclic rx socket(%p) net_sock(%p) peer id: %u\n", sock, sock->net_sock, sock->peer_id);
    log_info("valid frames  : %u\n", sock->stats_snap.valid_frames);
    log_info("err id        : %u\n", sock->stats_snap.err_id);
    log_info("err ts        : %u\n", sock->stats_snap.err_ts);
    log_info("err underflow : %u\n", sock->stats_snap.err_underflow);
    log_info("link %s\n", sock->stats_snap.link_status ? "up" : "down");

    stats_print(&sock->stats_snap.traffic_latency);
    hist_print(&sock->stats_snap.traffic_latency_hist);

    sock->stats_snap.pending = false;
}

static void socket_stats_dump(struct socket *sock)
{
    if (sock->stats_snap.pending)
        return;

    memcpy(&sock->stats_snap, &sock->stats, sizeof(struct socket_stats));
    stats_reset(&sock->stats.traffic_latency);
    sock->stats_snap.pending = true;

    if (rtos_apps_async_call(sock->async, &socket_stats_print, sock) < 0)
        sock->stats_snap.pending = false;
}

static void cyclic_stats_dump(struct cyclic_task *c_task)
{
    int i;

    for (i = 0; i < c_task->num_peers; i++)
        socket_stats_dump(&c_task->rx_socket[i]);
}

static void timer_callback(void *data, int count)
{
    struct tsn_task *task = (struct tsn_task *)data;
    struct cyclic_task *c_task = task->ctx;
    struct cyclic_event e;
    bool yield = false;

    if (count < 0)
        task->clock_discont = 1;

    e.type = CYCLIC_EVENT_TYPE_TIMER;

    rtos_mqueue_send_from_isr(c_task->queue_h, &e, RTOS_NO_WAIT, &yield);
    rtos_yield_from_isr(yield);
}

static void cyclic_net_receive(struct cyclic_task *c_task)
{
    int i;
    int status;
    struct socket *sock;
    struct tsn_task *task = c_task->task;
    struct tsn_common_hdr *hdr;
    int rx_frame;
    uint32_t traffic_latency;

    for (i = 0; i < c_task->num_peers; i++) {
        sock = &c_task->rx_socket[i];
        rx_frame = 0;
    retry:
        status = tsn_net_receive_sock(sock->net_sock);
        if (status == NET_NO_FRAME && !rx_frame)
            sock->stats.err_underflow++;

        if (status != NET_OK) {
            sock->stats.link_status = 0;
            continue;
        }

        rx_frame = 1;

        hdr = tsn_net_payload(sock->net_sock, 0);

        if (hdr->sched_time != (tsn_task_get_time(task) - task->params->transfer_time_ns)) {
            if (sock->net_sock->zero_copy)
                tsn_net_receive_free(sock->net_sock, sock->net_sock->n);

            sock->stats.err_ts++;
            goto retry;
        }

        if (hdr->src_id != sock->peer_id) {
            if (sock->net_sock->zero_copy)
                tsn_net_receive_free(sock->net_sock, sock->net_sock->n);

            sock->stats.err_id++;
            goto retry;
        }

        traffic_latency = sock->net_sock->ts - hdr->sched_time;

        stats_update(&sock->stats.traffic_latency, traffic_latency);
        hist_update(&sock->stats.traffic_latency_hist, traffic_latency);

        if (traffic_latency > sock->stats.traffic_latency_max)
            sock->stats.traffic_latency_max = traffic_latency;

        if (traffic_latency < sock->stats.traffic_latency_min)
            sock->stats.traffic_latency_min = traffic_latency;

        sock->stats.valid_frames++;
        sock->stats.link_status = 1;

        if (c_task->net_rx_func)
            c_task->net_rx_func(c_task->ctx, hdr->msg_id, hdr->src_id,
                                hdr + 1, hdr->len);

        if (sock->net_sock->zero_copy)
            tsn_net_receive_free(sock->net_sock, sock->net_sock->n);
    }
}

int cyclic_net_transmit(struct cyclic_task *c_task, unsigned int msg_id, void *buf, unsigned int len)
{
    struct tsn_task *task = c_task->task;
    struct socket *sock = &c_task->tx_socket;
    struct tsn_common_hdr *hdr[NET_TX_BATCH];
    struct tsn_task_params *params = &c_task->params;
    unsigned int payload_len;
    int status, i;

    payload_len = (len + sizeof(*hdr[0]));
    if (payload_len >= task->params->tx_buf_size)
        goto err;

    if (sock->net_sock->zero_copy) {
        if (tsn_net_transmit_done(sock->net_sock, sock->net_sock->n) != NET_OK)
            goto err;

    }

    for (i = 0; i < sock->net_sock->n; i++) {
        hdr[i] = tsn_net_payload(sock->net_sock, i);

        hdr[i]->msg_id = msg_id;
        hdr[i]->src_id = c_task->id;
        hdr[i]->sched_time = tsn_task_get_time(task);
        hdr[i]->len = len;

        if (len)
            memcpy(hdr[i] + 1, buf, len);

        sock->net_sock->iovec[i].iov_len = sock->net_sock->hdr_len + payload_len;
    }

    status = tsn_net_transmit_sock(sock->net_sock, params->tx_time_enabled, hdr[0]->sched_time + params->tx_time_offset_ns);
    if (status != NET_OK)
        goto err;

    return 0;

err:
    return -1;
}

static void main_cyclic(void *data)
{
    struct cyclic_task *c_task = data;
    struct tsn_task *task = c_task->task;
    struct cyclic_event e;
    int ret;
    rtos_tick_t timeout = RTOS_MS_TO_TICKS(2000);
    unsigned int num_sched_stats = CYCLIC_STAT_PERIOD_SEC * (NSECS_PER_SEC / task->params->task_period_ns);

    while (true) {
        /*
         * Wait to be woken-up by the timer
         */
        ret = rtos_mqueue_receive(c_task->queue_h, &e, timeout);
        if (ret < 0) {
            if (c_task->loop_func)
                c_task->loop_func(c_task->ctx, -1);

            cyclic_task_stop(c_task);
            cyclic_task_start(c_task);
            task->stats.sched_timeout++;
            continue;
        }

        if (task->clock_discont) {
            if (c_task->loop_func)
                c_task->loop_func(c_task->ctx, -1);

            task->clock_discont = 0;
            cyclic_task_stop(c_task);
            cyclic_task_start(c_task);
            task->stats.clock_discont++;
            continue;
        }

        if (tsn_task_stats_start(task) < 0) {
            if (c_task->loop_func)
                c_task->loop_func(c_task->ctx, -1);

            cyclic_task_stop(c_task);
            cyclic_task_start(c_task);
            task->stats.clock_discont++;
            continue;
        }

        /*
         * Receive, frames should be available
         */
        cyclic_net_receive(c_task);

        /*
         * Main loop
         */
        if (c_task->loop_func)
            c_task->loop_func(c_task->ctx, 0);

        tsn_task_stats_end(task);

        if (!(task->stats.sched % num_sched_stats)) {
            c_task->log_update_time(task->params->clk_id);
            tsn_stats_dump(task);
            cyclic_stats_dump(c_task);
        }
    }
}

void cyclic_task_set_period(struct cyclic_task_config *cfg, struct rtos_apps_tsn_config *config)
{
    struct tsn_task_params *params = &cfg->params;

    params->task_period_ns = config->period_ns;

    params->transfer_time_ns = (config->offset + 1) * config->period_ns / 2;

    if (cfg->type == CYCLIC_CONTROLLER)
        params->task_period_offset_ns = 0;
    else
        params->task_period_offset_ns = params->transfer_time_ns;
}

void cyclic_task_set_tx_time(struct cyclic_task_config *cfg, unsigned int tx_time_offset_ns, bool tx_time_enabled)
{
    struct tsn_task_params *params = &cfg->params;

    params->tx_time_enabled = tx_time_enabled;
    params->tx_time_offset_ns = tx_time_offset_ns;
}

void cyclic_task_get_monitoring(struct cyclic_task *task, struct monitoring_msg_cyclic_task *mon_cyclic_task,
                                uint32_t num_socket_monitored)
{
    uint32_t i;

    mon_cyclic_task->sched_err_max = task->task->stats.sched_err_max;
    task->task->stats.sched_err_max = 0;
    for (i = 0; i < task->num_peers; i++) {
        mon_cyclic_task->net_stats.socket_traffic_latency_max[i] = task->rx_socket[i].stats.traffic_latency_max;
        mon_cyclic_task->net_stats.socket_traffic_latency_min[i] = task->rx_socket[i].stats.traffic_latency_min;

        task->rx_socket[i].stats.traffic_latency_max = 0;
        task->rx_socket[i].stats.traffic_latency_min = 0xffffffff;

        if (i >= num_socket_monitored)
            return;
    }
}

int cyclic_task_start(struct cyclic_task *c_task)
{
    unsigned int period_ns = c_task->params.task_period_ns;

    if (!period_ns || ((NSECS_PER_SEC / period_ns) * period_ns != NSECS_PER_SEC)) {
        log_err("invalid task period(%u ns), needs to be an integer divider of 1 second\n", period_ns);
        return -1;
    }

    return tsn_task_start(c_task->task);
}

void cyclic_task_stop(struct cyclic_task *c_task)
{
    tsn_task_stop(c_task->task);
}

int cyclic_task_init(struct cyclic_task *c_task, struct cyclic_task_config *cfg,
                     void (*net_rx_func)(void *ctx, unsigned int msg_id, unsigned int src_id, void *buf, unsigned int len),
                     void (*loop_func)(void *ctx, int timer_status), void *ctx)
{
    struct tsn_task_params *params = &c_task->params;
    const struct tsn_stream *rx_stream, *tx_stream;
    int i;
    int rc;

    memcpy(params, &cfg->params, sizeof(struct tsn_task_params));
    c_task->id = cfg->id;
    c_task->num_peers = cfg->num_peers;
    c_task->log_update_time = cfg->log_update_time;

    log_info("cyclic task type: %d, id: %u\n\n", cfg->type, c_task->id);
    log_info("task params\n");
    log_info("task_period_ns        : %u\n", params->task_period_ns);
    log_info("task_period_offset_ns : %u\n", params->task_period_offset_ns);
    log_info("transfer_time_ns      : %u\n", params->transfer_time_ns);
    log_info("priority              : %u\n", params->stream_priority);
    log_info("port id               : %u\n", params->port_id);
    log_info("packets               : %u\n", params->num_packets);
    log_info("zero copy             : %s\n", params->zero_copy ? "enabled" : "disabled");
    log_info("rx tc mask            : 0x%02x\n", params->rx_tc_mask);

    tx_stream = tsn_conf_get_stream(cfg->tx_socket.stream_id);
    if (!tx_stream)
        goto err_get_config;

    memcpy(&params->tx_params[0].addr, &tx_stream->address,
           sizeof(struct net_address));
    params->num_tx_socket = 1;
    params->tx_params[0].addr.port = params->port_id;

    params->tx_params[0].addr.priority = params->stream_priority;

    c_task->tx_socket.peer_id = cfg->tx_socket.peer_id;

    for (i = 0; i < c_task->num_peers; i++) {
        rx_stream = tsn_conf_get_stream(cfg->rx_socket[i].stream_id);
        if (!rx_stream)
            goto err_get_config;

        memcpy(&params->rx_params[i].addr, &rx_stream->address,
               sizeof(struct net_address));
        params->rx_params[i].addr.port = params->port_id;
        params->num_rx_socket++;

        c_task->rx_socket[i].peer_id = cfg->rx_socket[i].peer_id;
        c_task->rx_socket[i].async = cfg->params.async;
    }

    c_task->queue_h = rtos_mqueue_alloc_init(CYCLIC_EVENT_QUEUE_LENGTH, sizeof(struct cyclic_event));
    if (!c_task->queue_h) {
        log_err("rtos_mqueue_alloc_init() failed\n");
        goto err_mqueue;
    }

    c_task->net_rx_func = net_rx_func;
    c_task->loop_func = loop_func;
    c_task->ctx = ctx;

    rc = tsn_task_register(&c_task->task, params, c_task->id, &main_cyclic, c_task, &timer_callback);
    if (rc < 0) {
        log_err("tsn_task_register() failed: rc = %d\n", rc);
        goto err_task_register;
    }

    for (i = 0; i < c_task->num_peers; i++) {
        c_task->rx_socket[i].net_sock = tsn_net_sock_rx(c_task->task, i);
        c_task->rx_socket[i].stats.traffic_latency_min = 0xffffffff;

        stats_init(&c_task->rx_socket[i].stats.traffic_latency, 31, "traffic latency", NULL);
        hist_init(&c_task->rx_socket[i].stats.traffic_latency_hist, 100, 1000);
    }

    c_task->tx_socket.net_sock = tsn_net_sock_tx(c_task->task, 0);

    log_info("success\n");

    return 0;

err_task_register:
    rtos_mqueue_destroy(c_task->queue_h);
err_mqueue:
err_get_config:
    return -1;
}

void cyclic_task_exit(struct cyclic_task *c_task)
{
    tsn_task_unregister(&c_task->task);

    if (c_task->queue_h)
        rtos_mqueue_destroy(c_task->queue_h);
}
