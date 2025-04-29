# Description: Component providing example audio application
include_guard(GLOBAL)

message("rtos-apps audio headers are included for target ${RTOS_APPS_TARGET}")

target_include_directories(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

message("rtos-apps audio sources are included for target ${RTOS_APPS_TARGET}")

target_sources(${RTOS_APPS_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio_buffer.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio_element_dtmf.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio_element_pll.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio_element_routing.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio_element_sai_sink.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio_element_sai_source.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio_element_sine.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio_element.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio_pipeline.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/play_pipeline.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/sai_drv.c
    ${CMAKE_CURRENT_LIST_DIR}/audio/audio.c
)

if (CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/audio/audio_element_avtp_sink.c
        ${CMAKE_CURRENT_LIST_DIR}/audio/audio_element_avtp_source.c
    )

    target_compile_definitions(${RTOS_APPS_TARGET} PRIVATE
        CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE=${CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE}
    )
endif()

if (CONFIG_RTOS_APPS_AUDIO_PLL_ENABLE)
    target_sources(${RTOS_APPS_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/audio/audio_pll14xx.c
    )
endif()