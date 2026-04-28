/*
 * Copyright 2019-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "genavb/helpers.h"
#include "genavb/stats.h"

#include "storage.h"

static shell_status_t port_stats(shell_handle_t shell, int32_t argc, char **argv);

SHELL_COMMAND_DEFINE(port_stats,
                     "\nport_stats <port_id>\n"
                     "    parameters:\n"
                     "        port_id: logical port index\n",
                     &port_stats,
                     1);

static int __port_stats(shell_handle_t shell, unsigned int port_id, int n)
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

static shell_status_t port_stats(shell_handle_t shell, int32_t argc, char **argv)
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
    return kStatus_SHELL_Success;
}
