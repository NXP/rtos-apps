/*
 * Copyright 2020-2021, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "command_client.h"

#include "rtos_abstraction_layer.h"

#ifdef CONFIG_RTOS_APPS_LWIP

#include "lwip/sockets.h"

#include "rtos_abstraction_layer.h"

#include "rtos_apps/log.h"

struct command_msg_type {
    uint32_t seq_id;
    int32_t left_param;
    int32_t right_param;
    uint8_t go;
} __attribute__((packed));

struct command_client_ctx {
    rtos_thread_t command_client_task;
    int socket_fd;
    struct command_msg_type buf;
    struct sockaddr_in client_address;
    enum command_client_state state;
};

#define COMMAND_CLIENT_TASK_STACK_SIZE (RTOS_MINIMAL_STACK_SIZE + 512)
#define COMMAND_CLIENT_TASK_PRIO       (RTOS_MAX_PRIORITY - 1)

#define COMMAND_CLIENT_PORT 8000

static void command_client_task(void *pvParameters)
{
    int len;
    struct sockaddr_storage addr_from;
    int from_len = sizeof(addr_from);
    struct command_client_ctx *ctx = pvParameters;
    struct command_msg_type *buf_recv = &ctx->buf;

    ctx->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->socket_fd < 0) {
        log_err("socket() failed\n");
        goto exit;
    }

    ctx->client_address.sin_family = AF_INET;
    ctx->client_address.sin_addr.s_addr = htonl(INADDR_ANY);
    ctx->client_address.sin_port = htons(COMMAND_CLIENT_PORT);

    if (bind(ctx->socket_fd, (struct sockaddr *)&ctx->client_address, sizeof(ctx->client_address)) == -1) {
        log_err("bind() failed\n");
    }

    log_info("Command server initialized succesfully\n");
    log_info("Client waiting for message...\n");

    for (;;) {
        len = recvfrom(ctx->socket_fd, buf_recv, sizeof(*buf_recv), 0, (struct sockaddr *)&addr_from, (socklen_t *)&from_len);
        if (len != sizeof(*buf_recv))
            continue;

        buf_recv->seq_id = ntohl(buf_recv->seq_id);
        buf_recv->left_param = ntohl(buf_recv->left_param);
        buf_recv->right_param = ntohl(buf_recv->right_param);

        log_debug("RECEIVED:\n");
        log_debug("        seq id: %u\n", buf_recv->seq_id);
        log_debug("        left:   %d\n", buf_recv->left_param);
        log_debug("        right:  %d\n", buf_recv->right_param);
        log_debug("        go:     %hhu\n", buf_recv->go);

        switch (ctx->state) {
        case CMD_STATE_GO:
            if (buf_recv->go == STOP_EVENT) {
                ctx->state = CMD_STATE_STOP;
                log_info("Stop command received\n");
            }
            break;
        case CMD_STATE_STOP:
            if (buf_recv->go == 0 && buf_recv->left_param < 0 && buf_recv->right_param < 0) {
                ctx->state = CMD_STATE_GO;
                log_info("Start command received\n");
            }
            break;
        default:
            break;
        }
    }

exit:
    rtos_thread_abort(NULL);
}

int command_client_start(struct command_client_ctx **ctx)
{
    *ctx = rtos_malloc(sizeof(struct command_client_ctx));
    if (!*ctx)
        goto err_malloc;

    // By default, the command is set to start
    (*ctx)->state = CMD_STATE_GO;

    if (rtos_thread_create(&(*ctx)->command_client_task, COMMAND_CLIENT_TASK_PRIO, 0, COMMAND_CLIENT_TASK_STACK_SIZE,
        "command client task", &command_client_task, (*ctx)) < 0) {
        log_err("rtos_thread_create() failed\n");
        goto err;
    }

    return 0;

err:
    rtos_free(*ctx);

err_malloc:
    return -1;
}

void command_client_exit(struct command_client_ctx *ctx)
{
    rtos_thread_abort(&ctx->command_client_task);
    close(ctx->socket_fd);
    rtos_free(ctx);
}

int command_client_get_state(struct command_client_ctx *ctx)
{
    return ctx->state;
}
#else
int command_client_start(struct command_client_ctx **ctx) { return 0; }
void command_client_exit(struct command_client_ctx *ctx) { return; }

int command_client_get_state(struct command_client_ctx *ctx) { return 0; }

#endif
