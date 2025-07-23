/*
 * Copyright 2019-2020, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_abstraction_layer.h"

#include "rtos_apps/log.h"

#include "motor_control_api.h"
#include "m1_sm_snsless_enc.h"
#include "mcdrv.h"
#include "mlib_types.h"

extern mcdef_pmsm_t g_sM1Drive;

struct tsn_motor {
    mcdef_pmsm_t *motor_drive;
    sm_app_ctrl_t *sm_motor_controller;
    void (*slow_loop_func)(sm_app_ctrl_t *sm_motor_controller);
    uint16_t (*app_state_getter)(void);
    void (*app_switch_setter)(bool_t);
    bool_t (*app_switch_getter)(void);
    void (*speed_setter)(float);
    void (*position_setter)(acc32_t);
    float (*speed_getter)(void);
    float (*position_getter)(void);
    uint32_t (*last_index_getter)(void);
};

/** Initialize motor control API
 *
 * \return None
 * \param id id of motor control implementation
 * \param motor pointer to tsn_motor struct
 */
int mcapi_init(uint16_t id, struct tsn_motor **motor)
{
    switch (id) {
    case 0:
        *motor = rtos_malloc(sizeof(struct tsn_motor));
        if (!*motor)
            goto err;

        /* Init peripheral motor control driver for motor M1 */
        MCDRV_Init_M1();

        /* Turn off application */
        M1_SetAppSwitch(0);

        log_info("MC Drivers init\n");

        rtos_sleep(RTOS_MS_TO_TICKS(3000));

        (*motor)->sm_motor_controller = &g_sM1Ctrl;
        (*motor)->motor_drive = &g_sM1Drive;
        (*motor)->slow_loop_func = &SM_StateMachineSlow;
        (*motor)->app_state_getter = &M1_GetAppState;
        (*motor)->app_switch_setter = &M1_SetAppSwitch;
        (*motor)->app_switch_getter = &M1_GetAppSwitch;
        (*motor)->speed_setter = &M1_SetSpeed;
        (*motor)->speed_getter = &M1_GetRealSpeed;
        (*motor)->position_setter = &M1_SetPosition;
        (*motor)->position_getter = &M1_GetPositionFloat;
        (*motor)->last_index_getter = &M1_GetLastIndexPosition;

        break;
    default:
        goto err;
        break;
    }

    return 0;

err:
    return -1;
}

/** Get state of motor control state machines
 *
 * \return state of motor control state machines
 * \param motor pointer to tsn_motor struct
 */
uint16_t mcapi_get_motor_state(struct tsn_motor *motor)
{
    return motor->app_state_getter();
}

/** Set speed using legacy motor control functions
 *
 * \return None
 * \param motor pointer to tsn_motor struct
 * \param speed of the motor in rpm
 */
void mcapi_set_speed(struct tsn_motor *motor, float speed)
{
    motor->motor_drive->sMCATctrl.ui16PospeSensor = MCAT_ENC_CTRL;
    motor->app_switch_setter(TRUE);
    motor->motor_drive->eControl = kControlMode_SpeedFOC;

    motor->speed_setter(speed);
}

/** Set position using legacy motor control functions
 *
 * \return None
 * \param motor pointer to tsn_motor struct
 * \param position of the motor in revolutions
 */
void mcapi_set_position(struct tsn_motor *motor, float pos)
{
    motor->motor_drive->eControl = kControlMode_PositionFOC;
    motor->motor_drive->sMCATctrl.ui16PospeSensor = MCAT_ENC_CTRL;
    motor->app_switch_setter(TRUE);

    motor->position_setter((MLIB_Conv_A32f(pos)));
}

/** Get motor feedback
 *
 * \return
 * \param motor pointer to tsn_motor struct
 * \param structure that will receive feedback data
 */
int mcapi_get_motor_feedback(struct tsn_motor *motor, struct motor_feedback *feedback)
{
    feedback->pos = motor->position_getter();
    feedback->speed = motor->speed_getter();
    feedback->uq_applied = motor->motor_drive->sFocPMSM.sUDQReq.fltQ;
    feedback->dc_bus = motor->motor_drive->sFocPMSM.fltUDcBusFilt;
    feedback->iq_meas = motor->motor_drive->sFocPMSM.sIDQ.fltQ;
    feedback->id_meas = motor->motor_drive->sFocPMSM.sIDQ.fltD;
    feedback->cur_a = motor->motor_drive->sFocPMSM.sIABC.fltA;
    feedback->cur_b = motor->motor_drive->sFocPMSM.sIABC.fltB;
    feedback->cur_c = motor->motor_drive->sFocPMSM.sIABC.fltC;

    return 0;
}

/** Motor Control slow loop
 *
 * \return None
 * \param None
 */
void mcapi_slowloop(struct tsn_motor *motor)
{
    // Execute Motor Control State Machine
    motor->slow_loop_func(motor->sm_motor_controller);
}

/** Get initialization status of the motor
 *
 * \return True when initialized, false otherwise
 * \param motor pointer to tsn_motor struct
 */
bool mcapi_motor_initialized(struct tsn_motor *motor)
{
    return motor->motor_drive->bInitOver;
}

/** Send Iq computed externally to the motor
 *
 * \return           None
 * \param motor pointer to tsn_motor struct
 * \param iq iq to be executed by motor state machines
 */
void mcapi_set_iq_req(struct tsn_motor *motor, float_t iq)
{
    motor->motor_drive->bNetworkingCommand = true;

    if (iq >= M1_SPEED_LOOP_HIGH_LIMIT) {
        iq = M1_SPEED_LOOP_HIGH_LIMIT;
    } else if (iq <= M1_SPEED_LOOP_LOW_LIMIT) {
        iq = M1_SPEED_LOOP_LOW_LIMIT;
    }

    motor->motor_drive->iq_external_command = iq;
}

/** Enable/Disable closed current control loop
 *
 * \return           None
 * \param motor pointer to tsn_motor struct
 * \param state true to enable, false to disable
 */
void mcapi_set_closed_current_loop(struct tsn_motor *motor, bool state)
{
    motor->motor_drive->sFocPMSM.bEnableCurrentPI = state;
}

/** Enable/Disable external Iq control from outside of state machines
 *
 *  Allows one to implement all control logic seperately from motor control code
 *
 * \return           None
 * \param motor pointer to tsn_motor struct
 * \param state true to enable, false to disable
 */
void mcapi_set_external_control(struct tsn_motor *motor, bool state)
{
    motor->motor_drive->bNetworkingCommand = state;
}

/** Get last index position. Used for debug purpose.
 *
 * \return last index position
 * \param motor pointer to tsn_motor struct
 */
uint32_t mcapi_get_last_index(struct tsn_motor *motor)
{
    return motor->last_index_getter();
}

/** Get counter of missed slow loops. Used for debug purpose.
 *
 * \return Number of missed slow loops
 * \param motor pointer to tsn_motor struct
 */
uint32_t mcapi_get_missed_slow_loop(struct tsn_motor *motor)
{
    return motor->motor_drive->ui32CounterSlowLoopMissed;
}

/** Get number of revolution jumps
 *
 * \return Number of of revolution jumps
 * \param motor pointer to tsn_motor struct
 */
uint32_t mcapi_get_revolution_jumps(struct tsn_motor *motor)
{
	return M1_GetCounterRevJumps();
}

/** Get counter of fast loops executed. Used for debug purpose.
 *
 * \return Number of fast loops
 * \param motor pointer to tsn_motor struct
 */
uint32_t mcapi_get_fast_loop_executed(struct tsn_motor *motor)
{
    return motor->motor_drive->ui32counterFastLoopExec;
}
