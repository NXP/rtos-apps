/*
 * Copyright 2019-2020, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "network_stats.h"

#ifdef CONFIG_RTOS_APPS_LWIP
#include "rtos_abstraction_layer.h"

#include "lwip/api.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "rtos_apps/log.h"

#define UDP_SERVER_IP   "192.168.1.1"
#define UDP_SERVER_PORT 6000

#define NUM_MSG_PER_FRAME 4

struct net_stat_frame {
    struct net_stat_msg stats[NUM_MSG_PER_FRAME];
};

struct network_stats_ctx {
    int socket_fd;
    struct sockaddr_in server_address;
    struct net_stat_frame buffer_tx;
    uint32_t counter_frame;
};

int network_stats_open(struct network_stats_ctx **ctx)
{
    *ctx = rtos_malloc(sizeof(struct network_stats_ctx));
    if (!*ctx)
        goto err;

    (*ctx)->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if ((*ctx)->socket_fd < 0) {
        log_err("socket() failed\n");
        goto err_free;
    }

    memset(&(*ctx)->server_address, 0, sizeof(struct sockaddr_in));
    (*ctx)->server_address.sin_family = AF_INET;
    (*ctx)->server_address.sin_addr.s_addr = inet_addr(UDP_SERVER_IP);
    (*ctx)->server_address.sin_port = htons(UDP_SERVER_PORT);
    (*ctx)->counter_frame = 0;

    return 0;

err_free:
    rtos_free(*ctx);
err:
    return -1;
}

int network_stats_send(struct network_stats_ctx *ctx, struct net_stat_msg *dg)
{
    int rc;

    memcpy(&ctx->buffer_tx.stats[ctx->counter_frame & (NUM_MSG_PER_FRAME - 1)], dg, sizeof(struct net_stat_msg));

    ctx->counter_frame++;
    if (!(ctx->counter_frame & (NUM_MSG_PER_FRAME - 1))) {
        rc = sendto(ctx->socket_fd, &ctx->buffer_tx, sizeof(ctx->buffer_tx), MSG_DONTWAIT,
                    (struct sockaddr *)&ctx->server_address, sizeof(ctx->server_address));
        if (rc < 0) {
            log_err("sendto() failed: rc = %d\n", rc);
            goto err;
        }
    }
    return 0;

err:
    return -1;
}
#else
int network_stats_open(struct network_stats_ctx **ctx) { return 0; }

int network_stats_send(struct network_stats_ctx *ctx, struct net_stat_msg *dg) { return 0; }

#endif
