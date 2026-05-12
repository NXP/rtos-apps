/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_FRER_H__
#define __RTOS_APPS_SHELL_FRER_H__

#define CMD_SEQG_UPDATE_HELP \
	"<index> [-h <h1>[,<h2>,..,<hn>]] [-r] [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -h <h1>: list of stream handles (comma separated)\n" \
	"        -p: update entry in permanent database\n" \
	"<index> [-r]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -r: reset sequence generation"

#define CMD_SEQG_DELETE_HELP \
	"<index> [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -p: delete entry from permanent database"

#define CMD_SEQG_READ_HELP \
	"<index> [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -p: read entry from permanent database"

#define CMD_SEQR_UPDATE_HELP \
	"<index> [-h <h1>[,<h2>,..,<hn>]] [-P <p1>[,<p2>,..,<pn>]] [-a <algorithm>] [-r] [-H <history>] [-s <enable>] [-i <enable>] [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -h <h1>: list of stream handles (comma separated)\n" \
	"        -P <p1>: list of logical port ids (comma separated)\n" \
	"        -a <algorithm>: recovery algorithm, 0: Vector, 1: Match\n" \
	"        -r: reset sequence recovery\n" \
	"        -H <history>: history length\n" \
	"        -s <enable>: take no sequence, 0: disable (default), 1: enable\n" \
	"        -i <enable>: individual recovery, 0: disable (default), 1: enable\n" \
	"        -p: update entry in permanent database"

#define CMD_SEQR_DELETE_HELP \
	"<index> [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -p: delete entry from permanent database"

#define CMD_SEQR_READ_HELP \
	"<index> [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -p: read entry from permanent database"

#define CMD_SEQI_UPDATE_HELP \
	"<port_id> [-h h1[,<h2>,..,<hn>]] [-a] [-e <encapsulation>] [-i <id>] [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port id\n" \
	"    options:\n" \
	"        -h <h1>: list of stream handles (comma separated)\n" \
	"        -a: active\n" \
	"        -e <encapsulation>: encapsulation tag, 1: R-TAG, 2: HSR, 3: PRP (not supported), 4: R-TAG (draft 2.0)\n" \
	"        -i <id>: path id (HSR) or lan id (PRP)\n" \
	"        -p: update entry in permanent database"

#define CMD_SEQI_DELETE_HELP \
	"<port_id> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port id\n" \
	"    options:\n" \
	"        -p: delete entry from permanent database"

#define CMD_SEQI_READ_HELP \
	"<port_id> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port id\n" \
	"    options:\n" \
	"        -p: read entry from permanent database"

void cmd_frer_init(void *shell);

/* Sequence generation */
int cmd_seqg_update(void *shell, int32_t argc, char **argv);
int cmd_seqg_delete(void *shell, int32_t argc, char **argv);
int cmd_seqg_read(void *shell, int32_t argc, char **argv);

/* Sequence recovery */
int cmd_seqr_update(void *shell, int32_t argc, char **argv);
int cmd_seqr_delete(void *shell, int32_t argc, char **argv);
int cmd_seqr_read(void *shell, int32_t argc, char **argv);

/* Sequence identification */
int cmd_seqi_update(void *shell, int32_t argc, char **argv);
int cmd_seqi_delete(void *shell, int32_t argc, char **argv);
int cmd_seqi_read(void *shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_FRER_H__ */
