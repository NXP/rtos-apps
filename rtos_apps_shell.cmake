# Description: Component providing shell components
include_guard(GLOBAL)

message("rtos-apps shell component is included for target ${RTOS_APPS_TARGET}")

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/shell/common.c
    ${CMAKE_CURRENT_LIST_DIR}/shell/fp.c
    ${CMAKE_CURRENT_LIST_DIR}/shell/port_stats.c
    ${CMAKE_CURRENT_LIST_DIR}/shell/qbv.c
    ${CMAKE_CURRENT_LIST_DIR}/shell/shell.c
)

if(CONFIG_APP_FP)
    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
        CONFIG_RTOS_APPS_FP=1
    )
endif()

if(CONFIG_APP_QBV)
    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
        CONFIG_RTOS_APPS_QBV=1
    )
endif()

if(CONFIG_APP_TSN_BRIDGE)
    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/shell/fdb.c
        ${CMAKE_CURRENT_LIST_DIR}/shell/frer.c
        ${CMAKE_CURRENT_LIST_DIR}/shell/hsr.c
        ${CMAKE_CURRENT_LIST_DIR}/shell/psfp.c
        ${CMAKE_CURRENT_LIST_DIR}/shell/stream_identification.c
        ${CMAKE_CURRENT_LIST_DIR}/shell/vlan.c
    )
endif()
