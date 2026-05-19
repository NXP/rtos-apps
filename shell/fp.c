/*
 * Copyright 2023-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifdef CONFIG_APP_FP

#include <stdio.h>

#include "genavb/frame_preemption.h"

#include "genavb/error.h"
#include "genavb/helpers.h"

#include "rtos_abstraction_layer.h"
#include "rtos_apps/storage.h"
#include "shell_config.h"
#include "rtos_apps/shell/fp.h"
#include "fp.h"

#define PATH_SZ         30

static void print_fp_set_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_FP_SET_HELP);
}

static void print_fp_get_usage(void *shell)
{
    shell_printf(shell, "Usage: ");
    shell_printf(shell, CMD_FP_GET_HELP);
}

static void read_fp_status_table(uint8_t status_fp, genavb_fp_admin_status_t *status_admin)
{
    int i;

    for (i = 0; i < QOS_PRIORITY_MAX; i++) {
        if (status_fp & (1 << i)) {
            status_admin[i] = GENAVB_FP_ADMIN_STATUS_EXPRESS;
        } else {
            status_admin[i] = GENAVB_FP_ADMIN_STATUS_PREEMPTABLE;
        }
    }
}

static void fp_config_print(void *shell, unsigned int port_id, unsigned int type, struct genavb_fp_config *config, bool fs)
{
    char status[QOS_PRIORITY_MAX + 1] = {0};
    int i, off = 0;

    switch (type) {
    case GENAVB_FP_CONFIG_802_1Q:
        shell_printf(shell, "\n802.1Q:\n");
        for (i = QOS_PRIORITY_MAX - 1; i >= 0; i--)
            off += h_snprintf(status+off, 2, "%s", config->u.cfg_802_1Q.admin_status[i] == GENAVB_FP_ADMIN_STATUS_PREEMPTABLE ? "p" : "E");

        shell_printf(shell, "admin status         : %s\n", status);
        if (!fs) {
            shell_printf(shell, "preemption active    : %u\n", config->u.cfg_802_1Q.preemption_active);
            shell_printf(shell, "hold request         : %s\n", config->u.cfg_802_1Q.hold_request == GENAVB_FP_HOLD_REQUEST_HOLD ? "HOLD" : "RELEASE");
            shell_printf(shell, "hold advance         : %u\n", config->u.cfg_802_1Q.hold_advance);
            shell_printf(shell, "release advance      : %u\n", config->u.cfg_802_1Q.release_advance);
        }

        break;

    case GENAVB_FP_CONFIG_802_3:
        shell_printf(shell, "\n802.3:\n");
        if (!fs) {
            shell_printf(shell, "support           : %u\n", config->u.cfg_802_3.support);
            shell_printf(shell, "status verify     : %u\n", config->u.cfg_802_3.status_verify);
            shell_printf(shell, "status tx         : %u\n", config->u.cfg_802_3.status_tx);
        }

        shell_printf(shell, "enable tx         : %u\n", config->u.cfg_802_3.enable_tx);
        shell_printf(shell, "verify disable tx : %u\n", config->u.cfg_802_3.verify_disable_tx);
        shell_printf(shell, "verify time       : %u\n", config->u.cfg_802_3.verify_time);
        shell_printf(shell, "add frag size     : %u\n", config->u.cfg_802_3.add_frag_size);
        break;

    default:
        break;
    }
}

static int fp_read_permanent(void *shell, unsigned int port_id, struct genavb_fp_config *config, unsigned int type)
{
    uint32_t enable_tx = 0, verify_disable_tx = 0, verify_time = 10, add_frag_size = 0;
    char path[PATH_SZ];
    uint8_t status_fp = 0xFF;
    int rc = -1;

    switch (type) {
    case GENAVB_FP_CONFIG_802_1Q:
        if (h_snprintf_strict(path, PATH_SZ, "/fp/port%u/802_1Q", port_id) < 0)
            goto out_802_1q;

        if (!storage_cd(path, true)) {
            storage_read_u8("admin_status", &status_fp);

            rc = 0;

            storage_cd("-", true);
        }

    out_802_1q:
        read_fp_status_table(status_fp, config->u.cfg_802_1Q.admin_status);

        break;

    case GENAVB_FP_CONFIG_802_3:
        if (h_snprintf_strict(path, PATH_SZ, "/fp/port%u/802_3", port_id) < 0)
            goto out_802_3;

        if (!storage_cd(path, true)) {
            storage_read_u32("enable_tx", &enable_tx);

            storage_read_u32("verify_disable_tx", &verify_disable_tx);

            storage_read_u32("verify_time", &verify_time);

            storage_read_u32("add_frag_size", &add_frag_size);

            rc = 0;

            storage_cd("-", true);
        }

    out_802_3:
        config->u.cfg_802_3.enable_tx = enable_tx;
        config->u.cfg_802_3.verify_disable_tx = verify_disable_tx;
        config->u.cfg_802_3.verify_time = verify_time;
        config->u.cfg_802_3.add_frag_size = add_frag_size;

        break;

    default:
        break;
    }

    return rc;
}

int fp_write_802_1q_permanent(void *shell, unsigned int port_id, struct genavb_fp_config config)
{
    char path[PATH_SZ], str_tmp[5];
    int i, rc = 0;
    uint8_t tmp = 0;

    if (h_snprintf_strict(path, PATH_SZ, "/fp/port%u/802_1Q", port_id) < 0)
        goto err;

    if (storage_mkdir(path, true) < 0) {
        goto err;
    }

    if (storage_cd(path, false) < 0) {
        shell_printf(shell, "Missing fp configuration\n");
        rc = -1;
        goto err;
    }

    for (i = 0; i < QOS_PRIORITY_MAX; i++) {
        if (config.u.cfg_802_1Q.admin_status[i] == GENAVB_FP_ADMIN_STATUS_EXPRESS) 
            tmp |= 1 << i;
    }

    h_snprintf(str_tmp, 5, "0x%02x", tmp);
    storage_write("admin_status", str_tmp, strlen(str_tmp)+1);

    storage_cd("-", false);
    return rc;
err:
    return rc;
}

int fp_write_802_3_permanent(void *shell, unsigned int port_id, struct genavb_fp_config config)
{
    char path[PATH_SZ] = {0};
    int rc = 0;

    if (h_snprintf_strict(path, PATH_SZ, "/fp/port%u/802_3", port_id) < 0)
        goto err;

    if (storage_mkdir(path, true) < 0) {
        goto err;
    }

    if (storage_cd(path, false) < 0) {
        shell_printf(shell, "Missing fp configuration\n");
        rc = -1;
        goto err;
    }

    storage_write_uint("enable_tx", config.u.cfg_802_3.enable_tx);
    storage_write_uint("verify_disable_tx", config.u.cfg_802_3.verify_disable_tx);
    storage_write_uint("verify_time", config.u.cfg_802_3.verify_time);
    storage_write_uint("add_frag_size", config.u.cfg_802_3.add_frag_size);

    storage_cd("-", false);
    return rc;
err:
    rc = -1;
    return rc;
}

int cmd_fp_set(void *shell, int32_t argc, char **argv)
{
    bool permanent = false, is_cf_8021 = false;
    struct genavb_fp_config config_8021q;
    struct genavb_fp_config config_8023;
    unsigned int port_id;
    uint8_t tmp_status;
    unsigned long tmp;
    int opt, rc;

    if (argc < 2) {
        print_fp_set_usage(shell);
        goto err;
    }

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;
    if (port_id >= CONFIG_APP_LOGICAL_PORTS) {
        shell_printf(shell, "invalid port_id %u\n", port_id);
        goto err;
    }

    fp_read_permanent(shell, port_id, &config_8021q, GENAVB_FP_CONFIG_802_1Q);
    fp_read_permanent(shell, port_id, &config_8023, GENAVB_FP_CONFIG_802_3);

    rtos_getopt_init(2);
    while ((opt = getopt(argc, argv, "qt:e:d:v:a:p")) != -1) {
        switch (opt) {
        case 'q':
            is_cf_8021 = true;
            break;

        case 't':
            h_strtoul(&tmp, optarg, NULL, 16);
            tmp_status = tmp;
            read_fp_status_table(tmp_status, config_8021q.u.cfg_802_1Q.admin_status);
            break;

        case 'e':
            h_strtoul(&tmp, optarg, NULL, 0);
            config_8023.u.cfg_802_3.enable_tx = tmp;
            if (config_8023.u.cfg_802_3.enable_tx > 2) {
                shell_printf(shell, "Bad argument enable preemption %u\n", config_8023.u.cfg_802_3.enable_tx);
                goto err;
            }
            break;

        case 'd':
            h_strtoul(&tmp, optarg, NULL, 0);
            config_8023.u.cfg_802_3.verify_disable_tx = tmp;
            if (config_8023.u.cfg_802_3.verify_disable_tx > 1) {
                shell_printf(shell, "Bad argument verify_disable_tx %u\n", config_8023.u.cfg_802_3.verify_disable_tx);
                goto err;
            }
            break;

        case 'v':
            h_strtoul(&tmp, optarg, NULL, 0);
            config_8023.u.cfg_802_3.verify_time = tmp;
            if (config_8023.u.cfg_802_3.verify_time < 1 || config_8023.u.cfg_802_3.verify_time > 128) {
                shell_printf(shell, "Bad argument verify_time %u\n", config_8023.u.cfg_802_3.verify_time);
                goto err;
            }
            break;

        case 'a':
            h_strtoul(&tmp, optarg, NULL, 0);
            config_8023.u.cfg_802_3.add_frag_size = tmp;
            if (config_8023.u.cfg_802_3.add_frag_size > 3) {
                shell_printf(shell, "Bad argument add_frag_size %u\n", config_8023.u.cfg_802_3.add_frag_size);
                goto err;
            }
            break;

        case 'p':
            permanent = true;
            break;

        default:
            break;
        }
    }

    if (permanent) {
        if (is_cf_8021) {
            if (fp_write_802_1q_permanent(shell, port_id, config_8021q) < 0) {
                shell_printf(shell, "fp_write_802_1q_permanent(%u) failed\n", port_id);
                goto err;
            }
        }
    }

    if (is_cf_8021) {
        rc = genavb_fp_set(port_id, GENAVB_FP_CONFIG_802_1Q, &config_8021q);
        if (rc < 0) {
            shell_printf(shell, "genavb_fp_set(%u, 802.1Q) failed: %s\n", port_id, genavb_strerror(rc));
            goto err;
        }
    }

    if (permanent) {
        if (!is_cf_8021) {
            if (fp_write_802_3_permanent(shell, port_id, config_8023) < 0) {
                shell_printf(shell, "fp_write_802_3_permanent(%u) failed\n", port_id);
                goto err;
            }
        }
    }

    if (!is_cf_8021) {
        rc = genavb_fp_set(port_id, GENAVB_FP_CONFIG_802_3, &config_8023);
        if (rc < 0) {
            shell_printf(shell, "genavb_fp_set(%u, 802.3) failed: %s\n", port_id, genavb_strerror(rc));
            goto err;
        }
    }

    return 0;
err:
    return -1;
}

int cmd_fp_get(void *shell, int32_t argc, char **argv)
{
    struct genavb_fp_config config;
    bool permanent = false;
    unsigned int port_id;
    unsigned long tmp;
    int opt, rc;

    if (argc < 2) {
        goto err_usage;
    }

    h_strtoul(&tmp, argv[1], NULL, 0);
    port_id = tmp;
    if (port_id >= CONFIG_APP_LOGICAL_PORTS) {
        shell_printf(shell, "invalid port_id %u\n", port_id);
        goto err;
    }

    rtos_getopt_init(2);
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
        if (fp_read_permanent(shell, port_id, &config, GENAVB_FP_CONFIG_802_1Q) < 0) {
            shell_printf(shell, "fp_read_permanent(%u, 802.1Q) failed\n", port_id);
            goto err;
        }

    } else {
        rc = genavb_fp_get(port_id, GENAVB_FP_CONFIG_802_1Q, &config);
        if (rc < 0) {
            shell_printf(shell, "genavb_fp_get(%u, 802.1Q) failed: %s\n", port_id, genavb_strerror(rc));
            goto err;
        }
    }

    fp_config_print(shell, port_id, GENAVB_FP_CONFIG_802_1Q, &config, permanent);

    if (permanent) {
        if (fp_read_permanent(shell, port_id, &config, GENAVB_FP_CONFIG_802_3) < 0) {
            shell_printf(shell, "fp_read_permanent(%u, 802.3) failed\n", port_id);
            goto err;
        }

    } else {
        rc = genavb_fp_get(port_id, GENAVB_FP_CONFIG_802_3, &config);
        if (rc < 0) {
            shell_printf(shell, "genavb_fp_get(%u, 802.3) failed: %s\n", port_id, genavb_strerror(rc));
            goto err;
        }
    }

    fp_config_print(shell, port_id, GENAVB_FP_CONFIG_802_3, &config, permanent);

    return 0;
err_usage:
    print_fp_get_usage(shell);
err:
    return -1;
}

int fp_apply_permanent(void *shell, unsigned int port_id)
{
    struct genavb_fp_config cfg;
    int rc = 0;

    if (fp_read_permanent(shell, port_id, &cfg, GENAVB_FP_CONFIG_802_1Q) == 0) {
        rc = genavb_fp_set(port_id, GENAVB_FP_CONFIG_802_1Q, &cfg);
        if (rc < 0) {
            shell_printf(shell, "genavb_fp_set(%u, 802.1Q) failed: %s\n", port_id, genavb_strerror(rc));
            goto err;
        }
    }

    if (fp_read_permanent(shell, port_id, &cfg, GENAVB_FP_CONFIG_802_3) == 0) {
        rc = genavb_fp_set(port_id, GENAVB_FP_CONFIG_802_3, &cfg);
        if (rc < 0) {
            shell_printf(shell, "genavb_fp_set(%u, 802.3) failed: %s\n", port_id, genavb_strerror(rc));
        }
    }

err:
    return rc;
}

int fp_apply(void *shell, unsigned int port_id, struct genavb_fp_config *config_fp_8021q, struct genavb_fp_config *config_fp_8023)
{
    int rc;

    rc = genavb_fp_set(port_id, GENAVB_FP_CONFIG_802_1Q, config_fp_8021q);
    if (rc < 0) {
        shell_printf(shell, "genavb_fp_set(%u, 802.1Q) failed: %s\n", port_id, genavb_strerror(rc));
        goto err;
    }

    rc = genavb_fp_set(port_id, GENAVB_FP_CONFIG_802_3, config_fp_8023);
    if (rc < 0) {
        shell_printf(shell, "genavb_fp_set(%u, 802.3) failed: %s\n", port_id, genavb_strerror(rc));
        goto err;
    }

err:
    return rc;
}

void cmd_fp_init(void *shell)
{
    unsigned int port_id;

    for (port_id = 0; port_id < CONFIG_APP_LOGICAL_PORTS; port_id++)
        if (fp_apply_permanent(shell, port_id) < 0)
            log_err("fp_apply_permanent(%u) failed\n", port_id);
}

#else
#include "fp.h"

void cmd_fp_init_shell(void *shell) {return;}
int fp_write_802_1q_permanent(void *shell, unsigned int port_id, struct genavb_fp_config config) {return -1;}
int fp_write_802_3_permanent(void *shell, unsigned int port_id, struct genavb_fp_config config) {return -1;}
int fp_apply_permanent(void *shell, unsigned int port_id) {return -1;}
#endif /* CONFIG_APP_FP */
