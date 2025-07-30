/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "rtos_abstraction_layer.h"

#include "rtos_apps/audio/audio_app.h"
#include "rtos_apps/audio/audio_ctrl.h"
#include "rtos_apps/log.h"

#include "audio_avb.h"
#include "audio_pipeline.h"
#include "audio.h"

#include "avb_tsn/common/genavb.h"

#include "genavb/control.h"
#include "genavb/genavb.h"

static int clock_domain_set_source(struct genavb_msg_clock_domain_set_source *set_source, struct pipeline_ctx *ctx)
{
    genavb_msg_type_t msg_type = GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE;
    struct genavb_msg_clock_domain_response set_source_rsp;
    unsigned int msg_len = sizeof(*set_source);
    int rc;

    rc = genavb_control_send_sync(ctx->avb.clk_h, &msg_type, set_source, msg_len, &set_source_rsp, &msg_len, 1000);
    if ((rc != GENAVB_SUCCESS) || (msg_type != GENAVB_MSG_CLOCK_DOMAIN_RESPONSE)) {
        log_err("genavb_control_send_sync(GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE) failed %s\n", genavb_strerror(rc));
        rc = -1;
    } else {
        rc = 0;
    }

    return rc;
}

static void crf_disconnect(struct pipeline_ctx *ctx)
{
    struct crf_stream *crf_stream = &ctx->avb.crf_stream;
    int rc;

    if (!crf_stream->connected)
        goto exit;

    if (!crf_stream->stream.stream_handle) {
        log_err("CRF stream(%p) already disconnected for domain_index (%u)\n", crf_stream,
                crf_stream->stream.stream_params.clock_domain);
        return;
    }

    rc = genavb_stream_destroy(crf_stream->stream.stream_handle);
    if (rc != GENAVB_SUCCESS)
        log_err("CRF stream(%p): genavb_stream_destroy() failed %s\n", crf_stream, genavb_strerror(rc));

    crf_stream->stream.stream_handle = NULL;
    crf_stream->connected = 0;
    crf_stream->index = -1;

    log_info("CRF stream(%p) disconnected\n", crf_stream);

exit:
    return;
}

static void crf_connect(struct pipeline_ctx *ctx, unsigned int stream_index, struct genavb_stream_params *params)
{
    struct crf_stream *crf_stream = &ctx->avb.crf_stream;
    int rc;

    if (crf_stream->connected) {
        log_err("CRF stream(%p) already connected to stream_index(%u)\n", crf_stream, crf_stream->index);

        goto exit;
    }

    /* FIXME - override avdecc clock domain for now */
    if (params)
        params->clock_domain = GENAVB_CLOCK_DOMAIN_0;

    rc = genavb_stream_create(ctx->avb.avb_handle, &crf_stream->stream.stream_handle, params,
                             &crf_stream->stream.cur_batch_size, (genavb_stream_create_flags_t)0);
    if (rc != GENAVB_SUCCESS) {
        log_err("CRF stream(%p): genavb_stream_create() failed %s\n", crf_stream, genavb_strerror(rc));

        goto exit;
    } else {
        crf_stream->index = stream_index;
        crf_stream->connected = 1;
        log_info("CRF stream(%p) connected\n", crf_stream);
    }

exit:
    return;
}

static void listener_disconnect(unsigned int stream_index)
{
    struct audio_cmd_element_avtp_disconnect disconnect;
    int i;

    /* need to disconnect streams in AVTP audio element */
    disconnect.type = AUDIO_CMD_TYPE_ELEMENT_AVTP_SOURCE_DISCONNECT;
    disconnect.pipeline.id = 0;
    disconnect.element.type = AUDIO_ELEMENT_AVTP_SOURCE;
    disconnect.element.id = 0;
    disconnect.stream_index = stream_index;

    for (i = 0; i < AUDIO_PIPELINE_MAX_PIPELINES; i++) {
        disconnect.pipeline.id = i;
        audio_pipeline_ctrl((struct audio_cmd_pipeline *)&disconnect, sizeof(disconnect), NULL);
    }
}

static void listener_connect(struct genavb_msg_media_stack_connect *media_stack_connect)
{
    struct audio_cmd_element_avtp_connect connect;
    int i;

    /* need to connect streams in AVTP audio element */
    connect.type = AUDIO_CMD_TYPE_ELEMENT_AVTP_SOURCE_CONNECT;
    connect.pipeline.id = 0;
    connect.element.type = AUDIO_ELEMENT_AVTP_SOURCE;
    connect.element.id = 0;
    connect.stream_index = media_stack_connect->stream_index;
    connect.stream_params = media_stack_connect->stream_params;

    for (i = 0; i < AUDIO_PIPELINE_MAX_PIPELINES; i++) {
        connect.pipeline.id = i;
        audio_pipeline_ctrl((struct audio_cmd_pipeline *)&connect, sizeof(connect), NULL);
    }
}

