/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_abstraction_layer.h"

#include "rtos_apps/audio/audio_app.h"
#include "rtos_apps/audio/audio_ctrl.h"
#include "rtos_apps/audio/audio_entry.h"
#include "rtos_apps/log.h"
#include "rtos_apps/types.h"

#include "audio.h"
#include "audio_pipeline.h"
#include "sai_drv.h"

struct mode_handler {
    void *(*init)(void *);
    void (*exit)(void *);
    void (*stats)(void *);
    void (*ctrl)(void *);
    int (*run)(void *, struct event *e);
};

#define DEFAULT_PERIOD      8
#define DEFAULT_SAMPLE_RATE 48000U
#define USE_TX_IRQ          1

struct ctrl_ctx {
    void *ctrl_handle;
};

struct data_ctx {
    uint8_t thread_count;
    uint8_t pipeline_count;

    /* SAI data for hardware setup */
    struct sai_device dev[SAI_TX_MAX_INSTANCE];
    uint32_t sai_dev_irq_source;
    sai_sample_rate_t sample_rate;
    uint8_t period;
    bool use_alternate_config;

    uint64_t callback_err;
    uint64_t callback;

    /* The first thread is used for parent pipeline, others are for child pipeline */
    struct thread_data_ctx_t {
        rtos_mutex_t mutex;
        rtos_sem_t async_sem;
        rtos_mqueue_t *mqueue_h;
        /* pipeline_ctx handle for current thread */
        void *handle;
        unsigned int id;
        rtos_thread_t thread;
    } thread_data_ctx[AUDIO_APP_MAX_DATA_THREADS];

    struct ctrl_ctx ctrl;
    const struct mode_handler *handler;
    rtos_mutex_t reset_mut;
    rtos_thread_t thread;
};

const static struct mode_handler g_handler = {
    .init = play_pipeline_init,
    .exit = play_pipeline_exit,
    .run = play_pipeline_run,
    .stats = play_pipeline_stats,
    .ctrl = play_pipeline_ctrl,
};

static void data_send_event(struct data_ctx *ctx, uint8_t status)
{
    bool yield = false;
    struct event e;
    int i;

    e.type = EVENT_TYPE_DATA;
    e.data = status;

    for (i = 0; i < ctx->pipeline_count; i++)
        if (rtos_mqueue_send_from_isr(ctx->thread_data_ctx[i].mqueue_h, &e, RTOS_NO_WAIT, &yield) < 0)
            ctx->callback_err++;

    rtos_yield_from_isr(yield);
}

static void rx_callback(uint8_t status, void *user_data)
{
    struct data_ctx *ctx = (struct data_ctx *)user_data;

#if USE_TX_IRQ
    sai_disable_irq(&ctx->dev[ctx->sai_dev_irq_source], false, true);
#else
    sai_disable_irq(&ctx->dev[ctx->sai_dev_irq_source], true, false);
#endif

    ctx->callback++;

    data_send_event(ctx, status);
}

static void pll_adjust_disable(struct data_ctx *ctx)
{
    struct audio_cmd_element_pll cmd;
    int i;

    /* need to disable PLL audio element */
    cmd.u.common.type = AUDIO_CMD_TYPE_ELEMENT_PLL_DISABLE;
    cmd.u.common.element.type = AUDIO_ELEMENT_PLL;
    cmd.u.common.element.id = 0;

    for (i = 0; i < AUDIO_PIPELINE_MAX_PIPELINES; i++) {
        cmd.u.common.pipeline.id = i;
        audio_pipeline_ctrl((struct audio_cmd_pipeline *)&cmd, sizeof(cmd), NULL);
    }
}

static void pll_adjust_set_pll_id(struct data_ctx *ctx, uint32_t pll_id)
{
    struct audio_cmd_element_pll cmd;
    int i;

    /* PLL element needs to know the sampling rate to determine the input PLL */
    cmd.u.common.type = AUDIO_CMD_TYPE_ELEMENT_PLL_ID;
    cmd.u.common.element.type = AUDIO_ELEMENT_PLL;
    cmd.u.common.element.id = 0;
    cmd.pll_id = pll_id;

    for (i = 0; i < AUDIO_PIPELINE_MAX_PIPELINES; i++) {
        cmd.u.common.pipeline.id = i;
        audio_pipeline_ctrl((struct audio_cmd_pipeline *)&cmd, sizeof(cmd), NULL);
    }
}

