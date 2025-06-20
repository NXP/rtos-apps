/*
 * Copyright 2020-2021, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMAND_SERVER_H_
#define _COMMAND_SERVER_H_

#include <stdbool.h>
#include <stdint.h>

#define STOP_EVENT 255

enum command_client_state {
    CMD_STATE_GO,
    CMD_STATE_STOP,
};

struct command_client_ctx;

#if ENABLE_LWIP == 1
int command_client_start(struct command_client_ctx **ctx);
int command_client_get_state(struct command_client_ctx *ctx);
#else
static inline int command_client_start(struct command_client_ctx **ctx)
{
    return 0;
}

static inline int command_client_get_state(struct command_client_ctx *ctx)
{
    return 0;
}
#endif

#endif /* _COMMAND_SERVER_H_ */
