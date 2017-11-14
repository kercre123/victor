/**
 ****************************************************************************************
 *
 * @file wssc.c
 *
 * @brief Weight Scale Service Collector Implementation.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
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
 * @addtogroup wssc
 * @{
 ****************************************************************************************
 */

 /*
  * INCLUDE FILES
  ****************************************************************************************
  */
#include "rwip_config.h"
#if (BLE_WSS_COLLECTOR)
#include "wssc.h"
#include "wssc_task.h"
#include "gap.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct wssc_env_tag **wssc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Wieght Scale Service task descriptor
static const struct ke_task_desc TASK_DESC_WSSC = {wssc_state_handler, &wssc_default_handler, wssc_state, WSSC_STATE_MAX, WSSC_IDX_MAX};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void wssc_init(void)
{
    PRF_CLIENT_RESET(wssc_envs, WSSC);
}

void wssc_enable_cfm_send(struct wssc_env_tag *wssc_env, struct prf_con_info *con_info, uint8_t status)
{
    struct wssc_enable_cfm *cfm = KE_MSG_ALLOC(WSSC_ENABLE_CFM,
                                               con_info->appid,
                                               con_info->prf_id,
                                               wssc_enable_cfm);

    // Connection handle
    cfm->conhdl = gapc_get_conhdl(con_info->conidx);
    // Status
    cfm->status = status;

    if(status == PRF_ERR_OK)
    {
        cfm->wss = wssc_env->wss;

        prf_register_atthdl2gatt(&wssc_env->con_info, &wssc_env->wss.svc);

        // Go to connected state
        ke_state_set(con_info->prf_id, WSSC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(wssc_envs,con_info->prf_id,WSSC);
    }

    ke_msg_send(cfm);
};

void wssc_unpack_meas_value(struct wssc_env_tag *wssc_env, uint8_t *packed_data, uint8_t lenght)
{
    uint8_t cursor = 0;
    
    if(lenght >= WSSC_PACKED_MEAS_MIN_LEN)
    {
        struct wssc_ws_ind *ind = KE_MSG_ALLOC(WSSC_WS_IND,
                                               wssc_env->con_info.appid,
                                               wssc_env->con_info.prf_id,
                                               wssc_ws_ind);

        ind->conhdl = gapc_get_conhdl(wssc_env->con_info.conidx);

        // Unpack flags
        ind->meas_val.flags = *(packed_data + cursor);
        cursor += 1;

        // Unpack weight value
        ind->meas_val.weight = co_read16p(packed_data + cursor) * 
                               (ind->meas_val.flags & WSS_MEAS_FLAG_UNIT_IMPERIAL ? 0.01 : 0.005);

        cursor += 2;

        // Unpack Time Stampt if present
        if(ind->meas_val.flags & WSS_MEAS_FLAG_TIME_STAMP)
        {
            cursor += prf_unpack_date_time(packed_data + cursor, &(ind->meas_val.datetime));
        }

        // Unpack user ID if present
        if(ind->meas_val.flags & WSS_MEAS_FLAG_USERID_PRESENT)
        {
            ind->meas_val.userid = *(packed_data + cursor);
            cursor += 1;
        }
        // Unpack BMI and height if present
        if(ind->meas_val.flags & WSS_MEAS_FLAG_BMI_HT_PRESENT)
        {   // BMI
            ind->meas_val.bmi = co_read16p(packed_data + cursor) * 0.1;
            cursor += 2;

            // Height
            ind->meas_val.height = co_read16p(packed_data + cursor) * 
                                   (ind->meas_val.flags & WSS_MEAS_FLAG_UNIT_IMPERIAL ? 0.01 : 0.005);
        }

        ke_msg_send(ind);

    }
}

void wssc_error_ind_send(struct wssc_env_tag *wssc_env, uint8_t status)
{
    struct wssc_error_ind *ind = KE_MSG_ALLOC(WSSC_ERROR_IND,
                                              wssc_env->con_info.appid, wssc_env->con_info.prf_id,
                                              wssc_error_ind);

    ind->conhdl    = gapc_get_conhdl(wssc_env->con_info.conidx);
    // It will be an HTPC status code
    ind->status    = status;

    // Send the message
    ke_msg_send(ind);
}

#endif //BLE_WSS_COLLECTOR
/// @} WSSC
