/*
 * Copyright 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_apps/shell/qbv.h"

#ifdef CONFIG_RTOS_APPS_QBV

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "genavb/genavb.h"
#include "genavb/helpers.h"
#include "genavb/qos.h"

#include "rtos_abstraction_layer.h"
#include "rtos_apps/types.h"

#include "rtos_apps/shell/common.h"
#include "rtos_apps/shell/shell_storage_app.h"
#include "rtos_apps/storage/storage.h"
#include "rtos_apps/storage/storage_app.h"

#include "shell_config.h"
#include "genavb_sdk.h"

#include "storage.h"
#include "common.h"

#define QOS_MAX_SDU_DEFAULT 0
#define QOS_MAX_SDU_BUF_SIZE (20 + SHELL_STORAGE_ROOT_SIZE)

static const struct genavb_st_max_sdu max_sdu_default[QOS_TRAFFIC_CLASS_MAX] = {
    [0] = {
        .traffic_class = 0,
        .queue_max_sdu = QOS_MAX_SDU_DEFAULT,
    },
    [1] = {
        .traffic_class = 1,
        .queue_max_sdu = QOS_MAX_SDU_DEFAULT,
    },
    [2] = {
        .traffic_class = 2,
        .queue_max_sdu = QOS_MAX_SDU_DEFAULT,
    },
    [3] = {
        .traffic_class = 3,
        .queue_max_sdu = QOS_MAX_SDU_DEFAULT,
    },
    [4] = {
        .traffic_class = 4,
        .queue_max_sdu = QOS_MAX_SDU_DEFAULT,
    },
    [5] = {
        .traffic_class = 5,
        .queue_max_sdu = QOS_MAX_SDU_DEFAULT,
    },
    [6] = {
        .traffic_class = 6,
        .queue_max_sdu = QOS_MAX_SDU_DEFAULT,
    },
    [7] = {
        .traffic_class = 7,
        .queue_max_sdu = QOS_MAX_SDU_DEFAULT,
    },
};

static const genavb_clock_id_t port_clock_id[CONFIG_APP_LOGICAL_PORTS] = {
#if CONFIG_APP_LOGICAL_PORTS > 0
    [0] = GENAVB_CLOCK_GPTP_0_0,
#endif
#if CONFIG_APP_LOGICAL_PORTS > 1
    [1] = GENAVB_CLOCK_GPTP_1_0,
#endif
#if CONFIG_APP_LOGICAL_PORTS > 2
    [2] = GENAVB_CLOCK_BR_0_0,
#endif
#if CONFIG_APP_LOGICAL_PORTS > 3
    [3] = GENAVB_CLOCK_BR_0_0,
#endif
#if CONFIG_APP_LOGICAL_PORTS > 4
    [4] = GENAVB_CLOCK_BR_0_0,
#endif
#if CONFIG_APP_LOGICAL_PORTS > 5
    [5] = GENAVB_CLOCK_BR_0_0,
#endif
#if CONFIG_APP_LOGICAL_PORTS > 6
    [6] = GENAVB_CLOCK_BR_0_0,
#endif
};

static const struct genavb_st_config st_config_default = {
        .enable = 0,
        .base_time = 0,
        .cycle_time_p = 1000000,
        .cycle_time_q = NSECS_PER_SEC,
        .list_length = 1
};

static const struct genavb_st_gate_control_entry st_list_default = {
        .operation = GENAVB_ST_SET_GATE_STATES,
        .time_interval = 100000,
        .gate_states = 0xff
};

static void dump_genavb_st_config(void *shell, unsigned int port_id, struct genavb_st_config *config)
{
    int i;

    shell_printf(shell, "\n");
    shell_printf(shell, "port_id        %u\n", port_id);
    shell_printf(shell, "enable         %d\n", config->enable);
    shell_printf(shell, "base_time      %llu (ns)\n", config->base_time);
    shell_printf(shell, "cycle_time     %llu (ns)\n", (NSECS_PER_SEC * (uint64_t)config->cycle_time_p) / config->cycle_time_q);
    shell_printf(shell, "cycle_time_ext %d (ns)\n", config->cycle_time_ext);
    shell_printf(shell, "list_length    %d\n", config->list_length);

    if (config->list_length > QBV_LIST_MAX_ENTRIES) {
        shell_printf(shell, "invalid list length\n");
        return;
    }

    if (config->list_length && config->control_list) {
        shell_printf(shell, "\ngate list:\n");
        shell_printf(shell, " entry | oper | gate | interval (ns) |\n");
        shell_printf(shell, "-------+------+------+---------------+\n");
        for (i = 0; i < config->list_length; i++) {
            shell_printf(shell, " %5u |", i);
            shell_printf(shell, " 0x%02x |", config->control_list[i].operation);
            shell_printf(shell, " 0x%02x |", config->control_list[i].gate_states);
            shell_printf(shell, " % 13d |\n", config->control_list[i].time_interval);
        }
    }
    shell_printf(shell, "\n");
}

static int qbv_read_permanent(void *shell, unsigned int port_id, struct genavb_st_config *config)
{
    struct genavb_st_gate_control_entry *gate_list;
    unsigned int entry_id;
    uint32_t interval = 0;
    char port[15 + SHELL_STORAGE_ROOT_SIZE] = {0};
    uint8_t mask = 0, operation = GENAVB_ST_SET_GATE_STATES;

    gate_list = config->control_list;
    if (!gate_list)
        goto err;

    if (h_snprintf_strict(port, 15 + SHELL_STORAGE_ROOT_SIZE, CONFIG_STORAGE_ROOT "/qbv/port%u", port_id) < 0)
        goto err;

    storage_read_int(port, "enabled", &config->enable);
    storage_read_u64(port, "base_time", &config->base_time);
    storage_read_u32(port, "cycle_time", &config->cycle_time_p);
    storage_read_u32(port, "cycle_time_ext", &config->cycle_time_ext);

    for (entry_id = 0; entry_id < QBV_LIST_MAX_ENTRIES; entry_id++) {
        char entry[10] = {0};
        h_snprintf(entry, 10, "entry%u", entry_id);

        if (storage_read_qbv_entry(port, entry, &mask, &interval, &operation) < 0)
            break;

        gate_list[entry_id].gate_states = mask;
        gate_list[entry_id].time_interval = interval;
        gate_list[entry_id].operation = operation;
    }

    config->cycle_time_q = NSECS_PER_SEC;
    config->list_length = entry_id;

    return 0;

err:
    return -1;
}

int qbv_write_permanent(void *shell, unsigned int port_id, struct genavb_st_config *config)
{
    struct genavb_st_gate_control_entry *gate_list = config->control_list;
    char buf[20] = {0}, entry[10];
    char dir[20 + SHELL_STORAGE_ROOT_SIZE] = {0};
    int i;

    if (h_snprintf_strict(dir, 20 + SHELL_STORAGE_ROOT_SIZE, CONFIG_STORAGE_ROOT "/qbv/port%u", port_id) < 0) {
        goto err;
    }

    storage_rm(dir, true, true);

    /* create qbv directory since it doesn't exist */
    if (storage_mkdir(dir, true) < 0) {
        goto err;
    }

    storage_write_uint(dir, "enabled", (unsigned int)config->enable);
    storage_write_u64(dir, "base_time", config->base_time);
    storage_write_uint(dir, "cycle_time", config->cycle_time_p);
    storage_write_uint(dir, "cycle_time_ext", config->cycle_time_ext);

    if (gate_list) {
        for (i = 0; i < config->list_length; i++) {
            h_snprintf(buf, 15, "%2x,%lu,%u", gate_list[i].gate_states, gate_list[i].time_interval, gate_list[i].operation);
            h_snprintf(entry, 10, "entry%u", i);
            storage_write(dir, entry, buf, strlen(buf));
        }
    }

    return 0;
