/*
 * Copyright 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "genavb/error.h"
#include "genavb/fdb.h"
#include "genavb/helpers.h"

#include "rtos_abstraction_layer.h"
#include "rtos_apps/shell/common.h"
#include "rtos_apps/shell/fdb.h"
#include "rtos_apps/storage.h"
#include "rtos_apps/storage_common.h"

#include "common.h"
#include "storage.h"

#include "shell_config.h"

static void fdb_print_description(void *shell)
{
    shell_printf(shell, "\n");
    shell_printf(shell, "        mac        |  vid | dynamic | status |   forwarding   |    filtering   |\n");
    shell_printf(shell, "-------------------+------+---------+--------+----------------+----------------+\n");
}

static void fdb_print_entry(void *shell, uint8_t *address, uint16_t vid, bool dynamic,
        struct genavb_fdb_port_map *port_map, genavb_fdb_status_t status)
{
    char buf[15];
    int i, count;

    shell_printf(shell, " " RTOS_APPS_MAC_STR_FMT " |", RTOS_APPS_MAC_STR(address));
    shell_printf(shell, " %4u |", vid);
    shell_printf(shell, " %7s |", dynamic ? " true": "false");
    shell_printf(shell, " % 6d |", status);

    /* forwarding column */
    count = 0;
    buf[0] = '\0';
    for (i = 0; i < CONFIG_APP_BR_NUM_PORTS; i++) {
        if (port_map[i].control == GENAVB_FDB_PORT_CONTROL_FORWARDING) {
            if (!count)
                count += h_snprintf(buf + count, 15 - count, "%u", port_map[i].port_id);
            else
                count += h_snprintf(buf + count, 15 - count, ", %u", port_map[i].port_id);
        }
    }

    shell_printf(shell, " %14s |", buf);

    /* filtering column */
    count = 0;
    buf[0] = '\0';
    for (i = 0; i < CONFIG_APP_BR_NUM_PORTS; i++) {
        if (port_map[i].control == GENAVB_FDB_PORT_CONTROL_FILTERING) {
            if (!count)
                count += h_snprintf(buf + count, 15 - count, "%u", port_map[i].port_id);
            else
                count += h_snprintf(buf + count, 15 - count, ", %u", port_map[i].port_id);
        }
    }

    shell_printf(shell, " %14s |\n", buf);
}

static void print_fdb_update_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_FDB_UPDATE_HELP);
}

static int fdb_port_mask_2_port_map(uint32_t port_mask, struct genavb_fdb_port_map *map)
{
    int i;

    for (i = 0; i < CONFIG_APP_BR_NUM_PORTS; i++) {
        map[i].port_id = br_port_list[i];
        if (port_mask & (1 << br_port_list[i]))
            map[i].control = GENAVB_FDB_PORT_CONTROL_FORWARDING;
        else
            map[i].control = GENAVB_FDB_PORT_CONTROL_FILTERING;
    }

    return 0;
}

static int fdb_read_storage(uint8_t *mac, uint16_t vid, uint32_t *port_mask)
{
    char filename[SHELL_STORAGE_MAX_FILENAME];

    if (h_snprintf_strict(filename, SHELL_STORAGE_MAX_FILENAME, CONFIG_STORAGE_ROOT "/fdb/" RTOS_APPS_MAC_STR_FMT ",%u", RTOS_APPS_MAC_STR(mac), vid) < 0)
        return -1;

    return storage_read_u32(NULL, filename, port_mask);
}

static int fdb_write_storage(uint8_t *mac, uint16_t vid, uint32_t port_mask)
{
    char filename[SHELL_STORAGE_MAX_FILENAME] = {0};

    /* write '/fdb/xx:xx:xx:xx:xx:xx,vid' file with port bitmask value */
    if (h_snprintf_strict(filename, SHELL_STORAGE_MAX_FILENAME, CONFIG_STORAGE_ROOT "/fdb/" RTOS_APPS_MAC_STR_FMT ",%u", RTOS_APPS_MAC_STR(mac), vid) < 0)
        return -1;

    return storage_write_uint_hex(NULL, filename, port_mask);
}