static inline uint8_t sai_get_effective_channels_count(uint32_t mask, uint32_t channels_num)
{
    uint8_t num_unmasked = 0;
    uint32_t max_count;
    unsigned int i;

    max_count = (channels_num < sizeof(mask) * 8) ? channels_num : sizeof(mask) * 8;
    for (i = 0; i < max_count; i++) {
        if ((1U << i) & ~mask)
            num_unmasked++;
    }

    return num_unmasked;
}

static int sai_setup(struct data_ctx *ctx)
{
    struct sai_cfg sai_config;
    bool pll_disable = true;
    int rc = 0;
    int i;

    log_info("enter\n");

    audio_app_sai_clock_setup();

    /* Always use last SAI device as IRQ source. */
    ctx->sai_dev_irq_source = audio_app_sai_active_list_nelems - 1;

    /* Configure each active SAI */
    for (i = 0; i < audio_app_sai_active_list_nelems; i++) {
        uint32_t pll_id;
        int sai_id;
        enum codec_id cid;
        int32_t ret;

        sai_config.sai_base = audio_app_sai_active_list[i].sai_base;
        sai_config.bit_width = audio_app_sai_active_list[i].slot_size;
        sai_config.chan_numbers = audio_app_sai_active_list[i].slot_count;
        sai_config.rx_mask = audio_app_sai_active_list[i].rx_mask;
        sai_config.tx_mask = audio_app_sai_active_list[i].tx_mask;
        sai_config.sample_rate = ctx->sample_rate;

        sai_id = sai_get_id(audio_app_sai_active_list[i].sai_base);

        sai_config.source_clock_hz = audio_app_sai_get_clock_freq(i);

        sai_config.tx_sync_mode = audio_app_sai_active_list[i].tx_sync_mode;
        sai_config.rx_sync_mode = audio_app_sai_active_list[i].rx_sync_mode;
        sai_config.msel = audio_app_sai_active_list[i].msel;

        if (i == ctx->sai_dev_irq_source) {
            /* SAI instance used as IRQ source */
            sai_config.rx_callback = rx_callback;
            sai_config.rx_user_data = ctx;
            sai_config.working_mode = SAI_RX_IRQ_MODE;
        } else {
            sai_config.rx_callback = NULL;
            sai_config.rx_user_data = NULL;
            sai_config.working_mode = SAI_POLLING_MODE;
        }

        /* Configure attached codec */
        cid = audio_app_sai_active_list[i].cid;
        ret = audio_app_codec_setup(cid);
        if (ret != kStatus_Success) {
            if (audio_app_sai_active_list[i].masterSlave == kSAI_Slave) {
                log_info("No codec found on SAI%d, forcing master mode\n",
                         sai_get_id(audio_app_sai_active_list[i].sai_base));
                audio_app_sai_active_list[i].masterSlave = kSAI_Master;
            }
        } else {
            audio_app_codec_set_format(cid, sai_config.source_clock_hz, sai_config.sample_rate, sai_config.bit_width);
        }

        sai_config.masterSlave = audio_app_sai_active_list[i].masterSlave;

        if (sai_config.masterSlave == kSAI_Slave)
            pll_disable = false;

        /* Set FIFO water mark to be period size of all channels*/
#if USE_TX_IRQ
        sai_config.rx_fifo_water_mark =
            ctx->period * sai_get_effective_channels_count(sai_config.rx_mask, sai_config.chan_numbers) - 1;
        sai_config.tx_fifo_water_mark =
            ctx->period * sai_get_effective_channels_count(sai_config.tx_mask, sai_config.chan_numbers) - 1;
#else
        sai_config.rx_fifo_water_mark =
            ctx->period * sai_get_effective_channels_count(sai_config.rx_mask, sai_config.chan_numbers);
        sai_config.tx_fifo_water_mark =
            ctx->period * sai_get_effective_channels_count(sai_config.tx_mask, sai_config.chan_numbers);
#endif

        if (sai_drv_setup(&ctx->dev[i], &sai_config) < 0) {
            log_err("sai_drv_setup() failed\n");
            rc = -1;
            goto out;
        }

        pll_id = audio_app_sai_select_audio_pll_mux(sai_id, sai_config.sample_rate);
        pll_adjust_set_pll_id(ctx, pll_id);
    }

    if (pll_disable)
        pll_adjust_disable(ctx);

out:
    return rc;
}

static void sai_close(struct data_ctx *ctx)
{
    int i;

    /* Close each active SAI */
    for (i = 0; i < audio_app_sai_active_list_nelems; i++) {
        sai_drv_exit(&ctx->dev[i]);
    }
}

