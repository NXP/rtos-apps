# Description: Component providing helper functions for statistics
if(CONFIG_MCUX_COMPONENT_component.rtos_apps.stats)
include_guard(GLOBAL)
message("rtos_apps: stats component is included.")

mcux_add_source(
    SOURCES
    stats/stats.c
    rtos_apps/stats.h
)

mcux_add_include(
    INCLUDES
    .
)

endif()
