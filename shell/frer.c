/*
 * Copyright 2023-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdio.h>

#include "genavb/error.h"
#include "genavb/frer.h"
#include "genavb/helpers.h"

#include "rtos_abstraction_layer.h"
#include "rtos_apps/storage.h"
#include "rtos_apps/storage_common.h"

#include "shell_config.h"
#include "rtos_apps/shell/frer.h"

#define PATH_MAX_SIZE          50
#define BUF_MAX_SIZE           31
#define STREAM_HANDLE_MAX      32
#define STREAM_HANDLE_DEFAULT  0
#define PORT_MAX               6
#define PORT_DEFAULT           2

static const struct genavb_sequence_generation seqg_default = {
    .stream_n = 1,
    .stream = NULL,
    .direction_out_facing = false,
    .reset = false,
};

static const struct genavb_sequence_recovery seqr_default = {
    .stream_n = 1,
    .stream = NULL,

    .port_n = 1,
    .port = NULL,

    .direction_out_facing = true,
    .reset = false,

    .algorithm = GENAVB_SEQR_VECTOR,

    .history_length = 2,

    .reset_timeout = 100,

    .take_no_sequence = false,
    .individual_recovery = false,

    .latent_error_detection = false,
};

static const struct genavb_sequence_identification seqi_default = {
    .stream_n = 1,
    .stream = NULL,

    .active = false,

    .encapsulation = GENAVB_SEQI_RTAG,

    .path_id_lan_id = 0,
};

static void list_u32_to_buf(uint32_t *array, unsigned int n, char *buf, unsigned int len)
{
    unsigned int off = 0;
    int i;

    if (n)
        off = h_snprintf(buf + off, len, "%" PRIu32, array[0]);
    else
        off = h_snprintf(buf + off, len, " ");

    for (i = 1; i < n; i++)
        off += h_snprintf(buf + off, len - off, ",%" PRIu32, array[i]);
}

static void buf_to_list_u32(char *buf, uint32_t *array, unsigned int *n, unsigned int n_max)
{
    unsigned long tmp;
    char *token;
    int i = 0;

    token = strtok(buf, ",");

    while (token && (i < n_max)) {
        h_strtoul(&tmp, token, NULL, 0);
        array[i] = tmp;
        i++;
        token = strtok(NULL, ",");
    }

    if (i)
        *n = i;
}

static void list_uint_to_buf(unsigned int *array, unsigned int n, char *buf, unsigned int len)
{
    unsigned int off = 0;
    int i;

    if (n)
        off = h_snprintf(buf + off, len, "%u", array[0]);
    else
        off = h_snprintf(buf + off, len, " ");

    for (i = 1; i < n; i++)
        off += h_snprintf(buf + off, len - off, ",%u", array[i]);
}

static void buf_to_list_uint(char *buf, unsigned int *array, unsigned int *n, unsigned int n_max)
{
    unsigned long tmp; 
    char *token;
    int i = 0;

    token = strtok(buf, ",");

    while (token && (i < n_max)) {
        h_strtoul(&tmp, token, NULL, 0);
        array[i] = tmp;
        i++;
        token = strtok(NULL, ",");
    }

    if (i)
        *n = i;
}

static int seqg_update_permanent(uint32_t index, struct genavb_sequence_generation *entry)
{
    char path[PATH_MAX_SIZE];
    char buf[BUF_MAX_SIZE];

    if (h_snprintf_strict(path, PATH_MAX_SIZE, "/seqg/%" PRIu32, index) < 0)
        goto err;

    if (storage_mkdir(path, true) < 0)
        goto err;

    list_u32_to_buf(entry->stream, entry->stream_n, buf, BUF_MAX_SIZE);

    storage_write(path, "handle", buf, strlen(buf));

    return 0;

err:
    return -1;
}

static int seqg_delete_permanent(uint32_t index)
{
    char path[PATH_MAX_SIZE];

    if (h_snprintf_strict(path, PATH_MAX_SIZE, "/seqg/%" PRIu32, index) < 0)
        return -1;

    return storage_rm(path, true, true);
}

static int seqg_read_permanent(uint32_t index, struct genavb_sequence_generation *entry, unsigned int stream_max)
{
    char path[PATH_MAX_SIZE];
    char buf[BUF_MAX_SIZE] = {0};

    /* permanent values */
    if (h_snprintf_strict(path, PATH_MAX_SIZE, "/seqg/%" PRIu32 "/handle", index) < 0)
        return -1;

    storage_read(NULL, path, buf, BUF_MAX_SIZE);

    buf_to_list_u32(buf, entry->stream, &entry->stream_n, stream_max);

    return 0;
}