err:
    return -1;
}

static int qbv_set_enabled(void *shell, unsigned int port_id, bool enabled)
{
    char file[SHELL_STORAGE_MAX_FILENAME] = {0};
    int old_state;
    char new_state;

    if (h_snprintf_strict(file, SHELL_STORAGE_MAX_FILENAME, CONFIG_STORAGE_ROOT "/qbv/port%u/enabled", port_id) < 0)
        goto err;

    if (enabled)
        new_state = '1';
    else
        new_state = '0';

    if ((storage_read_int(NULL, file, &old_state) < 0) || ((bool)old_state != enabled))
        if (storage_write(NULL, file, &new_state, 1) < 0)
            goto err;

    return 0;
err:
    return -1;
}

/**
 * return: 1 when config applied, 0 when qbv not enabled, -1 on error
 */
int qbv_apply_permanent(void *shell, unsigned int port_id)
{
    struct genavb_st_gate_control_entry gate_list[QBV_LIST_MAX_ENTRIES];
    struct genavb_st_config config = st_config_default;
    int rc = -1;

    config.control_list = gate_list;
    gate_list[0] = st_list_default;

    if (qbv_read_permanent(shell, port_id, &config) < 0) {
        shell_printf(shell, "qbv_read_permanent(%u) failed\n", port_id);
        goto err_get_config;
    }

    rc = qbv_apply(shell, port_id, &config);

err_get_config:
    return rc;
}

