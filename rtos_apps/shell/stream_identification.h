/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_STREAM_IDENTIFICATION_H__
#define __RTOS_APPS_SHELL_STREAM_IDENTIFICATION_H__

void cmd_stream_identification_init(shell_handle_t shell);

shell_status_t cmd_si_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_si_read(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_si_delete(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_STREAM_IDENTIFICATION_H__ */
