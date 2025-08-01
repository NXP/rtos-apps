/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <inttypes.h>
#include <string.h>

#include "rtos_abstraction_layer.h"

#include "rtos_apps/audio/audio_app.h"
#include "rtos_apps/audio/audio_ctrl.h"
#include "rtos_apps/log.h"

#include "app_board.h"
#include "audio_pipeline.h"
#include "audio.h"

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
#include "audio_avb.h"
#endif

void play_pipeline_stats(void *handle)
{
    struct pipeline_ctx *ctx = handle;

    log_info("pipeline%d  run: %" PRIu64 ", err: %" PRIu64 "\n", ctx->id, ctx->stats.run, ctx->stats.err);

    audio_pipeline_stats(ctx->pipeline);
}

int play_pipeline_run(void *handle, struct event *e)
{
    struct pipeline_ctx *ctx = handle;
    int err = 0;

    switch (e->type) {
    case EVENT_TYPE_DATA:
        ctx->stats.run++;

        err = audio_pipeline_run(ctx->pipeline);
        if (err) {
            ctx->stats.err++;
            break;
        }
        break;

    case EVENT_TYPE_RESET:
        audio_pipeline_reset(ctx->pipeline);
        break;

    case EVENT_TYPE_RESET_ASYNC:
        audio_pipeline_reset(ctx->pipeline);
        rtos_sem_give(ctx->async_sem);
        break;

    default:
        break;
    }

    return err;
}

void *play_pipeline_init(void *parameters)
{
    struct audio_config *cfg = parameters;
    struct audio_pipeline_config *pipeline_cfg;
    struct pipeline_ctx *ctx;

    ctx = rtos_malloc(sizeof(struct pipeline_ctx));
    if (!ctx) {
        log_err("rtos_malloc(pipeline) failed\n");
        goto err_alloc_ctx;
    }

    memset(ctx, 0, sizeof(struct pipeline_ctx));

    pipeline_cfg = rtos_malloc(sizeof(struct audio_pipeline_config));
    if (!pipeline_cfg) {
        log_err("rtos_malloc(config) failed\n");
        goto err_alloc_cfg;
    }

    memcpy(pipeline_cfg, cfg->data, sizeof(struct audio_pipeline_config));

    /* override pipeline configuration */
    pipeline_cfg->sample_rate = cfg->rate;
    pipeline_cfg->period = cfg->period;
    pipeline_cfg->id = cfg->pipeline_id;

    ctx->id = cfg->pipeline_id;
    ctx->pipeline = audio_pipeline_init(pipeline_cfg);
    if (!ctx->pipeline)
        goto err_init;

    ctx->async_sem = cfg->async_sem;

    log_info("Starting %s (Sample Rate: %d Hz, Period: %u frames)\n", pipeline_cfg->name, pipeline_cfg->sample_rate,
             (uint32_t)pipeline_cfg->period);

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    if (ctx->id == 0 && pipeline_cfg->avb) {
        if (audio_avb_init(ctx) < 0) {
            play_pipeline_exit(ctx);
            goto err_init;
        }
    }
#endif

    rtos_free(pipeline_cfg);

    return ctx;

err_init:
    rtos_free(pipeline_cfg);

err_alloc_cfg:
    rtos_free(ctx);

err_alloc_ctx:
    return NULL;
}

void play_pipeline_ctrl(void *handle)
{
    struct pipeline_ctx *ctx = handle;

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    if (ctx->avb.genavb_handle)
        audio_avb_ctrl(handle);
#else
    return;
#endif
}

void play_pipeline_exit(void *handle)
{
    struct pipeline_ctx *ctx = handle;

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    if (ctx->id == 0 && ctx->avb.genavb_handle)
        audio_avb_exit(ctx);
#endif

    audio_pipeline_exit(ctx->pipeline);

    rtos_free(ctx);

    log_info("\nEnd.\n");
}
