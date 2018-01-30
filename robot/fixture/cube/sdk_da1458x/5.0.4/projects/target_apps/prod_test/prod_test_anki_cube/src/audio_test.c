/**
 ****************************************************************************************
 *
 * @file audio_test.c
 *
 * @brief Audio 439 test.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup Audio
 * @{
 ****************************************************************************************
 */

#if HAS_AUDIO

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
    #include <stdlib.h>
    #include "app_audio439.h"
    #include "audio_task.h"
    #include "audio_test.h"
    #include "co_bt.h"
    #include "customer_prod.h"
    #include "uart.h"


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
static const struct ke_task_desc TASK_DESC_AUDIO = {
    audio_state_handler,
    NULL,
    audio_state,
    AUDIO_STATE_MAX,
    AUDIO_IDX_MAX
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void audio_init(void)
{
    // Create audio task
    ke_task_create(TASK_APP, &TASK_DESC_AUDIO);

    ke_state_set(TASK_APP, AUDIO_OFF);
}

void audio_send(int16_t *buffer)
{
    struct audio_samples_ind *ind = KE_MSG_ALLOC(AUDIO_SAMPLES_IND, TASK_APP, TASK_GTL,
                                                 audio_samples_ind);
    
    #ifdef CFG_AUDIO439_IMA_ADPCM    
    ind->encoding = AUDIO_IMA;
    t_IMAData *ima = &app_audio439_env.imaState;
    ima->inp = buffer;
    ima->out = ind->bytes;
    app_ima_enc(ima);
    #elif defined(CFG_AUDIO439_ALAW)
    ind->encoding = AUDIO_ALAW;
    #else
    ind->encoding = AUDIO_RAW;
    memcpy(ind->bytes, buffer, AUDIO_ENC_SIZE);
    #endif    

    // Send the message
    ke_msg_send(ind);
}

size_t audio_start(struct msg_audio_test_cfm* msg, uint16_t packets)
{
    if (ke_state_get(TASK_APP) == AUDIO_ON) {
        msg->results.audio_start.status = AUDIO_WAS_ACTIVE;
    } else {
        msg->results.audio_start.status = AUDIO_WAS_INACTIVE;
    #ifdef CFG_AUDIO439_IMA_ADPCM    
        t_IMAData *ima = &app_audio439_env.imaState;
        ima->len = AUDIO439_NR_SAMP;
        ima->index = 0;
        ima->predictedSample = 0;
    #endif
        struct audio_start_req *req = KE_MSG_ALLOC(AUDIO_START_REQ, TASK_APP, TASK_GTL,
                                                   audio_start_req);
        req->packets_required = packets;
        ke_msg_send(req);
    }
    
    return sizeof(*msg) - sizeof(msg->results) + sizeof(msg->results.audio_start);
}

size_t audio_stop(struct msg_audio_test_cfm* msg)
{    
    if (ke_state_get(TASK_APP) == AUDIO_OFF) {
        msg->results.audio_stop.status = AUDIO_WAS_INACTIVE;
    } else {
        msg->results.audio_stop.status = AUDIO_WAS_ACTIVE;
        ke_msg_send_basic(AUDIO_STOP_REQ, TASK_APP, TASK_GTL);
    }
    
    return sizeof(*msg) - sizeof(msg->results) + sizeof(msg->results.audio_stop);
}

#endif

/// @} APP