static int fdb_get_storage_entry(unsigned int i, uint8_t *mac, uint16_t *vid, uint32_t *port_mask)
{
    char filename[SHELL_STORAGE_MAX_FILESIZE];
    int rc = -1;

    if (!storage_get_file(CONFIG_STORAGE_ROOT "/fdb", i, filename, SHELL_STORAGE_MAX_FILESIZE)) {
        unsigned int tmp[7];
        uint32_t tmp_port_mask;

        if (sscanf(filename, RTOS_APPS_MAC_STR_FMT ",%u", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5], &tmp[6]) != 7) {
            rc = -1;
            goto err;
        }

        mac[0] = tmp[0];
        mac[1] = tmp[1];
        mac[2] = tmp[2];
        mac[3] = tmp[3];
        mac[4] = tmp[4];
        mac[5] = tmp[5];
        *vid = tmp[6];

        if (fdb_read_storage(mac, *vid, &tmp_port_mask) < 0) {
            rc = -1;
            goto err;
        }

        *port_mask = tmp_port_mask;

        rc = 0;
    }

err:
    return rc;
}

static int fdb_delete_storage(uint8_t *mac, uint16_t vid)
{
    char filename[SHELL_STORAGE_MAX_FILENAME];

    if (h_snprintf_strict(filename, SHELL_STORAGE_MAX_FILENAME, CONFIG_STORAGE_ROOT "/fdb/" RTOS_APPS_MAC_STR_FMT ",%u", RTOS_APPS_MAC_STR(mac), vid) < 0)
        return -1;

    return storage_rm(filename, false, true);
}

static int fdb_read_permanent(uint8_t *address, uint16_t vid, bool *dynamic,
        struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
    uint32_t port_mask = 0;

    if (fdb_read_storage(address, vid, &port_mask) < 0)
        return -1;

    if (fdb_port_mask_2_port_map(port_mask, map) < 0)
        return -1;

    *dynamic = false;
    *status = GENAVB_FDB_STATUS_OTHER;

    return 0;
}

static int fdb_dump_permanent(uint32_t *next, uint8_t *address, uint16_t *vid, bool *dynamic,
        struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
    uint32_t port_mask = 0;
    int rc;

    rc = fdb_get_storage_entry(*next, address, vid, &port_mask);
    if (rc != 0)
        goto out;

    if (fdb_port_mask_2_port_map(port_mask, map) < 0)
        return -1;

    *dynamic = false;
    *status = GENAVB_FDB_STATUS_OTHER;
    *next += 1;

out:
    return rc;
}

static int fdb_update_permanent(void *shell, uint8_t *address, uint16_t vid, struct genavb_fdb_port_map *map)
{
    uint32_t port_mask = 0;

    if (storage_mkdir(CONFIG_STORAGE_ROOT "/fdb", true) < 0) {
        return -1;
    }

    fdb_read_storage(address, vid, &port_mask);

    if (map->control == GENAVB_FDB_PORT_CONTROL_FORWARDING) {
        port_mask |= (1 << map->port_id);
    } else if (map->control == GENAVB_FDB_PORT_CONTROL_FILTERING) {
        port_mask &= ~(1 << map->port_id);
    } else {
        shell_printf(shell, "Unknown control type\n");
        return -1;
    }

    return fdb_write_storage(address, vid, port_mask);
}

static int fdb_delete_permanent(uint8_t *address, uint16_t vid)
{
    uint32_t port_mask;

    if (fdb_read_storage(address, vid, &port_mask) < 0)
        return -1;

    return fdb_delete_storage(address, vid);
}

