# Description: Component providing helper functions for statistics
include_guard(GLOBAL)
message("rtos_apps: stats component is included.")

target_sources(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/stats/stats.c
)

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
