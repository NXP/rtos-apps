/*
 * Copyright 2023-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#include "genavb/error.h"
#include "genavb/stream_identification.h"
#include "genavb/helpers.h"

#include "rtos_apps/shell/common.h"
#include "storage.h"
#include "common.h"
#include "shell_config.h"
#include "rtos_apps/shell/stream_identification.h"

#define SI_DEFAULT_PORT_NUM     1
#define SI_DEFAULT_PORT_SIZE    CONFIG_APP_LOGICAL_PORTS
#define SI_DEFAULT_PORT {                           \
    [0 ... SI_DEFAULT_PORT_SIZE - 1] = {            \
        .id = 2,                                    \
        .pos = GENAVB_SI_PORT_POS_OUT_FACING_INPUT, \
    }                                               \
}

static const struct genavb_stream_identity si_default_entry = {
    .handle = 0,
    .type = GENAVB_SI_NULL,
    .port_n = 0,
    .port = NULL,
    .parameters.null.destination_mac = {0x00, 0x00, 0xaa, 0xbb, 0xcc, 0xdd},
    .parameters.null.tagged = GENAVB_SI_PRIORITY,
    .parameters.null.vlan = 0,
};

/* converts "1,2,3" type string into entry->port array */
static void si_parse_port_list(char *str, struct genavb_stream_identity *entry, unsigned int *n, unsigned int max)
{
    const char *sep = ",";
    int port_n = 0;
    char *token;
    unsigned int var;

    if (!entry->port)
        return;

    token = strtok(str, sep);
    while (token && (port_n < max)) {
        if (sscanf(token, "%u", &var) == 1) {
            entry->port[port_n].id = var;
            // FIXME get port position from user
            entry->port[port_n].pos = GENAVB_SI_PORT_POS_OUT_FACING_INPUT;
            port_n++;
        }
        token = strtok(NULL, sep);
    }

    if (port_n)
        *n = port_n;
}

static void si_print_entry(void *shell, uint32_t index, struct genavb_stream_identity *entry)
{
    char buf[15];
    int i, count;

    shell_printf(shell, "\n");
    shell_printf(shell, " index | handle |      port      |     type     |\n");
    shell_printf(shell, "-------+--------+----------------+--------------+\n");
    shell_printf(shell, " %5" PRIu32 " |", index);
    shell_printf(shell, " %6u |", entry->handle);

    // TODO add port position

    count = 0;
    buf[0] = '\0';
    for (i = 0; i < entry->port_n; i++) {
        if (!count)
            count += h_snprintf(buf + count, 15 - count, "%u", entry->port[i].id);
        else
            count += h_snprintf(buf + count, 15 - count, ", %u", entry->port[i].id);
    }

    shell_printf(shell, " %14s |", buf);

    switch (entry->type) {
    case GENAVB_SI_NULL:
        shell_printf(shell, "         null |\n");
        shell_printf(shell, "       +----------------------------------------+\n");
        shell_printf(shell, "       |       dst mac      |  tagged  |  vlan  |\n");
        shell_printf(shell, "       +--------------------+----------+--------+\n");
        shell_printf(shell, "       |  " RTOS_APPS_MAC_STR_FMT " |", RTOS_APPS_MAC_STR(entry->parameters.null.destination_mac));
        shell_printf(shell, " %8u |", entry->parameters.null.tagged);
        shell_printf(shell, " %6u |\n", entry->parameters.null.vlan);
        break;

    case GENAVB_SI_SRC_MAC_VLAN:
        shell_printf(shell, " src mac vlan |\n");
        shell_printf(shell, "       +----------------------------------------+\n");
        shell_printf(shell, "       |       src mac      |  tagged  |  vlan  |\n");
        shell_printf(shell, "       +--------------------+----------+--------+\n");
        shell_printf(shell, "       |  " RTOS_APPS_MAC_STR_FMT " |", RTOS_APPS_MAC_STR(entry->parameters.smac_vlan.source_mac));
        shell_printf(shell, " %8u |", entry->parameters.smac_vlan.tagged);
        shell_printf(shell, " %6u |\n", entry->parameters.smac_vlan.vlan);
        break;

    case GENAVB_SI_DST_MAC_VLAN:
    case GENAVB_SI_IP:
    case GENAVB_SI_MASK_AND_MATCH:
    default:
        break;
    }
}

