/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_HSR_H__
#define __RTOS_APPS_SHELL_HSR_H__

#define CMD_HSR_MODE_SET_HELP \
	"<mode> [-p]\n" \
	"    parameters:\n" \
	"        mode: HSR node operation mode\n" \
	"        	    0: Mode H(HSR-tagged forwarding)\n" \
	"        	    1: Mode N(No forwarding)\n" \
	"        	    2: Mode T(Transparent forwarding)\n" \
	"        	    3: Mode U(Unicast forwarding)\n" \
	"    options:\n" \
	"        -p: update mode in permanent database"

void cmd_hsr_init(void *shell);

int cmd_hsr_mode_set(void *shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_HSR_H__ */