static void audio_reset(struct data_ctx *ctx, unsigned int id)
{
    struct event e;
    int i, ret;

    /*
     *  If reset process has already been raised by another pipeline, return immediately
     *  because reset process will cover all pipeline.
     */
    ret = rtos_mutex_lock(&ctx->reset_mut, RTOS_NO_WAIT);
    if (ret)
        return;

    e.type = EVENT_TYPE_RESET;
    ctx->handler->run(ctx->thread_data_ctx[id].handle, &e);

    /* reset all other handlers */
    e.type = EVENT_TYPE_RESET_ASYNC;

    for (i = 0; i < ctx->pipeline_count; i++) {
        if (i == id)
            continue;

        rtos_mqueue_send(ctx->thread_data_ctx[i].mqueue_h, &e, RTOS_NO_WAIT);
    }

    /* wait for asynchronous reset execution */
    for (i = 0; i < ctx->pipeline_count; i++) {
        if (i == id)
            continue;

        rtos_sem_take(&ctx->thread_data_ctx[i].async_sem, RTOS_WAIT_FOREVER);
    }

    e.type = EVENT_TYPE_DATA;
    if (ctx->handler->run(ctx->thread_data_ctx[id].handle, &e) < 0)
        rtos_assert(false, "handler couldn't restart\n");

    if (id == 0) {
#if USE_TX_IRQ
        sai_enable_irq(&ctx->dev[ctx->sai_dev_irq_source], false, true);
#else
        sai_enable_irq(&ctx->dev[ctx->sai_dev_irq_source], true, false);
#endif
    }

    /* restart other handlers */
    for (i = 0; i < ctx->pipeline_count; i++) {
        if (i == id)
            continue;

        rtos_mqueue_send(ctx->thread_data_ctx[i].mqueue_h, &e, RTOS_NO_WAIT);
    }
    rtos_mutex_unlock(&ctx->reset_mut);
}

static void response(void *ctrl_handle, uint32_t status)
{
    struct audio_resp_audio resp;

    resp.type = AUDIO_RESP_TYPE;
    resp.status = status;
    audio_app_ctrl_send(ctrl_handle, &resp, sizeof(resp));
}

static void data_task(void *context)
{
    struct thread_data_ctx_t *thread = context;
    struct data_ctx *ctx = container_of(thread, struct data_ctx, thread_data_ctx[thread->id]);
    struct event e;

    do {

        if (!rtos_mqueue_receive(thread->mqueue_h, &e, RTOS_WAIT_FOREVER)) {

            rtos_mutex_lock(&thread->mutex, RTOS_WAIT_FOREVER);

            if (ctx->handler) {
                if (ctx->handler->run(thread->handle, &e) != 0) {
                    audio_reset(ctx, thread->id);
                } else {
                    if (thread->id == 0 && e.type == EVENT_TYPE_DATA) {
#if USE_TX_IRQ
                        sai_enable_irq(&ctx->dev[ctx->sai_dev_irq_source], false, true);
#else
                        sai_enable_irq(&ctx->dev[ctx->sai_dev_irq_source], true, false);
#endif
                    }
                }
            }

            rtos_mutex_unlock(&thread->mutex);
        }
    } while (1);
}

static void audio_stats(struct data_ctx *ctx)
{
    int i;

    if (ctx->handler) {
        for (i = 0; i < ctx->pipeline_count; i++)
            ctx->handler->stats(ctx->thread_data_ctx[i].handle);

        log_info("callback: count: %llu, error: %llu\n", ctx->callback, ctx->callback_err);
    }
}

