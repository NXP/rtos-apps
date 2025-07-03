# Description: Component providing example tsn application
if(CONFIG_MCUX_COMPONENT_component.rtos_apps.tsn)
    include_guard(GLOBAL)

    mcux_add_include(
        INCLUDES
        .
    )

    mcux_add_source(
        SOURCES
        rtos_apps/slist.h
        rtos_apps/tsn/tsn_entry.h
        rtos_apps/tsn/tsn_tasks_config.h
        rtos_apps/tsn/user_button.h
        rtos_apps/types.h
    )

    message("rtos-apps tsn sources are included for target ${RTOS_APPS_TARGET}")

    mcux_add_source(
        SOURCES
        tsn/alarm_task.c
        tsn/alarm_task.h
        tsn/cyclic_task.c
        tsn/cyclic_task.h
        tsn/monitoring_stats.h
        tsn/serial_iodevice.h
        tsn/tsn_app.c
        tsn/tsn_task.c
        tsn/tsn_task.h
        tsn/tsn_tasks_config.c
        tsn/tsn_tasks_config.h
        tsn/user_button.c
        tsn/monitoring_stats.c
        tsn/serial_iodevice.c
    )

    if(CONFIG_MCUX_COMPONENT_component.rtos_apps.tsn.motor.controller)
        mcux_add_source(
            SOURCES
            tsn/motor/controller.c
            tsn/motor/controller.h
            tsn/motor/control_strategies.c
            tsn/motor/control_strategies.h
            tsn/motor/current_control.c
            tsn/motor/current_control.h
            tsn/motor/motor_control.h
            tsn/motor/motor_params.c
            tsn/motor/motor_params.h
            tsn/motor/scenarios.c
            tsn/motor/scenarios.h
            tsn/motor/traj_planner.c
            tsn/motor/traj_planner.h
            tsn/motor/command_client.c
            tsn/motor/command_client.h
        )
    endif()

    if(CONFIG_MCUX_COMPONENT_component.rtos_apps.tsn.motor.iodevice)
        mcux_add_source(
            SOURCES
            tsn/motor/io_device.c
            tsn/motor/io_device.h
            tsn/motor/motor_control.h
            tsn/motor/motor_control_api.c
            tsn/motor/motor_control_api.h
            tsn/motor/network_stats.c
            tsn/motor/network_stats.h
        )
    endif()
endif()

if(CONFIG_MCUX_COMPONENT_component.rtos_apps.tsn_headers)
    include_guard(GLOBAL)

    mcux_add_source(
        SOURCES
        rtos_apps/tsn/tsn_entry.h
        rtos_apps/tsn/tsn_tasks_config.h
        rtos_apps/tsn/user_button.h
        rtos_apps/types.h
    )
endif()
