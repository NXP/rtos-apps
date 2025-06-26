/*
 * Copyright 2019-2021, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MOTOR_CONTROL_H_
#define _MOTOR_CONTROL_H_

#include <stdint.h>

#define NB_IO_DEVICE_MAX  2
#define NB_MOTORS_MAX 1

enum controller_action {
    HOLD = 0,
    APPLY_IQ = 1,
};

struct motor_feedback {
    uint16_t motor_id;
    float pos;
    float speed;
    float iq_meas;
    float id_meas;
    float uq_applied;
    float dc_bus;
    float cur_a;
    float cur_b;
    float cur_c;
};

struct msg_feedback {
    uint16_t status;
    uint16_t num_msg;
    struct motor_feedback msg_array[NB_MOTORS_MAX];
};

struct motor_set_iq {
    uint16_t io_device_id;
    uint16_t motor_id;
    float iq_req;
};

struct msg_set_iq {
    uint16_t action;
    uint16_t num_msg;
    struct motor_set_iq msg_array[NB_MOTORS_MAX * NB_IO_DEVICE_MAX];
};

#endif /* _MOTOR_CONTROL_H_ */