static void list_u32_to_buf(void *data, uint32_t (*get_val)(void *data, unsigned int i), unsigned int n, char *buf, unsigned int len)
{
    unsigned int off = 0;
    int i;

    if (n)
        off = h_snprintf(buf + off, len, "%" PRIu32, get_val(data, 0));

    for (i = 1; i < n; i++)
        off += h_snprintf(buf + off, len - off, ",%" PRIu32, get_val(data, i));
}

static uint32_t get_port_id(void *data, unsigned int i)
{
    struct genavb_stream_identity *entry = data;
    return entry->port[i].id;
}

static int si_update_permanent(void *shell, uint32_t index, struct genavb_stream_identity *entry)
{
    char filename[MAX_FILENAME_LENGTH];
    char tmp_str[MAX_FILE_SIZE];

    if (h_snprintf_strict(filename, MAX_FILENAME_LENGTH, "/si/%lu", index) < 0)
        goto err;

    if (storage_mkdir(filename, true) < 0)
        goto err;

    if (storage_cd(filename, true) < 0)
        goto err;

    storage_write_uint("handle", entry->handle);

    list_u32_to_buf(entry, &get_port_id, entry->port_n, tmp_str, MAX_FILE_SIZE);
    storage_write("port_list", tmp_str, strlen(tmp_str));

    storage_write_uint("type", entry->type);

    switch (entry->type) {
    case GENAVB_SI_NULL:
        mac2str(entry->parameters.null.destination_mac, tmp_str, MAX_FILE_SIZE);
        storage_write("mac", tmp_str, strlen(tmp_str));

        storage_write_uint("tagged", entry->parameters.null.tagged);

        storage_write_uint("vlan", entry->parameters.null.vlan);

        break;

    case GENAVB_SI_SRC_MAC_VLAN:
        mac2str(entry->parameters.smac_vlan.source_mac, tmp_str, MAX_FILE_SIZE);
        storage_write("mac", tmp_str, strlen(tmp_str));

        storage_write_uint("tagged", entry->parameters.smac_vlan.tagged);

        storage_write_uint("vlan", entry->parameters.smac_vlan.vlan);

        break;

    default:
        break;
    }

    storage_cd("-", true);

    return 0;

err:
    return -1;
}

static int si_delete_permanent(void *shell, uint32_t index)
{
    char filename[MAX_FILENAME_LENGTH];

    if (h_snprintf_strict(filename, MAX_FILENAME_LENGTH, "/si/%lu", index) < 0)
        return -1;

    return storage_rm(filename, true, true);
}

static int si_read_permanent(void *shell, uint32_t index, struct genavb_stream_identity *entry)
{
    char filename[MAX_FILENAME_LENGTH];
    char tmp_str[MAX_FILE_SIZE];
    uint8_t tmp;

    if (h_snprintf_strict(filename, MAX_FILENAME_LENGTH, "/si/%lu", index) < 0)
        goto err;

    if (storage_cd(filename, true) < 0)
        goto err;

    storage_read_u32("handle", &entry->handle);

    if (storage_read("port_list", tmp_str, MAX_FILE_SIZE) > 0)
        si_parse_port_list(tmp_str, entry, &entry->port_n, SI_DEFAULT_PORT_SIZE);

    storage_read_u8("type", &tmp);
    entry->type = (genavb_si_t)tmp;

    switch (entry->type) {
    case GENAVB_SI_NULL:
        if (storage_read("mac", tmp_str, MAX_FILE_SIZE) > 0)
            str2mac(tmp_str, entry->parameters.null.destination_mac);

        storage_read_u8("tagged", &tmp);
        entry->parameters.null.tagged = (genavb_si_vlan_tag_t)tmp;

        storage_read_u16("vlan", &entry->parameters.null.vlan);

        break;

    case GENAVB_SI_SRC_MAC_VLAN:
        if (storage_read("mac", tmp_str, MAX_FILE_SIZE) > 0)
            str2mac(tmp_str, entry->parameters.smac_vlan.source_mac);

        storage_read_u8("tagged", &tmp);
        entry->parameters.smac_vlan.tagged = (genavb_si_vlan_tag_t)tmp;

        storage_read_u16("vlan", &entry->parameters.smac_vlan.vlan);

        break;

    default:
        break;
    }

    storage_cd("-", true);

    return 0;

err:
    return -1;
}