static void seqg_apply_permanent(void *shell)
{
    struct genavb_sequence_generation entry;
    uint32_t stream[STREAM_HANDLE_MAX] = {STREAM_HANDLE_DEFAULT, };
    char subdirname[MAX_DIR_NAME_LEN];
    unsigned int i, index;
    int rc;

    i = 0;
    while (!storage_get_dir("/seqg", i, subdirname, MAX_DIR_NAME_LEN)) {
        i++;

        if (sscanf(subdirname, "%u", &index) != 1)
            continue;

        entry = seqg_default;
        entry.stream = stream;

        if (seqg_read_permanent(index, &entry, STREAM_HANDLE_MAX) == 0) {
            if ((rc = genavb_sequence_generation_update(index, &entry)) < 0)
                shell_printf(shell, "genavb_sequence_generation_update(%u) failed: %s\n", index, genavb_strerror(rc));
        }
    }
}

static void seqg_print(void *shell, uint32_t index, struct genavb_sequence_generation *entry)
{
    char buf[BUF_MAX_SIZE];

    list_u32_to_buf(entry->stream, entry->stream_n, buf, BUF_MAX_SIZE);

    shell_printf(shell, "\n");
    shell_printf(shell, " index |             handle              | direction_out_facing | reset | \n");
    shell_printf(shell, "-------+---------------------------------+----------------------+-------+\n");
    shell_printf(shell, " %5" PRIu32 " |", index);
    shell_printf(shell, " %31s |", buf);
    shell_printf(shell, " %20s |", entry->direction_out_facing ? "true" : "false");
    shell_printf(shell, " %5s |\n", entry->reset ? "true" : "false");
}

int cmd_seqg_update(void *shell, int32_t argc, char **argv)
{
    struct genavb_sequence_generation entry = seqg_default;
    uint32_t stream[STREAM_HANDLE_MAX] = {STREAM_HANDLE_DEFAULT, };
    uint32_t index;
    bool permanent = false;
    unsigned long tmp;
    int rc, opt;

    if (argc < 2)
        goto err;

    h_strtoul(&tmp, argv[1], NULL, 0);
    index = tmp;

    entry.stream = stream;

    seqg_read_permanent(index, &entry, STREAM_HANDLE_MAX);

    rtos_getopt_init(2);
    while ((opt = rtos_getopt(argc, argv, "h:rp")) != -1) {
        switch (opt) {
        case 'h':
            buf_to_list_u32(rtos_getopt_optarg(), entry.stream, &entry.stream_n, STREAM_HANDLE_MAX);
            break;

        case 'r':
            entry.reset = true;
            break;

        case 'p':
            permanent = true;
            break;

        default:
            break;
        }
    }

    if (entry.reset) {
        rc = genavb_sequence_generation_reset(index);
        if (rc < 0) {
            shell_printf(shell, "genavb_sequence_generation_reset(%u) failed: %s\n", index, genavb_strerror(rc));
            goto err;
        }
    } else {
        if (permanent)
            if (seqg_update_permanent(index, &entry) < 0)
                shell_printf(shell, "seqg_update_permanent(%u) failed\n", index);

        if ((rc = genavb_sequence_generation_update(index, &entry)) < 0) {
            shell_printf(shell, "genavb_sequence_generation_update(%u) failed: %s\n", index, genavb_strerror(rc));
            goto err;
        }
    }

    return 0;

err:
    shell_printf(shell, CMD_SEQG_UPDATE_HELP);

    return -1;
}

