/*
 * Copyright 2018-2021, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdio.h>

#include "genavb/genavb.h"
#include "genavb/ether.h"
#include "genavb/helpers.h"
#include "genavb/qos.h"
#include "genavb/timer.h"

#include "rtos_apps/async.h"
#include "rtos_apps/log.h"
#include "rtos_apps/types.h"

#include "tsn_task.h"
#include "tsn_tasks_config.h"

#define SOCKET_DIR_RX 0
#define SOCKET_DIR_TX 1

void tsn_task_stats_init(struct tsn_task *task)
{
    stats_init(&task->stats.sched_err, 31, "sched err", NULL);
    hist_init(&task->stats.sched_err_hist, 100, 100);

    stats_init(&task->stats.proc_time, 31, "processing time", NULL);
    hist_init(&task->stats.proc_time_hist, 100, 1000);

    stats_init(&task->stats.total_time, 31, "total time", NULL);
    hist_init(&task->stats.total_time_hist, 100, 1000);

    task->stats.sched_err_max = 0;
}

int tsn_task_stats_start(struct tsn_task *task)
{
    uint64_t now = 0;
    int32_t sched_err;
    int rc = 0;

    task->stats.sched++;

    genavb_clock_gettime64(task->params->clk_id, &now);

    sched_err = now - task->sched_time;

    if (sched_err >= (task->params->task_period_ns)) {
        task->stats.sched_missed++;
        rc = -1;
    }

    if (sched_err < 0) {
        task->stats.sched_early++;
        sched_err = -sched_err;
        rc = -1;
    }

    stats_update(&task->stats.sched_err, sched_err);
    hist_update(&task->stats.sched_err_hist, sched_err);

    if (sched_err > task->stats.sched_err_max)
        task->stats.sched_err_max = sched_err;

    task->sched_now = now;

    return rc;
}

void tsn_task_stats_end(struct tsn_task *task)
{
    uint64_t now = 0;
    int32_t proc_time;
    int32_t total_time;

    genavb_clock_gettime64(task->params->clk_id, &now);

    proc_time = now - task->sched_now;
    total_time = now - task->sched_time;

    stats_update(&task->stats.proc_time, proc_time);
    hist_update(&task->stats.proc_time_hist, proc_time);

    stats_update(&task->stats.total_time, total_time);
    hist_update(&task->stats.total_time_hist, total_time);

    task->sched_time += task->params->task_period_ns;
}

static void tsn_task_stats_print(void *data)
{
    struct tsn_task *task = data;

    stats_compute(&task->stats_snap.sched_err);
    stats_compute(&task->stats_snap.proc_time);
    stats_compute(&task->stats_snap.total_time);

    log_info("tsn task(%p)\n", task);
    log_info("sched           : %u\n", task->stats_snap.sched);
    log_info("sched early     : %u\n", task->stats_snap.sched_early);
    log_info("sched missed    : %u\n", task->stats_snap.sched_missed);
    log_info("sched timeout   : %u\n", task->stats_snap.sched_timeout);
    log_info("clock discont   : %u\n", task->stats_snap.clock_discont);

    stats_print(&task->stats_snap.sched_err);
    hist_print(&task->stats_snap.sched_err_hist);

    stats_print(&task->stats_snap.proc_time);
    hist_print(&task->stats_snap.proc_time_hist);

    stats_print(&task->stats_snap.total_time);
    hist_print(&task->stats_snap.total_time_hist);

    task->stats_snap.pending = false;
}

static void tsn_task_stats_dump(struct tsn_task *task)
{
    if (task->stats_snap.pending)
        return;

    memcpy(&task->stats_snap, &task->stats, sizeof(struct tsn_task_stats));
    stats_reset(&task->stats.sched_err);
    stats_reset(&task->stats.proc_time);
    stats_reset(&task->stats.total_time);
    task->stats_snap.pending = true;

    if (rtos_apps_async_call(task->params->async, &tsn_task_stats_print, task) < 0)
        task->stats_snap.pending = false;
}

static void net_socket_stats_print(void *data)
{
    struct net_socket *sock = data;

    log_info("net %s socket(%p) %d\n", sock->dir ? "tx" : "rx", sock, sock->id);
    log_info("frames     : %u\n", sock->stats_snap.frames);
    log_info("err        : %u\n", sock->stats_snap.err);

    sock->stats_snap.pending = false;
}

static void net_socket_stats_dump(struct net_socket *sock)
{
    if (sock->stats_snap.pending)
        return;

    memcpy(&sock->stats_snap, &sock->stats, sizeof(struct net_socket_stats));
    sock->stats_snap.pending = true;

    if (rtos_apps_async_call(sock->async, &net_socket_stats_print, sock) < 0)
        sock->stats_snap.pending = false;
}

void tsn_stats_dump(struct tsn_task *task)
{
    int i;

    tsn_task_stats_dump(task);

    for (i = 0; i < task->params->num_rx_socket; i++)
        net_socket_stats_dump(&task->sock_rx[i]);

    for (i = 0; i < task->params->num_tx_socket; i++)
        net_socket_stats_dump(&task->sock_tx[i]);
}

void tsn_net_receive_free(struct net_socket *sock, unsigned int n)
{
    void *buf[n];
    int i;

    for (i = 0; i < n; i++)
        buf[i] = tsn_net_sock_buf(sock, i);

    genavb_socket_rx_free(buf, n);
}

void *tsn_net_payload(struct net_socket *sock, int idx)
{
    uint8_t *buf;

    buf = tsn_net_sock_buf(sock, idx);

    if (sock->zero_copy) {
        struct eth_hdr *eth = (struct eth_hdr *)buf;

        buf += sizeof(struct eth_hdr);

        if (eth->type == htons(ETHERTYPE_VLAN))
            buf += sizeof(struct vlanhdr);
    }

    return buf;
}

int tsn_net_receive_sock(struct net_socket *sock)
{
    struct tsn_task *task = container_of(sock, struct tsn_task, sock_rx[sock->id]);
    struct genavb_socket_rx_receive_params params[NET_RX_BATCH] = {[0 ... NET_RX_BATCH - 1] = { .flags = 0 }};
    int pkts;
    int status, i, n;

    n = sock->n;

    params[0].flags |= GENAVB_SOCKET_RX_TS;

    if (!sock->zero_copy) {
        for (i = 0; i < n; i++) {
            sock->iovec[i].iov_len = task->params->rx_buf_size;
        }
    }

    pkts = genavb_socket_rx_receive_iov(sock->genavb_rx, sock->iovec, params, n);
    if (pkts == n) {
        status = NET_OK;
        sock->stats.frames += pkts;
        if (params[0].flags & GENAVB_SOCKET_RX_TS)
            sock->ts = params[0].ts;
    } else {
        if (sock->zero_copy && pkts > 0)
            tsn_net_receive_free(sock, pkts);

        status = NET_NO_FRAME;
    }

    return status;
}

static void tsn_net_transmit_fill_hdr(struct net_socket *sock, int idx)
{
    struct tsn_task *task = container_of(sock, struct tsn_task, sock_tx[sock->id]);
    struct genavb_socket_tx_params *tx_params = &task->params->tx_params[sock->id];
    struct eth_hdr *eth = (struct eth_hdr *)tsn_net_sock_buf(sock, idx);
    struct vlanhdr *vlan;

    genavb_socket_get_hwaddr(tx_params->addr.port, eth->src);

    memcpy(eth->dst, tx_params->addr.u.l2.dst_mac, 6);
    eth->type = htons(ETHERTYPE_VLAN);

    vlan = (struct vlanhdr *)(eth + 1);
    vlan->type = tx_params->addr.u.l2.protocol;
    vlan->label = VLAN_LABEL(ntohs(tx_params->addr.vlan_id), tx_params->addr.priority, 0);

    sock->hdr_len = sizeof(*eth) + sizeof(*vlan);
}

int tsn_net_transmit_init(struct net_socket *sock, unsigned int size)
{
    struct genavb_iovec *iovec;
    void *buf[sock->n];
    int rc, i;

    size += sizeof(struct eth_hdr) + sizeof(struct vlanhdr);

    rc = genavb_socket_tx_alloc(sock->genavb_tx, buf, sock->n, size);
    if (rc == sock->n) {
        iovec = sock->iovec;
        for (i = 0; i < sock->n; i++) {
            iovec[i].iov_base = buf[i];

            tsn_net_transmit_fill_hdr(sock, i);
        }

        rc = NET_OK;
    } else {
        if (rc > 0)
            genavb_socket_tx_free(buf, sock->n - rc);

        rc = NET_ERR;
    }

    return rc;
}

int tsn_net_transmit_done(struct net_socket *sock, unsigned int n)
{
    void *buf[n];
    int rc, i, ready_pkts;

    rc = NET_OK;

    if (!sock->tx_pending)
        goto out;

    for (i = 0; i < n; i++)
        buf[i] = tsn_net_sock_buf(sock, i);

    ready_pkts = genavb_socket_tx_done(sock->genavb_tx, buf, sock->n);
    if (ready_pkts != n) {
        rc = NET_ERR;
        goto out;
    }

    sock->tx_pending = false;

out:
    return rc;
}

void tsn_net_transmit_free(struct net_socket *sock, unsigned int n)
{
    void *buf[n];
    int i;

    for (i = 0; i < n; i++)
        buf[i] = tsn_net_sock_buf(sock, i);

    genavb_socket_tx_free(buf, n);
}

int tsn_net_transmit_sock(struct net_socket *sock, bool tx_time, uint64_t ts)
{
    struct genavb_socket_tx_send_params params[NET_RX_BATCH] = {[0 ... NET_RX_BATCH - 1] = { .flags = (genavb_socket_tx_send_flags_t)0 }};
    int rc;
    int status, n;

    if (tx_time) {
        params[0].flags |= GENAVB_SOCKET_TX_TIME;
        params[0].ts = ts;
    }

    n = sock->n;

    rc = genavb_socket_tx_send_iov(sock->genavb_tx, sock->iovec, params, n);
    if (rc == n) {
        status = NET_OK;
        sock->stats.frames += n;
        sock->tx_pending = true;
    } else {
        if (rc > 0)
            sock->tx_pending = true;

        sock->stats.err += n;
        status = NET_ERR;
    }

    return status;
}

int tsn_net_receive_set_cb(struct net_socket *sock, void (*net_rx_cb)(void *data))
{
    int rc;

    rc = genavb_socket_rx_set_callback(sock->genavb_rx, net_rx_cb, sock);
    if (rc != GENAVB_SUCCESS)
        return -1;

    return 0;
}

int tsn_net_receive_enable_cb(struct net_socket *sock)
{
    int rc;

    rc = genavb_socket_rx_enable_callback(sock->genavb_rx);
    if (rc != GENAVB_SUCCESS)
        return -1;

    return 0;
}

int tsn_task_start(struct tsn_task *task)
{
    uint64_t now, start_time;

    if (!task->timer)
        goto err;

    if (genavb_clock_gettime64(task->params->clk_id, &now) != GENAVB_SUCCESS) {
        log_err("genavb_clock_gettime64() failed\n");
        goto err;
    }

    /* Start time = rounded up second + 1 second */
    start_time = ((now + NSECS_PER_SEC / 2) / NSECS_PER_SEC + 1) * NSECS_PER_SEC;

    /* Align on cycle time and add offset */
    start_time = (start_time / task->params->task_period_ns) * task->params->task_period_ns + task->params->task_period_offset_ns;

    if (genavb_timer_start(task->timer, start_time,
                           task->params->task_period_ns, GENAVB_TIMERF_ABS) != GENAVB_SUCCESS) {
        log_err("genavb_timer_start() failed\n");
        goto err;
    }

    task->sched_time = start_time + task->params->task_period_ns;

    return 0;

