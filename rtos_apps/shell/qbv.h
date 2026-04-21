/*
 * Copyright 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_QBV_H__
#define __RTOS_APPS_SHELL_QBV_H__

void cmd_qbv_init(shell_handle_t shell);

shell_status_t cmd_qbv_set(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_qbv_get(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_qbv_disable(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_qbv_set_max_sdu(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_qbv_get_max_sdu(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_QBV_H__ */