int cmd_seqg_delete(void *shell, int32_t argc, char **argv)
{
    uint32_t index = 0;
    bool permanent = false;
    unsigned long tmp;
    int rc, opt;

    if (argc < 2)
        goto err;

    h_strtoul(&tmp, argv[1], NULL, 0);
    index = tmp;

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

    if (permanent)
        if (seqg_delete_permanent(index) < 0)
            shell_printf(shell, "seqg_delete_permanent(%u) failed\n", index);

    if ((rc = genavb_sequence_generation_delete(index)) < 0)
        shell_printf(shell, "genavb_sequence_generation_delete(%u) failed %s\n", index, genavb_strerror(rc));

    return 0;

err:
    shell_printf(shell, CMD_SEQG_DELETE_HELP);

    return -1;
}

int cmd_seqg_read(void *shell, int32_t argc, char **argv)
{
    struct genavb_sequence_generation entry = seqg_default;
    uint32_t stream[STREAM_HANDLE_MAX] = {STREAM_HANDLE_DEFAULT, };
    uint32_t index = 0;
    bool permanent = false;
    unsigned long tmp;
    int rc, opt;

    if (argc < 2)
        goto err_usage;

    h_strtoul(&tmp, argv[1], NULL, 0);
    index = tmp;

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

    entry.stream = stream;

    if (permanent) {
        if (seqg_read_permanent(index, &entry, STREAM_HANDLE_MAX) < 0) {
            shell_printf(shell, "seqg_read_permanent(%u) failed\n", index);
            goto err;
        }
    } else {
        entry.stream_n = STREAM_HANDLE_MAX;
        if ((rc = genavb_sequence_generation_read(index, &entry)) < 0) {
            shell_printf(shell, "genavb_sequence_generation_read(%u) failed: %s\n", index, genavb_strerror(rc));
            goto err;
        }
    }

    seqg_print(shell, index, &entry);

    return 0;

err_usage:
    shell_printf(shell, CMD_SEQG_READ_HELP);

err:
    return -1;
}

static int seqr_update_permanent(uint32_t index, struct genavb_sequence_recovery *entry)
{
    char path[PATH_MAX_SIZE];
    char buf[BUF_MAX_SIZE];

    if (h_snprintf_strict(path, PATH_MAX_SIZE, "/seqr/%" PRIu32, index) < 0)
        goto err;

    if (storage_mkdir(path, true) < 0)
        goto err;

    list_u32_to_buf(entry->stream, entry->stream_n, buf, BUF_MAX_SIZE);

    storage_write(path, "handle", buf, strlen(buf));

    list_uint_to_buf(entry->port, entry->port_n, buf, BUF_MAX_SIZE);

    storage_write(path, "port", buf, strlen(buf));

    storage_write_uint(path, "algorithm", entry->algorithm);

    storage_write_uint(path, "history_length", entry->history_length);

    storage_write_uint(path, "reset_timeout", entry->reset_timeout);

    storage_write_uint(path, "take_no_sequence", entry->take_no_sequence);

    storage_write_uint(path, "individual_recovery", entry->individual_recovery);

    storage_write_uint(path, "latent_error_detection", entry->latent_error_detection);

    return 0;

err:
    return -1;
}

static int seqr_delete_permanent(uint32_t index)
{
    char path[PATH_MAX_SIZE];

    if (h_snprintf_strict(path, PATH_MAX_SIZE, "/seqr/%" PRIu32, index) < 0)
        return -1;

    return storage_rm(path, true, true);
}

static int seqr_read_permanent(uint32_t index, struct genavb_sequence_recovery *entry, unsigned int stream_max, unsigned int port_max)
{
    char path[PATH_MAX_SIZE];
    char buf[BUF_MAX_SIZE];
    unsigned int tmp = 0;

    /* permanent values */
    if (h_snprintf_strict(path, PATH_MAX_SIZE, "/seqr/%" PRIu32, index) < 0)
        goto err;

    storage_read(path, "handle", buf, BUF_MAX_SIZE);

    buf_to_list_u32(buf, entry->stream, &entry->stream_n, stream_max);

    storage_read(path, "port", buf, BUF_MAX_SIZE);

    buf_to_list_uint(buf, entry->port, &entry->port_n, port_max);

    storage_read_uint(path, "algorithm", &tmp);
    entry->algorithm = (genavb_seqr_algorithm_t)tmp;

    storage_read_u32(path, "history_length", &entry->history_length);

    storage_read_u32(path, "reset_timeout", &entry->reset_timeout);

    storage_read_bool(path, "take_no_sequence", &entry->take_no_sequence);

    storage_read_bool(path, "individual_recovery", &entry->individual_recovery);

    storage_read_bool(path, "latent_error_detection", &entry->latent_error_detection);

    return 0;

err:
    return -1;
}

