/**
 ****************************************************************************************
 *
 * @file tips_task.c
 *
 * @brief Time Profile Server Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup TIPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_TIP_SERVER)
#include "gap.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "tips.h"
#include "tips_task.h"

#include "prf_utils.h"


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_CREATE_DB_REQ message.
 * The handler adds CTS, NDCS and RTUS into the database using the database
 * configuration value given in param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tips_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct tips_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    //Service Configuration Flag - For CTS, Current Time Char. is mandatory
    uint8_t cfg_flag = TIPS_CTS_CURRENT_TIME_MASK;
    //Database Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    //Save Database Configuration
    tips_env.features = param->features;

    /*---------------------------------------------------*
     * Current Time Service Creation
     *---------------------------------------------------*/
    //Set Configuration Flag Value
    if (TIPS_IS_SUPPORTED(TIPS_CTS_LOC_TIME_INFO_SUP))
    {
        cfg_flag |= TIPS_CTS_LOC_TIME_INFO_MASK;
    }
    if (TIPS_IS_SUPPORTED(TIPS_CTS_REF_TIME_INFO_SUP))
    {
        cfg_flag |= TIPS_CTS_REF_TIME_INFO_MASK;
    }

    //Add Service Into Database
    status = attm_svc_create_db(&tips_env.cts_shdl, (uint8_t *)&cfg_flag, CTS_IDX_NB, &tips_env.cts_att_tbl[0],
                                dest_id, &cts_att_db[0]);
    //Disable CTS
    attmdb_svc_set_permission(tips_env.cts_shdl, PERM(SVC, DISABLE));

    //All attributes are mandatory for NDCS and RTUS
    cfg_flag = 0xFF;
    /*---------------------------------------------------*
     * Next DST Change Service Creation
     *---------------------------------------------------*/
    if ((status == ATT_ERR_NO_ERROR) && (TIPS_IS_SUPPORTED(TIPS_NDCS_SUP)))
    {
        status = attm_svc_create_db(&tips_env.ndcs_shdl, (uint8_t *)&cfg_flag, NDCS_IDX_NB, &tips_env.ndcs_att_tbl[0],
                                   dest_id, &ndcs_att_db[0]);

        //Disable NDCS
        attmdb_svc_set_permission(tips_env.ndcs_shdl, PERM(SVC, DISABLE));
    }

    /*---------------------------------------------------*
     * Reference Time Update Service Creation
     *---------------------------------------------------*/
    if ((status == ATT_ERR_NO_ERROR) && (TIPS_IS_SUPPORTED(TIPS_RTUS_SUP)))
    {
        status = attm_svc_create_db(&tips_env.rtus_shdl, (uint8_t *)&cfg_flag, RTUS_IDX_NB, &tips_env.rtus_att_tbl[0],
                                   dest_id, &rtus_att_db[0]);

        //Disable RTUS
        attmdb_svc_set_permission(tips_env.rtus_shdl, PERM(SVC, DISABLE));
    }

    //Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle test
        ke_state_set(TASK_TIPS, TIPS_IDLE);
    }

    //Send response to application
    struct tips_create_db_cfm * cfm = KE_MSG_ALLOC(TIPS_CREATE_DB_CFM, src_id, TASK_TIPS,
                                                   tips_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_ENABLE_REQ message.
 * The handler enables the Time Server Profile.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tips_enable_req_handler(ke_msg_id_t const msgid,
                                   struct tips_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Time Profile Server Role Task Index Environment
    struct tips_idx_env_tag *tips_idx_env;
    // Connection Information
    struct prf_con_info con_info;

    // Value used to initialize readable value in DB
    uint16_t value = 0;
    // Value used to initialize readable value whose size is upper than 2 bytes
    struct attm_elmt *att = NULL;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status = PRF_CLIENT_ENABLE(con_info, param, tips_idx);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        //If this connection is a not configuration one, apply configuration saved by APP
        if(param->con_type == PRF_CON_NORMAL)
        {
            memcpy(&value, &param->current_time_ntf_en, sizeof(uint16_t));

            if (param->current_time_ntf_en == PRF_CLI_START_NTF)
            {
                // Get the environment for the device
                tips_idx_env = PRF_CLIENT_GET_ENV(dest_id, tips_idx);

                tips_idx_env->ntf_state |= TIPS_CTS_CURRENT_TIME_CFG;
            }
        }

        //Set Current Time Char. NTF Configuration in database
        attmdb_att_set_value(tips_env.cts_shdl + CTS_IDX_CURRENT_TIME_CFG, sizeof(uint16_t),
                             (uint8_t *)&value);

        //Reset value
        value = 0;

        // Initialize Local Time Info Char. Value - (UTC+0.00 / Standard Time)
        if (TIPS_IS_SUPPORTED(TIPS_CTS_LOC_TIME_INFO_SUP))
        {
            //sizeof(struct tip_ref_time_info) = sizeof(uint16_t)
            attmdb_att_set_value(tips_env.cts_shdl + CTS_IDX_LOCAL_TIME_INFO_VAL,
                                 sizeof(struct tip_loc_time_info), (uint8_t *)&value);
        }

        // Initialize Reference Time Info Char. Value
        if (TIPS_IS_SUPPORTED(TIPS_CTS_REF_TIME_INFO_SUP))
        {
            att = attmdb_get_attribute(tips_env.cts_shdl + tips_env.cts_att_tbl[CTS_REF_TIME_INFO_CHAR] + 1);

            att->length = sizeof(struct tip_ref_time_info);
            memset(att->value, 0, sizeof(struct tip_ref_time_info));
        }

        //Enable CTS + Set Security Level
        attmdb_svc_set_permission(tips_env.cts_shdl, param->sec_lvl);

        if (TIPS_IS_SUPPORTED(TIPS_NDCS_SUP))
        {
            att = attmdb_get_attribute(tips_env.ndcs_shdl + NDCS_IDX_TIME_DST_VAL);

            // Initialize Time With DST Char. Value - (Idle / Successful)
            att->length = NDCS_TIME_DST_VAL_LEN;
            memset(att->value, 0, NDCS_TIME_DST_VAL_LEN);

            attmdb_svc_set_permission(tips_env.ndcs_shdl, param->sec_lvl);
        }
        if (TIPS_IS_SUPPORTED(TIPS_RTUS_SUP))
        {
            // Initialize Time Update State Char. Value - (Idle / Successful)
            attmdb_att_set_value(tips_env.rtus_shdl + RTUS_IDX_TIME_UPD_STATE_VAL,
                                 sizeof(struct tip_time_upd_state), (uint8_t *)&value);

            attmdb_svc_set_permission(tips_env.rtus_shdl, param->sec_lvl);
        }

        // Go to Connected State
        ke_state_set(dest_id, TIPS_CONNECTED);

        //send APP confirmation that can start normal connection
        tips_enable_cfm_send(&con_info, PRF_ERR_OK);
    }
    else
    {
        // Send confirmation that can start normal connection
        tips_enable_cfm_send(&con_info, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_UPD_CURR_TIME_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tips_upd_curr_time_req_handler(ke_msg_id_t const msgid,
                                          struct tips_upd_curr_time_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Packed Current Time value
    uint8_t pckd_time[CTS_CURRENT_TIME_VAL_LEN];
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Get the address of the environment
    struct tips_idx_env_tag *tips_idx_env = PRF_CLIENT_GET_ENV(dest_id, tips_idx);

    if (param->conhdl == gapc_get_conhdl(tips_idx_env->con_info.conidx))
    {
        // Pack the Current Time value
        tips_pack_curr_time_value(&pckd_time[0], &param->current_time);
        // Set the value in the database
        status = attmdb_att_set_value(tips_env.cts_shdl + CTS_IDX_CURRENT_TIME_VAL,
                                      CTS_CURRENT_TIME_VAL_LEN, (uint8_t *)&pckd_time[0]);

        //Check if Notifications are enabled
        if ((tips_idx_env->ntf_state & TIPS_CTS_CURRENT_TIME_CFG) == TIPS_CTS_CURRENT_TIME_CFG)
        {
            //Check if notification can be sent
            if ((param->current_time.adjust_reason & TIPS_FLAG_EXT_TIME_UPDATE) == TIPS_FLAG_EXT_TIME_UPDATE)
            {
                if (param->enable_ntf_send == 0)
                {
                    status = PRF_ERR_REQ_DISALLOWED;
                }
            }

            if (status == PRF_ERR_OK)
            {
                // The notification can be sent, send the notification
                prf_server_send_event((prf_env_struct *)tips_idx_env, false,
                                      tips_env.cts_shdl + CTS_IDX_CURRENT_TIME_VAL);
            }
        }
        else
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }

    if (status != PRF_ERR_OK)
    {
        // Wrong Connection Handle
        tips_error_ind_send(&(tips_idx_env->con_info), status, TIPS_UPD_CURR_TIME_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_UPD_LOCAL_TIME_INFO_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tips_upd_local_time_info_req_handler(ke_msg_id_t const msgid,
                                                struct tips_upd_local_time_info_req const *param,
                                                ke_task_id_t const dest_id,
                                                ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Get the address of the environment
    struct tips_idx_env_tag *tips_idx_env = PRF_CLIENT_GET_ENV(dest_id, tips_idx);

    // Check the Connection Handle
    if(param->conhdl == gapc_get_conhdl(tips_idx_env->con_info.conidx))
    {
        // Check if the Local Time Info Char. has been added in the database
        if (TIPS_IS_SUPPORTED(TIPS_CTS_LOC_TIME_INFO_SUP))
        {
            // Set the value in the database
            status = attmdb_att_set_value(tips_env.cts_shdl + CTS_IDX_LOCAL_TIME_INFO_VAL,
                                          sizeof(struct tip_loc_time_info), (uint8_t *)&(param->loc_time_info));
        }
        else
        {
            status = PRF_ERR_FEATURE_NOT_SUPPORTED;
        }
    }

    // Send a message to the application if an error has been raised.
    if (status != PRF_ERR_OK)
    {
        //Wrong Connection Handle
        tips_error_ind_send(&(tips_idx_env->con_info), status, TIPS_UPD_LOCAL_TIME_INFO_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_UPD_REF_TIME_INFO_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tips_upd_ref_time_info_req_handler(ke_msg_id_t const msgid,
                                              struct tips_upd_ref_time_info_req const *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Get the address of the environment
    struct tips_idx_env_tag *tips_idx_env = PRF_CLIENT_GET_ENV(dest_id, tips_idx);

    // Check the Connection Handle
    if (param->conhdl == gapc_get_conhdl(tips_idx_env->con_info.conidx))
    {
        // Check if the Reference Time Info Char. has been added in the database
        if (TIPS_IS_SUPPORTED(TIPS_CTS_REF_TIME_INFO_SUP))
        {
            status = attmdb_att_set_value(tips_env.cts_shdl + tips_env.cts_att_tbl[CTS_REF_TIME_INFO_CHAR] + 1,
                                          sizeof(struct tip_ref_time_info), (uint8_t *)&(param->ref_time_info));
        }
        else
        {
            status = PRF_ERR_FEATURE_NOT_SUPPORTED;
        }
    }

    if (status != PRF_ERR_OK)
    {
        //Wrong Connection Handle
        tips_error_ind_send(&(tips_idx_env->con_info), status, TIPS_UPD_REF_TIME_INFO_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_UPD_TIME_DST_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tips_upd_time_dst_req_handler(ke_msg_id_t const msgid,
                                                struct tips_upd_time_dst_req const *param,
                                                ke_task_id_t const dest_id,
                                                ke_task_id_t const src_id)
{
    // Packed Time With DST value
    uint8_t pckd_time_dst[NDCS_TIME_DST_VAL_LEN];
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Get the address of the environment
    struct tips_idx_env_tag *tips_idx_env = PRF_CLIENT_GET_ENV(dest_id, tips_idx);

    // Check the provided Connection Handle
    if (param->conhdl == gapc_get_conhdl(tips_idx_env->con_info.conidx))
    {
        // Check if the Next DST Change Service is supported
        if (TIPS_IS_SUPPORTED(TIPS_NDCS_SUP))
        {
            //Pack the Time with DST value and Save it into the DB
            tips_pack_time_dst_value(&pckd_time_dst[0], &param->time_with_dst);

            // Set the value in the database
            status = attmdb_att_set_value(tips_env.ndcs_shdl + NDCS_IDX_TIME_DST_VAL,
                                          NDCS_TIME_DST_VAL_LEN, (uint8_t *)&pckd_time_dst[0]);
        }
        else
        {
            status = PRF_ERR_FEATURE_NOT_SUPPORTED;
        }
    }

    if (status != PRF_ERR_OK)
    {
        //Wrong Connection Handle
        tips_error_ind_send(&(tips_idx_env->con_info), status, TIPS_UPD_TIME_DST_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_UPD_TIME_UPD_STATE_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tips_upd_time_upd_state_req_handler(ke_msg_id_t const msgid,
                                              struct tips_upd_time_upd_state_req const *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Get the address of the environment
    struct tips_idx_env_tag *tips_idx_env = PRF_CLIENT_GET_ENV(dest_id, tips_idx);

    // Check the provided Connection Handle
    if (param->conhdl == gapc_get_conhdl(tips_idx_env->con_info.conidx))
    {
        // Check if the Reference Time Update Service is supported
        if (TIPS_IS_SUPPORTED(TIPS_RTUS_SUP))
        {
            status = attmdb_att_set_value(tips_env.rtus_shdl + RTUS_IDX_TIME_UPD_STATE_VAL,
                                          sizeof(struct tip_time_upd_state), (uint8_t *)&(param->time_upd_state));
        }
        else
        {
            status = PRF_ERR_FEATURE_NOT_SUPPORTED;
        }
    }

    if (status != PRF_ERR_OK)
    {
        //Wrong Connection Handle
        tips_error_ind_send(&(tips_idx_env->con_info), status, TIPS_UPD_TIME_UPD_STATE_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY message meaning that Measurement
 * notification has been correctly sent to peer device (but not confirmed by peer device).
 * *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct tips_idx_env_tag *tips_idx_env = PRF_CLIENT_GET_ENV(dest_id, tips_idx);

    if (tips_idx_env != NULL)
    {
        if (param->req_type == GATTC_NOTIFY)
        {
            tips_error_ind_send(&(tips_idx_env->con_info), param->status, TIPS_UPD_CURR_TIME_REQ);
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_cmd_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint16_t cfg_value           = 0x0000;
    uint8_t time_upd_ctnl_pt     = 0x00;
    uint8_t status               = CO_ERROR_NO_ERROR;

    // Get the address of the environment
    struct tips_idx_env_tag *tips_idx_env = PRF_CLIENT_GET_ENV(dest_id, tips_idx);

        //CTS : Current Time Char. - Notification Configuration
    if (param->handle == tips_env.cts_shdl + CTS_IDX_CURRENT_TIME_CFG)
    {
        //Extract value before check
        memcpy(&cfg_value, &param->value[0], sizeof(uint16_t));

        //only update configuration if value for stop or notification enable
        if ((cfg_value == PRF_CLI_STOP_NTFIND) || (cfg_value == PRF_CLI_START_NTF))
        {
            //Update value in DB
            attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&cfg_value);

            if (cfg_value == PRF_CLI_STOP_NTFIND)
            {
                tips_idx_env->ntf_state &= ~TIPS_CTS_CURRENT_TIME_CFG;
            }
            else //PRF_CLI_START_NTF
            {
                tips_idx_env->ntf_state |= TIPS_CTS_CURRENT_TIME_CFG;
            }
            if (param->last)
            {
                //Inform APP of configuration change
                struct tips_current_time_ccc_ind * msg = KE_MSG_ALLOC(TIPS_CURRENT_TIME_CCC_IND,
                                                                      tips_idx_env->con_info.appid, dest_id,
                                                                      tips_current_time_ccc_ind);

                msg->conhdl  = gapc_get_conhdl(tips_idx_env->con_info.conidx);
                msg->cfg_val = cfg_value;

                ke_msg_send(msg);
            }
        }
        else
        {
            //Invalid value => send Error Response with Error Code set to App Error
            status = PRF_APP_ERROR;
        }
    }
    //RTUS : Time Update Control Point Char. - Val
    else if (param->handle == tips_env.rtus_shdl + RTUS_IDX_TIME_UPD_CTNL_PT_VAL)
    {
        time_upd_ctnl_pt = param->value[0];

        //Check if value to write is in allowed range
        if ((time_upd_ctnl_pt == TIPS_TIME_UPD_CTNL_PT_GET) ||
            (time_upd_ctnl_pt == TIPS_TIME_UPD_CTNL_PT_CANCEL))
        {
            //Update value in DB
            attmdb_att_set_value(param->handle, sizeof(uint8_t), (uint8_t *)&time_upd_ctnl_pt);

            if(param->last)
            {
                //Send APP the indication with the new value
                struct tips_time_upd_ctnl_pt_ind * msg = KE_MSG_ALLOC(TIPS_TIME_UPD_CTNL_PT_IND,
                                                                      tips_idx_env->con_info.appid, dest_id,
                                                                      tips_time_upd_ctnl_pt_ind);

                msg->conhdl  = gapc_get_conhdl(tips_idx_env->con_info.conidx);
                /*
                 * Time Update Control Point Characteristic Value
                 * 0x01 - Get Reference Update
                 * 0x02 - Cancel Reference Update
                 */
                msg->value = time_upd_ctnl_pt;

                ke_msg_send(msg);
            }
        }
    }

    //Send Write Response
    if (param->response == 0x01)
    {
        atts_write_rsp_send(tips_idx_env->con_info.conidx, param->handle, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to TIPS.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                      struct gapc_disconnect_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct tips_idx_env_tag *tips_idx_env = PRF_CLIENT_GET_ENV(dest_id, tips_idx);

    if (tips_idx_env != NULL)
    {
        //Check Connection Handle
        tips_disable(tips_idx_env, param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler tips_disabled[] =
{
    {TIPS_CREATE_DB_REQ,            (ke_msg_func_t)tips_create_db_req_handler}
};

/// Idle State handler definition.
const struct ke_msg_handler tips_idle[] =
{
    {TIPS_ENABLE_REQ,               (ke_msg_func_t)tips_enable_req_handler}
};

/// Connected State handler definition.
const struct ke_msg_handler tips_connected[] =
{
    {TIPS_UPD_CURR_TIME_REQ,            (ke_msg_func_t)tips_upd_curr_time_req_handler},
    {TIPS_UPD_LOCAL_TIME_INFO_REQ,      (ke_msg_func_t)tips_upd_local_time_info_req_handler},
    {TIPS_UPD_REF_TIME_INFO_REQ,        (ke_msg_func_t)tips_upd_ref_time_info_req_handler},
    {TIPS_UPD_TIME_DST_REQ,             (ke_msg_func_t)tips_upd_time_dst_req_handler},
    {TIPS_UPD_TIME_UPD_STATE_REQ,       (ke_msg_func_t)tips_upd_time_upd_state_req_handler},
    {GATTC_WRITE_CMD_IND,               (ke_msg_func_t)gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,                     (ke_msg_func_t)gattc_cmp_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler tips_default_state[] =
{
    {GAPC_DISCONNECT_IND,        (ke_msg_func_t)gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler tips_state_handler[TIPS_STATE_MAX] =
{
    [TIPS_DISABLED]    = KE_STATE_HANDLER(tips_disabled),
    [TIPS_IDLE]        = KE_STATE_HANDLER(tips_idle),
    [TIPS_CONNECTED]   = KE_STATE_HANDLER(tips_connected),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler tips_default_handler = KE_STATE_HANDLER(tips_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t tips_state[TIPS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_TIP_SERVER

/// @} TIPSTASK