static int si_update_parse_optional_arguments(void *shell, int32_t argc, char **argv, bool *permanent, struct genavb_stream_identity *entry)
{
    genavb_si_vlan_tag_t tagged;
    genavb_si_t type;
    uint32_t handle;
    uint16_t vlan = 0;
    unsigned long tmp;
    int opt;
    int rc = 0;
    char *endptr = NULL;

    optind = 2;
    while ((opt = getopt(argc, argv, "h:P:t:m:T:v:p")) != -1) {
        switch (opt) {
        case 'h':
            h_strtoul(&tmp, optarg, &endptr, 0);
            handle = tmp;
            if (*endptr) {
                shell_printf(shell, "invalid handle value\n");
                return -1;
            }
            entry->handle = handle;
            break;
        case 'P':
            si_parse_port_list(optarg, entry, &entry->port_n, SI_DEFAULT_PORT_SIZE);
            if (!entry->port_n) {
                shell_printf(shell, "Invalid port number\n");
                return -1;
            }
            break;
        case 't':
            h_strtoul(&tmp, optarg, &endptr, 0);
            type = (genavb_si_t)tmp;
            if (*endptr || type < GENAVB_SI_NULL || type > GENAVB_SI_MASK_AND_MATCH) {
                shell_printf(shell, "invalid type\n");
                return -1;
            }
            entry->type = type;
            break;
        case 'm':
            switch (entry->type) {
            case GENAVB_SI_NULL:
                rc = str2mac(optarg, entry->parameters.null.destination_mac);
                break;
            case GENAVB_SI_SRC_MAC_VLAN:
                rc = str2mac(optarg, entry->parameters.smac_vlan.source_mac);
                break;
            default:
                break;
            }
            if (rc < 0) {
                shell_printf(shell, "invalid mac address\n");
                return -1;
            }
            break;
        case 'T':
            h_strtoul(&tmp, optarg, &endptr, 0);
            tagged = (genavb_si_vlan_tag_t)tmp;
            if (*endptr || tagged < GENAVB_SI_TAGGED || tagged > GENAVB_SI_ALL) {
                shell_printf(shell, "invalid tagged value\n");
                return -1;
            }
            switch (entry->type) {
            case GENAVB_SI_NULL:
                entry->parameters.null.tagged = tagged;
                break;
            case GENAVB_SI_SRC_MAC_VLAN:
                entry->parameters.smac_vlan.tagged = tagged;
                break;
            default:
                break;
            }
            break;
        case 'v':
            h_strtoul(&tmp, optarg, &endptr, 0);
            vlan = tmp;
            if (*endptr || vlan > 4095) {
                shell_printf(shell, "invalid vlan value\n");
                return -1;
            }
            switch (entry->type) {
            case GENAVB_SI_NULL:
                entry->parameters.null.vlan = vlan;
                break;
            case GENAVB_SI_SRC_MAC_VLAN:
                entry->parameters.smac_vlan.vlan = vlan;
                break;
            default:
                break;
            }
            break;
        case 'p':
            *permanent = true;
            break;
        default:
            break;
        }
    }

    return 0;
}

static void si_update_print_usage(void *shell)
{
    shell_printf(shell, "Usage:");
    shell_printf(shell, CMD_SI_UPDATE_HELP);
}

int cmd_si_update(void *shell, int32_t argc, char **argv)
{
    struct genavb_si_port ports[SI_DEFAULT_PORT_SIZE] = SI_DEFAULT_PORT;
    struct genavb_stream_identity entry = si_default_entry;
    bool permanent = false;
    unsigned long tmp;
    uint32_t index;
    int rc;

    entry.port = ports;
    entry.port_n = SI_DEFAULT_PORT_NUM;

    if (argc < 2)
        goto err_usage;

    h_strtoul(&tmp, argv[1], NULL, 0);
    index = tmp;

    si_read_permanent(shell, index, &entry);

    if (si_update_parse_optional_arguments(shell, argc, argv, &permanent, &entry) < 0)
        goto err_usage;

    if (permanent) {
        if (si_update_permanent(shell, index, &entry) < 0) {
            shell_printf(shell, "si_update_permanent(%u) failed: Unable to save permanent entry\n", index);
        }
    }

    rc = genavb_stream_identification_update(index, &entry);
    if (rc < 0) {
        shell_printf(shell, "genavb_stream_identification_update(%u) failed: %s\n", index, genavb_strerror(rc));
        goto err;
    }

    return 0;

err_usage:
    si_update_print_usage(shell);

err:
    return -1;
}

