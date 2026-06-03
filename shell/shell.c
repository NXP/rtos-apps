/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>

#include "genavb/log.h"

#include "rtos_apps/log.h"
#include "rtos_apps/shell/shell.h"

#include "shell_config.h"

static int gavb_log_level(char *component_str, char *level_str)
{
    genavb_log_component_id_t component;
    genavb_log_level_t level;
    int all = 0;

    if (!strcmp(level_str, "crit"))
        level = GENAVB_LOG_LEVEL_CRIT;
    else if (!strcmp(level_str, "err"))
        level = GENAVB_LOG_LEVEL_ERR;
    else if (!strcmp(level_str, "init"))
        level = GENAVB_LOG_LEVEL_INIT;
    else if (!strcmp(level_str, "info"))
        level = GENAVB_LOG_LEVEL_INFO;
    else if (!strcmp(level_str, "dbg"))
        level = GENAVB_LOG_LEVEL_DEBUG;
    else
        return -1;

    if (!strcmp(component_str, "avtp"))
        component = GENAVB_LOG_COMPONENT_ID_AVTP;
    else if (!strcmp(component_str, "avdecc"))
        component = GENAVB_LOG_COMPONENT_ID_AVDECC;
    else if (!strcmp(component_str, "srp"))
        component = GENAVB_LOG_COMPONENT_ID_SRP;
    else if (!strcmp(component_str, "maap"))
        component = GENAVB_LOG_COMPONENT_ID_MAAP;
    else if (!strcmp(component_str, "common"))
        component = GENAVB_LOG_COMPONENT_ID_COMMON;
    else if (!strcmp(component_str, "os"))
        component = GENAVB_LOG_COMPONENT_ID_OS;
    else if (!strcmp(component_str, "fgptp"))
        component = GENAVB_LOG_COMPONENT_ID_GPTP;
    else if (!strcmp(component_str, "api"))
        component = GENAVB_LOG_COMPONENT_ID_API;
    else if (!strcmp(component_str, "mgmt"))
        component = GENAVB_LOG_COMPONENT_ID_MGMT;
    else if (!strcmp(component_str, "all"))
        all = 1;
    else
        return -1;

    if (all) {
        for (component = GENAVB_LOG_COMPONENT_ID_AVTP; component <= GENAVB_LOG_COMPONENT_ID_MGMT; component++)
            genavb_log_level_set(component, level);
    } else {
        genavb_log_level_set(component, level);
    }

    return 0;
}

static int app_log_level_set(char *level_str)
{
    rtos_apps_log_level_t level;

    if (!strcmp(level_str, "crit"))
        level = LOG_CRIT;
    else if (!strcmp(level_str, "err"))
        level = LOG_ERR;
    else if (!strcmp(level_str, "init"))
        level = LOG_INFO;
    else if (!strcmp(level_str, "info"))
        level = LOG_INFO;
    else if (!strcmp(level_str, "dbg"))
        level = LOG_DEBUG;
    else
        return -1;

    rtos_apps_log_level_config_set(level);

    return 0;
}

int cmd_shell_log(void *shell, int32_t argc, char **argv)
{
    if (!strcmp(argv[1], "app") || !strcmp(argv[1], "all")) {
        if (app_log_level_set(argv[2]) < 0)
            goto usage;
    }

    if (strcmp(argv[1], "app")) {
        if (!gavb_stack_handle())
            shell_printf(shell, "genAVB stack is not ready\n");
        else if (gavb_log_level(argv[1], argv[2]) < 0)
            goto usage;
    }

    return 0;

usage:
    shell_printf(shell, CMD_SHELL_LOG_HELP);

    return -1;
}