static void talker_disconnect(unsigned int stream_index)
{
    struct audio_cmd_element_avtp_disconnect disconnect;
    int i;

    /* need to disconnect streams in AVTP audio element */
    disconnect.type = AUDIO_CMD_TYPE_ELEMENT_AVTP_SINK_DISCONNECT;
    disconnect.pipeline.id = 0;
    disconnect.element.type = AUDIO_ELEMENT_AVTP_SINK;
    disconnect.element.id = 0;
    disconnect.stream_index = stream_index;

    for (i = 0; i < AUDIO_PIPELINE_MAX_PIPELINES; i++) {
        disconnect.pipeline.id = i;
        audio_pipeline_ctrl((struct audio_cmd_pipeline *)&disconnect, sizeof(disconnect), NULL);
    }
}

static void talker_connect(struct genavb_msg_media_stack_connect *media_stack_connect)
{
    struct audio_cmd_element_avtp_connect connect;
    int i;

    /* need to connect streams in AVTP audio element */
    connect.type = AUDIO_CMD_TYPE_ELEMENT_AVTP_SINK_CONNECT;
    connect.pipeline.id = 0;
    connect.element.type = AUDIO_ELEMENT_AVTP_SINK;
    connect.element.id = 0;
    connect.stream_index = media_stack_connect->stream_index;
    connect.stream_params = media_stack_connect->stream_params;

    for (i = 0; i < AUDIO_PIPELINE_MAX_PIPELINES; i++) {
        connect.pipeline.id = i;
        audio_pipeline_ctrl((struct audio_cmd_pipeline *)&connect, sizeof(connect), NULL);
    }
}

static void handle_avdecc_event(struct pipeline_ctx *ctx, struct genavb_control_handle *ctrl_h)
{
    struct genavb_msg_media_stack_connect *media_stack_connect;
    struct genavb_msg_media_stack_disconnect *media_stack_disconnect;
    struct genavb_msg_clock_domain_set_source *media_stack_set_clock_source;
    union genavb_media_stack_msg msg;
    genavb_msg_type_t msg_type;
    unsigned int msg_len;
    int rc;

    msg_len = sizeof(union genavb_media_stack_msg);

    rc = genavb_control_receive(ctrl_h, &msg_type, &msg, &msg_len);
    if (rc != GENAVB_SUCCESS) {
        /* no event message*/

        goto exit;
    }

    switch (msg_type) {
    case GENAVB_MSG_MEDIA_STACK_CONNECT:

        media_stack_connect = &msg.media_stack_connect;

        log_info("GENAVB_MSG_MEDIA_STACK_CONNECT stream index: %u\n", media_stack_connect->stream_index);

        if (avdecc_format_is_crf(&media_stack_connect->stream_params.format)) {
            crf_connect(ctx, media_stack_connect->stream_index, &media_stack_connect->stream_params);
        } else {
            if (media_stack_connect->stream_params.direction == AVTP_DIRECTION_LISTENER)
                listener_connect(media_stack_connect);
            else
                talker_connect(media_stack_connect);
        }

        break;

    case GENAVB_MSG_MEDIA_STACK_DISCONNECT:

        media_stack_disconnect = &msg.media_stack_disconnect;

        log_info("GENAVB_MSG_MEDIA_STACK_DISCONNECT stream index: %u\n", media_stack_disconnect->stream_index);

        if ((int)media_stack_disconnect->stream_index == ctx->avb.crf_stream.index) {
            crf_disconnect(ctx);
        } else {
            if (media_stack_disconnect->direction == AVTP_DIRECTION_LISTENER)
                listener_disconnect(media_stack_disconnect->stream_index);
            else
                talker_disconnect(media_stack_disconnect->stream_index);
        }
        break;

    case GENAVB_MSG_MEDIA_SET_CLOCK_SOURCE:

        media_stack_set_clock_source = &msg.media_stack_set_clock_source;

        log_info("GENAVB_MSG_MEDIA_SET_CLOCK_SOURCE: domain(%u) type(%u)\n", media_stack_set_clock_source->domain,
                 media_stack_set_clock_source->source_type);

        if (clock_domain_set_source(media_stack_set_clock_source, ctx) < 0) {
            log_err("clock_domain_set_source(%u) failed\n", media_stack_set_clock_source->domain);
            goto exit;
        }

        break;

    default:
        log_err("Error, unknown message type: %d\n", msg_type);
        break;
    }

exit:
    return;
}