err:
    return -1;
}

void tsn_task_stop(struct tsn_task *task)
{
    if (task->timer) {
        genavb_timer_stop(task->timer);
    }
}

static int tsn_task_net_init(struct tsn_task *task)
{
    int i, j, k, l, rc;
    struct net_socket *sock;
    void *buf_rx, *buf_tx;
    genavb_sock_f_t rx_flags, tx_flags;
    unsigned int rx_alloc_size, tx_alloc_size;

    rx_flags = GENAVB_SOCKF_NONBLOCK;
    rx_alloc_size = task->params->num_packets * sizeof(struct genavb_iovec);
    tx_flags = 0;
    tx_alloc_size = task->params->num_packets * sizeof(struct genavb_iovec);

    if (task->params->zero_copy) {
        rx_flags |= GENAVB_SOCKF_ZEROCOPY | GENAVB_SOCKF_RAW;
        tx_flags |= GENAVB_SOCKF_ZEROCOPY | GENAVB_SOCKF_RAW | GENAVB_SOCKF_TX_REUSE;
    } else {
        rx_alloc_size += task->params->num_packets * task->params->rx_buf_size;
        tx_alloc_size += task->params->num_packets * task->params->tx_buf_size;
    }

    for (i = 0; i < task->params->num_rx_socket; i++) {
        sock = &task->sock_rx[i];
        sock->id = i;
        sock->dir = SOCKET_DIR_RX;
        sock->async = task->params->async;

        if (genavb_socket_rx_open(&sock->genavb_rx, rx_flags,
                                  &task->params->rx_params[i]) != GENAVB_SUCCESS) {
            log_err("genavb_socket_rx_open() failed\n");
            goto close_sock_rx;
        }

        sock->zero_copy = task->params->zero_copy;
        sock->n = task->params->num_packets;

        sock->iovec = rtos_malloc(rx_alloc_size);
        if (!sock->iovec) {
            genavb_socket_rx_close(sock->genavb_rx);
            log_err("rtos_malloc(iovec rx buffer array) failed\n");
            goto close_sock_rx;
        }

        memset(sock->iovec, 0, rx_alloc_size);

        if (!sock->zero_copy) {
            buf_rx = (void *)(sock->iovec + sock->n);
            for (l = 0; l < sock->n; l++)
                sock->iovec[l].iov_base = (unsigned char *)buf_rx + (l * task->params->rx_buf_size);
        }

        rc = genavb_socket_rx_set_option(sock->genavb_rx, GENAVB_SOCKET_RX_OPTION_TC_MASK, task->params->rx_tc_mask);
        if (rc < 0)
            log_err("genavb_socket_rx_set_option() failed: %s\n", genavb_strerror(rc));
    }

    for (j = 0; j < task->params->num_tx_socket; j++) {
        sock = &task->sock_tx[j];
        sock->id = j;
        sock->dir = SOCKET_DIR_TX;
        sock->async = task->params->async;

        if (genavb_socket_tx_open(&sock->genavb_tx, tx_flags, &task->params->tx_params[j]) != GENAVB_SUCCESS) {
            log_err("genavb_socket_tx_open() failed\n");
            goto close_sock_tx;
        }

        sock->zero_copy = task->params->zero_copy;
        sock->n = task->params->num_packets;
        sock->tx_pending = false;

        sock->iovec = rtos_malloc(tx_alloc_size);
        if (!sock->iovec) {
            genavb_socket_tx_close(sock->genavb_tx);
            log_err("rtos_malloc(iovec tx buffer array) failed\n");
            goto close_sock_tx;
        }

        memset(sock->iovec, 0, tx_alloc_size);

        if (!sock->zero_copy) {
            buf_tx = (void *)(sock->iovec + sock->n);
            for (l = 0; l < sock->n; l++)
                sock->iovec[l].iov_base = (unsigned char *)buf_tx + (l * task->params->tx_buf_size);
        } else {
            if (tsn_net_transmit_init(sock, task->params->tx_buf_size) != NET_OK) {
                log_err("tsn_net_transmit_init() failed\n");
                goto close_sock_tx;
            }
        }
    }

    return 0;

close_sock_tx:
    for (k = 0; k < j; k++) {
        sock = &task->sock_tx[k];

        rtos_free(sock->iovec);
        genavb_socket_tx_close(sock->genavb_tx);
    }

close_sock_rx:
    for (k = 0; k < i; k++) {
        sock = &task->sock_rx[k];

        rtos_free(sock->iovec);
        genavb_socket_rx_close(sock->genavb_rx);
    }

    return -1;
}

