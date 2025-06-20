/*
 * Copyright 2019, 2021, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>
#include <string.h>

#include "rtos_apps/log.h"

#include "io_device.h"
#include "cyclic_task.h"
#include "local_network.h"
#include "stats_task.h"
#include "types.h"
#include "user_button.h"

#define IO_DEVICE_STAT_PERIOD_SEC 2
#define POSITION_ACCURACY         0.02
#define STAY_INDEX_DELAY_MS       2000
#define MONITORING_STAT_PERIOD_MS 1000

char *state_io_device_names[] = {"WAIT_FOR_INPUT", "INIT", "STAY_INDEX", "REMOTE_CONTROL", "FREERUNNING", "MOTOR_FAULT"};
char *state_motor_control_names[] = {"FAULT", "INIT", "STOP", "RUN"};

static void io_device_stats_print(void *data)
{
    struct stats_io_device *stats_snap = data;
    struct io_device_ctx *ctx = container_of(data, struct io_device_ctx, stats_snap);

    log_info("id    : %u\n", ctx->c_task->id);
    log_info("state : %s\n", state_io_device_names[stats_snap->state]);
    log_info("errors nb cmds: %u, msg id: %u, invalid len: %u \n",
        stats_snap->err_cmd_recv, stats_snap->err_msg_id,
        stats_snap->err_invalid_len_received);

    stats_snap->pending = false;
}

static void io_device_stats_dump(struct io_device_ctx *ctx)
{
    if (ctx->stats_snap.pending)
        return;

    ctx->stats.state = ctx->state;

    memcpy(&ctx->stats_snap, &ctx->stats, sizeof(struct stats_io_device));
    ctx->stats_snap.pending = true;

    if (STATS_Async(io_device_stats_print, &ctx->stats_snap) != pdTRUE)
        ctx->stats_snap.pending = false;
}

void __io_device_monitoring_send(void *data)
{
    struct io_device_ctx *ctx = data;

    monitoring_stats_send(ctx->monitoring_stats_ctx, &ctx->msg);
    ctx->msg_pending = false;
}

void io_device_monitoring_send(struct io_device_ctx *ctx)
{
    if (ctx->msg_pending)
        return;

    ctx->msg.id = ctx->c_task->id;
    ctx->msg.state = ctx->state;
    ctx->msg.msg_type = MSG_IO_DEVICE;

    cyclic_task_get_monitoring(ctx->c_task, &ctx->msg.cyclic_task_stats, MONITOR_MAX_SOCKET);
    ctx->msg_pending = true;

    if (STATS_Async(__io_device_monitoring_send, ctx) != pdTRUE)
        ctx->msg_pending = false;
}

static void motor_stats_print(void *data)
{
    struct stats_motor *stats_snap = data;
    struct motor_controlled *ctx = container_of(data, struct motor_controlled, stats_snap);

    log_info("motor ctx(%x) %u\n", ctx, ctx->motor_id);
    log_info("  state      : %s\n", state_motor_control_names[stats_snap->state]);
    log_info("  iq req     : %f\n", stats_snap->iq_req);
    log_info("  pos real   : %f\n", stats_snap->pos_real);
    log_info("  speed real : %f\n", stats_snap->speed_real);
    log_info("  last index : %lu\n", stats_snap->last_index);
    log_info("  missed slow loops : %lu\n", stats_snap->missed_slow_loop);
    log_info("  counter rev jumps : %lu\n", stats_snap->counter_rev_jumps);
    log_info("  fast loops executed: %lu\n", stats_snap->fast_loop_executed);

    stats_snap->pending = false;
}

static void motor_stats_dump(struct motor_controlled *ctx)
{
    struct motor_feedback stats_feedback;

    if (ctx->stats_snap.pending)
        return;

    // Retrieve data
    mcapi_get_motor_feedback(ctx->motor, &stats_feedback);

    ctx->stats.state = (uint8_t)mcapi_get_motor_state(ctx->motor);
    ctx->stats.iq_req = ctx->iq_req;
    ctx->stats.pos_real = stats_feedback.pos + ctx->motor_offset;
    ctx->stats.speed_real = stats_feedback.speed;
    ctx->stats.last_index = mcapi_get_last_index(ctx->motor);
    ctx->stats.missed_slow_loop = mcapi_get_missed_slow_loop(ctx->motor);
    ctx->stats.counter_rev_jumps = mcapi_get_revolution_jumps(ctx->motor);
    ctx->stats.fast_loop_executed = mcapi_get_fast_loop_executed(ctx->motor);

    memcpy(&ctx->stats_snap, &ctx->stats, sizeof(struct stats_motor));
    ctx->stats_snap.pending = true;

    if (STATS_Async(motor_stats_print, &ctx->stats_snap) != pdTRUE)
        ctx->stats_snap.pending = false;
}

static void io_device_send(struct io_device_ctx *ctx)
{
    unsigned int i = 0;
    struct msg_feedback msg_to_send;

    msg_to_send.num_msg = ctx->num_motors;
    msg_to_send.status = ctx->status;

    for (i = 0; i < ctx->num_motors; i++) {
        mcapi_get_motor_feedback(ctx->motors_controlled[i].motor, &msg_to_send.msg_array[i]);

        // Correct position offset on feedback
        msg_to_send.msg_array[i].pos += ctx->motors_controlled[i].motor_offset;

        msg_to_send.msg_array[i].motor_id = ctx->motors_controlled[i].motor_id;
    }

#if BUILD_MOTOR_CONTROLLER == 1
    if (ctx->controller_local) {
        local_io_device_transmit(ctx, &msg_to_send);
    } else
#endif
    {
        cyclic_net_transmit(ctx->c_task, MSG_FEEDBACK, &msg_to_send, sizeof(msg_to_send));
    }
}

static void io_device_set_state(struct io_device_ctx *ctx, sm_io_device_state_t new_state)
{
    if (new_state != ctx->state) {
        log_info("state change from %s to %s\n",
            state_io_device_names[ctx->state],
            state_io_device_names[new_state]);
        ctx->state = new_state;
    }
}

static void io_device_state_init(struct io_device_ctx *ctx)
{
    unsigned int i;
    uint16_t nb_motors_initialized = 0;

    for (i = 0; i < ctx->num_motors; i++) {
        if (mcapi_motor_initialized(ctx->motors_controlled[i].motor))
            nb_motors_initialized++;

        // Execute Motor Control State Machine
        mcapi_slowloop(ctx->motors_controlled[i].motor);
    }

    if (nb_motors_initialized == ctx->num_motors) {
        io_device_set_state(ctx, STAY_INDEX);
        ctx->stay_index_delay = STAY_INDEX_DELAY_MS *
                                (NSECS_PER_MSEC / ctx->c_task->task->params->task_period_ns);
        ctx->offset_reached = false;
        log_info("Local motor initialization over\n");
        log_info("Delay to pass in STAY_INDEX : %u\n", ctx->stay_index_delay);
    }
}

static void io_device_stay_index(struct io_device_ctx *ctx)
{
    unsigned int i;
    struct motor_feedback feedback;
    unsigned int nb_motors_at_index = 0;

    for (i = 0; i < ctx->num_motors; i++) {
        mcapi_set_position(ctx->motors_controlled[i].motor, 0.0 - ctx->motors_controlled[i].motor_offset);

        // Execute Motor Control State Machine
        mcapi_slowloop(ctx->motors_controlled[i].motor);

        // Retrieve data
        mcapi_get_motor_feedback(ctx->motors_controlled[i].motor, &feedback);

        // Check if the motor has reached the offset
        if (fabs(feedback.pos + ctx->motors_controlled[i].motor_offset) < POSITION_ACCURACY)
            nb_motors_at_index++;
    }

    if (nb_motors_at_index == ctx->num_motors && (!ctx->stay_index_delay--)) {
        ctx->offset_reached = true;
        log_info("All motors holding offset and delay elapsed\n");
    }
}

static void io_device_state_freerunning(struct io_device_ctx *ctx)
{
    unsigned int i;

    // Stop the motor
    for (i = 0; i < ctx->num_motors; i++) {
        mcapi_set_closed_current_loop(ctx->motors_controlled[i].motor, false);
        ctx->motors_controlled[i].iq_req = 0.0;
        mcapi_set_iq_req(ctx->motors_controlled[i].motor, ctx->motors_controlled[i].iq_req);
    }
}

static void io_device_state_remote_control(struct io_device_ctx *ctx)
{
    unsigned int i;

    for (i = 0; i < ctx->num_motors; i++) {
        // Send Iq through motor control API
        mcapi_set_iq_req(ctx->motors_controlled[i].motor, ctx->motors_controlled[i].iq_req);

        // Execute Motor Control State Machine
        mcapi_slowloop(ctx->motors_controlled[i].motor);
    }
}

static void run_io_device_state(struct io_device_ctx *ctx)
{
    switch (ctx->state) {
    case WAIT_FOR_INPUT:
        break;
    case INIT:
        io_device_state_init(ctx);
        break;
    case STAY_INDEX:
        io_device_stay_index(ctx);
        break;
    case FREERUNNING:
        io_device_state_freerunning(ctx);
        break;
    case REMOTE_CONTROL:
        io_device_state_remote_control(ctx);
        break;
    case MOTOR_FAULT:
        break;
    default:
        log_err("Unknown io_device state\n");
        break;
    }
}

static void io_device_loop(void *data, int timer_status)
{
    unsigned int i;
    struct io_device_ctx *ctx = data;
    unsigned int num_sched_stats = IO_DEVICE_STAT_PERIOD_SEC *
                                   (NSECS_PER_SEC / ctx->c_task->task->params->task_period_ns);
    unsigned int num_sched_monitoring = MONITORING_STAT_PERIOD_MS *
                                        (NSECS_PER_MSEC / ctx->c_task->task->params->task_period_ns);
    enum event_motor evt;
    bool current_loop = true;

    if (!rtos_mqueue_receive(ctx->event_queue, &evt, RTOS_NO_WAIT)) {
        if (evt == BUTTON_PRESSED) {
            log_info("BUTTON_PRESSED event received\n");
            // Start initializing motors
            if (ctx->state == WAIT_FOR_INPUT)
                io_device_set_state(ctx, INIT);
        }
    }

    // Check if motor state machine is not in FAULT state, if so go in error mode
    for (i = 0; i < ctx->num_motors; i++)
        if (mcapi_get_motor_state(ctx->motors_controlled[i].motor) == 0) {
            io_device_set_state(ctx, MOTOR_FAULT);
            io_device_status_set_error_fault(&ctx->status);
        }

    // If all Iqs are received there is no bit set in the status,
    // we switch to REMOTE_CONTROL state
    if (!ctx->status && ctx->control_action == APPLY_IQ) {
        if (ctx->state == FREERUNNING) {
            io_device_set_state(ctx, REMOTE_CONTROL);
        } else if (ctx->state == STAY_INDEX && ctx->offset_reached) {
            io_device_set_state(ctx, REMOTE_CONTROL);
        }
    } else {
        // If one of Iqs is not received while we are in REMOTE_CONTROL state,
        // we switch to FREERUNNING state
        if (ctx->state == REMOTE_CONTROL)
            io_device_set_state(ctx, FREERUNNING);
    }

    // If we receive a timer problem we disable current loop
    if (timer_status < 0)
        current_loop = false;

    // Update current loop enablement
    for (i = 0; i < ctx->num_motors; i++)
        mcapi_set_closed_current_loop(ctx->motors_controlled[i].motor, current_loop);

    run_io_device_state(ctx);

    if (ctx->state >= STAY_INDEX && ctx->offset_reached)
        io_device_send(ctx);

    // Monitoring stats
    if (ctx->c_task->task->stats.sched % num_sched_monitoring == 0) {
        io_device_monitoring_send(ctx);
    }

    // Print stats
    if (ctx->c_task->task->stats.sched % num_sched_stats == 0) {
        io_device_stats_dump(ctx);
        for (i = 0; i < ctx->num_motors; i++)
            motor_stats_dump(&ctx->motors_controlled[i]);
    }

    /* Set error here in case no frame is received */
    io_device_status_set_error_network(&ctx->status);
}

