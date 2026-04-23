/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_STREAM_IDENTIFICATION_H__
#define __RTOS_APPS_SHELL_STREAM_IDENTIFICATION_H__

#define CMD_SI_UPDATE_HELP \
	"\nsi_update <index> [-h <handle>] [-P <p1>[,<p2>,...,<pn>]] [-t <type>] [-m <mac>] [-T <tagged>] [-v <vid>] [-p]\n" \
	"    parameters:\n" \
	"        index: stream identity table index\n" \
	"    options:\n" \
	"        -h <handle>: stream handle value (default: 0)\n" \
	"        -P <p1>: list of logical port ids (comma separated)\n" \
	"        -t <type>: stream identification type\n" \
	"                  1: Null (default)\n" \
	"                  2: Source MAC Vlan\n" \
	"        -m <mac>: stream identification source or destination mac address (default: 00:00:aa:bb:cc:dd)\n" \
	"                  if type == Null, Destination MAC\n" \
	"                  if type == Source MAC Vlan, Source MAC\n" \
	"        -T <tagged>: Tagged value:\n" \
	"                     1: Frame is tagged\n" \
	"                     2: Frame is untagged or tagged with vid = 0 (supported only untagged) (default)\n" \
	"                     3: Frame is tagged or not (unsupported)\n" \
	"        -v <vlan>: vid value (default: 0)\n" \
	"        -p: update entry in permanent database\n"

#define CMD_SI_READ_HELP \
	"\nsi_read <index> [-p]\n" \
	"    parameters:\n" \
	"        index: stream identity table index\n" \
	"    options:\n" \
	"        -p: read entry from permanent database\n"

#define CMD_SI_DELETE_HELP \
	"\nsi_delete <index> [-p]\n" \
	"    parameters:\n" \
	"        index: stream identity table index\n" \
	"    options:\n" \
	"        -p: delete entry from permanent database\n"

void cmd_stream_identification_init(void *shell);

int cmd_si_update(void *shell, int32_t argc, char **argv);
int cmd_si_read(void *shell, int32_t argc, char **argv);
int cmd_si_delete(void *shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_STREAM_IDENTIFICATION_H__ */