int qbv_apply(void *shell, unsigned int port_id, struct genavb_st_config *config)
{
    genavb_clock_id_t clk_id;
    int rc = -1;

    clk_id = port_clock_id[port_id];

    if (config->enable) {
        shell_printf(shell, "applying st configuration:\n");
        dump_genavb_st_config(shell, port_id, config);

        rc = genavb_st_set_admin_config(port_id, clk_id, config);
        if (rc < 0) {
            shell_printf(shell, "genavb_st_set_admin_config(%u) failed: %s\n", port_id, genavb_strerror(rc));
            goto err_set_config;
        }
        shell_printf(shell, "scheduled traffic config enabled\n");

        rc = 1;
    } else {
        rc = genavb_st_set_admin_config(port_id, clk_id, config);
        if (rc < 0) {
            shell_printf(shell, "genavb_st_set_admin_config(%u) failed: %s\n", port_id, genavb_strerror(rc));
            goto err_set_config;
        }

        rc = 0;
    }

err_set_config:
    return rc;
}

static void print_qbv_set_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_QBV_SET_HELP);
}

int cmd_qbv_set(void *shell, int32_t argc, char **argv)
{
    struct genavb_st_gate_control_entry gate_list[QBV_LIST_MAX_ENTRIES];
    unsigned int gate_list_entry[3], num_entries = 0;
    struct genavb_st_config config = st_config_default;
    unsigned long long base_time;
    bool permanent = false;
    unsigned int port_id;
    unsigned long cycle_time, cycle_time_ext, tmp;
    int rc = 0, opt;

    gate_list[0] = st_list_default;

    config.control_list = &gate_list[0];

    if (argc < 2) {
        print_qbv_set_usage(shell);
        goto err;
    }

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;
    if (port_id >= CONFIG_APP_LOGICAL_PORTS) {
        shell_printf(shell, "invalid port_id %u\n", port_id);
        goto err;
    }

    rc = qbv_read_permanent(shell, port_id, &config);

    config.enable = 1;

    rtos_getopt_init(2);
    while ((opt = rtos_getopt(argc, argv, "b:c:C:l:p")) != -1) {
        switch (opt) {
        case 'b':
            h_strtoull(&base_time, rtos_getopt_optarg(), NULL, 0);
            config.base_time = base_time;
            break;
        case 'c':
            h_strtoul(&cycle_time, rtos_getopt_optarg(), NULL, 0);
            config.cycle_time_p = cycle_time;
            break;
        case 'C':
            h_strtoul(&cycle_time_ext, rtos_getopt_optarg(), NULL, 0);
            config.cycle_time_ext = cycle_time_ext;
            break;
        case 'l':
            rc = sscanf(rtos_getopt_optarg(), "%2x,%u,%u", &gate_list_entry[0], &gate_list_entry[1], &gate_list_entry[2]);
            if (rc < 2)
                goto err;

            if (num_entries >= QBV_LIST_MAX_ENTRIES) {
                shell_printf(shell, "number of entries must be less than or equal to %d\n", QBV_LIST_MAX_ENTRIES);
                goto err;
            }

            gate_list[num_entries].gate_states = gate_list_entry[0];
            gate_list[num_entries].time_interval = gate_list_entry[1];
            gate_list[num_entries].operation = rc == 3 ? gate_list_entry[2] : GENAVB_ST_SET_GATE_STATES;
            num_entries++;
            break;
        case 'p':
            permanent = true;
            break;
        default:
            break;
        }
    }

    if (num_entries)
        config.list_length = num_entries;

    if (permanent) {
        if (qbv_write_permanent(shell, port_id, &config) < 0) {
            shell_printf(shell, "qbv_write_permanent(%u) failed\n", port_id);
            goto err;
        }
    }

    rc = qbv_apply(shell, port_id, &config);
    if (rc < 0)
        goto err;

    return 0;

err:
    return -1;
}

