# Description: Component providing helper functions for async processing
include_guard(GLOBAL)
message("rtos_apps: async component is included.")

target_sources(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/async/async.c
)

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
