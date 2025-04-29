/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_apps/log.h"
#include "rtos_apps/audio/audio_app.h"
#include "rtos_apps/audio/audio_element.h"
#include "rtos_apps/audio/audio_pipeline.h"
#include "rtos_apps/audio/audio_ctrl.h"

/* clang-format off */
const char *element_name[AUDIO_ELEMENT_MAX] = {
    [AUDIO_ELEMENT_DTMF_SOURCE] = "DTMF_SOURCE",
    [AUDIO_ELEMENT_ROUTING] = "ROUTING",
    [AUDIO_ELEMENT_SAI_SINK] = "SAI_SINK",
    [AUDIO_ELEMENT_SAI_SOURCE] = "SAI_SOURCE",
    [AUDIO_ELEMENT_SINE_SOURCE] = "SINE_SOURCE",
    [AUDIO_ELEMENT_PLL] = "PLL",
#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    [AUDIO_ELEMENT_AVTP_SOURCE] = "AVTP_SOURCE",
    [AUDIO_ELEMENT_AVTP_SINK] = "AVTP_SINK",
#endif
};
/* clang-format on */

static void audio_element_response(void *ctrl_handle, uint32_t status)
{
    struct audio_resp_element resp;

    if (ctrl_handle) {
        resp.type = AUDIO_RESP_TYPE_ELEMENT;
        resp.status = status;
        audio_app_ctrl_send(ctrl_handle, &resp, sizeof(resp));
    }
}

int audio_element_ctrl(struct audio_element *element, struct audio_cmd_element *cmd, unsigned int len,
                       void *ctrl_handle)
{
    int rc = 0;

    switch (cmd->u.common.type) {
    case AUDIO_CMD_TYPE_ELEMENT_DUMP:
        if (len != sizeof(struct audio_cmd_element_dump))
            goto err;

        if (!element)
            goto err;

        audio_element_dump(element);

        audio_element_response(ctrl_handle, AUDIO_RESP_STATUS_SUCCESS);

        break;

    case AUDIO_CMD_TYPE_ELEMENT_ROUTING_CONNECT:
    case AUDIO_CMD_TYPE_ELEMENT_ROUTING_DISCONNECT:
        rc = routing_element_ctrl(element, &cmd->u.routing, len, ctrl_handle);
        break;

    case AUDIO_CMD_TYPE_ELEMENT_PLL_ENABLE:
    case AUDIO_CMD_TYPE_ELEMENT_PLL_DISABLE:
    case AUDIO_CMD_TYPE_ELEMENT_PLL_ID:
        rc = pll_element_ctrl(element, &cmd->u.pll, len, ctrl_handle);
        break;

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    case AUDIO_CMD_TYPE_ELEMENT_AVTP_SOURCE_CONNECT:
    case AUDIO_CMD_TYPE_ELEMENT_AVTP_SOURCE_DISCONNECT:
        rc = avtp_source_element_ctrl(element, &cmd->u.avtp, len, ctrl_handle);
        break;
    case AUDIO_CMD_TYPE_ELEMENT_AVTP_SINK_CONNECT:
    case AUDIO_CMD_TYPE_ELEMENT_AVTP_SINK_DISCONNECT:
        rc = avtp_sink_element_ctrl(element, &cmd->u.avtp, len, ctrl_handle);
        break;
#endif

    default:
        goto err;
        break;
    }

    return rc;

err:
    audio_element_response(ctrl_handle, AUDIO_RESP_STATUS_ERROR);

    return -1;
}

void audio_element_exit(struct audio_element *element)
{
    element->exit(element);
}

void audio_element_dump(struct audio_element *element)
{
    if (element->dump)
        element->dump(element);
}

void audio_element_stats(struct audio_element *element)
{
    if (element->stats)
        element->stats(element);
}

int audio_element_check_config(struct audio_element_config *config)
{
    int rc;

    switch (config->type) {
    case AUDIO_ELEMENT_DTMF_SOURCE:
        rc = dtmf_element_check_config(config);
        break;

    case AUDIO_ELEMENT_PLL:
        rc = pll_element_check_config(config);
        break;

    case AUDIO_ELEMENT_ROUTING:
        rc = routing_element_check_config(config);
        break;

    case AUDIO_ELEMENT_SAI_SINK:
        rc = sai_sink_element_check_config(config);
        break;

    case AUDIO_ELEMENT_SAI_SOURCE:
        rc = sai_source_element_check_config(config);
        break;

    case AUDIO_ELEMENT_SINE_SOURCE:
        rc = sine_element_check_config(config);
        break;

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    case AUDIO_ELEMENT_AVTP_SOURCE:
        rc = avtp_source_element_check_config(config);
        break;
    case AUDIO_ELEMENT_AVTP_SINK:
        rc = avtp_sink_element_check_config(config);
        break;
#endif

    default:
        rc = -1;
        break;
    }

    return rc;
}

unsigned int audio_element_data_size(struct audio_element_config *config)
{
    unsigned int size;

    switch (config->type) {
    case AUDIO_ELEMENT_DTMF_SOURCE:
        size = dtmf_element_size(config);
        break;

    case AUDIO_ELEMENT_PLL:
        size = pll_element_size(config);
        break;

    case AUDIO_ELEMENT_ROUTING:
        size = routing_element_size(config);
        break;

    case AUDIO_ELEMENT_SAI_SINK:
        size = sai_sink_element_size(config);
        break;

    case AUDIO_ELEMENT_SAI_SOURCE:
        size = sai_source_element_size(config);
        break;

    case AUDIO_ELEMENT_SINE_SOURCE:
        size = sine_element_size(config);
        break;

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    case AUDIO_ELEMENT_AVTP_SOURCE:
        size = avtp_source_element_size(config);
        break;
    case AUDIO_ELEMENT_AVTP_SINK:
        size = avtp_source_element_size(config);
        break;
#endif

    default:
        size = 0;
        break;
    }

    return size;
}

int audio_element_init(struct audio_element *element, struct audio_element_config *config, struct audio_buffer *buffer)
{
    int rc;

    log_info("enter, type %d\n", config->type);

    element->type = config->type;
    element->sample_rate = config->sample_rate;
    element->period = config->period;

    switch (config->type) {
    case AUDIO_ELEMENT_DTMF_SOURCE:
        rc = dtmf_element_init(element, config, buffer);
        break;

    case AUDIO_ELEMENT_PLL:
        rc = pll_element_init(element, config, buffer);
        break;

    case AUDIO_ELEMENT_ROUTING:
        rc = routing_element_init(element, config, buffer);
        break;

    case AUDIO_ELEMENT_SAI_SINK:
        rc = sai_sink_element_init(element, config, buffer);
        break;

    case AUDIO_ELEMENT_SAI_SOURCE:
        rc = sai_source_element_init(element, config, buffer);
        break;

    case AUDIO_ELEMENT_SINE_SOURCE:
        rc = sine_element_init(element, config, buffer);
        break;

#if defined(CONFIG_RTOS_APPS_AUDIO_GENAVB_ENABLE)
    case AUDIO_ELEMENT_AVTP_SOURCE:
        rc = avtp_source_element_init(element, config, buffer);
        break;
    case AUDIO_ELEMENT_AVTP_SINK:
        rc = avtp_sink_element_init(element, config, buffer);
        break;
#endif

    default:
        rc = -1;
        break;
    }

    log_info("done\n");

    return rc;
}
