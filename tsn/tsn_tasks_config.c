/*
 * Copyright 2018-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "genavb/qos.h"

#include "rtos_apps/tsn/tsn_tasks_config.h"

#include "tsn_tasks_config.h"

static const struct tsn_stream tsn_streams[] = {
    [0] = {
        .address = {
            .ptype = PTYPE_L2,
            .vlan_id = htons(VLAN_ID),
            .priority = ISOCHRONOUS_DEFAULT_PRIORITY,
            .u.l2 = {
                .dst_mac = {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x70},
                .protocol = htons(ETHERTYPE_MOTOROLA),
            },
        },
    },
    [1] = {
        .address = {
            .ptype = PTYPE_L2,
            .vlan_id = htons(VLAN_ID),
            .priority = ISOCHRONOUS_DEFAULT_PRIORITY,
            .u.l2 = {
                .dst_mac = {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x71},
                .protocol = htons(ETHERTYPE_MOTOROLA),
            },
        },
    },
    [2] = {
        .address = {
            .ptype = PTYPE_L2,
            .vlan_id = htons(VLAN_ID),
            .priority = ISOCHRONOUS_DEFAULT_PRIORITY,
            .u.l2 = {
                .dst_mac = {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x80},
                .protocol = htons(ETHERTYPE_MOTOROLA),
            },
        },
    },
    [3] = {
        .address = {
            .ptype = PTYPE_L2,
            .vlan_id = htons(VLAN_ID),
            .priority = EVENTS_DEFAULT_PRIORITY,
            .u.l2 = {
                .dst_mac = {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x90},
                .protocol = htons(ETHERTYPE_MOTOROLA),
            },
        },
    },
    [4] = {
        .address = {
            .ptype = PTYPE_L2,
            .vlan_id = htons(VLAN_ID),
            .priority = EVENTS_DEFAULT_PRIORITY,
            .u.l2 = {
                .dst_mac = {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0xa0},
                .protocol = htons(ETHERTYPE_MOTOROLA),
            },
        },
    },
};

#define CYCLIC_TASK_DEFAULT_PARAMS                    \
    {                                                 \
        .clk_id = GENAVB_CLOCK_GPTP_0_0,              \
        .priority = TASK_DEFAULT_PRIORITY,            \
        .stack_depth = TASK_DEFAULT_STACK_SIZE,       \
        .rx_buf_size = PACKET_SIZE,                   \
        .tx_buf_size = PACKET_SIZE,                   \
    }

static struct cyclic_task_config cyclic_tasks[] = {
    [0] = {
        .type = CYCLIC_CONTROLLER,
        .id = CONTROLLER_0,
        .params = CYCLIC_TASK_DEFAULT_PARAMS,
        .num_peers = 2,
        .rx_socket = {
            [0] = {
                .peer_id = IO_DEVICE_0,
                .stream_id = 1,
            },
            [1] = {
                .peer_id = IO_DEVICE_1,
                .stream_id = 2,
            },
        },
        .tx_socket = {
            .stream_id = 0,
        },
    },
    [1] = {
        .type = CYCLIC_IO_DEVICE,
        .id = IO_DEVICE_0,
        .params = CYCLIC_TASK_DEFAULT_PARAMS,
        .num_peers = 1,
        .rx_socket = {
            [0] = {
                .peer_id = CONTROLLER_0,
                .stream_id = 0,
            },
        },
        .tx_socket = {
            .stream_id = 1,
        },
    },
    [2] = {
        .type = CYCLIC_IO_DEVICE,
        .id = IO_DEVICE_1,
        .params = CYCLIC_TASK_DEFAULT_PARAMS,
        .num_peers = 1,
        .rx_socket = {
            [0] = {
                .peer_id = CONTROLLER_0,
                .stream_id = 0,
            },
        },
        .tx_socket = {
            .stream_id = 2,
        },
    },
};

#define ALARM_TASK_DEFAULT_PARAMS               \
    {                                           \
        .clk_id = GENAVB_CLOCK_GPTP_0_0,        \
        .priority = TASK_DEFAULT_PRIORITY - 1,  \
        .stack_depth = TASK_DEFAULT_STACK_SIZE, \
        .rx_buf_size = PACKET_SIZE,             \
        .tx_buf_size = PACKET_SIZE,             \
        .port_id = 0,                           \
        .num_packets = 1,                       \
        .zero_copy = 1,                         \
        .rx_tc_mask = 0,                        \
    }

static struct alarm_task_config alarm_tasks[] = {
    [0] = {
        .type = ALARM_MONITOR,
        .id = CONTROLLER_0,
        .params = ALARM_TASK_DEFAULT_PARAMS,
        .stream_id = 4,
        .length = TASK_DEFAULT_QUEUE_LENGTH,
    },
    [1] = {
        .type = ALARM_IO_DEVICE,
        .id = IO_DEVICE_0,
        .params = ALARM_TASK_DEFAULT_PARAMS,
        .stream_id = 4,
    },
    [2] = {
        .type = ALARM_IO_DEVICE,
        .id = IO_DEVICE_1,
        .params = ALARM_TASK_DEFAULT_PARAMS,
        .stream_id = 4,
    },
};

const struct tsn_stream *tsn_conf_get_stream(int index)
{
    if (index >= (sizeof(tsn_streams) / sizeof(struct tsn_stream)))
        return NULL;

    return &tsn_streams[index];
}

struct cyclic_task_config *tsn_conf_get_cyclic_task(int index)
{
    if (index >= (sizeof(cyclic_tasks) / sizeof(struct cyclic_task_config)))
        return NULL;

    return &cyclic_tasks[index];
}

struct alarm_task_config *tsn_conf_get_alarm_task(int index)
{
    if (index >= (sizeof(alarm_tasks) / sizeof(struct alarm_task_config)))
        return NULL;

    return &alarm_tasks[index];
}