static void tsn_task_net_exit(struct tsn_task *task)
{
    int i;
    struct net_socket *sock;

    for (i = 0; i < task->params->num_rx_socket; i++) {
        sock = &task->sock_rx[i];

        rtos_free(sock->iovec);
        genavb_socket_rx_close(sock->genavb_rx);
    }

    for (i = 0; i < task->params->num_tx_socket; i++) {
        sock = &task->sock_tx[i];

        rtos_free(sock->iovec);
        genavb_socket_tx_close(sock->genavb_tx);
    }
}

int tsn_task_register(struct tsn_task **task, struct tsn_task_params *params,
                      int id, void (*main_loop)(void *ctx), void *ctx,
                      void (*timer_callback)(void *data, int count))
{
    char task_name[20] = {0, };

    *task = rtos_malloc(sizeof(struct tsn_task));
    if (!(*task))
        goto err;

    memset(*task, 0, sizeof(struct tsn_task));

    (*task)->id = id;
    (*task)->params = params;
    (*task)->ctx = ctx;

    snprintf(task_name, 19, "tsn task%1d", (*task)->id);
    task_name[19] = '\0';

    if (tsn_task_net_init(*task) < 0) {
        log_err("tsn_task_net_init() failed\n");
        goto err_free;
    }

    tsn_task_stats_init(*task);

    if (main_loop) {
        if (rtos_thread_create(&(*task)->thread, params->priority, 0, params->stack_depth, task_name, main_loop, ctx) < 0) {
            log_err("rtos_thread_create() failed\n");
            goto net_exit;
        }
    }

    if (timer_callback) {
        if (genavb_timer_create(&(*task)->timer, params->clk_id, 0) != GENAVB_SUCCESS) {
            log_err("genavb_timer_create() failed\n");
            goto task_delete;
        }

        if (genavb_timer_set_callback((*task)->timer, timer_callback, *task) != GENAVB_SUCCESS) {
            log_err("genavb_timer_set_callback() failed\n");
            goto timer_destroy;
        }
    } else {
        (*task)->timer = NULL;
    }

    return 0;

timer_destroy:
    if ((*task)->timer)
        genavb_timer_destroy((*task)->timer);

task_delete:
    if (main_loop)
        rtos_thread_abort(&(*task)->thread);

net_exit:
    tsn_task_net_exit(*task);

err_free:
    rtos_free(*task);

err:
    return -1;
}

void tsn_task_unregister(struct tsn_task **task)
{
    /* timer_destroy */
    if ((*task)->timer)
        genavb_timer_destroy((*task)->timer);

    /* task_delete */
    rtos_thread_abort(&(*task)->thread);

    /* net_exit */
    tsn_task_net_exit(*task);

    /* err_free */
    rtos_free(*task);
}
