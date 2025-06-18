/*
 * Copyright 2020-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "serial_iodevice.h"
#include "board.h"
#include "log.h"
#include "stats_task.h"
#include "semphr.h"
#include "fsl_lpuart.h"
#include "types.h"

// UART RX Task parameters
#define UART_RX_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 256)
#define UART_RX_TASK_PRIORITY   1
#define UART_RX_QUEUE_SIZE      5
#define UART_RX_MAX_SEM_COUNT   10

#define SERIAL_IODEVICE_STAT_PERIOD_SEC 5
#define SERIAL_IODEVICE_UART_LOOPBACK   0

#define MAX_SERIAL_COMMAND_LEN 50

struct msg_serial {
    uint16_t cmd_len;
    uint8_t cmd[MAX_SERIAL_COMMAND_LEN];
};

struct serial_iodevice_stats {
    uint32_t num_cmds;
    uint32_t num_null_cmds;
    uint32_t uart_rx;
    uint32_t max_uart_buf_len;
    uint32_t uart_rx_err_alloc;
    uint32_t max_feedback_queue_len;
    uint32_t net_invalid_msg_id;
    uint32_t net_invalid_msg_len;
    uint32_t uart_tx_busy;
};

struct serial_iodevice_ctx {
    struct cyclic_task *c_task;
    LPUART_Type *uart_base;
    SemaphoreHandle_t sem_uart_rx;
    QueueHandle_t feedback_rx_queue;
    lpuart_handle_t lpuart_handler;
    uint8_t drv_rx_ring_buffer[BOARD_IODEV_UART_RING_BUFFER_LEN];
    uint8_t uart_rx_buf[MAX_SERIAL_COMMAND_LEN];
    uint32_t uart_rx_index;
    bool new_cmd;
    uint8_t cmd[MAX_SERIAL_COMMAND_LEN];
    uint16_t cmd_len;
    struct serial_iodevice_stats stats;
    struct serial_iodevice_stats stats_snap;
};

static struct serial_iodevice_ctx serial_iodev;

struct uart_rx_msg {
    uint16_t cmd_len;
    uint8_t cmd_buffer[MAX_SERIAL_COMMAND_LEN];
};

static void lpuart_cb(LPUART_Type *base, lpuart_handle_t *handle, status_t status, void *userData)
{
    struct serial_iodevice_ctx *ctx = userData;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (status == kStatus_LPUART_IdleLineDetected) {
        xSemaphoreGiveFromISR(ctx->sem_uart_rx, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static int lpuart_init(struct serial_iodevice_ctx *ctx)
{
    lpuart_config_t config;
    status_t status;

    status = BOARD_InitLPUARTPins();
    if (status != kStatus_Success) {
        log_err("BOARD_InitLPUARTPins() failed\n");
        goto err;
    }

    ctx->uart_base = BOARD_IODEV_UART_BASEADDR;

    /*
     * config.baudRate_Bps = 115200U;
     * config.parityMode = kLPUART_ParityDisabled;
     * config.stopBitCount = kLPUART_OneStopBit;
     * config.txFifoWatermark = 0;
     * config.rxFifoWatermark = 0;
     * config.enableTx = false;
     * config.enableRx = false;
     */
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_IODEV_UART_BAUDRATE;
    config.enableTx = true;
    config.enableRx = true;

    status = LPUART_Init(ctx->uart_base, &config, BOARD_IODEV_UART_CLK_FREQ);
    if (status != kStatus_Success) {
        log_err("LPUART_Init() failed\n");
        goto err;
    }

#if SERIAL_IODEVICE_UART_LOOPBACK
    ctx->uart_base->CTRL |= LPUART_CTRL_LOOPS(1);
#endif

    LPUART_TransferCreateHandle(ctx->uart_base, &ctx->lpuart_handler, lpuart_cb, ctx);
    LPUART_TransferStartRingBuffer(ctx->uart_base, &ctx->lpuart_handler, ctx->drv_rx_ring_buffer, BOARD_IODEV_UART_RING_BUFFER_LEN);
    LPUART_EnableInterrupts(ctx->uart_base, BOARD_IODEV_UART_INTERRUPT_MASK);

    return 0;

err:
    return -1;
}

static void uart_rx_buffer_reset(struct serial_iodevice_ctx *ctx)
{
    memset(ctx->uart_rx_buf, 0, MAX_SERIAL_COMMAND_LEN);
    ctx->uart_rx_index = 0;
}