static void handle_avdecc_controlled_event(struct pipeline_ctx *ctx, struct genavb_control_handle *controlled_h)
{
    unsigned short status = AECP_AEM_SUCCESS;
    union genavb_controlled_msg msg;
    struct aecp_aem_pdu *pdu;
    genavb_msg_type_t msg_type;
    uint16_t cmd_type;
    unsigned int msg_len;
    int rc;

    msg_len = sizeof(union genavb_controlled_msg);

    rc = genavb_control_receive(controlled_h, &msg_type, &msg, &msg_len);
    if (rc != GENAVB_SUCCESS) {
        /* no event messages */

        goto exit;
    }

    switch (msg_type) {
    case GENAVB_MSG_AECP:
        pdu = (struct aecp_aem_pdu *)msg.aecp.buf;

        cmd_type = AECP_AEM_GET_CMD_TYPE(pdu);
        log_info("AECP command type (0x%x) seq_id (%d)\n", cmd_type, ntohs(pdu->sequence_id));

        switch (cmd_type) {
        case AECP_AEM_CMD_GET_AUDIO_MAP: {
            /* GET_AUDIO_MAP not fully supported, simply respond with empty audio mappings */
            struct aecp_aem_get_audio_map_rsp_pdu *audio_map_rsp = (struct aecp_aem_get_audio_map_rsp_pdu *)(pdu + 1);

            audio_map_rsp->number_of_maps = htons(0);
            audio_map_rsp->number_of_mappings = htons(0);
            audio_map_rsp->reserved = htons(0);

            /* Set the AECP Response PDU length to match the aecp_aem_get_audio_map_rsp_pdu to be sent back */
            msg.aecp.len = sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_get_audio_map_rsp_pdu);

            break;
        }
        default:
            log_info("AECP command type (0x%x) not handled in this app, skip\n", cmd_type);
            status = AECP_AEM_NOT_IMPLEMENTED;

            break;
        }

        msg.aecp.msg_type = AECP_AEM_RESPONSE;
        msg.aecp.status = status;

        rc = genavb_control_send(controlled_h, msg_type, &msg, msg_len);
        if (rc != GENAVB_SUCCESS) {
            log_info("AECP command response send failed: %d(%s)\n", rc, genavb_strerror(rc));
        }
        break;
    default:
        log_warn("Unsupported AVDECC message type (%d).\n", msg_type);
        break;
    }

exit:
    return;
}

int audio_avb_init(struct pipeline_ctx *ctx)
{
    genavb_msg_type_t msg_type = GENAVB_MSG_MEDIA_STACK_ENTITY_START;
    struct genavb_msg_media_stack_start media_stack_start;
    unsigned int msg_len = sizeof(media_stack_start);
    int rc;

    log_info("enter\n");

    ctx->avb.avb_handle = audio_app_avb_init();
    if (ctx->avb.avb_handle == NULL) {
        log_err("audio_app_avb_init() failed\n");
        rc = -1;

        goto exit;
    }

    rc = genavb_control_open(ctx->avb.avb_handle, &ctx->avb.clk_h, GENAVB_CTRL_CLOCK_DOMAIN);
    if (rc != GENAVB_SUCCESS) {
        log_err("genavb_control_open(GENAVB_CTRL_CLOCK_DOMAIN) failed: %s\n", genavb_strerror(rc));
        rc = -1;

        goto exit;
    }

    /* open avdecc control channel */
    rc = genavb_control_open(ctx->avb.avb_handle, &ctx->avb.ctrl_h, GENAVB_CTRL_AVDECC_MEDIA_STACK);
    if (rc != GENAVB_SUCCESS) {
        log_err("genavb_control_open(GENAVB_CTRL_AVDECC_MEDIA_STACK) failed: %s\n", genavb_strerror(rc));
        rc = -1;

        goto exit;
    }
    /*
    * Open controlled channel for AVDECC commands.
    */
    rc = genavb_control_open(ctx->avb.avb_handle, &ctx->avb.controlled_h, GENAVB_CTRL_AVDECC_CONTROLLED);
    if (rc != GENAVB_SUCCESS) {
        log_err("genavb_control_open(GENAVB_CTRL_AVDECC_CONTROLLED) failed: %s\n", genavb_strerror(rc));
        rc = -1;

        goto exit;
    }

    /*
    * Start the AVDECC (non-controller) entity
    */
    media_stack_start.entity_id = 0;
    rc = genavb_control_send(ctx->avb.ctrl_h, msg_type, &media_stack_start, msg_len);
    if (rc != GENAVB_SUCCESS) {
        log_err("genavb_control_send(GENAVB_MSG_MEDIA_STACK_ENTITY_START) failed: %s\n", genavb_strerror(rc));
        rc = -1;

        goto exit;
    }

exit:
    return rc;
}

void audio_avb_exit(struct pipeline_ctx *ctx)
{
    genavb_control_close(ctx->avb.controlled_h);
    genavb_control_close(ctx->avb.ctrl_h);

    genavb_control_close(ctx->avb.clk_h);
    ctx->avb.clk_h = NULL;

    audio_app_avb_exit();
}

void audio_avb_ctrl(void *handle)
{
    struct pipeline_ctx *ctx = handle;

    handle_avdecc_event(ctx, ctx->avb.ctrl_h);
    handle_avdecc_controlled_event(ctx, ctx->avb.controlled_h);
}
