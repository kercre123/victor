 /**
 ****************************************************************************************
 *
 * @file sps_server.c
 *
 * @brief Serial Port Service receive application.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
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
 * @addtogroup SPS_SERVER
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "sps_server_task.h"
#include "sps_server.h"

#if (BLE_SPS_SERVER)

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct sps_server_env_tag sps_server_env;

/// SPS_SERVER task descriptor
static const struct ke_task_desc TASK_DESC_SPS_SERVER = {sps_server_state_handler, &sps_server_default_handler, sps_server_state, SPS_SERVER_STATE_MAX, SPS_SERVER_IDX_MAX};

/**
 ****************************************************************************************
 * @brief 		Init of the SPS device
 * @param[in] Void
 * @return 		None
 ****************************************************************************************
 */
void sps_server_init(void)
{
    // Reset the sps_receivererometer environment
    memset(&sps_server_env, 0, sizeof(sps_server_env));
    
    // Create SPS task
    ke_task_create(TASK_SPS_SERVER, &TASK_DESC_SPS_SERVER);

    // Go to IDLE state
    ke_state_set(TASK_SPS_SERVER, SPS_SERVER_DISABLED);
}


#endif // BLE_SPS_SERVER

/// @} SPS_SERVER

