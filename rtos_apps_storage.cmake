# Description: Component providing storage components
include_guard(GLOBAL)

message("rtos-apps storage component is included for target ${RTOS_APPS_TARGET}")

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

if(CONFIG_APP_STORAGE)
    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
        CONFIG_RTOS_APPS_STORAGE=1
    )

    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/storage/storage_common.c
    )
endif()
