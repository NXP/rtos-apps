/*
 * Copyright 2022-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_VLAN_H__
#define __RTOS_APPS_SHELL_VLAN_H__

void cmd_vlan_init(shell_handle_t shell);

shell_status_t cmd_vlan_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_vlan_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_vlan_read(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_vlan_dump(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_vlan_set_pvid(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_vlan_get_pvid(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_VLAN_H__ */
