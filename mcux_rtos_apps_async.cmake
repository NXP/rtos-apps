# Description: Component providing helper functions for async processing
if(CONFIG_MCUX_COMPONENT_component.rtos_apps.async)
    include_guard(GLOBAL)
    message("rtos_apps: async component is included.")

    mcux_add_source(
        SOURCES
        async/async.c
        rtos_apps/async.h
    )

    mcux_add_include(
        INCLUDES
        .
    )
endif()