static void uart_rx_task(void *pvParameters)
{
    struct serial_iodevice_ctx *ctx = pvParameters;
    int i = 0, j = 0;
    size_t num_bytes_to_read, received_bytes;
    lpuart_transfer_t xfer;
    uint32_t len;
    struct uart_rx_msg *feedback;

    while (true) {
        xSemaphoreTake(ctx->sem_uart_rx, portMAX_DELAY);

        num_bytes_to_read = LPUART_TransferGetRxRingBufferLength(ctx->uart_base, &ctx->lpuart_handler);
        num_bytes_to_read = MIN(num_bytes_to_read, MAX_SERIAL_COMMAND_LEN - ctx->uart_rx_index);

        xfer.dataSize = num_bytes_to_read;
        xfer.data = ctx->uart_rx_buf + ctx->uart_rx_index;

        // xfer buffer is filled by the non-blocking API
        LPUART_TransferReceiveNonBlocking(ctx->uart_base, &ctx->lpuart_handler, &xfer, &received_bytes);
        if (received_bytes != num_bytes_to_read)
            log_err("Number of bytes received different from number of bytes to read");

        // Update the number of bytes in the command buffer
        ctx->uart_rx_index += received_bytes;

        if (ctx->uart_rx_index > ctx->stats.max_uart_buf_len)
            ctx->stats.max_uart_buf_len = ctx->uart_rx_index;

        // Parse all bytes received since last EOL
        while (i < ctx->uart_rx_index) {
            // When EOL is detected, a command is found
            if (ctx->uart_rx_buf[i] == '\n' || ctx->uart_rx_buf[i] == '\r') {
                len = i + 1;

                // Enqueue received message
                feedback = pvPortMalloc(sizeof(struct uart_rx_msg));
                if (feedback) {
                    memcpy(feedback->cmd_buffer, ctx->uart_rx_buf, len);
                    feedback->cmd_len = len;

                    if (xQueueSend(ctx->feedback_rx_queue, &feedback, 0) != pdPASS) {
                        log_err("Feeback RX queue is already full\n");
                    }

                } else {
                    ctx->stats.uart_rx_err_alloc++;
                }

                // If there is still data after the EOL, the data is shifted to the beginning of the buffer
                if (ctx->uart_rx_index > len) {
                    for (j = 0; j < (ctx->uart_rx_index - len); j++)
                        ctx->uart_rx_buf[j] = ctx->uart_rx_buf[len + j];

                    ctx->uart_rx_index = j;

                    // Restart loop to parse the remaining bytes
                    i = 0;
                    continue;
                } else {
                    // If there is no more data after EOL, the command buffer is reset
                    uart_rx_buffer_reset(ctx);
                    i = 0;
                    break;
                }
            }
            i++;

            // If the buffer is full but has no command inside, it is erased
            if (i == MAX_SERIAL_COMMAND_LEN) {
                uart_rx_buffer_reset(ctx);
                i = 0;
            }
        }
    }
}

static void serial_iodevice_stats_print(void *data)
{
    struct serial_iodevice_stats *stats_snap = data;

    log_info("net: \n");
    log_info("    cmds                      : %u\n", stats_snap->num_cmds);
    log_info("    null cmds                 : %u\n", stats_snap->num_null_cmds);
    log_info("    invalid msg id            : %u\n", stats_snap->net_invalid_msg_id);
    log_info("    invalid msg len           : %u\n", stats_snap->net_invalid_msg_len);
    log_info("uart: \n");
    log_info("    uart rx                   : %u\n", stats_snap->uart_rx);
    log_info("    uart rx err alloc         : %u\n", stats_snap->uart_rx_err_alloc);
    log_info("    max uart buf length       : %u\n", stats_snap->max_uart_buf_len);
    log_info("    max feedback queue length : %u\n", stats_snap->max_feedback_queue_len);
    log_info("    uart tx busy              : %u\n", stats_snap->uart_tx_busy);
}

static void serial_iodevice_stats_dump(struct serial_iodevice_ctx *ctx)
{
    memcpy(&ctx->stats_snap, &ctx->stats, sizeof(struct serial_iodevice_stats));

    // Print serial iodevice data
    STATS_Async(serial_iodevice_stats_print, &ctx->stats_snap);
}

