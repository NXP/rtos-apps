/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_SHELL_H__
#define __RTOS_APPS_SHELL_SHELL_H__

#define CMD_SHELL_LOG_HELP \
	"\n<component_id> <level>\n" \
	"    <component_id>\n" \
	"        all, app, avtp, avdecc, srp, maap, common, os, fgptp, api or mgmt\n" \
	"    <level>\n" \
	"        crit, err, init, info or dbg"

int cmd_shell_log(void *shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_SHELL_H__ */
