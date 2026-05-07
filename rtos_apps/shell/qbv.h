/*
 * Copyright 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_QBV_H__
#define __RTOS_APPS_SHELL_QBV_H__

#define CMD_QBV_SET_HELP \
	"\nqbv_set <port_id> [-b <base_time>] [-c <cycle_time>] [-C <cycle_time_ext>] [-l <gate_states>,<time_interval>[,<gate_operation>] [-l ...]] [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -b <base_time>: base time in ns, 0 to (2^64 - 1), default: 0\n" \
	"        -c <cycle_time>: cycle time in ns, 0 to 4294967295, default: 100000\n" \
	"        -C <cycle_time_ext>: cycle time extension in ns, 0 to 4294967295, default: 0\n" \
	"        -l <gate_states>,<time_interval>[,<gate_operation>]: gate control list, default: ff,100000,0 (one option per list entry)\n" \
	"        -p: update configuration in permanent database\n"

#define CMD_QBV_GET_HELP \
	"\nqbv_get <port_id> [-p] [-t type]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -p: print from permanent database\n" \
	"        -t <type>: configuration type, 0: operational, 1: administrative, default: 0\n"

#define CMD_QBV_DISABLE_HELP \
	"\nqbv_disable <port_id> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -p: update configuration in permanent database\n"

#define CMD_QBV_SET_MAX_SDU_HELP \
	"\nqbv_set_max_sdu <port_id> [-l <traffic class queue>,<maxSDU> [-l ...]] [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -l <traffic class queue>,<maxSDU>: list : maximum Service Data Unit size for each traffic class queue\n" \
	"        -p: update configuration in permanent database\n"

#define CMD_QBV_GET_MAX_SDU_HELP \
	"\nqbv_get_max_sdu <port_id> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -p: read from permanent database\n"

void cmd_qbv_init(shell_handle_t shell);

shell_status_t cmd_qbv_set(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_qbv_get(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_qbv_disable(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_qbv_set_max_sdu(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_qbv_get_max_sdu(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_QBV_H__ */
