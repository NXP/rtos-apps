set(RtosAppsDirPath "${CMAKE_CURRENT_LIST_DIR}/..")

message("rtos-apps audio headers are included for target ${RTOS_APPS_TARGET}")

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

message("rtos-apps audio sources are included for target ${RTOS_APPS_TARGET}")

target_sources(${RTOS_APPS_TARGET} PRIVATE
)