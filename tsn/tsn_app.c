/*
 * Copyright 2018-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "stats_task.h"
#include "fp.h"
#include "lwip.h"
#include "lwip_iperf.h"
#include "log.h"
#include "tsn_app/shell.h"
#include "storage.h"
#include "qbv.h"

#include "cyclic_task.h"
#include "alarm_task.h"
#include "tsn_tasks_config.h"
#include "system_config.h"

#include "tsn_app.h"

#if BUILD_MOTOR_CONTROLLER == 1
#include "controller.h"
#endif

#if BUILD_MOTOR_IO_DEVICE == 1
#include "io_device.h"
#include "local_network.h"
#endif

#include "user_button.h"
#include "serial_iodevice.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MAIN_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 512)
#define MAIN_TASK_PRIORITY   1

#define STATS_PERIOD_MS 2000

#if BUILD_MOTOR_CONTROLLER == 1
static struct controller_ctx ctrl1;
static struct controller_ctx *ctrl_h = NULL;
#endif

#if BUILD_MOTOR_IO_DEVICE == 1
static struct io_device_ctx io_device1;
static struct io_device_ctx *io_device_h = NULL;
#endif

static struct cyclic_task *opt_io_device_task;
static char *task_id_names[] = {"CONTROLLER_0", "IO_DEVICE_0", "IO_DEVICE_1", "MAX_TASK_ID"};
static char *app_mode_names[] = {"MOTOR_NETWORK", "MOTOR_LOCAL", "NETWORK_ONLY", "SERIAL"};
static struct gavb_pps pps;

/*******************************************************************************
 * Code
 ******************************************************************************/

static void null_loop(void *data, int timer_status)
{
    struct cyclic_task *c_task = data;

    cyclic_net_transmit(c_task, 0, NULL, 0);
}

extern struct system_config system_cfg;

static struct tsn_app_config *system_config_get_tsn_app(void)
{
    struct tsn_app_config *config = &system_cfg.app.tsn_app_config;

    if (storage_cd("/tsn_app", true) == 0) {
        storage_read_uint("mode", &config->mode);
        storage_read_uint("role", &config->role);
        storage_read_uint("num_io_devices", &config->num_io_devices);
        storage_read_float("motor_offset", &config->motor_offset);
        storage_read_uint("control_strategy", &config->control_strategy);
        storage_read_uint("cmd_client", &config->cmd_client);
        storage_read_bool("zero_copy", &config->zero_copy);

        if (config->mode == SERIAL)
            config->period_ns = APP_PERIOD_SERIAL_DEFAULT;

        storage_read_uint("period_ns", &config->period_ns);
        storage_read_uint("priority", &config->priority);
        storage_read_uint("port_id", &config->port_id);
        storage_read_uint("packets", &config->packets);
        storage_read_uint("rx_tc_mask", &config->rx_tc_mask);

        storage_cd("/", true);
    }

    return config;
}

static void init(shell_handle_t shell)
{
    default_qos_init(shell);
    fp_init(shell);
    qbv_init(shell);
    lwip_stack_init();
    lwip_iperf_start(shell);
}

