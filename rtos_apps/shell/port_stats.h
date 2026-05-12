/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_PORT_STATS_H__
#define __RTOS_APPS_SHELL_PORT_STATS_H__

#define CMD_PORT_STATS_HELP \
	"<port_id>\n" \
	"    parameters:\n" \
	"        port_id: logical port index"

int cmd_port_stats(void *shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_PORT_STATS_H__ */
