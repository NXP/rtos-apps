# Description: Component providing example audio application
if(CONFIG_MCUX_COMPONENT_component.rtos_apps.audio)
include_guard(GLOBAL)

mcux_add_source(
    SOURCES
    rtos_apps/audio/*.h
)

mcux_add_source(
    SOURCES
    audio/audio.c
    audio/audio.h
    audio/audio_buffer.c
    audio/audio_buffer.h
    audio/audio_element.c
    audio/audio_element.h
    audio/audio_element_dtmf.c
    audio/audio_element_dtmf.h
    audio/audio_element_pll.c
    audio/audio_element_pll.h
    audio/audio_element_routing.c
    audio/audio_element_routing.h
    audio/audio_element_sai_sink.c
    audio/audio_element_sai_sink.h
    audio/audio_element_sai_source.c
    audio/audio_element_sai_source.h
    audio/audio_element_sine.c
    audio/audio_element_sine.h
    audio/audio_format.h
    audio/audio_pipeline.c
    audio/audio_pipeline.h
    audio/play_pipeline.c
    audio/sai_drv.c
    audio/sai_drv.h
)

if(CONFIG_MCUX_COMPONENT_component.rtos_apps.audio.genavb)
    mcux_add_source(
        SOURCES
        audio/audio_element_avtp_sink.c
        audio/audio_element_avtp_sink.h
        audio/audio_element_avtp_source.c
        audio/audio_element_avtp_source.h
        audio/audio_avb.c
        audio/audio_avb.h
    )
endif()

if (CONFIG_MCUX_COMPONENT_component.rtos_apps.audio.pll)
    mcux_add_source(
        SOURCES
        audio/audio_pll14xx.c
    )
endif()

mcux_add_include(
    INCLUDES
    .
)

message("rtos-apps audio sources are included for target ${RTOS_APPS_TARGET}")
endif()
