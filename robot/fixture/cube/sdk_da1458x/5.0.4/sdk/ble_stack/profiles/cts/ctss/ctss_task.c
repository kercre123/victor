/**
 ****************************************************************************************
 *
 * @file ctss_task.c
 *
 * @brief Current Time Service Task Implementation.
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
 * @addtogroup CTSSTASK
 * @{
 ****************************************************************************************
 */
#include "rwip_config.h"

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_CTS_SERVER)
#include "gapc_task.h"
#include "gattc_task.h"
#include "prf_utils.h"
#include "atts_util.h"
#include "attm_util.h"

#include "cts_common.h"
#include "ctss.h"
#include "ctss_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// CTS Configuration Flag Masks
#define CTSS_CURRENT_TIME_MASK      (0x0F)
#define CTSS_LOC_TIME_INFO_MASK     (0x30)
#define CTSS_REF_TIME_INFO_MASK     (0xC0)

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CTSS_CREATE_DB_REQ message.
 * The handler adds CTS into the database using the database
 * configuration value given in param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ctss_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct ctss_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Service Configuration Flag - For CTS, Current Time Char. is mandatory
    uint8_t cfg_flag = CTSS_CURRENT_TIME_MASK;
    // Database Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    // Save Database Configuration
    ctss_env.features = param->features;

    // Set Configuration Flag Value
    if (CTSS_IS_SUPPORTED(CTSS_LOC_TIME_INFO_SUP))
    {
        cfg_flag |= CTSS_LOC_TIME_INFO_MASK;
    }
    if (CTSS_IS_SUPPORTED(CTSS_REF_TIME_INFO_SUP))
    {
        cfg_flag |= CTSS_REF_TIME_INFO_MASK;
    }

    // Add Service Into Database
    status = attm_svc_create_db(&ctss_env.shdl, (uint8_t *)&cfg_flag, CTSS_IDX_NB,
                                    &ctss_env.att_tbl[0], dest_id, &att_db[0]);
    // Disable CTS
    attmdb_svc_set_permission(ctss_env.shdl, PERM(SVC, DISABLE));

    // Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle test
        ke_state_set(TASK_CTSS, CTSS_IDLE);
    }

    // Send response to application
    struct ctss_create_db_cfm *cfm = KE_MSG_ALLOC(CTSS_CREATE_DB_CFM, src_id,
                                                    TASK_CTSS, ctss_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CTSS_ENABLE_REQ message.
 * The handler enables the Current Time Service.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ctss_enable_req_handler(ke_msg_id_t const msgid,
                                   struct ctss_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Value used to initialize readable value in DB
    uint8_t value_buf[CTSS_CURRENT_TIME_VAL_LEN];

    // Fill the Connection Information structure
    ctss_env.con_info.conidx = gapc_get_conidx(param->conhdl);
    ctss_env.con_info.prf_id = dest_id;
    ctss_env.con_info.appid  = src_id;

    if (ctss_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        ctss_error_ind_send(&(ctss_env.con_info), PRF_ERR_REQ_DISALLOWED,
                                                            CTSS_ENABLE_REQ);

        return (KE_MSG_CONSUMED);
    }

    // If this connection is a not configuration one, apply configuration saved by APP
    if(param->con_type == PRF_CON_NORMAL)
    {
        memcpy(value_buf, &param->current_time_ntf_en, sizeof(uint16_t));

        if (param->current_time_ntf_en == PRF_CLI_START_NTF)
        {
            ctss_env.features |= CTSS_CURRENT_TIME_CFG;
        }
    }

    memset(value_buf, 0, CTSS_CURRENT_TIME_VAL_LEN);

    // Set Current Time Char. NTF Configuration in database
    attmdb_att_set_value(ctss_env.shdl + CTSS_IDX_CURRENT_TIME_CFG, sizeof(uint16_t),
                         (uint8_t *) value_buf);

    // Set Current Time Char. Default Value in database
    attmdb_att_set_value(ctss_env.shdl + CTSS_IDX_CURRENT_TIME_VAL,
                                        CTSS_CURRENT_TIME_VAL_LEN, (uint8_t *) value_buf);

    // Initialize Local Time Info Char. Value - (UTC+0.00 / Standard Time)
    if (CTSS_IS_SUPPORTED(CTSS_LOC_TIME_INFO_SUP))
    {
        // sizeof(struct tip_ref_time_info) = sizeof(uint16_t)
        attmdb_att_set_value(ctss_env.shdl + CTSS_IDX_LOCAL_TIME_INFO_VAL,
                             sizeof(struct cts_loc_time_info), (uint8_t *) value_buf);
    }

    // Initialize Reference Time Info Char. Value
    if (CTSS_IS_SUPPORTED(CTSS_REF_TIME_INFO_SUP))
    {
        attmdb_att_set_value(ctss_env.shdl + ctss_env.att_tbl[CTSS_REF_TIME_INFO_CHAR] + 1,
                             sizeof(struct cts_ref_time_info), (uint8_t *) value_buf);
    }

    // Enable CTS + Set Security Level
    attmdb_svc_set_permission(ctss_env.shdl, param->sec_lvl);

    // Go to Connected State
    ke_state_set(dest_id, CTSS_CONNECTED);

    // Send APP confirmation that can start normal connection
    ctss_enable_cfm_send(&ctss_env.con_info, PRF_ERR_OK);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CTSS_UPD_CURR_TIME_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ctss_upd_curr_time_req_handler(ke_msg_id_t const msgid,
                                          struct ctss_upd_curr_time_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Packed Current Time value
    uint8_t pckd_time[CTSS_CURRENT_TIME_VAL_LEN];
    // Status
    uint8_t status;

    // Pack the Current Time value
    ctss_pack_curr_time_value(&pckd_time[0], &param->current_time);
    // Set the value in the database
    status = attmdb_att_set_value(ctss_env.shdl + CTSS_IDX_CURRENT_TIME_VAL,
                                  CTSS_CURRENT_TIME_VAL_LEN, (uint8_t *)&pckd_time[0]);
    if (status != PRF_ERR_OK)
        goto done;

    if (param->conhdl == gapc_get_conhdl(ctss_env.con_info.conidx))
    {
        // Check if Notifications are enabled
        if ((ctss_env.features & CTSS_CURRENT_TIME_CFG) == CTSS_CURRENT_TIME_CFG)
        {
            // Check if notification can be sent
            if ((param->current_time.adjust_reason & CTSS_REASON_FLAG_EXT_TIME_UPDATE) == CTSS_REASON_FLAG_EXT_TIME_UPDATE)
            {
                if (param->enable_ntf_send == 0)
                {
                    status = PRF_ERR_REQ_DISALLOWED;
                }
            }

            if (status == PRF_ERR_OK)
            {
                // The notification can be sent, send the notification
                prf_server_send_event((prf_env_struct *)&ctss_env, false,
                                      ctss_env.shdl + CTSS_IDX_CURRENT_TIME_VAL);
            }
        }
        else
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }
    else if (param->enable_ntf_send)
    {
         status = PRF_ERR_INVALID_PARAM;
    }

done:
    if (status != PRF_ERR_OK)
    {
        // Wrong Connection Handle
        ctss_error_ind_send(&(ctss_env.con_info), status, CTSS_UPD_CURR_TIME_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CTSS_UPD_LOCAL_TIME_INFO_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ctss_upd_local_time_info_req_handler(ke_msg_id_t const msgid,
                                                struct ctss_upd_local_time_info_req const *param,
                                                ke_task_id_t const dest_id,
                                                ke_task_id_t const src_id)
{
    // Status
    uint8_t status;

    // Check if the Local Time Info Char. has been added in the database
    if (CTSS_IS_SUPPORTED(CTSS_LOC_TIME_INFO_SUP))
    {
        // Set the value in the database
        status = attmdb_att_set_value(ctss_env.shdl + ctss_env.att_tbl[CTSS_LOCAL_TIME_INFO_CHAR] + 1,
                                      sizeof(struct cts_loc_time_info), (uint8_t *)&(param->loc_time_info));
    }
    else
    {
        status = PRF_ERR_FEATURE_NOT_SUPPORTED;
    }

    // Send a message to the application if an error has been raised.
    if (status != PRF_ERR_OK)
    {
        //Wrong Connection Handle
        ctss_error_ind_send(&(ctss_env.con_info), status, CTSS_UPD_LOCAL_TIME_INFO_REQ);
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CTSS_UPD_REF_TIME_INFO_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ctss_upd_ref_time_info_req_handler(ke_msg_id_t const msgid,
                                              struct ctss_upd_ref_time_info_req const *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Check if the Reference Time Info Char. has been added in the database
    if (CTSS_IS_SUPPORTED(CTSS_REF_TIME_INFO_SUP))
    {
        status = attmdb_att_set_value(ctss_env.shdl + ctss_env.att_tbl[CTSS_REF_TIME_INFO_CHAR] + 1,
                                      sizeof(struct cts_ref_time_info), (uint8_t *)&(param->ref_time_info));
    }
    else
    {
        status = PRF_ERR_FEATURE_NOT_SUPPORTED;
    }

    if (status != PRF_ERR_OK)
    {
        // Wrong Connection Handle
        ctss_error_ind_send(&(ctss_env.con_info), status, CTSS_UPD_REF_TIME_INFO_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY message meaning that Measurement
 * notification has been correctly sent to peer device (but not confirmed by peer device).
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
    if (param->req_type == GATTC_NOTIFY)
        ctss_error_ind_send(&(ctss_env.con_info), param->status,
                                                    CTSS_UPD_CURR_TIME_REQ);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_CMD_IND message.
 *
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
    uint8_t status               = CO_ERROR_NO_ERROR;

    // CTS : Current Time Char. - Notification Configuration
    if (param->handle == ctss_env.shdl + CTSS_IDX_CURRENT_TIME_CFG)
    {
        // Extract value before check
        memcpy(&cfg_value, &param->value[0], sizeof(uint16_t));

        // Only update configuration if value for stop or notification enable
        if ((cfg_value == PRF_CLI_STOP_NTFIND) || (cfg_value == PRF_CLI_START_NTF))
        {
            // Update value in DB
            attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&cfg_value);

            if (cfg_value == PRF_CLI_STOP_NTFIND)
            {
                ctss_env.features &= ~CTSS_CURRENT_TIME_CFG;
            }
            else // PRF_CLI_START_NTF
            {
                ctss_env.features |= CTSS_CURRENT_TIME_CFG;
            }
            if (param->last)
            {
                // Inform APP of configuration change
                struct ctss_current_time_ccc_ind *msg = KE_MSG_ALLOC(CTSS_CURRENT_TIME_CCC_IND,
                                                                      ctss_env.con_info.appid, dest_id,
                                                                      ctss_current_time_ccc_ind);

                msg->conhdl  = gapc_get_conhdl(ctss_env.con_info.conidx);
                msg->cfg_val = cfg_value;

                ke_msg_send(msg);
            }
        }
        else
        {
            // Invalid value => send Error Response with Error Code set to App Error
            status = PRF_APP_ERROR;
        }
    }

    // Send Write Response
    if (param->response == 0x01)
    {
        atts_write_rsp_send(ctss_env.con_info.conidx, param->handle, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to CTSS.
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
    if (KE_IDX_GET(src_id) == ctss_env.con_info.conidx)
    {
        ctss_disable(param->conhdl);
    }

	return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler ctss_disabled[] =
{
    {CTSS_CREATE_DB_REQ,            (ke_msg_func_t)ctss_create_db_req_handler}
};

/// Idle State handler definition.
const struct ke_msg_handler ctss_idle[] =
{
    {CTSS_ENABLE_REQ,               (ke_msg_func_t)ctss_enable_req_handler},
    {CTSS_UPD_CURR_TIME_REQ,        (ke_msg_func_t)ctss_upd_curr_time_req_handler},
    {CTSS_UPD_LOCAL_TIME_INFO_REQ,  (ke_msg_func_t)ctss_upd_local_time_info_req_handler},
    {CTSS_UPD_REF_TIME_INFO_REQ,    (ke_msg_func_t)ctss_upd_ref_time_info_req_handler},
};

/// Connected State handler definition.
const struct ke_msg_handler ctss_connected[] =
{
    {CTSS_UPD_CURR_TIME_REQ,            (ke_msg_func_t)ctss_upd_curr_time_req_handler},
    {CTSS_UPD_LOCAL_TIME_INFO_REQ,      (ke_msg_func_t)ctss_upd_local_time_info_req_handler},
    {CTSS_UPD_REF_TIME_INFO_REQ,        (ke_msg_func_t)ctss_upd_ref_time_info_req_handler},
    {GATTC_WRITE_CMD_IND,               (ke_msg_func_t)gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,                     (ke_msg_func_t)gattc_cmp_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler ctss_default_state[] =
{
    {GAPC_DISCONNECT_IND,        (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler ctss_state_handler[CTSS_STATE_MAX] =
{
    [CTSS_DISABLED]    = KE_STATE_HANDLER(ctss_disabled),
    [CTSS_IDLE]        = KE_STATE_HANDLER(ctss_idle),
    [CTSS_CONNECTED]   = KE_STATE_HANDLER(ctss_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler ctss_default_handler = KE_STATE_HANDLER(ctss_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t ctss_state[CTSS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_CTS_SERVER

/// @} CTSSTASK
