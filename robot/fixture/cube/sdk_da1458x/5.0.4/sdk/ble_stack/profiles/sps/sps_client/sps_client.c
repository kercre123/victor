/**
 ****************************************************************************************
 *
 * @file sps_client.c
 *
 * @brief SPS transmit implementation.
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
 * @addtogroup SPS_CLIENT
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"
#include "app_spsc.h"
#include "sps_client.h"
#include "sps_client_task.h"
#include "prf_utils.h"
#include "prf_utils_128.h"

#if (BLE_SPS_CLIENT)

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct sps_client_env_tag **sps_client_envs __attribute__((section("exchange_mem_case1")));

static const struct ke_task_desc TASK_DESC_SPS_CLIENT = {sps_client_state_handler, &sps_client_default_handler, sps_client_state, SPS_CLIENT_STATE_MAX, SPS_CLIENT_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief sps_client_init  init sps_client
 * @param[in] void
 * @return void
 ****************************************************************************************
 */
void sps_client_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(sps_client_envs, SPS_CLIENT);
}

/**
 ****************************************************************************************
 * @brief     When all handels received correctly it can send and receive data or flow control else it returns an error
 * @param[in] env_tag enviroment 
 * @param[in] con_inf Connection information
 * @param[in] status
 * @return    void
 ****************************************************************************************
 */
void sps_client_enable_cfm_send(struct sps_client_env_tag *sps_client_env, struct prf_con_info *con_info, uint8_t status)
{
    //format response to app
    struct sps_client_enable_cfm * cfm = KE_MSG_ALLOC(SPS_CLIENT_ENABLE_CFM,
                                                 con_info->appid, con_info->prf_id,
                                                 sps_client_enable_cfm);

    cfm->conhdl = gapc_get_conhdl(con_info->conidx);
    cfm->status = status;

    if (status == PRF_ERR_OK)
    {
        cfm->sps_client  = sps_client_env->sps_server;
        
        // Register SPS_CLIENT task in gatt for indication/notifications
        prf_register_atthdl2gatt(&(sps_client_env->con_info), &(sps_client_env->sps_server.svc));
        
        //Set value 0x0100 to CFG
        uint8_t val[]={0x01,0x00};
        prf_gatt_write(con_info, sps_client_env->sps_server.chars[SPS_SERVER_TX_DATA].val_hdl+1, val, sizeof(val) ,GATTC_WRITE);	
        prf_gatt_write(con_info, sps_client_env->sps_server.chars[SPS_FLOW_CTRL].val_hdl+1, val, sizeof(val) ,GATTC_WRITE);	
        
        // Go to connected state
        ke_state_set(con_info->prf_id, SPS_CLIENT_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(sps_client_envs, con_info->prf_id, SPS_CLIENT);
    }
    
    ke_msg_send(cfm);
}

#endif //BLE_SPS_CLIENT

/// @} SPS_CLIENT
