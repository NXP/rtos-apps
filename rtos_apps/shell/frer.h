/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_FRER_H__
#define __RTOS_APPS_SHELL_FRER_H__

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
