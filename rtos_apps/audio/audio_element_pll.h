/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_AUDIO_ELEMENT_PLL_H_
#define _RTOS_APPS_AUDIO_ELEMENT_PLL_H_

struct pll_element_config {
    unsigned int src_sai_id;
    unsigned int dst_sai_id;
    unsigned int pll_id;
};

#endif /* _RTOS_APPS_AUDIO_ELEMENT_PLL_H_ */
