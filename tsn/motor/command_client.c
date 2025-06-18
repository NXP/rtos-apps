/*
 * Copyright 2020-2021, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "command_client.h"
#include "lwip/sockets.h"
#include "log.h"
#include "FreeRTOS.h"

#define COMMAND_CLIENT_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 512)
#define COMMAND_CLIENT_TASK_PRIO       (configMAX_PRIORITIES - 1)

#define COMMAND_CLIENT_PORT 8000

struct command_client_ctx {
    TaskHandle_t command_client_task;
    int socket_fd;
    struct command_msg_type buf;
    struct sockaddr_in client_address;
    enum command_client_state state;
};

struct command_client_ctx command_client_context;
struct command_client_ctx *command_client_context_h = NULL;

static void command_client_task(void *pvParameters)
{
    int len;
    struct sockaddr_storage addr_from;
    int from_len = sizeof(addr_from);
    struct command_client_ctx *ctx = pvParameters;
    struct command_msg_type *buf_recv = &ctx->buf;

    ctx->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->socket_fd < 0) {
        log_err("socket call failed\n");
        goto exit;
    }

    ctx->client_address.sin_family = AF_INET;
    ctx->client_address.sin_addr.s_addr = htonl(INADDR_ANY);
    ctx->client_address.sin_port = htons(COMMAND_CLIENT_PORT);

    if (bind(ctx->socket_fd, (struct sockaddr *)&ctx->client_address, sizeof(ctx->client_address)) == -1) {
        log_err("bind call failed\n");
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
    vTaskDelete(NULL);
}

int command_client_start(struct command_client_ctx **ctx)
{
    int rc = 0;

    if (command_client_context_h == NULL) {
        command_client_context_h = &command_client_context;

        // By default, the command is set to start
        command_client_context_h->state = CMD_STATE_GO;

        if (xTaskCreate(command_client_task, "command client task", COMMAND_CLIENT_TASK_STACK_SIZE,
                        command_client_context_h, COMMAND_CLIENT_TASK_PRIO, &command_client_context_h->command_client_task) != pdPASS) {
            log_err("xTaskCreate() failed\n");
            rc = -1;
        }
    } else {
        log_err("Command client task already started\n");
        rc = -1;
    }

    if (ctx)
        *ctx = command_client_context_h;

    return rc;
}

int command_client_get_state(struct command_client_ctx *ctx)
{
    return ctx->state;
}
