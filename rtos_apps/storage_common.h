/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_STORAGE_COMMON_H_
#define _RTOS_APPS_STORAGE_COMMON_H_

#include <stdbool.h>
#include <stdint.h>

#if CONFIG_RTOS_APPS_STORAGE == 1

int storage_read_bool(const char *dirname, const char *filename, bool *value);
int storage_read_float(const char *dirname, const char *filename, float *value);
int storage_read_int(const char *dirname, const char *filename, int *value);

int storage_read_ipv4_address(const char *dirname, const char *filename, uint8_t *addr);
int storage_read_mac_address(const char *dirname, const char *filename, uint8_t *mac);
int storage_read_qbv_entry(const char *dirname, const char *filename, uint8_t *mask, uint32_t *offset, uint8_t *state);

int storage_read_s8(const char *dirname, const char *filename, int8_t *value);
int storage_read_s16(const char *dirname, const char *filename, int16_t *value);
int storage_read_s32(const char *dirname, const char *filename, int32_t *value);

int storage_read_uint(const char *dirname, const char *filename, unsigned int *value);
int storage_read_u8(const char *dirname, const char *filename, uint8_t *value);
int storage_read_u16(const char *dirname, const char *filename, uint16_t *value);
int storage_read_u32(const char *dirname, const char *filename, uint32_t *value);
int storage_read_u64(const char *dirname, const char *filename, uint64_t *value);

int storage_write_uint_hex(const char *dirname, const char *filename, unsigned int value);
int storage_write_uint(const char *dirname, const char *filename, unsigned int value);
int storage_write_u64(const char *dirname, const char *filename, uint64_t value);

#else
static inline int storage_read_ipv4_address(const char *dirname, const char *filename, uint8_t *addr)
{
    return -1;
}
static inline int storage_read_mac_address(const char *dirname, const char *filename, uint8_t *mac)
{
    return -1;
}
static inline int storage_read_qbv_entry(const char *dirname, const char *filename, uint8_t *mask, uint32_t *offset, uint8_t *state)
{
    return -1;
}
static inline int storage_read_uint(const char *dirname, const char *filename, unsigned int *value)
{
    return -1;
}
static inline int storage_read_int(const char *dirname, const char *filename, int *value)
{
    return -1;
}
static inline int storage_read_float(const char *dirname, const char *filename, float *value)
{
    return -1;
}
static inline int storage_read_bool(const char *dirname, const char *filename, bool *value)
{
    return -1;
}
static inline int storage_read_u8(const char *dirname, const char *filename, uint8_t *value)
{
    return -1;
}
static inline int storage_read_u16(const char *dirname, const char *filename, uint16_t *value)
{
    return -1;
}
static inline int storage_read_u32(const char *dirname, const char *filename, uint32_t *value)
{
    return -1;
}
static inline int storage_read_u64(const char *dirname, const char *filename, uint64_t *value)
{
    return -1;
}
static inline int storage_read_s8(const char *dirname, const char *filename, int8_t *value)
{
    return -1;
}
static inline int storage_read_s16(const char *dirname, const char *filename, int16_t *value)
{
    return -1;
}
static inline int storage_read_s32(const char *dirname, const char *filename, int32_t *value)
{
    return -1;
}
static inline int storage_write_uint_hex(const char *dirname, const char *filename, unsigned int value)
{
    return -1;
}
static inline int storage_write_uint(const char *dirname, const char *filename, unsigned int value)
{
    return -1;
}
static inline int storage_write_u64(const char *dirname, const char *filename, uint64_t value)
{
    return -1;
}

#endif /* CONFIG_RTOS_APPS_STORAGE */

#endif /* _RTOS_APPS_STORAGE_COMMON_H_ */
