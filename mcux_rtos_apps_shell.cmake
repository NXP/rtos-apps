# Description: Component providing shell component
if(CONFIG_MCUX_COMPONENT_component.rtos_apps.shell)
include_guard(GLOBAL)
message("rtos_apps: shell component is included.")

mcux_add_source(
    SOURCES
    ./shell/common.c
    ./shell/fp.c
    ./shell/port_stats.c
    ./shell/qbv.c
    ./shell/shell.c
)

mcux_add_include(
    INCLUDES
    .
)

if(CONFIG_MCUX_COMPONENT_component.rtos_apps.shell.tsn_bridge)
    mcux_add_source(
        SOURCES
        ./shell/fdb.c
        ./shell/frer.c
        ./shell/hsr.c
        ./shell/psfp.c
        ./shell/stream_identification.c
        ./shell/vlan.c
    )
endif()

endif()

