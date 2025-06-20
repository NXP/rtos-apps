/*
 * Copyright 2018-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_apps/log.h"
#include "rtos_apps/tsn/tsn_entry.h"

#include "alarm_task.h"
#include "cyclic_task.h"
#include "serial_iodevice.h"
#include "tsn_tasks_config.h"

#if BUILD_MOTOR_CONTROLLER == 1
#include "controller.h"
#endif

#if BUILD_MOTOR_IO_DEVICE == 1
#include "io_device.h"
#include "local_network.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

struct tsn_app_ctx {
#if BUILD_MOTOR_CONTROLLER == 1
    struct controller_ctx ctrl;
#endif
#if BUILD_MOTOR_IO_DEVICE == 1
    struct io_device_ctx io_device;
#endif

    struct cyclic_task *c_task;
    struct alarm_task *a_task;
    struct cyclic_task *opt_io_device_task;
};

static const char *app_mode_names[] = {"MOTOR_NETWORK", "MOTOR_LOCAL", "NETWORK_ONLY", "SERIAL"};

/*******************************************************************************
 * Code
 ******************************************************************************/

static void null_loop(void *data, int timer_status)
{
    struct cyclic_task *c_task = data;

    cyclic_net_transmit(c_task, 0, NULL, 0);
}

int tsn_app_init(struct tsn_app_config *config)
{
    struct tsn_app_ctx *ctx;

    ctx = rtos_malloc(sizeof(struct tsn_app_ctx));
    if (!ctx) {
        log_err("rtos_malloc() failed\n");
        goto err_malloc;
    }

    memset(ctx, 0, sizeof(struct tsn_app_ctx));

    ctx->a_task = tsn_conf_get_alarm_task(config->role);
    if (!ctx->a_task) {
        log_err("tsn_conf_get_alarm_task() failed\n");
        goto err_init;
    }

    log_info("tsn_app config\n");
    log_info("mode             : %s\n", app_mode_names[config->mode]);
    log_info("role             : %u\n", config->role);
    log_info("num_io_devices   : %u\n", config->num_io_devices);
    log_info("motor_offset     : %f\n", config->motor_offset);
    log_info("control_strategy : %u\n", config->control_strategy);
    log_info("app period       : %u\n", config->period_ns);
    if (config->tx_time_enabled) {
        log_info("tx time offset   : %u\n", config->tx_time_offset_ns);
    }
    log_info("port id          : %u\n", config->port_id);
    log_info("num packets      : %u\n", config->packets);
    log_info("zero copy        : %s\n", config->zero_copy ? "enabled" : "disabled");

    if (config->period_ns < APP_PERIOD_MIN) {
        log_err("invalid application period, minimum is %u ns\n", APP_PERIOD_MIN);
        goto err_init;
    }

#if ((BUILD_MOTOR_CONTROLLER == 0) && (BUILD_MOTOR_IO_DEVICE == 0))
    log_info("BUILD_MOTOR disabled, MOTOR_NETWORK and MOTOR_LOCAL modes cannot be used\n");
#endif

#if ((BUILD_MOTOR_CONTROLLER == 1) || (BUILD_MOTOR_IO_DEVICE == 1))
    if (config->mode == MOTOR_NETWORK) {
        if ((config->period_ns != 100000) && (config->period_ns != 250000)) {
            log_err("invalid application period, only 100000 us and 250000 us are supported\n");
            goto err_init;
        }
    }
#endif

#if ((BUILD_MOTOR_CONTROLLER == 1) && (BUILD_MOTOR_IO_DEVICE == 1))
    if (config->mode == MOTOR_LOCAL) {
        if ((config->period_ns != 100000) && (config->period_ns != 250000)) {
            log_err("invalid application period, only 100000 us and 250000 us are supported\n");
            goto err_init;
        }

        ctx->c_task = tsn_conf_get_cyclic_task(0);
        if (!ctx->c_task) {
            log_err("tsn_conf_get_cyclic_task() failed\n");
            goto err_init;
        }

        cyclic_task_set_period(ctx->c_task, config->period_ns);

        ctx->c_task->num_peers = 0;
        ctx->c_task->params.clk_id = GENAVB_CLOCK_MONOTONIC;

        ctx->opt_io_device_task = tsn_conf_get_cyclic_task(1);
        if (!ctx->opt_io_device_task) {
            log_err("tsn_conf_get_cyclic_task() failed\n");
            goto err_init;
        }

        ctx->opt_io_device_task->num_peers = 0;
        ctx->opt_io_device_task->params.clk_id = GENAVB_CLOCK_MONOTONIC;

        cyclic_task_set_period(ctx->opt_io_device_task, config->period_ns);

        if (io_device_init(&ctx->io_device, ctx->opt_io_device_task, 1, true) < 0) {
            log_err("io_device_init() failed\n");
            goto err_init;
        }

        local_bind_controller_io_device(&ctx->ctrl, &ctx->io_device);
    } else
#endif
    {
        ctx->c_task = tsn_conf_get_cyclic_task(config->role);
        if (!ctx->c_task) {
            log_err("tsn_conf_get_cyclic_task() failed\n");
            goto err_init;
        }

        cyclic_task_set_period(ctx->c_task, config->period_ns);
    }

    if (ctx->c_task->type == CYCLIC_CONTROLLER) {
        ctx->c_task->num_peers = config->num_io_devices;
        ctx->a_task->num_peers = config->num_io_devices;
    }

    ctx->c_task->params.stream_priority = config->priority;
    ctx->c_task->params.port_id = config->port_id;
    ctx->c_task->params.zero_copy = config->zero_copy;
    ctx->a_task->params.zero_copy = config->zero_copy;

    if (config->rx_tc_mask > 0xFF)
        config->rx_tc_mask &= 0xFF;

    ctx->c_task->params.rx_tc_mask = config->rx_tc_mask;

    if (config->packets < 1)
        config->packets = 1;

    if (config->packets > NET_RX_BATCH)
        config->packets = NET_RX_BATCH;

    ctx->c_task->params.num_packets = config->packets;

    cyclic_task_set_tx_time(ctx->c_task, config->tx_time_offset_ns, config->tx_time_enabled);

#if (BUILD_MOTOR_CONTROLLER == 1) || (BUILD_MOTOR_IO_DEVICE == 1)
    if (config->mode == MOTOR_NETWORK || config->mode == MOTOR_LOCAL) {
#if BUILD_MOTOR_CONTROLLER == 1
        if (ctx->c_task->type == CYCLIC_CONTROLLER) {
            if (controller_init(&ctx->ctrl, ctx->c_task, config->mode == MOTOR_LOCAL,
                            (control_strategies_t)config->control_strategy, (bool)config->cmd_client) < 0) {
                log_err("controller_init() failed\n");
                goto err_init;
            }
        }
#endif
#if BUILD_MOTOR_IO_DEVICE == 1
        if (ctx->c_task->type == CYCLIC_IO_DEVICE) {
            if (io_device_init(&ctx->io_device, ctx->c_task, 1, false) < 0) {
                log_err("io_device_init() failed\n");
                goto err_init;
            }

            io_device_set_motor_offset(&ctx->io_device, 0, config->motor_offset);
        }
#endif
        if (ctx->c_task->type != CYCLIC_CONTROLLER && ctx->c_task->type != CYCLIC_IO_DEVICE) {
            log_err("Unknown cyclic task type\n");
            goto err_init;
        }
    } else
#endif
    {
        if (config->mode == SERIAL) {
            ctx->c_task->params.task_period_ns = APP_PERIOD_SERIAL_DEFAULT;
            ctx->c_task->params.task_period_offset_ns = NET_DELAY_OFFSET_SERIAL_DEFAULT;
            ctx->c_task->params.transfer_time_ns = NET_DELAY_OFFSET_SERIAL_DEFAULT;

            if (serial_iodevice_init(ctx->c_task) < 0) {
                log_err("serial_iodevice_init() failed\n");
                goto err_init;
            }
        } else {
            if (cyclic_task_init(ctx->c_task, NULL, null_loop, ctx->c_task) < 0) {
                log_err("cyclic_task_init() failed\n");
                goto err_init;
            }
        }
    }

    cyclic_task_start(ctx->c_task);

    if (ctx->opt_io_device_task)
        cyclic_task_start(ctx->opt_io_device_task);

    if (ctx->a_task->type == ALARM_MONITOR)
        alarm_task_monitor_init(ctx->a_task, NULL, NULL);
    else if (ctx->a_task->type == ALARM_IO_DEVICE) {
        alarm_task_io_init(ctx->a_task);

        while (true) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            alarm_net_transmit(ctx->a_task, 0, NULL, 0);
        }
    }

    return 0;

err_init:
    rtos_free(ctx);

err_malloc:
    return -1;
}