static void seqr_apply_permanent(void *shell)
{
    struct genavb_sequence_recovery entry;
    uint32_t stream[STREAM_HANDLE_MAX] = {STREAM_HANDLE_DEFAULT, };
    unsigned int port[PORT_MAX] = {PORT_DEFAULT, };
    char subdirname[MAX_DIR_NAME_LEN];
    unsigned int i, index;
    int rc;

    i = 0;
    while (!storage_get_dir("/seqr", i, subdirname, MAX_DIR_NAME_LEN)) {
        i++;

        if (sscanf(subdirname, "%u", &index) != 1)
            continue;

        entry = seqr_default;
        entry.stream = stream;
        entry.port = port;

        if (seqr_read_permanent(index, &entry, STREAM_HANDLE_MAX, PORT_MAX) == 0) {
            if ((rc = genavb_sequence_recovery_update(index, &entry)) < 0)
                shell_printf(shell, "genavb_sequence_recovery_update(%u) failed %s\n", index, genavb_strerror(rc));
        }
    }
}

static void seqr_print(void *shell, uint32_t index, struct genavb_sequence_recovery *entry)
{
    char buf[BUF_MAX_SIZE];

    shell_printf(shell, "\n");
    shell_printf(shell, " index |              handle             |      port     |  direction_out_facing  |     algorithm    |    history_length   |\n");
    shell_printf(shell, "-------+---------------------------------+---------------+------------------------+------------------+---------------------+\n");

    shell_printf(shell, " %5" PRIu32 " |", index);

    list_u32_to_buf(entry->stream, entry->stream_n, buf, BUF_MAX_SIZE);
    shell_printf(shell, " %31s |", buf);

    list_uint_to_buf(entry->port, entry->port_n, buf, BUF_MAX_SIZE);
    shell_printf(shell, " %13s |", buf);

    shell_printf(shell, " %22s |", entry->direction_out_facing ? "true" : "false");
    shell_printf(shell, " % 16d |", entry->algorithm);
    shell_printf(shell, " %19u |\n", entry->history_length);

    shell_printf(shell, "       +---------------------------------+---------------+------------------------+------------------+---------------------+\n");
    shell_printf(shell, "       |               reset             | reset_timeout | invalid_sequence_value | take_no_sequence | individual_recovery |\n");
    shell_printf(shell, "       +---------------------------------+---------------+------------------------+------------------+---------------------+\n");

    shell_printf(shell, "       | %31s |", entry->reset ? "true" : "false");
    shell_printf(shell, " %13u |", entry->reset_timeout);
    shell_printf(shell, " %22u |", entry->invalid_sequence_value);
    shell_printf(shell, " %16s |", entry->take_no_sequence ? "true" : "false");
    shell_printf(shell, " %19s |\n", entry->individual_recovery ? "true" : "false");

    shell_printf(shell, "       +---------------------------------+---------------+------------------------+------------------+---------------------+\n");
    shell_printf(shell, "       |      latent_error_detection     |   difference  |         period         |       paths      |     reset_period    |\n");
    shell_printf(shell, "       +---------------------------------+---------------+------------------------+------------------+---------------------+\n");

    shell_printf(shell, "       | %31s |", entry->latent_error_detection ? "true" : "false");
    shell_printf(shell, " % 13" PRIi32 " |", entry->latent_error_parameters.difference);
    shell_printf(shell, " %22" PRIu32 " |", entry->latent_error_parameters.period);
    shell_printf(shell, " %16" PRIu16 " |", entry->latent_error_parameters.paths);
    shell_printf(shell, " %19" PRIu32 " |\n", entry->latent_error_parameters.reset_period);
}

