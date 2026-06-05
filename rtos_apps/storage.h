/*
 * Copyright 2020, 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_APPS_STORAGE_H_
#define _RTOS_APPS_STORAGE_H_

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

/**
 * @brief Initialize the storage module.
 *
 * Prepares the file system by either mouting and formatting it or check
 * it has properly been mounted by the OS and sets up the internal
 * storage context. Must be called before any other storage function.
 *
 * @retval 0 on success;
 * @retval -1 on error.
 */
int storage_init(void);

/**
 * @brief Set the shell handle used for storage log output.
 *
 * Assigns the shell instance to be used by storage functions to report
 * errors and informational messages.
 *
 * @param shell  Pointer to the shell instance.
 *
 * @retval 0 on success;
 * @retval -1 on error.
 */
int storage_set_shell(void *shell);

/**
 * @brief De-initialize the storage module.
 *
 * Unmounts the file system and releases the internal storage context.
 * No storage function should be called after this.
 */
void storage_exit(void);

/**
 * @brief Retrieve LittleFS system mount point.
 *
 * Returns a pointer to the mount structure used by the storage module.
 * This can be used to pass the file system context to other subsystems
 * (e.g., LittleFS-based APIs).
 *
 * @return Pointer to the file system structure, or NULL if not initialized.
 */
void *storage_get_lfs(void);

#else
static inline int storage_get_dir(const char *dirname, unsigned int n, char *subdirname, unsigned int len)
{
    return -1;
}
static inline int storage_get_file(const char *dirname, unsigned int n, char *filename, unsigned int len)
{
    return -1;
}
static inline int storage_get(const char *dirname, unsigned int n, void *file_info)
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
static inline int storage_read(const char *dirname, const char *filename, char *buf, unsigned int len)
{
    return -1;
}
static inline int storage_write(const char *dirname, const char *filename, const char *buf, unsigned int len)
{
    return -1;
}
static inline int storage_init(void)
{
    return -1;
}
static inline int storage_set_shell(void *shell)
{
    return -1;
}
static inline void storage_exit(void)
{
    return;
}

#endif /* CONFIG_RTOS_APPS_STORAGE */

#endif /* _RTOS_APPS_STORAGE_H_ */