static int audio_run(struct data_ctx *ctx, struct audio_cmd_run *run)
{
    int rc = AUDIO_RESP_STATUS_ERROR;
    struct audio_config cfg;
    const struct play_pipeline_config *play_cfg;
    struct event e;
    uint8_t pipeline_count = 0;
    size_t period = DEFAULT_PERIOD;
    uint32_t rate = DEFAULT_SAMPLE_RATE;
    int i;

    if (ctx->handler)
        goto exit;

    if (run->id >= AUDIO_APP_MAX_CFG)
        goto exit;

    if (run->use_alternate_config) {
        play_cfg = audio_app_play_alternate_config[run->id];
    } else {
        play_cfg = audio_app_play_config[run->id];
    }

    if (!play_cfg)
        goto exit;

    /* Check the count of pipeline */
    for (i = 0; i < ctx->thread_count; i++) {
        if (play_cfg->cfg[i] == NULL)
            break;
        else
            pipeline_count++;
    }

    if (pipeline_count == 0) {
        log_err("Unsupported configuration: use_alternate_config(%d) pipeline id(%u)\n", run->use_alternate_config,
                run->id);
        goto exit;
    }

    if (assign_nonzero_valid_val(period, run->period, audio_app_supported_period) != 0) {
        log_err("Unsupported period (%d frames)\n", run->period);
        goto exit;
    }

    /* If user configured rate is zero, set to default */
    rate = (run->frequency == 0) ? DEFAULT_SAMPLE_RATE : run->frequency;

    if (!audio_app_codec_is_rate_supported(rate, run->use_alternate_config)) {
        log_err("Unsupported rate(%d Hz)\n", run->frequency);
        goto exit;
    }

    ctx->callback_err = 0;
    ctx->callback = 0;
    ctx->sample_rate = rate;
    ctx->period = period;
    ctx->use_alternate_config = run->use_alternate_config;
    cfg.rate = rate;
    cfg.period = period;

    if (!audio_app_check_params(cfg.period, cfg.rate)) {
        log_warn("Unsupported combination: rate(%d Hz)/period(%d)\n", cfg.period, cfg.rate);
    }

    audio_app_pin_mux_dynamic_config(ctx->use_alternate_config);
    audio_app_sai_alternate_config(ctx->use_alternate_config, ctx->sample_rate);

    for (i = 0; i < pipeline_count; i++) {
        cfg.data = (void *)play_cfg->cfg[i];
        cfg.pipeline_id = i;
        cfg.async_sem = &ctx->thread_data_ctx[i].async_sem;
        ctx->thread_data_ctx[i].handle = g_handler.init(&cfg);
        if (!ctx->thread_data_ctx[i].handle)
            goto exit;
    }

    if (sai_setup(ctx) < 0) {
        log_err("sai_setup() failed\n");
        goto exit;
    }

    for (i = 0; i < pipeline_count; i++) {
        rtos_mutex_lock(&ctx->thread_data_ctx[i].mutex, RTOS_WAIT_FOREVER);
    }
    ctx->handler = &g_handler;
    ctx->pipeline_count = pipeline_count;
    for (i = 0; i < pipeline_count; i++) {
        rtos_mutex_unlock(&ctx->thread_data_ctx[i].mutex);
    }

    /* Send an event to trigger data thread processing */
    e.type = EVENT_TYPE_DATA;
    for (i = 0; i < pipeline_count; i++) {
        rtos_mqueue_send(ctx->thread_data_ctx[i].mqueue_h, &e, RTOS_NO_WAIT);
    }

    rc = AUDIO_RESP_STATUS_SUCCESS;

exit:
    return rc;
}

static int audio_stop(struct data_ctx *ctx)
{
    const struct mode_handler *handler;
    int i;

    if (!ctx->handler)
        goto exit;

    for (i = 0; i < ctx->pipeline_count; i++) {
        rtos_mutex_lock(&ctx->thread_data_ctx[i].mutex, RTOS_WAIT_FOREVER);
    }
    handler = ctx->handler;
    ctx->handler = NULL;
    for (i = 0; i < ctx->pipeline_count; i++) {
        rtos_mutex_unlock(&ctx->thread_data_ctx[i].mutex);
    }

    for (i = 0; i < ctx->pipeline_count; i++) {
        handler->exit(ctx->thread_data_ctx[i].handle);
    }

    sai_close(ctx);

    for (i = 0; i < audio_app_sai_active_list_nelems; i++)
        audio_app_codec_close(audio_app_sai_active_list[i].cid);

exit:
    return AUDIO_RESP_STATUS_SUCCESS;
}