static void si_delete_print_usage(void *shell)
{
    shell_printf(shell, "Usage:");
    shell_printf(shell, CMD_SI_DELETE_HELP);
}

int cmd_si_delete(void *shell, int32_t argc, char **argv)
{
    bool permanent = false;
    unsigned long tmp;
    uint32_t index;
    int opt, rc;

    if (argc < 2)
        goto err_usage;

    h_strtoul(&tmp, argv[1], NULL, 0);
    index = tmp;

    optind = 2;
    while ((opt = getopt(argc, argv, "p")) != -1) {
        switch (opt) {
        case 'p':
            permanent = true;
            break;
        default:
            break;
        }
    }

    if (permanent) {
        if (si_delete_permanent(shell, index) < 0) {
            shell_printf(shell, "si_delete_permanent(%u) failed: Unable to delete permanent entry\n", index);
            goto err;
        }
    } 

    rc = genavb_stream_identification_delete(index);
    if (rc < 0) {
        shell_printf(shell, "genavb_stream_identification_delete(%u) failed: %s\n", index, genavb_strerror(rc));
        goto err;
    }

    return 0;

err_usage:
    si_delete_print_usage(shell);
err:
    return -1;
}

static void si_read_print_usage(void *shell)
{
    shell_printf(shell, "Usage:");
    shell_printf(shell, CMD_SI_READ_HELP);
}

int cmd_si_read(void *shell, int32_t argc, char **argv)
{
    struct genavb_stream_identity entry = {0};
    struct genavb_si_port ports[CONFIG_APP_LOGICAL_PORTS];
    bool permanent = false;
    unsigned long tmp;
    uint32_t index;
    int opt, rc;

    if (argc < 2)
        goto err_usage;

    h_strtoul(&tmp, argv[1], NULL, 0);
    index = tmp;

    entry.port = ports;

    optind = 2;
    while ((opt = getopt(argc, argv, "p")) != -1) {
        switch (opt) {
        case 'p':
            permanent = true;
            break;
        default:
            break;
        }
    }

    if (permanent) {
        if (si_read_permanent(shell, index, &entry) < 0) {
            shell_printf(shell, "si_read_permanent(%u) failed: Unable to read permanent entry\n", index);
            goto err;
        }
    } else {
        rc = genavb_stream_identification_read(index, &entry);
        if (rc < 0) {
            shell_printf(shell, "genavb_stream_identification_read(%u) failed: %s\n", index, genavb_strerror(rc));
            goto err;
        }
    }

    si_print_entry(shell, index, &entry);

    return 0;

err_usage:
    si_read_print_usage(shell);
err:
    return -1;
}

static void si_apply_permanent(void *shell)
{
    struct genavb_si_port ports[SI_DEFAULT_PORT_SIZE] = SI_DEFAULT_PORT;
    struct genavb_stream_identity entry;
    char subdirname[MAX_DIR_NAME_LEN];
    unsigned int i, index;
    int rc;

    i = 0;
    while (!storage_get_dir("/si", i, subdirname, MAX_DIR_NAME_LEN)) {
        i++;

        if (sscanf(subdirname, "%u", &index) != 1)
            continue;

        entry = si_default_entry;
        entry.port = ports;
        entry.port_n = SI_DEFAULT_PORT_NUM;

        if (si_read_permanent(shell, index, &entry) == 0) {
            rc = genavb_stream_identification_update(index, &entry);
            if (rc < 0)
                shell_printf(shell, "genavb_stream_identification_update(%u) failed: %s\n", index, genavb_strerror(rc));
        }
    }
}

void cmd_stream_identification_init(void *shell)
{
    si_apply_permanent(shell);
}