static void serial_iodevice_loop(void *data, int timer_status)
{
    struct serial_iodevice_ctx *ctx = data;
    struct msg_serial msg_to_send;
    unsigned int num_sched_stats = SERIAL_IODEVICE_STAT_PERIOD_SEC *
                                   (NSECS_PER_SEC / ctx->c_task->task->params->task_period_ns);
    lpuart_transfer_t xfer;
    struct uart_rx_msg *feedback = NULL;
    status_t status;
    UBaseType_t queue_len;

    // Send command received from network to uart
    if (ctx->new_cmd) {
        xfer.data = ctx->cmd;
        xfer.dataSize = ctx->cmd_len;
        // FIXME: Command buffer received from the network should be protected, as no copy is performed in UART driver.
        status = LPUART_TransferSendNonBlocking(ctx->uart_base, &ctx->lpuart_handler, &xfer);
        if (status != kStatus_Success) {
            log_err("kStatus_LPUART_TxBusy\n");
            ctx->stats.uart_tx_busy++;
        }
        ctx->new_cmd = false;
    }

    queue_len = uxQueueMessagesWaiting(ctx->feedback_rx_queue);
    if (queue_len > ctx->stats.max_feedback_queue_len)
        ctx->stats.max_feedback_queue_len = queue_len;

    // Check if a feedback has been received on uart
    if (xQueueReceive(ctx->feedback_rx_queue, &feedback, 0) == pdTRUE) {
        if (feedback) {
            ctx->stats.uart_rx++;

            msg_to_send.cmd_len = feedback->cmd_len;
            memcpy(msg_to_send.cmd, feedback->cmd_buffer, feedback->cmd_len);

            vPortFree(feedback);
        }
    } else {
        msg_to_send.cmd_len = 0;
        memset(msg_to_send.cmd, 0, MAX_SERIAL_COMMAND_LEN);
    }

    cyclic_net_transmit(ctx->c_task, MSG_SERIAL, &msg_to_send, sizeof(msg_to_send));

    // Print stats
    if (ctx->c_task->task->stats.sched % num_sched_stats == 0)
        serial_iodevice_stats_dump(ctx);
}

static void serial_iodevice_net_receive(void *data, int msg_id, int src_id, void *buf, int len)
{
    struct serial_iodevice_ctx *ctx = data;
    struct msg_serial *msg_recv = buf;

    if (msg_id != MSG_SERIAL) {
        ctx->stats.net_invalid_msg_id++;
        return;
    }

    if (msg_recv->cmd_len != 0) {
        if (msg_recv->cmd_len <= MAX_SERIAL_COMMAND_LEN) {
            ctx->new_cmd = true;
            ctx->cmd_len = msg_recv->cmd_len;
            memcpy(ctx->cmd, msg_recv->cmd, msg_recv->cmd_len);
            ctx->stats.num_cmds++;
            log_debug("cmd received: %.*s\n", msg_recv->cmd_len, msg_recv->cmd);
        } else {
            ctx->stats.net_invalid_msg_len++;
        }
    } else {
        ctx->stats.num_null_cmds++;
    }
}

int serial_iodevice_init(struct cyclic_task *c_task)
{
    struct serial_iodevice_ctx *ctx = &serial_iodev;

    memset(ctx, 0, sizeof(struct serial_iodevice_ctx));

    ctx->feedback_rx_queue = xQueueCreate(UART_RX_QUEUE_SIZE, sizeof(struct uart_rx_msg *));
    if (!ctx->feedback_rx_queue) {
        log_err("xQueueCreate() failed\n");
        goto err;
    }

    ctx->sem_uart_rx = xSemaphoreCreateCounting(UART_RX_MAX_SEM_COUNT, 0);
    if (!ctx->sem_uart_rx) {
        log_err("xSemaphoreCreateBinary() failed\n");
        goto err;
    }

    if (lpuart_init(ctx) < 0) {
        log_err("lpuart_init() failed\n");
        goto err;
    }

    if (xTaskCreate(uart_rx_task, "uart rx task", UART_RX_TASK_STACK_SIZE,
                    ctx, UART_RX_TASK_PRIORITY, NULL) != pdPASS) {
        log_err("xTaskCreate() failed\n");
        goto err;
    }

    ctx->c_task = c_task;
    if (cyclic_task_init(c_task, serial_iodevice_net_receive, serial_iodevice_loop, ctx) < 0)
        goto err;

    log_info("Serial iodevice init successfully\n");

    return 0;

err:
    return -1;
}
