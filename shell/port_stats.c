/*
 * Copyright 2019-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>

#include "genavb/helpers.h"
#include "genavb/stats.h"

#include "rtos_apps/types.h"
#include "rtos_apps/log.h"

#include "storage.h"

#include "shell_config.h"
#include "rtos_apps/shell/port_stats.h"

static int __port_stats(void *shell, unsigned int port_id, int n)
{
    const char *names[n];
    uint64_t values[n];
    int i;

    if (genavb_port_stats_get_strings(port_id, names, n * sizeof(char *)) < 0) {
        log_err("genavb_port_stats_get_strings() failed\n");
        goto err;
    }

    if (genavb_port_stats_get(port_id, values, n * sizeof(uint64_t)) < 0) {
        log_err("genavb_port_stats_get() failed\n");
        goto err;
    }

    for (i = 0; i < n; i++) {
        shell_printf(shell, "%-32s %10llu\n", names[i], values[i]);
    }

    return 0;

err:
   return -1;
}

int cmd_port_stats(void *shell, int32_t argc, char **argv)
{
    unsigned int port_id;
    unsigned long tmp;
    int n;

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;

    n = genavb_port_stats_get_number(port_id);
    if (n < 0) {
        log_err("genavb_port_stats_get_number() error %d\n", n);
        goto exit;
    }

    __port_stats(shell, port_id, n);

exit:
    return 0;
}
