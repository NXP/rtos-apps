/*
 * Copyright 2022, 2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_FDB_H__
#define __RTOS_APPS_SHELL_FDB_H__

#define CMD_FDB_UPDATE_HELP \
	"\nfdb_update <mac> <vid> <port_id> [-c <control>] [-p]\n" \
	"    parameters:\n" \
	"        mac: mac address\n" \
	"        vid: vlan id\n" \
	"        port_id: logical port id\n" \
	"    options:\n" \
	"        -c <control>: filtering control, 0: filtering, 1: forwarding (default)\n" \
	"        -p: update entry in permanent database\n"

#define CMD_FDB_DELETE_HELP \
	"\nfdb_delete <mac> <vid> [-p]\n" \
	"    parameters:\n" \
	"        mac: mac address\n" \
	"        vid: vlan id\n" \
	"    options:\n" \
	"        -p: delete entry from permanent database\n"

#define CMD_FDB_READ_HELP \
	"\nfdb_read <mac> <vid> [-p]\n" \
	"    parameters:\n" \
	"        mac: mac address\n" \
	"        vid: vlan id\n" \
	"    options:\n" \
	"        -p: read entry from permanent database\n"

#define CMD_FDB_DUMP_HELP \
	"\nfdb_dump [-p]\n" \
	"    options:\n" \
	"        -p: print permanent entries\n"

void cmd_fdb_init(void *shell);

int cmd_fdb_update(void *shell, int32_t argc, char **argv);
int cmd_fdb_delete(void *shell, int32_t argc, char **argv);
int cmd_fdb_read(void *shell, int32_t argc, char **argv);
int cmd_fdb_dump(void *shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_FDB_H__ */