int cmd_seqr_update(void *shell, int32_t argc, char **argv)
{
    struct genavb_sequence_recovery entry = seqr_default;
    uint32_t stream[STREAM_HANDLE_MAX] = {STREAM_HANDLE_DEFAULT, };
    unsigned int port[PORT_MAX] = {PORT_DEFAULT, };
    uint32_t index;
    bool permanent = false;
    unsigned long tmp;
    int rc, opt;

    if (argc < 2)
        goto err;

    h_strtoul(&tmp, argv[1], NULL, 0);
    index = tmp;

    entry.stream = stream;
    entry.port = port;

    seqr_read_permanent(index, &entry, STREAM_HANDLE_MAX, PORT_MAX);

    rtos_getopt_init(2);
    while ((opt = rtos_getopt(argc, argv, "h:P:a:rH:s:i:p")) != -1) {
        switch (opt) {
        case 'h':
            buf_to_list_u32(rtos_getopt_optarg(), entry.stream, &entry.stream_n, STREAM_HANDLE_MAX);
            break;

        case 'P':
            buf_to_list_uint(rtos_getopt_optarg(), entry.port, &entry.port_n, PORT_MAX);
            break;

        case 'a':
            h_strtoul(&tmp, rtos_getopt_optarg(), NULL, 0);
            entry.algorithm = (genavb_seqr_algorithm_t)tmp;
            break;

        case 'r':
            entry.reset = true;
            break;

        case 'H':
            h_strtoul(&tmp, rtos_getopt_optarg(), NULL, 0);
            entry.history_length = tmp;
            break;

        case 's':
            h_strtoul(&tmp, rtos_getopt_optarg(), NULL, 0);
            entry.take_no_sequence = tmp;
            break;

        case 'i':
            h_strtoul(&tmp, rtos_getopt_optarg(), NULL, 0);
            entry.individual_recovery = tmp;
            break;

        case 'p':
            permanent = true;
            break;

        default:
            break;
        }
    }

    if (permanent)
        if (seqr_update_permanent(index, &entry) < 0)
            shell_printf(shell, "seqr_update_permanent(%u) failed\n", index);

    if ((rc = genavb_sequence_recovery_update(index, &entry)) < 0)
        shell_printf(shell, "genavb_sequence_recovery_update(%u) failed %s\n", index, genavb_strerror(rc));

    return 0;

err:
    shell_printf(shell, CMD_SEQR_UPDATE_HELP);

    return -1;
}

int cmd_seqr_delete(void *shell, int32_t argc, char **argv)
{
    uint32_t index = 0;
    bool permanent = false;
    unsigned long tmp;
    int rc, opt;

    if (argc < 2)
        goto err;

    h_strtoul(&tmp, argv[1], NULL, 0);
    index = tmp;

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

    if (permanent)
        if (seqr_delete_permanent(index) < 0)
            shell_printf(shell, "seqr_delete_permanent(%u) failed\n", index);

    if ((rc = genavb_sequence_recovery_delete(index)) < 0)
        shell_printf(shell, "genavb_sequence_recovery_delete(%u) failed %s\n", index, genavb_strerror(rc));

    return 0;

err:
    shell_printf(shell, CMD_SEQR_DELETE_HELP);

    return -1;
}

int cmd_seqr_read(void *shell, int32_t argc, char **argv)
{
    struct genavb_sequence_recovery entry = seqr_default;
    uint32_t stream[STREAM_HANDLE_MAX] = {STREAM_HANDLE_DEFAULT, };
    unsigned int port[PORT_MAX] = {PORT_DEFAULT, };
    uint32_t index = 0;
    bool permanent = false;
    unsigned long tmp;
    int rc, opt;

    if (argc < 2)
        goto err_usage;

    h_strtoul(&tmp, argv[1], NULL, 0);
    index = tmp;

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

    entry.stream = stream;
    entry.port = port;

    if (permanent) {
        if (seqr_read_permanent(index, &entry, STREAM_HANDLE_MAX, PORT_MAX) < 0) {
            shell_printf(shell, "seqr_read_permanent(%u) failed\n", index);
            goto err;
        }
    } else {
        entry.stream_n = STREAM_HANDLE_MAX;
        entry.port_n = STREAM_HANDLE_MAX;
        if ((rc = genavb_sequence_recovery_read(index, &entry)) < 0) {
            shell_printf(shell, "genavb_sequence_recovery_read(%u) failed %s\n", index, genavb_strerror(rc));
            goto err;
        }
    }

    seqr_print(shell, index, &entry);

    return 0;

err_usage:
    shell_printf(shell, CMD_SEQR_READ_HELP);

err:
    return -1;
}

