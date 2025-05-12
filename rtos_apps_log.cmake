# Description: Component providing helper functions for logging
include_guard(GLOBAL)
message("rtos_apps: log component is included.")

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/log/log.c
)

if(CONFIG_RTOS_APPS_LOG_TIMESTAMP)
    target_compile_definitions(${MCUX_SDK_PROJECT_NAME} PRIVATE CONFIG_RTOS_APPS_LOG_TIMESTAMP)
endif()

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
