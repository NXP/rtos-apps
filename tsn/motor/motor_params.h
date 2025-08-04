/*
 * Copyright 2019, 2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MOTOR_PARAMS_H_
#define _MOTOR_PARAMS_H_

#include "rtos_apps/tsn/tsn_entry.h"

void motor_params_init(struct rtos_apps_tsn_motor_params *params, unsigned int id,
                       void (*app_motor_params_init)(struct rtos_apps_tsn_motor_params *params, unsigned int id));

#define MP_IO_DEVICES_1170
#define MP_TEKNIC_NEW_COUPLERS

/* Parameters specific to Teknic motor but independant from boards */

// Network Controller Gains parameters
//----------------------------------------------------------------------
#define NETCTRL_SPEED_KP                   (0.02)
#define NETCTRL_SPEED_KI                   (0.000032)

#define NETCTRL_POS_KP                     (20000.0F)
//----------------------------------------------------------------------

// Feedforward parameters
//----------------------------------------------------------------------
#define MAX_ACCEL_RPM_P_S                  (12000.0) // rpm/s
#define MAX_VEL_RPM                        (4000.0)  // rpm
#define FF_GAIN                            (1.0)
//----------------------------------------------------------------------

// P-PI Controllers parameters
//----------------------------------------------------------------------
#define POS_GAIN                           (1.0)
#define SPEED_GAIN                         (1.0)
//----------------------------------------------------------------------

/* Model parameters depend on "ADC performance" for Iq measurements,
 * i.e, these parameters are SoC dependant */

#if defined(MP_IO_DEVICES_1170)

    #if defined(MP_TEKNIC_NEW_COUPLERS)

        #define MOTOR_J           (0.00015)
        #define MOTOR_b           (0.00011)
        #define MOTOR_Tm          (0.065)

    #elif defined(MP_TEKNIC_OLD_COUPLERS)

        #define MOTOR_J           (0.000092)
        #define MOTOR_b           (0.00013)
        #define MOTOR_Tm          (0.017)

    #elif defined(MP_TEKNIC_NO_LOAD)

        #define MOTOR_J           (0.000026)
        #define MOTOR_b           (0.00011)
        #define MOTOR_Tm          (0.045)

    #else

        #define MOTOR_J           (0.0)
        #define MOTOR_b           (0.0)
        #define MOTOR_Tm          (0.0)

    #endif /* Load type */

#elif defined(MP_IO_DEVICES_1050)

    #if defined(MP_TEKNIC_NEW_COUPLERS)

        #define MOTOR_J           (0.00015)
        #define MOTOR_b           (0.00011)
        #define MOTOR_Tm          (0.18)

    #elif defined(MP_TEKNIC_OLD_COUPLERS)

        #define MOTOR_J           (0.000083)
        #define MOTOR_b           (0.00011)
        #define MOTOR_Tm          (0.20)

    #elif defined(MP_TEKNIC_NO_LOAD)

        #define MOTOR_J           (0.000025)
        #define MOTOR_b           (0.000087)
        #define MOTOR_Tm          (0.25)

    #else

        #define MOTOR_J           (0.0)
        #define MOTOR_b           (0.0)
        #define MOTOR_Tm          (0.0)

    #endif /* Load type */

#else

    #define MOTOR_J           (0.0)
    #define MOTOR_b           (0.0)
    #define MOTOR_Tm          (0.0)

#endif  /* io_device type */

#endif /* _MOTOR_PARAMS_H_ */