int cmd_fdb_update(void *shell, int32_t argc, char **argv)
{
    struct genavb_fdb_port_map port_map = {0};
    uint8_t address[6];
    uint16_t vid;
    bool permanent = false;
    unsigned long tmp;
    int opt, rc;

    if (argc < 4)
        goto err_usage;

    if (str2mac(argv[1], address) < 0) {
        shell_printf(shell, "invalid mac address format\n");
        goto err_usage;
    }

    h_strtoul(&tmp, argv[2], NULL, 0);
    vid = tmp;
    if (vid > 4095) {
        shell_printf(shell, "invalid vid value\n");
        goto err_usage;
    }

    h_strtoul(&tmp, argv[3], NULL, 0);
    port_map.port_id = tmp;
    if (port_map.port_id >= CONFIG_APP_LOGICAL_PORTS) {
        shell_printf(shell, "invalid port_id %u\n", port_map.port_id);
        goto err_usage;
    }

    port_map.control = GENAVB_FDB_PORT_CONTROL_FORWARDING;

    rtos_getopt_init(4);
    while ((opt = rtos_getopt(argc, argv, "c:p")) != -1) {
        switch (opt) {
        case 'c':
            h_strtoul(&tmp, rtos_getopt_optarg(), NULL, 0);
            port_map.control = (genavb_fdb_port_control_t)tmp;
            break;
        case 'p':
            permanent = true;
            break;
        default:
            break;
        }
    }

    if (port_map.control > GENAVB_FDB_PORT_CONTROL_FORWARDING) {
        shell_printf(shell, "invalid control value\n");
        goto err_usage;
    }

    if (permanent) {
        if (fdb_update_permanent(shell, address, vid, &port_map) < 0) {
            shell_printf(shell, "fdb_update_permanent(" RTOS_APPS_MAC_STR_FMT ", %u, %u) failed\n",
                RTOS_APPS_MAC_STR(address), vid, port_map.port_id);
            goto err;
        }
    }

    rc = genavb_fdb_update(address, vid, &port_map);
    if (rc < 0) {
        shell_printf(shell, "genavb_fdb_update(" RTOS_APPS_MAC_STR_FMT ", %u, %u) failed: %s\n",
            RTOS_APPS_MAC_STR(address), vid, port_map.port_id, genavb_strerror(rc));
        goto err;
    }

    shell_printf(shell, "FDB update port(%u) address(" RTOS_APPS_MAC_STR_FMT ") vid(%u) control(%u) permanent(%u)\n",
        port_map.port_id,
        RTOS_APPS_MAC_STR(address),
        vid, port_map.control, permanent);

    return 0;

err_usage:
        print_fdb_update_usage(shell);
err:
    return -1;
}

static void print_fdb_read_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_FDB_READ_HELP);
}

int cmd_fdb_read(void *shell, int32_t argc, char **argv)
{
    struct genavb_fdb_port_map port_map[CONFIG_APP_BR_NUM_PORTS] = {0};
    genavb_fdb_status_t status;
    uint8_t address[6];
    uint16_t vid;
    bool dynamic;
    bool permanent = false;
    unsigned long tmp;
    int opt, rc;

    if (argc < 3)
        goto err_usage;

    if (str2mac(argv[1], address) < 0) {
        shell_printf(shell, "invalid mac address format\n");
        goto err_usage;
    }

    h_strtoul(&tmp, argv[2], NULL, 0);
    vid = tmp;
    if (vid > 4095) {
        shell_printf(shell, "invalid vid value\n");
        goto err_usage;
    }

    rtos_getopt_init(3);
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
        if (fdb_read_permanent(address, vid, &dynamic, port_map, &status) < 0) {
            shell_printf(shell, "fdb_read_permanent(" RTOS_APPS_MAC_STR_FMT ", %u) failed\n",
                RTOS_APPS_MAC_STR(address), vid);
            goto err;
        }
    } else {
        rc = genavb_fdb_read(&address[0], vid, &dynamic, port_map, &status);
        if (rc < 0) {
            shell_printf(shell, "genavb_fdb_read(" RTOS_APPS_MAC_STR_FMT ", %u) failed: %s\n",
                RTOS_APPS_MAC_STR(address), vid, genavb_strerror(rc));
            goto err;
        }
    }

    fdb_print_description(shell);
    fdb_print_entry(shell, address, vid, dynamic, port_map, status);

    return 0;