static void print_qbv_get_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_QBV_GET_HELP);
}

int cmd_qbv_get(void *shell, int32_t argc, char **argv)
{
    struct genavb_st_config config = st_config_default;
    struct genavb_st_gate_control_entry gate_list[QBV_LIST_MAX_ENTRIES];
    genavb_st_config_type_t st_config_type = GENAVB_ST_OPER;
    unsigned int port_id;
    bool permanent = false;
    unsigned long tmp;
    int opt, rc;

    gate_list[0] = st_list_default;
    config.control_list = gate_list;

    if (argc < 2) {
        print_qbv_get_usage(shell);
        goto err;
    }

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;
    if (port_id >= CONFIG_APP_LOGICAL_PORTS) {
        shell_printf(shell, "invalid port_id %u\n", port_id);
        goto err;
    }

    rtos_getopt_init(2);
    while ((opt = rtos_getopt(argc, argv, "pt:")) != -1) {
        switch (opt) {
        case 'p':
            permanent = true;
            break;
        case 't':
            h_strtoul(&tmp, rtos_getopt_optarg(), NULL, 0);
            st_config_type = (genavb_st_config_type_t)tmp;
            break;
        default:
            break;
        }
    }


    if (permanent) {
        shell_printf(shell, "reading permanent configuration\n");

        if (qbv_read_permanent(shell, port_id, &config) < 0) {
            shell_printf(shell, "qbv_read_permanent(%u) failed\n", port_id);
            goto err_get_config;
        }
    } else {
        shell_printf(shell, "reading hardware configuration\n");

        rc = genavb_st_get_config(port_id, st_config_type, &config, QBV_LIST_MAX_ENTRIES);
        if (rc < 0) {
            shell_printf(shell, "genavb_st_get_config(%u) failed: %s\n", port_id, genavb_strerror(rc));
            goto err_get_config;
        }
    }

    dump_genavb_st_config(shell, port_id, &config);

    return 0;

err_get_config:
err:
    return -1;
}

static void print_qbv_disable_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_QBV_DISABLE_HELP);
}

int cmd_qbv_disable(void *shell, int32_t argc, char **argv)
{
    struct genavb_st_config config = {
        .enable = 0
    };
    unsigned int port_id = -1;
    genavb_clock_id_t clk_id;
    bool permanent = false;
    unsigned long tmp;
    int opt, rc;

    if (argc < 2) {
        print_qbv_disable_usage(shell);
        goto err;
    }

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;
    if (port_id >= CONFIG_APP_LOGICAL_PORTS) {
        shell_printf(shell, "invalid port_id %u\n", port_id);
        goto err;
    }

    rtos_getopt_init(2);
    while ((opt = rtos_getopt(argc, argv, "p")) != -1) {
        switch (opt) {
        case 'p':
            permanent = true;
            break;
        default:
            break;
        }
    }

    if (permanent) {
        if (qbv_set_enabled(shell, port_id, false) < 0) {
            shell_printf(shell, "qbv_set_enabled(%u, false) failed\n", port_id);
            goto err;
        }
    }

    clk_id = port_clock_id[port_id];

    rc = genavb_st_set_admin_config(port_id, clk_id, &config);
    if (rc < 0)
        shell_printf(shell, "genavb_st_set_admin_config(%u) failed: %s\n", port_id, genavb_strerror(rc));
    else
        shell_printf(shell, "scheduled traffic disabled on port %u\n", port_id);

    return 0;
err:
    return -1;
}

static void print_qbv_set_max_sdu_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_QBV_SET_MAX_SDU_HELP);
}

static int qbv_write_sdu_permanent(void *shell, unsigned int port_id, struct genavb_st_max_sdu *queue_max_sdu, unsigned int n)
{
    char dir[QOS_MAX_SDU_BUF_SIZE], queue[3];
    int i;

    if (h_snprintf_strict(dir, QOS_MAX_SDU_BUF_SIZE, CONFIG_STORAGE_ROOT "/qbv/port%u/max_sdu", port_id) < 0) {
        goto err;
    }

    if (storage_mkdir(dir, true) < 0) {
        goto err;
    }

    for (i = 0; i < n; i++) {
        h_snprintf(queue, 3, "q%u", queue_max_sdu[i].traffic_class);
        storage_write_uint(dir, queue, queue_max_sdu[i].queue_max_sdu);
    }

    return 0;

err:
    return -1;
}

