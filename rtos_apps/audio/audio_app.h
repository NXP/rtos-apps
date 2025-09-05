/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_AUDIO_APP_H_
#define _RTOS_APPS_AUDIO_APP_H_

#include <stdint.h>
#include <stdbool.h>

#include "audio_pipeline.h"

#define AUDIO_APP_MAX_SUPPORTED_PERIOD 10
#define AUDIO_APP_MAX_CFG              9
#define AUDIO_APP_MAX_DATA_THREADS     2

struct sai_active_config {
    void *sai_base;
    int32_t clk_id;
    int32_t root_clk_id;
    uint32_t clk_freq;
    uint32_t masterSlave;
    uint32_t msel;
    int32_t audio_pll;
    uint32_t audio_pll_mul;
    uint32_t audio_pll_div;
    uint32_t slot_count;        /* Number of words in audio frame: channels count */
    int slot_size; /* Word size in bits */
    uint32_t rx_mask;
    uint32_t tx_mask;
    int rx_sync_mode;
    int tx_sync_mode;
    uint8_t codec_id;
};

struct play_pipeline_config {
    const struct audio_pipeline_config *cfg[AUDIO_APP_MAX_DATA_THREADS];
};

extern const int audio_app_supported_period[AUDIO_APP_MAX_SUPPORTED_PERIOD];
extern const struct play_pipeline_config *audio_app_play_config[AUDIO_APP_MAX_CFG];
extern const struct play_pipeline_config *audio_app_play_alternate_config[AUDIO_APP_MAX_CFG];
extern struct sai_active_config audio_app_sai_active_list[];
extern uint32_t audio_app_sai_active_list_nelems;

int audio_app_ctrl_send(void *ctrl_handle, void *data, uint32_t len);
int audio_app_ctrl_recv(void *ctrl_handle, void *data, uint32_t *len);

bool audio_app_check_params(uint32_t period, uint32_t rate);
void audio_app_pin_mux_dynamic_config(bool use_alternate_config);
void audio_app_sai_alternate_config(bool use_alternate_config, uint32_t rate);

int32_t audio_app_codec_setup(uint8_t codec_id);
int32_t audio_app_codec_set_format(uint8_t codec_id, uint32_t mclk, uint32_t sample_rate, uint32_t bitwidth);
int32_t audio_app_codec_close(uint8_t codec_id);
bool audio_app_codec_is_rate_supported(uint32_t rate, bool use_alternate_config);

struct genavb_handle *audio_app_avb_init(void);
void audio_app_avb_exit(void);

void audio_app_sai_clock_setup(void);
uint32_t audio_app_sai_select_audio_pll_mux(unsigned int index, uint32_t srate);
uint32_t audio_app_sai_get_clock_freq(unsigned int index);

#endif /* _RTOS_APPS_AUDIO_APP_H_ */
