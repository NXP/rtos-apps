/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SHELL_STORAGE_H_
#define _SHELL_STORAGE_H_

#include "shell_config.h"

#ifndef CONFIG_STORAGE_ROOT
#define CONFIG_STORAGE_ROOT ""
#endif

#define SHELL_STORAGE_ROOT_SIZE sizeof(CONFIG_STORAGE_ROOT)

#define SHELL_STORAGE_MAX_FILENAME   (30 + SHELL_STORAGE_ROOT_SIZE)
#define SHELL_STORAGE_MAX_DIRNAME    (8 + SHELL_STORAGE_ROOT_SIZE)
#define SHELL_STORAGE_MAX_FILESIZE   32

#endif /* _SHELL_STORAGE_H_ */
