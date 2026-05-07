/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_FRER_H__
#define __RTOS_APPS_SHELL_FRER_H__

#define CMD_SEQG_UPDATE_HELP \
	"\nseqg_update <index> [-h <h1>[,<h2>,..,<hn>]] [-r] [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -h <h1>: list of stream handles (comma separated)\n" \
	"        -p: update entry in permanent database\n" \
	"\nseqg_update <index> [-r]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -r: reset sequence generation\n"

#define CMD_SEQG_DELETE_HELP \
	"\nseqg_delete <index> [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -p: delete entry from permanent database\n"

#define CMD_SEQG_READ_HELP \
	"\nseqg_read <index> [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -p: read entry from permanent database\n"

#define CMD_SEQR_UPDATE_HELP \
	"\nseqr_update <index> [-h <h1>[,<h2>,..,<hn>]] [-P <p1>[,<p2>,..,<pn>]] [-a <algorithm>] [-r] [-H <history>] [-s <enable>] [-i <enable>] [-p]\n" \
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
	"        -p: update entry in permanent database\n"

#define CMD_SEQR_DELETE_HELP \
	"\nseqr_delete <index> [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -p: delete entry from permanent database\n"

#define CMD_SEQR_READ_HELP \
	"\nseqr_read <index> [-p]\n" \
	"    parameters:\n" \
	"        index: entry index\n" \
	"    options:\n" \
	"        -p: read entry from permanent database\n"

#define CMD_SEQI_UPDATE_HELP \
	"\nseqi_update <port_id> [-h h1[,<h2>,..,<hn>]] [-a] [-e <encapsulation>] [-i <id>] [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port id\n" \
	"    options:\n" \
	"        -h <h1>: list of stream handles (comma separated)\n" \
	"        -a: active\n" \
	"        -e <encapsulation>: encapsulation tag, 1: R-TAG, 2: HSR, 3: PRP (not supported), 4: R-TAG (draft 2.0)\n" \
	"        -i <id>: path id (HSR) or lan id (PRP)\n" \
	"        -p: update entry in permanent database\n"

#define CMD_SEQI_DELETE_HELP \
	"\nseqi_delete <port_id> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port id\n" \
	"    options:\n" \
	"        -p: delete entry from permanent database\n"

#define CMD_SEQI_READ_HELP \
	"\nseqi_read <port_id> [-p]\n" \
	"    parameters:\n" \
	"        port_id: logical port id\n" \
	"    options:\n" \
	"        -p: read entry from permanent database\n"

void cmd_frer_init(shell_handle_t shell);

/* Sequence generation */
shell_status_t cmd_seqg_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_seqg_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_seqg_read(shell_handle_t shell, int32_t argc, char **argv);

/* Sequence recovery */
shell_status_t cmd_seqr_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_seqr_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_seqr_read(shell_handle_t shell, int32_t argc, char **argv);

/* Sequence identification */
shell_status_t cmd_seqi_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_seqi_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_seqi_read(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_FRER_H__ */
