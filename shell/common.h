/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#define RTOS_APPS_STREAM_STR_FMT         "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x"
#define RTOS_APPS_STREAM_STR(_stream_id) (_stream_id)[0], (_stream_id)[1], (_stream_id)[2], (_stream_id)[3], (_stream_id)[4], (_stream_id)[5], (_stream_id)[6], (_stream_id)[7]

#define RTOS_APPS_MAC_STR_FMT       "%02x:%02x:%02x:%02x:%02x:%02x"
#define RTOS_APPS_MAC_STR(_dst_mac) (_dst_mac)[0], (_dst_mac)[1], (_dst_mac)[2], (_dst_mac)[3], (_dst_mac)[4], _dst_mac[5]

#define NSECS_PER_SEC   (1000000000)

#endif /* _COMMON_H_ */