static int seqi_update_permanent(unsigned int port_id, struct genavb_sequence_identification *entry)
{
    char path[PATH_MAX_SIZE];
    char buf[BUF_MAX_SIZE];

    if (h_snprintf_strict(path, PATH_MAX_SIZE, "/seqi/%u", port_id) < 0)
        goto err;

    if (storage_mkdir(path, true) < 0)
        goto err;

    list_u32_to_buf(entry->stream, entry->stream_n, buf, BUF_MAX_SIZE);

    storage_write(path, "handle", buf, strlen(buf));

    storage_write_uint(path, "active", entry->active);

    storage_write_uint(path, "encapsulation", entry->encapsulation);

    storage_write_uint(path, "path_id_lan_id", entry->path_id_lan_id);

    return 0;

err:
    return -1;
}

static int seqi_delete_permanent(unsigned int port_id)
{
    char path[PATH_MAX_SIZE];

    if (h_snprintf_strict(path, PATH_MAX_SIZE, "/seqi/%u", port_id) < 0)
        return -1;

    return storage_rm(path, true, true);
}

static int seqi_read_permanent(unsigned int port_id, struct genavb_sequence_identification *entry, unsigned int stream_max)
{
    char path[PATH_MAX_SIZE];
    char buf[BUF_MAX_SIZE];
    unsigned int tmp = 0;

    /* permanent values */
    if (h_snprintf_strict(path, PATH_MAX_SIZE, "/seqi/%u", port_id) < 0)
        goto err;

    storage_read(path, "handle", buf, BUF_MAX_SIZE);

    buf_to_list_u32(buf, entry->stream, &entry->stream_n, stream_max);

    storage_read_bool(path, "active", &entry->active);

    storage_read_uint(path, "encapsulation", &tmp);
    entry->encapsulation = (genavb_seqi_encapsulation_t)tmp;

    storage_read_s8(path, "path_id_lan_id", &entry->path_id_lan_id);

    return 0;

err:
    return -1;
}

static void seqi_apply_permanent(void *shell)
{
    struct genavb_sequence_identification entry;
    uint32_t stream[STREAM_HANDLE_MAX] = {STREAM_HANDLE_DEFAULT, };
    char subdirname[MAX_DIR_NAME_LEN];
    unsigned int i, port_id;
    int rc;

    i = 0;
    while (!storage_get_dir("/seqi", i, subdirname, MAX_DIR_NAME_LEN)) {
        i++;

        if (sscanf(subdirname, "%u", &port_id) != 1)
            continue;

        entry = seqi_default;
        entry.stream = stream;

        if (seqi_read_permanent(port_id, &entry, STREAM_HANDLE_MAX) == 0) {
            if ((rc = genavb_sequence_identification_update(port_id, true, &entry)) < 0)
                shell_printf(shell, "genavb_sequence_identification_update(%u) failed %s\n",
                    port_id, genavb_strerror(rc));
        }
    }
}

static void seqi_print(void *shell, unsigned int port_id, struct genavb_sequence_identification *entry)
{
    char buf[BUF_MAX_SIZE];

    shell_printf(shell, "\n");
    shell_printf(shell, "  port |              handle             | active | encapsulation | path_id_lan_id |\n");
    shell_printf(shell, "-------+---------------------------------+--------+---------------+----------------+\n");

    shell_printf(shell, " %5u |", port_id);

    list_u32_to_buf(entry->stream, entry->stream_n, buf, BUF_MAX_SIZE);
    shell_printf(shell, " %31s |", buf);

    shell_printf(shell, " %6s |", entry->active ? "true" : "false");
    shell_printf(shell, " % 13d |", entry->encapsulation);
    shell_printf(shell, " % 14" PRIi8 " |\n", entry->path_id_lan_id);
}

