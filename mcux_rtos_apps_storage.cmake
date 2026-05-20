# Description: Component providing helper functions for storage
if(CONFIG_MCUX_COMPONENT_component.rtos_apps.storage_common)
    include_guard(GLOBAL)
    message("rtos_apps: storage component is included.")

    mcux_add_source(
        SOURCES
        storage/storage_common.c
    )
endif()
