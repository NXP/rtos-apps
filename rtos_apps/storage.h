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

int storage_get_dir(const char *dirname, unsigned int n, char *subdirname, unsigned int len);
int storage_get_file(const char *dirname, unsigned int n, char *filename, unsigned int len);

int storage_rm(const char *filename, bool recursive, bool force);
int storage_mkdir(const char *dirname, bool parent);
int storage_read(const char *dirname, const char *filename, char *buf, unsigned int len);
int storage_write(const char *dirname, const char *filename, const char *buf, unsigned int len);
int storage_init(void);
int storage_set_shell(void *shell);
void storage_exit(void);
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