int cmd_seqi_update(void *shell, int32_t argc, char **argv)
{
    struct genavb_sequence_identification entry = seqi_default;
    uint32_t stream[STREAM_HANDLE_MAX] = {STREAM_HANDLE_DEFAULT, };
    unsigned int port_id;
    bool permanent = false;
    unsigned long tmp;
    int rc, opt;

    if (argc < 2)
        goto err;

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;

    entry.stream = stream;

    seqi_read_permanent(port_id, &entry, STREAM_HANDLE_MAX);

    rtos_getopt_init(2);
    while ((opt = rtos_getopt(argc, argv, "h:ae:i:p")) != -1) {
        switch (opt) {
        case 'h':
            buf_to_list_u32(rtos_getopt_optarg(), entry.stream, &entry.stream_n, STREAM_HANDLE_MAX);
            break;

        case 'a':
            entry.active = true;
            break;

        case 'e':
            h_strtoul(&tmp, rtos_getopt_optarg(), NULL, 0);
            entry.encapsulation = (genavb_seqi_encapsulation_t)tmp;
            break;

        case 'i':
            h_strtoul(&tmp, rtos_getopt_optarg(), NULL, 0);
            entry.path_id_lan_id = tmp;
            break;

        case 'p':
            permanent = true;
            break;

        default:
            break;
        }
    }

    if (permanent)
        if (seqi_update_permanent(port_id, &entry) < 0)
        shell_printf(shell, "seqi_update_permanent(%u) failed\n", port_id);

    if ((rc = genavb_sequence_identification_update(port_id, true, &entry)) < 0)
        shell_printf(shell, "genavb_sequence_identification_update(%u) failed: %s\n", port_id, genavb_strerror(rc));

    return 0;

err:
    shell_printf(shell, CMD_SEQI_UPDATE_HELP);

    return -1;
}

int cmd_seqi_delete(void *shell, int32_t argc, char **argv)
{
    unsigned int port_id = 0;
    bool permanent = false;
    unsigned long tmp;
    int rc, opt;

    if (argc < 2)
        goto err;

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;

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

    if (permanent)
        if (seqi_delete_permanent(port_id) < 0)
            shell_printf(shell, "seqi_delete_permanent(%u) failed\n", port_id);

    if ((rc = genavb_sequence_identification_delete(port_id, true)) < 0)
        shell_printf(shell, "genavb_sequence_identification_delete(%u) failed: %s\n", port_id, genavb_strerror(rc));

    return 0;

err:
    shell_printf(shell, CMD_SEQI_DELETE_HELP);

    return -1;
}

int cmd_seqi_read(void *shell, int32_t argc, char **argv)
{
    struct genavb_sequence_identification entry = seqi_default;
    uint32_t stream[STREAM_HANDLE_MAX] = {STREAM_HANDLE_DEFAULT, };
    unsigned int port_id = 0;
    bool permanent = false;
    unsigned long tmp;
    int rc, opt;

    if (argc < 2)
        goto err_usage;

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;

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

    entry.stream = stream;

    if (permanent) {
        if (seqi_read_permanent(port_id, &entry, STREAM_HANDLE_MAX) < 0) {
            shell_printf(shell, "seqi_read_permanent(%u) failed\n", port_id);
            goto err;
        }
    } else {
        entry.stream_n = STREAM_HANDLE_MAX;
        if ((rc = genavb_sequence_identification_read(port_id, true, &entry)) < 0) {
            shell_printf(shell, "genavb_sequence_identification_read(%u) failed: %s\n", port_id, genavb_strerror(rc));
            goto err;
        }
    }

    seqi_print(shell, port_id, &entry);

    return 0;

err_usage:
    shell_printf(shell, CMD_SEQI_READ_HELP);

err:
    return -1;
}

void cmd_frer_init(void *shell)
{
    seqi_apply_permanent(shell);
    seqr_apply_permanent(shell);
    seqg_apply_permanent(shell);
}