static int qbv_read_sdu_permanent(void *shell, unsigned int port_id, struct genavb_st_max_sdu *queue_max_sdu)
{
    char dir[QOS_MAX_SDU_BUF_SIZE], queue[3];
    uint32_t sdu_value = QOS_MAX_SDU_DEFAULT;
    int i;

    if (h_snprintf_strict(dir, QOS_MAX_SDU_BUF_SIZE, CONFIG_STORAGE_ROOT "/qbv/port%u/max_sdu", port_id) < 0) {
        goto err;
    }

    for (i = 0; i < QOS_TRAFFIC_CLASS_MAX; i++) {
        h_snprintf(queue, 3, "q%u", i);
        if (storage_read_u32(dir, queue, &sdu_value) < 0) {
            continue;
        }

        queue_max_sdu[i].traffic_class = i;
        queue_max_sdu[i].queue_max_sdu = sdu_value;
    }

    return 0;

err:
    return -1;
}

int cmd_qbv_set_max_sdu(void *shell, int32_t argc, char **argv)
{
    unsigned int port_id;
    unsigned long tmp;
    unsigned int tc, n = QOS_TRAFFIC_CLASS_MAX;
    uint32_t sdu_value = QOS_MAX_SDU_BYTES;
    struct genavb_st_max_sdu max_sdu[QOS_TRAFFIC_CLASS_MAX];
    bool permanent = false;
    int opt, i, j, rc;

    if (argc < 2) {
        print_qbv_set_max_sdu_usage(shell);
        goto err;
    }

    for (j = 0;j < n;j++)
        max_sdu[j] = max_sdu_default[j];

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;
    if (port_id >= CONFIG_APP_LOGICAL_PORTS) {
        shell_printf(shell, "invalid port_id %u\n", port_id);
        goto err;
    }

    qbv_read_sdu_permanent(shell, port_id, max_sdu);

    rtos_getopt_init(2);
    while ((opt = rtos_getopt(argc, argv, "l:p")) != -1) {
        switch (opt) {
        case 'l':
            if (sscanf(rtos_getopt_optarg(), "%u,%"SCNu32, &tc, &sdu_value) != 2) {
                goto err;
            }

            if (tc >= QOS_TRAFFIC_CLASS_MAX) {
                goto err;
            }

            max_sdu[tc].queue_max_sdu = sdu_value;
            break;
        case 'p':
            permanent = true;
            break;
        default:
            break;
        }
    }

    if (permanent) {
        if (qbv_write_sdu_permanent(shell, port_id, max_sdu, n) < 0) {
            shell_printf(shell, "qbv_write_sdu_permanent(%u) failed\n", port_id);
            goto err;
        }
    }

    rc = genavb_st_set_max_sdu(port_id, max_sdu, n);
    if (rc < 0) {
        shell_printf(shell, "genavb_st_set_max_sdu(%u) failed: %s\n", port_id, genavb_strerror(rc));
        goto err;
    }

    shell_printf(shell, "port %u :\n", port_id);
    for (i = 0; i < n; i++) {
        shell_printf(shell, "traffic_clas %u : max_sdu %u value set\n", max_sdu[i].traffic_class , max_sdu[i].queue_max_sdu);
    }

    return 0;

err:
    return -1;
}

static void print_qbv_get_max_sdu_usage(void *shell)
{
    shell_printf(shell, "Usage: ");

    shell_printf(shell, CMD_QBV_GET_MAX_SDU_HELP);
}

static void dump_genavb_st_max_sdu(void *shell, unsigned int port_id, struct genavb_st_max_sdu *queue_max_sdu, unsigned int n, bool permanent)
{
    int i;

    shell_printf(shell, "port_id        %u\n", port_id);
    shell_printf(shell, "\nmax sdu table:\n");

    if (permanent) {
        shell_printf(shell, " traffic class | max sdu |\n");
        shell_printf(shell, "---------------+---------+\n");
        for (i = 0; i < n; i++) {
            shell_printf(shell, " %13u |", queue_max_sdu[i].traffic_class);
            shell_printf(shell, " %7"PRIu32" |", queue_max_sdu[i].queue_max_sdu);
            shell_printf(shell, "\n");
        }
    } else {
        shell_printf(shell, " traffic class | max sdu | transmission overrun |\n");
        shell_printf(shell, "---------------+---------+----------------------+\n");
        for (i = 0; i < n; i++) {
            shell_printf(shell, " %13u |", queue_max_sdu[i].traffic_class);
            shell_printf(shell, " %7"PRIu32" |", queue_max_sdu[i].queue_max_sdu);
            shell_printf(shell, " %20"PRIu64" |", queue_max_sdu[i].transmission_overrun);
            shell_printf(shell, "\n");
        }
    }
    shell_printf(shell, "\n");
}

