/*
 * Copyright 2018-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "genavb/timer.h"

#include "rtos_apps/log.h"
#include "rtos_apps/tsn/tsn_entry.h"

#include "alarm_task.h"
#include "cyclic_task.h"
#include "serial_iodevice.h"
#include "tsn_tasks_config.h"

#ifdef CONFIG_RTOS_APPS_MOTOR_CONTROLLER
#include "motor/controller.h"
#endif

#ifdef CONFIG_RTOS_APPS_MOTOR_IO_DEVICE
#include "motor/io_device.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

struct tsn_app_ctx {
#ifdef CONFIG_RTOS_APPS_MOTOR_CONTROLLER
    struct controller_ctx ctrl;
#endif
#ifdef CONFIG_RTOS_APPS_MOTOR_IO_DEVICE
    struct io_device_ctx io_device;
#endif

    struct cyclic_task c_task;
    struct alarm_task a_task;
    struct serial_iodevice_ctx *s_task;
    unsigned int app_mode;
};

static const char *app_mode_names[] = {"MOTOR_NETWORK", "Not Supported", "NETWORK_ONLY", "SERIAL"};

/*******************************************************************************
 * Code
 ******************************************************************************/

static void main_alarm_io(void *data)
{
    struct alarm_task *a_task = data;

    while (true) {
        rtos_sleep(RTOS_MS_TO_TICKS(10000));
        alarm_net_transmit(a_task, 0, NULL, 0);
    }
}

static void null_loop(void *data, int timer_status)
{
    struct cyclic_task *c_task = data;

    cyclic_net_transmit(c_task, 0, NULL, 0);
}

