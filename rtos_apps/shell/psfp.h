/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_PSFP_H__
#define __RTOS_APPS_SHELL_PSFP_H__

void cmd_psfp_init(shell_handle_t shell);

shell_status_t cmd_sf_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_sf_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_sf_read(shell_handle_t shell, int32_t argc, char **argv);

shell_status_t cmd_sg_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_sg_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_sg_read(shell_handle_t shell, int32_t argc, char **argv);

shell_status_t cmd_fm_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_fm_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_fm_read(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_PSFP_H__ */
