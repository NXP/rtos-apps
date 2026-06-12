/*
 * Copyright 2020, 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "rtos_apps/storage/storage_app.h"
#include "rtos_apps/storage/storage.h"

#include "rtos_apps/shell/common.h"

#include "genavb/helpers.h"

#define MAX_FILE_SIZE          32

static int __storage_read_uint(const char *dirname, const char *filename, uint64_t *value)
{
    char buf[MAX_FILE_SIZE + 1];
    uint64_t tmp;
    int rc;

    rc = storage_read(dirname, filename, buf, MAX_FILE_SIZE);
    if (rc < 0)
        return -1;

    buf[rc] = '\0';

    errno = 0;

    tmp = strtoull(buf, NULL, 0);

    if (errno)
        return -1;

    *value = tmp;

    return 0;
}

static int __storage_read_int(const char *dirname, const char *filename, int64_t *value)
{
    char buf[MAX_FILE_SIZE + 1];
    int64_t tmp;
    int rc;

    rc = storage_read(dirname, filename, buf, MAX_FILE_SIZE);
    if (rc < 0)
        return -1;

    buf[rc] = '\0';

    errno = 0;

    tmp = strtoll(buf, NULL, 0);

    if (errno)
        return -1;

    *value = tmp;

    return 0;
}

int storage_read_ipv4_address(const char *dirname, const char *filename, uint8_t *addr)
{
    char buf[MAX_FILE_SIZE + 1];
    unsigned int tmp[4];
    int rc;

    rc = storage_read(dirname, filename, buf, MAX_FILE_SIZE);
    if (rc < 0)
        return -1;

    buf[rc] = '\0';

    if (sscanf(buf, "%u.%u.%u.%u", &tmp[0], &tmp[1], &tmp[2], &tmp[3]) != 4)
        return -1;

    addr[0] = tmp[0];
    addr[1] = tmp[1];
    addr[2] = tmp[2];
    addr[3] = tmp[3];

    return 0;
}

int storage_read_mac_address(const char *dirname, const char *filename, uint8_t *mac)
{
    char buf[MAX_FILE_SIZE + 1];
    int rc;

    rc = storage_read(dirname, filename, buf, MAX_FILE_SIZE);
    if (rc < 0)
        return -1;

    buf[rc] = '\0';

    if (str2mac(buf, mac) < 0)
        return -1;

    return 0;
}

int storage_read_qbv_entry(const char *dirname, const char *filename, uint8_t *mask, uint32_t *offset, uint8_t *state)
{
    char buf[MAX_FILE_SIZE + 1];
    unsigned int tmp[3];
    int rc, arg_val;

    rc = storage_read(dirname, filename, buf, MAX_FILE_SIZE);
    if (rc < 0)
        return -1;

    buf[rc] = '\0';

    arg_val = sscanf(buf, "%2x,%u,%u", &tmp[0], &tmp[1], &tmp[2]);

    if (arg_val < 2)
        return -1;

    *mask = tmp[0];
    *offset = tmp[1];

    if (arg_val == 3)
        *state = tmp[2];

    return 0;
}

int storage_read_uint(const char *dirname, const char *filename, unsigned int *value)
{
    uint64_t tmp;

    if (__storage_read_uint(dirname, filename, &tmp) < 0)
        return -1;

    *value = (unsigned int)tmp;

    return 0;
}

int storage_read_int(const char *dirname, const char *filename, int *value)
{
    int64_t tmp;

    if (__storage_read_int(dirname, filename, &tmp) < 0)
        return -1;

    *value = (int)tmp;

    return 0;
}

int storage_read_float(const char *dirname, const char *filename, float *value)
{
    char buf[MAX_FILE_SIZE + 1];
    float tmp;
    int rc;

    rc = storage_read(dirname, filename, buf, MAX_FILE_SIZE);
    if (rc < 0)
        return -1;

    buf[rc] = '\0';

    errno = 0;

    tmp = strtof(buf, NULL);

    if (errno)
        return -1;

    *value = tmp;

    return 0;
}

int storage_read_bool(const char *dirname, const char *filename, bool *value)
{
    uint64_t tmp;

    if (__storage_read_uint(dirname, filename, &tmp) < 0)
        return -1;

    *value = (bool)tmp;

    return 0;
}

int storage_read_u8(const char *dirname, const char *filename, uint8_t *value)
{
    uint64_t tmp;

    if (__storage_read_uint(dirname, filename, &tmp) < 0)
        return -1;

    *value = (uint8_t)tmp;

    return 0;
}

int storage_read_u16(const char *dirname, const char *filename, uint16_t *value)
{
    uint64_t tmp;

    if (__storage_read_uint(dirname, filename, &tmp) < 0)
        return -1;

    *value = (uint16_t)tmp;

    return 0;
}

int storage_read_u32(const char *dirname, const char *filename, uint32_t *value)
{
    uint64_t tmp;

    if (__storage_read_uint(dirname, filename, &tmp) < 0)
        return -1;

    *value = (uint32_t)tmp;

    return 0;
}

int storage_read_u64(const char *dirname, const char *filename, uint64_t *value)
{
    uint64_t tmp;

    if (__storage_read_uint(dirname, filename, &tmp) < 0)
        return -1;

    *value = tmp;

    return 0;
}

int storage_read_s8(const char *dirname, const char *filename, int8_t *value)
{
    int64_t tmp;

    if (__storage_read_int(dirname, filename, &tmp) < 0)
        return -1;

    *value = (int8_t)tmp;

    return 0;
}

int storage_read_s16(const char *dirname, const char *filename, int16_t *value)
{
    int64_t tmp;

    if (__storage_read_int(dirname, filename, &tmp) < 0)
        return -1;

    *value = (int16_t)tmp;

    return 0;
}

int storage_read_s32(const char *dirname, const char *filename, int32_t *value)
{
    int64_t tmp;

    if (__storage_read_int(dirname, filename, &tmp) < 0)
        return -1;

    *value = (int32_t)tmp;

    return 0;
}

int storage_write_uint_hex(const char *dirname, const char *filename, unsigned int value)
{
    char hex_str[12];

    if (h_snprintf_strict(hex_str, 12, "0x%08x", value) < 0)
        return -1;

    return storage_write(dirname, filename, hex_str, strlen(hex_str) + 1);
}

int storage_write_uint(const char *dirname, const char *filename, unsigned int value)
{
    char str[11];

    if (h_snprintf_strict(str, sizeof(str), "%u", value) < 0)
        return -1;

    return storage_write(dirname, filename, str, strlen(str) + 1);
}

int storage_write_u64(const char *dirname, const char *filename, uint64_t value)
{
    char str[22];
    unsigned int tmp[3];

    tmp[0] = value % 1000000000ULL;
    value /= 1000000000ULL;
    tmp[1] = value % 1000000000ULL;
    tmp[2] = value / 1000000000ULL;

    if (tmp[2]) {
        if (h_snprintf_strict(str, sizeof(str), "%u%09u%09u", tmp[2], tmp[1], tmp[0]) < 0)
            return -1;
    } else if (tmp[1]) {
        if (h_snprintf_strict(str, sizeof(str), "%u%09u", tmp[1], tmp[0]) < 0)
            return -1;
    } else {
        if (h_snprintf_strict(str, sizeof(str), "%u", tmp[0]) < 0)
            return -1;
    }

    return storage_write(dirname, filename, str, strlen(str) + 1);
}
