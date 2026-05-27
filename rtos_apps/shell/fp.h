/*
 * Copyright 2023-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_FP_H__
#define __RTOS_APPS_SHELL_FP_H__

#include "genavb/frame_preemption.h"

#define CMD_FP_SET_HELP \
	"<port_id> [-q] [-t 0x<express mask>] [-p]\n" \
	"<port_id> [-e <enable>] [-d <disable>] [-v <time ms>] [-a <frag size>] [-p]\n" \
	"   parameters:\n" \
	"       port_id: logical port index\n" \
	"   802.1Q options:\n" \
	"       -q : set only 802.1q (if not used, set only 802.3)\n" \
	"       -t 0x<express mask>: frame status priority bitmask in hexadecimal, where 0: preemptable, 1: express\n" \
	"   802.3 options:\n" \
	"       -e <enable>: enable preemption, 0: disable, 1: enable (64B boundaries), 2: enable (4B boundaries), default: 1\n" \
	"       -d <disable>: verify disable, 1: disable verify, 0: enable verify fsm, default: 0\n" \
	"       -v <time ms>: verify time (ms), range: 1 to 128, default: 10\n" \
	"       -a <frag size>: minimum size of non-final fragments, 0: 64B, 1: 128B, 3: 256B, 4: 512B, default: 0\n" \
	"       -p: save configuration to permanent database"

#define CMD_FP_GET_HELP \
	"<port_id> [-p]\n" \
	"   parameters:\n" \
	"       port_id: logical port index\n" \
	"   options:\n" \
	"       -p: read configuration from permanent database"

void cmd_fp_init(void *shell);

int cmd_fp_set(void *shell, int32_t argc, char **argv);
int cmd_fp_get(void *shell, int32_t argc, char **argv);

int fp_write_802_1q_permanent(void *shell, unsigned int port_id, struct genavb_fp_config *config);
int fp_write_802_3_permanent(void *shell, unsigned int port_id, struct genavb_fp_config *config);
int fp_apply_permanent(void *shell, unsigned int port_id);
int fp_apply(void *shell, unsigned int port_id, struct genavb_fp_config *config_fp_8021q, struct genavb_fp_config *config_fp_8023);

#endif /* __RTOS_APPS_SHELL_FP_H__ */