int rtos_apps_tsn_init(struct rtos_apps_tsn_config *config, struct tsn_app_ctx **tsn_ctx)
{
    struct tsn_app_ctx *ctx;
    struct alarm_task_config *a_cfg;
    struct cyclic_task_config *c_cfg;
    unsigned int processing_budget_ns;

    ctx = rtos_malloc(sizeof(struct tsn_app_ctx));
    if (!ctx) {
        log_err("rtos_malloc() failed\n");
        goto err_malloc;
    }

    memset(ctx, 0, sizeof(struct tsn_app_ctx));

    a_cfg = tsn_conf_get_alarm_task(config->role);
    if (!a_cfg) {
        log_err("tsn_conf_get_alarm_task() failed\n");
        goto err_init;
    }

    log_info("tsn_app config\n");
    log_info("mode             : %s\n", app_mode_names[config->mode]);
    log_info("role             : %u\n", config->role);
    log_info("num_io_devices   : %u\n", config->num_io_devices);
    log_info("motor_offset     : %f\n", (double)config->motor_offset);
    log_info("control_strategy : %u\n", config->control_strategy);
    log_info("app period       : %u\n", config->period_ns);
    log_info("app offset       : %u\n", config->offset);
    log_info("network budget   : %u\n", config->network_budget_ns);
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

    if (config->offset > APP_OFFSET_MAX) {
        log_err("invalid application offset, maximum is %u ns\n", APP_OFFSET_MAX);
        goto err_init;
    }

    processing_budget_ns = (config->offset + 1) * config->period_ns / 2;

    if (config->network_budget_ns > processing_budget_ns / 3) {
        log_err("invalid network budget, maximum is %u ns\n", processing_budget_ns / 3);
        goto err_init;
    }

#if (!defined(CONFIG_RTOS_APPS_MOTOR_CONTROLLER) && !defined(CONFIG_RTOS_APPS_MOTOR_IO_DEVICE))
    log_info("CONFIG_RTOS_APPS_MOTOR disabled, MOTOR_NETWORK mode cannot be used\n");
#endif

#if (defined(CONFIG_RTOS_APPS_MOTOR_CONTROLLER) || defined(CONFIG_RTOS_APPS_MOTOR_IO_DEVICE))
    if (config->mode == MOTOR_NETWORK) {
        if ((config->period_ns != 100000) && (config->period_ns != 250000)) {
            log_err("invalid application period, only 100000 us and 250000 us are supported\n");
            goto err_init;
        }
    }
#endif

    c_cfg = tsn_conf_get_cyclic_task(config->role);
    if (!c_cfg) {
        log_err("tsn_conf_get_cyclic_task() failed\n");
        goto err_init;
    }

    c_cfg->log_update_time = config->log_update_time;

    cyclic_task_set_period(c_cfg, config);

    if (c_cfg->type == CYCLIC_CONTROLLER) {
        c_cfg->num_peers = config->num_io_devices;
    }

    c_cfg->params.stream_priority = config->priority;
    c_cfg->params.port_id = config->port_id;
    c_cfg->params.zero_copy = config->zero_copy;

    a_cfg->params.port_id = config->port_id;
    a_cfg->params.zero_copy = config->zero_copy;

    if (config->rx_tc_mask > 0xFF)
        config->rx_tc_mask &= 0xFF;

    c_cfg->params.rx_tc_mask = config->rx_tc_mask;

    if (config->packets < 1)
        config->packets = 1;

    if (config->packets > NET_RX_BATCH)
        config->packets = NET_RX_BATCH;

    c_cfg->params.num_packets = config->packets;
    c_cfg->params.async = config->async;

    cyclic_task_set_tx_time(c_cfg, config->tx_time_offset_ns, config->tx_time_enabled);

#if (defined(CONFIG_RTOS_APPS_MOTOR_CONTROLLER) || defined(CONFIG_RTOS_APPS_MOTOR_IO_DEVICE))
    if (config->mode == MOTOR_NETWORK) {
#ifdef CONFIG_RTOS_APPS_MOTOR_CONTROLLER
        if (c_cfg->type == CYCLIC_CONTROLLER) {
            struct controller_config controller_cfg = {
                .cyclic_cfg = c_cfg,
                .first_strategy = (control_strategies_t)config->control_strategy,
                .cmd_client = (bool)config->cmd_client,
                .user_button = config->user_button,
                .app_motor_params_init = config->app_motor_params_init,
            };

            if (controller_init(&ctx->ctrl, &ctx->c_task, &controller_cfg) < 0) {
                log_err("controller_init() failed\n");
                goto err_init;
            }
        } else
#endif
#ifdef CONFIG_RTOS_APPS_MOTOR_IO_DEVICE
        if (c_cfg->type == CYCLIC_IO_DEVICE) {
             struct io_device_config io_device_cfg = {
                .cyclic_cfg = c_cfg,
                .nb_motors = 1,
                .user_button = config->user_button,
            };

            if (io_device_init(&ctx->io_device, &ctx->c_task, &io_device_cfg) < 0) {
                log_err("io_device_init() failed\n");
                goto err_init;
            }

            io_device_set_motor_offset(&ctx->io_device, 0, config->motor_offset);
        } else
#endif
        {
            log_err("role(%u) not supported for mode(%u)\n", config->role, config->mode);

            goto err_init;
        }

    } else
#endif
    {
        if (config->mode == SERIAL) {
            config->serial_cfg->cyclic_cfg = c_cfg;

            if (serial_iodevice_init(&ctx->s_task, &ctx->c_task, config->serial_cfg) < 0) {
                log_err("serial_iodevice_init() failed\n");
                goto err_init;
            }
        } else if (config->mode == NETWORK_ONLY) {
            if (cyclic_task_init(&ctx->c_task, c_cfg, NULL, &null_loop, &ctx->c_task) < 0) {
                log_err("cyclic_task_init() failed\n");
                goto err_init;
            }
        } else {
            log_err("mode(%u) not supported\n", config->mode);

            goto err_init;
        }
    }

    cyclic_task_start(&ctx->c_task);

    if (a_cfg->type == ALARM_MONITOR)
        alarm_task_monitor_init(&ctx->a_task, a_cfg, NULL, NULL);
    else if (a_cfg->type == ALARM_IO_DEVICE)
        alarm_task_io_init(&ctx->a_task, a_cfg, &main_alarm_io, &ctx->a_task);

    ctx->app_mode = config->mode;

    *tsn_ctx = ctx;

    return 0;

err_init:
    rtos_free(ctx);

err_malloc:
    return -1;
}

void rtos_apps_tsn_exit(struct tsn_app_ctx *tsn_ctx)
{
    alarm_task_exit(&tsn_ctx->a_task);

    cyclic_task_stop(&tsn_ctx->c_task);
    cyclic_task_exit(&tsn_ctx->c_task);

#ifdef CONFIG_RTOS_APPS_MOTOR_IO_DEVICE
    io_device_exit(&tsn_ctx->io_device, &tsn_ctx->c_task);
#endif
#ifdef CONFIG_RTOS_APPS_MOTOR_CONTROLLER
    if (tsn_ctx->app_mode == MOTOR_NETWORK) {
        controller_exit(&tsn_ctx->ctrl);
    }
    else
#endif
    if (tsn_ctx->app_mode == SERIAL)
    {
        serial_iodevice_exit(tsn_ctx->s_task);
    }
}
