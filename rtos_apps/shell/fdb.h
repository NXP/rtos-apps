/*
 * Copyright 2022, 2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_FDB_H__
#define __RTOS_APPS_SHELL_FDB_H__

void cmd_fdb_init(shell_handle_t shell);

shell_status_t cmd_fdb_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_fdb_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_fdb_read(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_fdb_dump(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_FDB_H__ */