int cmd_qbv_get_max_sdu(void *shell, int32_t argc, char **argv)
{
    struct genavb_st_max_sdu max_sdu[QOS_TRAFFIC_CLASS_MAX];
    unsigned int port_id;
    unsigned long tmp;
    int opt, rc, i;
    bool permanent = false;

    if (argc < 2) {
        print_qbv_get_max_sdu_usage(shell);
        goto err;
    }

    for (i = 0;i < QOS_TRAFFIC_CLASS_MAX;i++)
        max_sdu[i] = max_sdu_default[i];

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;
    if (port_id >= CONFIG_APP_LOGICAL_PORTS) {
        shell_printf(shell, "invalid port_id %u\n", port_id);
        goto err;
    }

    rtos_getopt_init(2);
    while ((opt = rtos_getopt(argc, argv, "p")) != -1) {
        switch (opt) {
        case 'p':
            permanent = true;
            break;
        default:
            break;
        }
    }

    if (permanent) {
        if (qbv_read_sdu_permanent(shell, port_id, max_sdu) < 0) {
            shell_printf(shell, "qbv_read_sdu_permanent(%u) failed\n", port_id);
            goto err;
        }
    } else {
        rc = genavb_st_get_max_sdu(port_id, max_sdu);
        if (rc < 0) {
            shell_printf(shell, "genavb_st_get_max_sdu(%u) failed: %s\n", port_id, genavb_strerror(rc));
            goto err;
        }
    }

    dump_genavb_st_max_sdu(shell, port_id, max_sdu, QOS_TRAFFIC_CLASS_MAX, permanent);

    return 0;

err:
    return -1;
}

static int qbv_apply_max_sdu_permanent(void *shell, unsigned int port_id)
{
    struct genavb_st_max_sdu max_sdu[QOS_TRAFFIC_CLASS_MAX];
    int rc, i;

    for (i = 0;i < QOS_TRAFFIC_CLASS_MAX;i++)
        max_sdu[i] = max_sdu_default[i];

    rc = qbv_read_sdu_permanent(shell, port_id, max_sdu);
    if (rc < 0) {
        goto err;
    }

    rc = genavb_st_set_max_sdu(port_id, max_sdu, QOS_TRAFFIC_CLASS_MAX);
    if (rc < 0) {
        shell_printf(shell, "genavb_st_set_max_sdu(%u) failed: %s\n", port_id, genavb_strerror(rc));
        goto err;
    }

    rc = 1;

err:
    return rc;
}

void cmd_qbv_init(void *shell)
{
    unsigned int port_id;

    for (port_id = 0; port_id < CONFIG_APP_LOGICAL_PORTS; port_id++) {
        qbv_apply_permanent(shell, port_id);

        qbv_apply_max_sdu_permanent(shell, port_id);
    }
}

#else
void cmd_qbv_init(void *shell) {return;}
int cmd_qbv_set(void *shell, int32_t argc, char **argv) {return -1;}
int cmd_qbv_get(void *shell, int32_t argc, char **argv) {return -1;}
int cmd_qbv_disable(void *shell, int32_t argc, char **argv) {return -1;}
int cmd_qbv_set_max_sdu(void *shell, int32_t argc, char **argv) {return -1;}
int cmd_qbv_get_max_sdu(void *shell, int32_t argc, char **argv) {return -1;}

int qbv_write_permanent(void *shell, unsigned int port_id, struct genavb_st_config *config) {return -1;}
int qbv_apply_permanent(void *shell, unsigned int port_id) {return -1;}
int qbv_apply(void *shell, unsigned int port_id, struct genavb_st_config *config) {return -1;}
#endif /* CONFIG_RTOS_APPS_QBV */
