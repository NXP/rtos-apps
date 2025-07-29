# Description: Component providing example tsn application
include_guard(GLOBAL)

message("rtos-apps tsn headers are included for target ${RTOS_APPS_TARGET}")

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

message("rtos-apps tsn sources are included for target ${RTOS_APPS_TARGET}")

target_sources(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/tsn/alarm_task.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/cyclic_task.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/tsn_app.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/tsn_task.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/tsn_tasks_config.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/user_button.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/monitoring_stats.c
)

if(CONFIG_APP_USER_BUTTON)
    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
        CONFIG_RTOS_APPS_USER_BUTTON=1
    )
endif()

if(CONFIG_APP_LWIP)
    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
        CONFIG_RTOS_APPS_LWIP=1
    )
endif()

if(CONFIG_APP_SERIAL)
    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
        CONFIG_RTOS_APPS_SERIAL=1
    )

    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/tsn/serial_iodevice.c
    )
endif()

if(CONFIG_APP_MOTOR_CONTROLLER)
    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
        CONFIG_RTOS_APPS_MOTOR_CONTROLLER=1
    )

    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/controller.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/control_strategies.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/current_control.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/motor_params.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/scenarios.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/traj_planner.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/command_client.c
    )
endif()

if(CONFIG_APP_MOTOR_IO_DEVICE)
    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
        CONFIG_RTOS_APPS_MOTOR_IO_DEVICE=1
    )

    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/io_device.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/motor_control_api.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/network_stats.c
    )
endif()
