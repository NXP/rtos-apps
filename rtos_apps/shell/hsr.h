/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_HSR_H__
#define __RTOS_APPS_SHELL_HSR_H__

void cmd_hsr_init(shell_handle_t shell);

shell_status_t cmd_hsr_mode_set(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_HSR_H__ */
