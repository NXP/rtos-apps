/*
 * Copyright 2020, 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_SHELL_STORAGE_APP_H_
#define _RTOS_APPS_SHELL_STORAGE_APP_H_

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#if defined(CONFIG_RTOS_APPS_STORAGE)

/**
 * @brief Get the name of the nth subdirectory in a directory.
 *
 * Iterates over the entries of @p dirname and retrieves the name of the nth
 * entry of type directory.
 *
 * @param dirname  Path to the directory to iterate.
 * @param n        Index of the subdirectory entry to retrieve.
 * @param subdirname  Buffer to store the retrieved subdirectory name.
 * @param len      Size of the @p subdirname buffer.
 *
 * @retval 0 on success;
 * @retval -1 on error or if the nth entry is not a directory.
 */
int storage_get_dir(const char *dirname, unsigned int n, char *subdirname, unsigned int len);

/**
 * @brief Get the name of the nth file in a directory.
 *
 * Iterates over the entries of @p dirname and retrieves the name of the nth
 * entry of type file.
 *
 * @param dirname   Path to the directory to iterate.
 * @param n         Index of the file entry to retrieve.
 * @param filename  Buffer to store the retrieved file name.
 * @param len       Size of the @p filename buffer.
 *
 * @retval 0 on success;
 * @retval -1 on error or if the nth entry is not a file.
 */
int storage_get_file(const char *dirname, unsigned int n, char *filename, unsigned int len);

/**
 * @brief Remove a file or directory.
 *
 * Deletes the file or directory pointed to by @p filename. If @p recursive
 * is set and the target is a directory, all its contents are deleted first.
 * If @p force is set, no error is reported if the target does not exist.
 *
 * @param filename   Path to the file or directory to remove.
 * @param recursive  If true, remove directory contents recursively.
 * @param force      If true, ignore errors when the target does not exist.
 *
 * @retval 0 on success;
 * @retval -1 on error.
 */
int storage_rm(const char *filename, bool recursive, bool force);

/**
 * @brief Create a directory.
 *
 * Creates the directory at @p dirname. If @p parent is set, all missing
 * intermediate directories in the path are created as well.
 *
 * @param dirname  Path of the directory to create.
 * @param parent   If true, create intermediate parent directories as needed.
 *
 * @retval 0 on success;
 * @retval -1 on error.
 */
int storage_mkdir(const char *dirname, bool parent);

#else

static inline int storage_get_dir(const char *dirname, unsigned int n, char *subdirname, unsigned int len)
{
    return -1;
}
static inline int storage_get_file(const char *dirname, unsigned int n, char *filename, unsigned int len)
{
    return -1;
}
static inline int storage_rm(const char *filename, bool recursive, bool force)
{
    return -1;
}
static inline int storage_mkdir(const char *dirname, bool parent)
{
    return -1;
}

#endif /* CONFIG_RTOS_APPS_STORAGE */

#endif /* _RTOS_APPS_SHELL_STORAGE_APP_H_ */