err_usage:
    print_fdb_read_usage(shell);
err:
    return -1;
}

static void print_fdb_delete_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_FDB_DELETE_HELP);
}

int cmd_fdb_delete(void *shell, int32_t argc, char **argv)
{
    bool permanent = false;
    unsigned long tmp;
    uint8_t address[6];
    uint16_t vid;
    int opt, rc;

    if (argc < 3)
        goto err_usage;

    if (str2mac(argv[1], address)) {
        shell_printf(shell, "invalid mac address format\n");
        goto err_usage;
    }

    h_strtoul(&tmp, argv[2], NULL, 0);
    vid = tmp;
    if (vid > 4095) {
        shell_printf(shell, "invalid vid value\n");
        goto err_usage;
    }

    rtos_getopt_init(3);
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
        if (fdb_delete_permanent(address, vid) < 0) {
            shell_printf(shell, "fdb_delete_permanent(" RTOS_APPS_MAC_STR_FMT ", %u) failed\n",
                RTOS_APPS_MAC_STR(address), vid);
            goto err;
        }
    }

    rc = genavb_fdb_delete(address, vid);
    if (rc < 0) {
        shell_printf(shell, "genavb_fdb_delete(" RTOS_APPS_MAC_STR_FMT ", %u) failed: %s\n",
            RTOS_APPS_MAC_STR(address), vid, genavb_strerror(rc));
        goto err;
    }

    return 0;

err_usage:
    print_fdb_delete_usage(shell);
err:
    return -1;
}

static void print_fdb_dump_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_FDB_DUMP_HELP);
}

int cmd_fdb_dump(void *shell, int32_t argc, char **argv)
{
    struct genavb_fdb_port_map port_map[CONFIG_APP_BR_NUM_PORTS] = {0};
    genavb_fdb_status_t status = GENAVB_FDB_STATUS_INVALID;
    bool permanent = false;
    uint8_t address[6];
    uint32_t next = 0;
    uint16_t vid;
    bool dynamic;
    int opt;

    if (argc > 3)
        goto err_usage;

    rtos_getopt_init(1);
    while ((opt = rtos_getopt(argc, argv, "p")) != -1) {
        switch (opt) {
        case 'p':
            permanent = true;
            break;
        default:
            break;
        }
    }

    fdb_print_description(shell);

    if (permanent) {
        while (fdb_dump_permanent(&next, address, &vid, &dynamic, port_map, &status) == 0)
            fdb_print_entry(shell, address, vid, dynamic, port_map, status);
    } else {
        while (genavb_fdb_dump(&next, address, &vid, &dynamic, port_map, &status) == GENAVB_SUCCESS)
            fdb_print_entry(shell, address, vid, dynamic, port_map, status);
    }

    return 0;

err_usage:
    print_fdb_dump_usage(shell);
    return -1;
}

static int fdb_apply_permanent(void *shell)
{
    struct genavb_fdb_port_map port_map[CONFIG_APP_BR_NUM_PORTS] = {0};
    genavb_fdb_status_t status = GENAVB_FDB_STATUS_INVALID;
    uint8_t address[6];
    uint32_t next = 0;
    uint16_t vid;
    bool dynamic;
    int i, rc;

    while (fdb_dump_permanent(&next, address, &vid, &dynamic, port_map, &status) == 0) {
        for (i = 0; i < CONFIG_APP_BR_NUM_PORTS; i++) {
            rc = genavb_fdb_update(address, vid, &port_map[i]);
            if (rc < 0) {
                shell_printf(shell, "genavb_fdb_update(" RTOS_APPS_MAC_STR_FMT ", %u, %u) failed: %s\n",
                    RTOS_APPS_MAC_STR(address), vid, port_map[i].port_id, genavb_strerror(rc));
            }
        }
    }

    return 0;
}

void cmd_fdb_init(void *shell)
{
    fdb_apply_permanent(shell);
}
