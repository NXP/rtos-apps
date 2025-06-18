/*
 * Copyright 2019, 2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "local_network.h"

static struct controller_ctx *controller_local = NULL;
static struct io_device_ctx *io_device_local = NULL;

void local_bind_controller_io_device(struct controller_ctx *controller, struct io_device_ctx *io_device)
{
    controller_local = controller;
    io_device_local = io_device;
}

void local_controller_transmit(struct controller_ctx *ctx, struct msg_set_iq *msg_to_send)
{
    io_device_net_receive(io_device_local, MSG_SET_IQ, ctx->c_task->id, msg_to_send, sizeof(struct msg_set_iq));
}

#if BUILD_MOTOR_CONTROLLER == 1
void local_io_device_transmit(struct io_device_ctx *ctx, struct msg_feedback *msg_to_send)
{
    controller_net_receive(controller_local, MSG_FEEDBACK, ctx->c_task->id, msg_to_send, sizeof(struct msg_feedback));
}
#endif
