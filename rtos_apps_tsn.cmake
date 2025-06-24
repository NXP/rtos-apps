# Description: Component providing example tsn application
include_guard(GLOBAL)

message("rtos-apps tsn headers are included for target ${RTOS_APPS_TARGET}")

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/tsn
    ${CMAKE_CURRENT_LIST_DIR}/tsn/motor
)

message("rtos-apps tsn sources are included for target ${RTOS_APPS_TARGET}")

target_sources(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/tsn/alarm_task.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/cyclic_task.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/tsn_app.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/tsn_task.c
    ${CMAKE_CURRENT_LIST_DIR}/tsn/tsn_tasks_config.c
)

if(enable_lwip)
    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/tsn/monitoring_stats.c
    )
endif()

if(build_serial)
    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/tsn/serial_iodevice.c
    )
endif()

if(build_motor_controller)
    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/controller.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/control_strategies.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/current_control.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/motor_params.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/scenarios.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/traj_planner.c
    )

    if(enable_lwip)
        target_sources(${RTOS_APPS_TARGET} PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/command_client.c
        )
    endif()
endif()

if(build_motor_io_device)
    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/io_device.c
        ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/motor_control_api.c
    )

    if(enable_lwip)
        target_sources(${RTOS_APPS_TARGET} PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/tsn/motor/network_stats.c
        )
    endif()
endif()