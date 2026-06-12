/*
 * Copyright 2020, 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_STORAGE_STORAGE_APP_H_
#define _RTOS_APPS_STORAGE_STORAGE_APP_H_

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#if defined(CONFIG_RTOS_APPS_STORAGE)

/**
 * @brief Read the content of a file into a buffer.
 *
 * Opens the file identified by @p filename, located in @p dirname if provided,
 * and reads up to @p len bytes into @p buf.
 *
 * @param dirname   Path of the directory containing the file, or NULL if
 *                  @p filename is an absolute path.
 * @param filename  Name of the file to read, or absolute path if @p dirname
 *                  is NULL.
 * @param buf       Buffer where the read data is stored.
 * @param len       Size of @p buf in bytes.
 *
 * @retval >=0 number of bytes read on success;
 * @retval -1 on error.
 */
int storage_read(const char *dirname, const char *filename, char *buf, unsigned int len);

/**
 * @brief Write a buffer to a file.
 *
 * Creates or truncates the file identified by @p filename, located in
 * @p dirname if provided, and writes @p len bytes from @p buf into it.
 *
 * @param dirname   Path of the directory containing the file, or NULL if
 *                  @p filename is an absolute path.
 * @param filename  Name of the file to write, or absolute path if @p dirname
 *                  is NULL.
 * @param buf       Buffer containing the data to write.
 * @param len       Number of bytes to write from @p buf.
 *
 * @retval 0 on success;
 * @retval -1 on error.
 */
int storage_write(const char *dirname, const char *filename, const char *buf, unsigned int len);

#else

static inline int storage_read(const char *dirname, const char *filename, char *buf, unsigned int len)
{
    return -1;
}
static inline int storage_write(const char *dirname, const char *filename, const char *buf, unsigned int len)
{
    return -1;
}

#endif /* CONFIG_RTOS_APPS_STORAGE */

#endif /* _RTOS_APPS_STORAGE_STORAGE_APP_H_ */
