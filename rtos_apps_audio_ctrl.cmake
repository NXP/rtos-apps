# Description: Component providing audio control headers
include_guard(GLOBAL)

message("rtos-apps audio control headers are included for target ${RTOS_APPS_TARGET}")

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
