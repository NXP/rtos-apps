# Description: Component providing shell component
if(CONFIG_MCUX_COMPONENT_component.rtos_apps.shell)
include_guard(GLOBAL)
message("rtos_apps: shell component is included.")

mcux_add_source(
    SOURCES
    ./shell/common.c
    ./shell/fdb.c
    ./shell/fp.c
    ./shell/frer.c
    ./shell/hsr.c
    ./shell/port_stats.c
    ./shell/psfp.c
    ./shell/qbv.c
    ./shell/stream_identification.c
    ./shell/vlan.c
)

mcux_add_include(
    INCLUDES
    .
)

endif()

