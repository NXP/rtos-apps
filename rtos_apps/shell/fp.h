/*
 * Copyright 2023-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_FP_H__
#define __RTOS_APPS_SHELL_FP_H__

void cmd_fp_init(shell_handle_t shell);

shell_status_t cmd_fp_set(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_fp_get(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_FP_H__ */