static void main_task(void *data)
{
    struct cyclic_task *c_task = NULL;
    struct alarm_task *a_task;
    struct tsn_app_config *config;
    int conf_task_id;
    shell_handle_t shell;

    if (STATS_TaskInit(NULL, NULL, STATS_PERIOD_MS) < 0)
        log_err("STATS_TaskInit() failed\n");

    storage_init();

    config = system_config_get_tsn_app();
    if (!config) {
        log_err("system_config_get_tsn_app() failed\n");
        goto exit;
    }

    conf_task_id = config->role;

    shell = tsn_init_shell(task_id_names[conf_task_id]);
    if (shell == NULL) {
        log_err("tsn_init_shell() failed\n");
        goto exit;
    }

    if (gavb_stack_init()) {
        log_err("gavb_stack_init() failed\n");
        goto exit;
    }

    shell_start(init);

    a_task = tsn_conf_get_alarm_task(config->role);
    if (!a_task) {
        log_err("tsn_conf_get_alarm_task() failed\n");
        goto exit;
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
        goto exit;
    }

    if (init_gpio_handling_task() < 0) {
        log_err("init_gpio_handling_task() failed\n");
        goto exit;
    }

#if ((BUILD_MOTOR_CONTROLLER == 0) && (BUILD_MOTOR_IO_DEVICE == 0))
    log_info("BUILD_MOTOR disabled, MOTOR_NETWORK and MOTOR_LOCAL modes cannot be used\n");
#endif

#if ((BUILD_MOTOR_CONTROLLER == 1) || (BUILD_MOTOR_IO_DEVICE == 1))
    if (config->mode == MOTOR_NETWORK) {
        if ((config->period_ns != 100000) && (config->period_ns != 250000)) {
            log_err("invalid application period, only 100000 us and 250000 us are supported\n");
            goto exit;
        }
    }
#endif

#if ((BUILD_MOTOR_CONTROLLER == 1) && (BUILD_MOTOR_IO_DEVICE == 1))
    if (config->mode == MOTOR_LOCAL) {
        if ((config->period_ns != 100000) && (config->period_ns != 250000)) {
            log_err("invalid application period, only 100000 us and 250000 us are supported\n");
            goto exit;
        }

        c_task = tsn_conf_get_cyclic_task(0);
        if (!c_task) {
            log_err("tsn_conf_get_cyclic_task() failed\n");
            goto exit;
        }

        cyclic_task_set_period(c_task, config->period_ns);

        c_task->num_peers = 0;
        c_task->params.clk_id = GENAVB_CLOCK_MONOTONIC;

        opt_io_device_task = tsn_conf_get_cyclic_task(1);
        if (!opt_io_device_task) {
            log_err("tsn_conf_get_cyclic_task() failed\n");
            goto exit;
        }
        opt_io_device_task->num_peers = 0;
        opt_io_device_task->params.clk_id = GENAVB_CLOCK_MONOTONIC;

        cyclic_task_set_period(opt_io_device_task, config->period_ns);

        if (io_device_init(&io_device1, opt_io_device_task, 1, true) < 0) {
            log_err("Local io_device initialization failed\n");
            goto exit;
        }

        local_bind_controller_io_device(&ctrl1, &io_device1);
    } else
#endif
    {
        c_task = tsn_conf_get_cyclic_task(config->role);
        if (!c_task) {
            log_err("tsn_conf_get_cyclic_task() failed\n");
            goto exit;
        }

        cyclic_task_set_period(c_task, config->period_ns);
    }

    if (gavb_pps_init(&pps, c_task->params.clk_id) < 0)
        log_err("gavb_pps_init() error,pps timer could not be started\n");

    if (c_task->type == CYCLIC_CONTROLLER) {
        c_task->num_peers = config->num_io_devices;
        a_task->num_peers = config->num_io_devices;
    }

    c_task->params.stream_priority = config->priority;
    c_task->params.port_id = config->port_id;
    c_task->params.zero_copy = config->zero_copy;
    a_task->params.zero_copy = config->zero_copy;

    if (config->rx_tc_mask > 0xFF)
        config->rx_tc_mask &= 0xFF;

    c_task->params.rx_tc_mask = config->rx_tc_mask;

    if (config->packets < 1)
        config->packets = 1;

    if (config->packets > NET_RX_BATCH)
        config->packets = NET_RX_BATCH;

    c_task->params.num_packets = config->packets;

    cyclic_task_set_tx_time(c_task, config->tx_time_offset_ns, config->tx_time_enabled);

#if (BUILD_MOTOR_CONTROLLER == 1) || (BUILD_MOTOR_IO_DEVICE == 1)
    if (config->mode == MOTOR_NETWORK || config->mode == MOTOR_LOCAL) {
#if BUILD_MOTOR_CONTROLLER == 1
        if (c_task->type == CYCLIC_CONTROLLER) {
            if (controller_init(&ctrl1, c_task, config->mode == MOTOR_LOCAL,
                            (control_strategies_t)config->control_strategy, (bool)config->cmd_client) < 0) {
                log_err("Controller initialization failed\n");
                goto exit;
            }
            ctrl_h = &ctrl1;
        }
#endif
#if BUILD_MOTOR_IO_DEVICE == 1
        if (c_task->type == CYCLIC_IO_DEVICE) {
            if (io_device_init(&io_device1, c_task, 1, false) < 0) {
                log_err("io_device initialization failed\n");
                goto exit;
            }
            io_device_h = &io_device1;
            io_device_set_motor_offset(io_device_h, 0, config->motor_offset);
        }
#endif
        if (c_task->type != CYCLIC_CONTROLLER && c_task->type != CYCLIC_IO_DEVICE) {
            log_err("Unknown cyclic task type\n");
            goto exit;
        }
    } else
#endif
    {
        if (config->mode == SERIAL) {
            c_task->params.task_period_ns = APP_PERIOD_SERIAL_DEFAULT;
            c_task->params.task_period_offset_ns = NET_DELAY_OFFSET_SERIAL_DEFAULT;
            c_task->params.transfer_time_ns = NET_DELAY_OFFSET_SERIAL_DEFAULT;

            if (serial_iodevice_init(c_task) < 0) {
                log_err("serial_iodevice_init() failed\n");
                goto exit;
            }
        } else {
            if (cyclic_task_init(c_task, NULL, null_loop, c_task) < 0) {
                log_err("cyclic_task_init() failed\n");
                goto exit;
            }
        }
    }

    cyclic_task_start(c_task);

    if (opt_io_device_task)
        cyclic_task_start(opt_io_device_task);

    if (a_task->type == ALARM_MONITOR)
        alarm_task_monitor_init(a_task, NULL, NULL);
    else if (a_task->type == ALARM_IO_DEVICE) {
        alarm_task_io_init(a_task);

        while (true) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            alarm_net_transmit(a_task, 0, NULL, 0);
        }
    }

exit:
    /*
     * For now nothing more to do, delete task.
     */
    vTaskDelete(NULL);
}

/*!
 * @brief TSN Main function
 */
int tsn_app_main(void)
{
    (void)xTaskCreate(main_task, "main task", MAIN_TASK_STACK_SIZE,
                    NULL, MAIN_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    return 0;
}
