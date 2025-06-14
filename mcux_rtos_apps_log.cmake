# Description: Component providing helper functions for logging
if(CONFIG_MCUX_COMPONENT_component.rtos_apps.log)
include_guard(GLOBAL)
message("rtos_apps: log component is included.")

mcux_add_source(
    SOURCES
    log/log.c
    rtos_apps/log.h
)

mcux_add_include(
    INCLUDES
    .
)

endif()
