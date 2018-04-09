/**
 ****************************************************************************************
 *
 * @file audio_task.h
 *
 * @brief Header file - audio_task.
 * @brief 128 UUID service. sample code
 *
 * Copyright (C) 2015 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef AUDIO_TASK_H_
#define AUDIO_TASK_H_


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "app_audio439.h"
#include "ke_task.h"
#include "rwip_config.h"


/*
 * DEFINES
 ****************************************************************************************
 */
/// Maximum number of Sample128 task instances
#define AUDIO_IDX_MAX                 (1)


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the AUDIO task
enum audio_states {
    AUDIO_ON,
    AUDIO_OFF,
    
    /// Number of defined states.
    AUDIO_STATE_MAX
};

enum audio_encoding {
    AUDIO_IMA,
    AUDIO_ALAW,
    AUDIO_RAW
};

/// Messages for Audio
enum audio_messages {
    AUDIO_START_REQ = KE_FIRST_MSG(TASK_APP),
    AUDIO_SAMPLES_IND,
    AUDIO_STOP_REQ
};


/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref AUDIO_START_REQ message
struct audio_start_req
{
    uint16_t packets_required;
};

/// Parameters of the @ref AUDIO_SAMPLES_IND message
struct audio_samples_ind
{
    enum audio_encoding encoding;
    uint8_t bytes[AUDIO_ENC_SIZE];
};


/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler audio_state_handler[AUDIO_STATE_MAX];
extern const struct ke_state_handler audio_default_handler;
extern ke_state_t audio_state[AUDIO_IDX_MAX];

#endif // AUDIO_TASK_H_