static void audio_command_handler(struct data_ctx *ctx)
{
    void *ctrl_handle = ctx->ctrl.ctrl_handle;
    struct audio_command cmd;
    unsigned int len;
    int rc = 0;

    len = sizeof(cmd);
    if (audio_app_ctrl_recv(ctrl_handle, &cmd, (uint32_t *)&len) < 0)
        return;

    switch (cmd.u.cmd.type) {
    case AUDIO_CMD_TYPE_RUN:
        if (len != sizeof(struct audio_cmd_run)) {
            response(ctrl_handle, AUDIO_RESP_STATUS_ERROR);
            break;
        }

        rc = audio_run(ctx, &cmd.u.audio_run);

        response(ctrl_handle, rc);

        break;

    case AUDIO_CMD_TYPE_STOP:
        if (len != sizeof(struct audio_cmd_stop)) {
            response(ctrl_handle, AUDIO_RESP_STATUS_ERROR);
            break;
        }

        rc = audio_stop(ctx);

        response(ctrl_handle, rc);

        break;

    case AUDIO_CMD_TYPE_PIPELINE_DUMP:
    case AUDIO_CMD_TYPE_ELEMENT_DUMP:
    case AUDIO_CMD_TYPE_ELEMENT_ROUTING_CONNECT:
    case AUDIO_CMD_TYPE_ELEMENT_ROUTING_DISCONNECT:
        audio_pipeline_ctrl(&cmd.u.audio_pipeline, len, ctrl_handle);

        break;

    default:
        response(ctrl_handle, AUDIO_RESP_STATUS_ERROR);
        break;
    }
}

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
static void audio_control_handler(struct data_ctx *ctx)
{
    if (ctx->handler && ctx->handler->ctrl)
        ctx->handler->ctrl(ctx->thread_data_ctx[0].handle);
}
#endif

#define CONTROL_POLL_PERIOD 100
#define STATS_POLL_PERIOD   10000
#define STATS_COUNT         (STATS_POLL_PERIOD / CONTROL_POLL_PERIOD)

static void ctrl_task(void *context)
{
    struct data_ctx *ctx = context;
    int count;

    count = STATS_COUNT;
    do {
        /* handle commands from Linux ctrl application */
        audio_command_handler(ctx);

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
        /* handle events from control channel(s) */
        audio_control_handler(ctx);
#endif

        count--;
        if (!count) {
            audio_stats(ctx);
            count = STATS_COUNT;
        }

        rtos_sleep(RTOS_MS_TO_TICKS(CONTROL_POLL_PERIOD));

    } while (1);
}

static int audio_thread_init(struct thread_data_ctx_t *thread, const struct rtos_apps_audio_config *config)
{
    if (rtos_mutex_init(&thread->mutex) < 0) {
        log_err("rtos_mutex_init(thread) failed\n");
        goto err_mutex;
    }

    if (rtos_sem_init(&thread->async_sem, 0) < 0) {
        log_err("rtos_sem_init(async) failed\n");
        goto err_sem;
    }

    thread->mqueue_h = rtos_mqueue_alloc_init(10, sizeof(struct event));
    if (!thread->mqueue_h) {
        log_err("rtos_mqueue_alloc_init() failed\n");
        goto err_mqueue;
    }

    if (rtos_thread_create(&thread->thread, config->data_priority, thread->id, config->data_stack_size, "audio data", data_task, thread) < 0) {
        log_err("rtos_thread_create(audio data) failed\n");
        goto err_thread;
    }

    return 0;

err_thread:
    rtos_mqueue_destroy(thread->mqueue_h);

err_mqueue:
    rtos_sem_destroy(&thread->async_sem);

err_sem:
err_mutex:
    return -1;
}

static void audio_thread_exit(struct thread_data_ctx_t *thread)
{
    rtos_thread_abort(&thread->thread);
    rtos_mqueue_destroy(thread->mqueue_h);
    rtos_sem_destroy(&thread->async_sem);
}

int rtos_apps_audio_init(const struct rtos_apps_audio_config *config)
{
    struct data_ctx *ctx;
    int i;

    ctx = rtos_malloc(sizeof(*ctx));
    if (!ctx) {
        log_err("rtos_malloc() failed\n");
        goto err_malloc;
    }

    memset(ctx, 0, sizeof(*ctx));

    ctx->thread_count = config->thread_count;

    ctx->ctrl.ctrl_handle = config->ctrl_handle;

    if (rtos_mutex_init(&ctx->reset_mut) < 0) {
        log_err("rtos_mutex_init(reset) failed\n");
        goto err_mutex;
    }

    for (i = 0; i < ctx->thread_count; i++) {
        ctx->thread_data_ctx[i].id = i;

        if (audio_thread_init(&ctx->thread_data_ctx[i], config) < 0)
            goto err_thread;
    }

    if (rtos_thread_create(&ctx->thread, config->ctrl_priority, 0, config->ctrl_stack_size, "audio ctrl", ctrl_task, ctx) < 0) {
        log_err("rtos_thread_create(audio ctrl) failed\n");
        goto err_ctrl;
    }

    return 0;

err_thread:
    while (i--)
        audio_thread_exit(&ctx->thread_data_ctx[i]);

err_mutex:
err_ctrl:
    rtos_free(ctx);

err_malloc:
    return -1;
}
