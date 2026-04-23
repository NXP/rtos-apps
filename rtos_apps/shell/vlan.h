/*
 * Copyright 2022-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_VLAN_H__
#define __RTOS_APPS_SHELL_VLAN_H__

#define CMD_VLAN_UPDATE_HELP \
	"\nvlan_update <vid> <port_id> [-c <control>] [-u] [-p]\n" \
	"    parameters:\n" \
	"        vid: vlan id\n" \
	"        port_id: logical port id\n" \
	"    options:\n" \
	"        -c <control>: registrar-admin-control, 0: forbidden, 1: fixed (default)\n" \
	"        -u: untagged transmit\n" \
	"        -p: update entry in permanent database\n"

#define CMD_VLAN_DELETE_HELP \
	"\nvlan_delete <vid> [-p]\n" \
	"    parameters:\n" \
	"        vid: vlan id\n" \
	"    options:\n" \
	"        -p: delete entry in permanent database\n"

#define CMD_VLAN_READ_HELP \
	"\nvlan_read <vid> [-p]\n" \
	"    parameters:\n" \
	"        vid: vlan id\n" \
	"    options:\n" \
	"        -p: read entry from permanent database\n"

#define CMD_VLAN_DUMP_HELP \
	"\nvlan_dump [-p]\n" \
	"    options:\n" \
	"        -p: print permanent database\n"

#define CMD_VLAN_SET_PVID_HELP \
	"\nvlan_set_pvid <port_id> <vid> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port id\n" \
	"        vid: port defaut vlan id (PVID)\n" \
	"    options:\n" \
	"        -p: update entry in permanent database\n"

#define CMD_VLAN_GET_PVID_HELP \
	"\nvlan_get_pvid <port_id> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port id\n" \
	"    options:\n" \
	"        -p: read entry from permanent database\n"

void cmd_vlan_init(void *shell);

int cmd_vlan_update(void *shell, int32_t argc, char **argv);
int cmd_vlan_delete(void *shell, int32_t argc, char **argv);
int cmd_vlan_read(void *shell, int32_t argc, char **argv);
int cmd_vlan_dump(void *shell, int32_t argc, char **argv);
int cmd_vlan_set_pvid(void *shell, int32_t argc, char **argv);
int cmd_vlan_get_pvid(void *shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_VLAN_H__ */
