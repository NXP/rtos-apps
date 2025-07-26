# Description: Component providing helper functions for logging
include_guard(GLOBAL)
message("rtos_apps: log component is included.")

target_sources(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/log/log.c
)

if(CONFIG_RTOS_APPS_LOG_TIMESTAMP)
    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE CONFIG_RTOS_APPS_LOG_TIMESTAMP=1)
endif()

target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
    CONFIG_RTOS_APPS_LOG_STR=\"${CONFIG_RTOS_APPS_LOG_STR}\"
)

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