void io_device_net_receive(void *data, int msg_id, int src_id, void *buf, int len)
{
    unsigned int i;
    struct io_device_ctx *ctx = data;
    struct msg_set_iq *msg_recv = buf;
    unsigned int num_matches = 0;
    uint16_t motor_id;

    if (msg_id == MSG_SET_IQ) {
        if (len != sizeof(struct msg_set_iq)) {
            ctx->stats_snap.err_invalid_len_received++;
            goto err;
        } else {
            ctx->control_action = msg_recv->action;

            // Parse message to find io_device corresponding to local io_device
            for (i = 0; i < msg_recv->num_msg; i++) {
                if (msg_recv->msg_array[i].io_device_id == ctx->c_task->id) {

                    motor_id = msg_recv->msg_array[i].motor_id;
                    // Parse Iqs to find the one corresponding to the right motor
                    // motor_id directly corresponds to the index in motor_controlled array
                    if (motor_id < ctx->num_motors) {
                        // Store Iq received

                        ctx->motors_controlled[motor_id].iq_req = msg_recv->msg_array[i].iq_req;

                        num_matches++;
                    }
                }
            }

            // If a wrong number of commands is received, we discard all received Iqs
            if (num_matches != ctx->num_motors) {
                ctx->stats_snap.err_cmd_recv++;
                goto err;
            }
        }
    } else {
        ctx->stats_snap.err_msg_id++;
        goto err;
    }

    io_device_status_clear_error_network(&ctx->status);
    return;

err:
    io_device_status_set_error_network(&ctx->status);
    return;
}

