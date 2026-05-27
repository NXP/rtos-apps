/*
 * Copyright 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_QBV_H__
#define __RTOS_APPS_SHELL_QBV_H__

#include "genavb/scheduled_traffic.h"

#define CMD_QBV_SET_HELP \
	"<port_id> [-b <base_time>] [-c <cycle_time>] [-C <cycle_time_ext>] [-l <gate_states>,<time_interval>[,<gate_operation>] [-l ...]] [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -b <base_time>: base time in ns, 0 to (2^64 - 1), default: 0\n" \
	"        -c <cycle_time>: cycle time in ns, 0 to 4294967295, default: 100000\n" \
	"        -C <cycle_time_ext>: cycle time extension in ns, 0 to 4294967295, default: 0\n" \
	"        -l <gate_states>,<time_interval>[,<gate_operation>]: gate control list, default: ff,100000,0 (one option per list entry)\n" \
	"        -p: update configuration in permanent database"

#define CMD_QBV_GET_HELP \
	"<port_id> [-p] [-t type]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -p: print from permanent database\n" \
	"        -t <type>: configuration type, 0: operational, 1: administrative, default: 0"

#define CMD_QBV_DISABLE_HELP \
	"<port_id> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -p: update configuration in permanent database"

#define CMD_QBV_SET_MAX_SDU_HELP \
	"<port_id> [-l <traffic class queue>,<maxSDU> [-l ...]] [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -l <traffic class queue>,<maxSDU>: list : maximum Service Data Unit size for each traffic class queue\n" \
	"        -p: update configuration in permanent database"

#define CMD_QBV_GET_MAX_SDU_HELP \
	"<port_id> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port index\n" \
	"    options:\n" \
	"        -p: read from permanent database"

void cmd_qbv_init(void *shell);

int cmd_qbv_set(void *shell, int32_t argc, char **argv);
int cmd_qbv_get(void *shell, int32_t argc, char **argv);
int cmd_qbv_disable(void *shell, int32_t argc, char **argv);
int cmd_qbv_set_max_sdu(void *shell, int32_t argc, char **argv);
int cmd_qbv_get_max_sdu(void *shell, int32_t argc, char **argv);

int qbv_write_permanent(void *shell, unsigned int port_id, struct genavb_st_config *config);
int qbv_apply_permanent(void *shell, unsigned int port_id);
int qbv_apply(void *shell, unsigned int port_id, struct genavb_st_config *config);

#endif /* __RTOS_APPS_SHELL_QBV_H__ */