void io_device_set_motor_offset(struct io_device_ctx *ctx, uint16_t motor_id, float offset)
{
    ctx->motors_controlled[motor_id].motor_offset = offset;
}

int io_device_init(struct io_device_ctx *ctx, struct cyclic_task *c_task, uint16_t nb_motors, bool controller_local)
{
    unsigned int i = 0;

    ctx->num_motors = nb_motors;
    ctx->controller_local = controller_local;

    if (controller_local) {
        ctx->state = INIT;
    } else {
        ctx->state = WAIT_FOR_INPUT;
    }

    ctx->status = 0;
    ctx->offset_reached = false;

    for (i = 0; i < ctx->num_motors; i++) {
        ctx->motors_controlled[i].motor_id = i;

        if (mcapi_init(i, &ctx->motors_controlled[i].motor) < 0) {
            log_err("mcapi_init() failed\n");
            goto err;
        }

        mcapi_set_position(ctx->motors_controlled[i].motor, 0.0);
        mcapi_set_external_control(ctx->motors_controlled[i].motor, false);
    }

    /* Initialize queue that handles button events */
    ctx->event_queue = rtos_mqueue_alloc_init(1, sizeof(enum event_motor));
    if (!ctx->event_queue) {
        log_err("rtos_mqueue_alloc_init() failed\n");
        goto err;
    }

    if (user_button_add_event_queue(ctx->event_queue) < 0) {
        log_err("user_button_add_event_queue() failed\n");
        goto err_del_queue;
    }

    if (monitoring_stats_open(&ctx->monitoring_stats_ctx) < 0) {
        log_err("monitoring_stats_open() failed\n");
        goto err_del_queue;
    }

    ctx->c_task = c_task;
    if (cyclic_task_init(c_task, io_device_net_receive, io_device_loop, ctx) < 0)
        goto err_del_queue;

    log_info("IO device %u initialized successfully\n", c_task->id);

    return 0;

err_del_queue:
    rtos_mqueue_destroy(ctx->event_queue);
err:
    return -1;
}
