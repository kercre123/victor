/**
 ****************************************************************************************
 *
 * @file pasps_task.c
 *
 * @brief Phone Alert Status Profile Server Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup PASPSTASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_PAS_SERVER)
#include "pasp_common.h"
#include "gap.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "pasps.h"
#include "pasps_task.h"
#include "prf_utils.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PASPS_CREATE_DB_REQ message.
 * @param[in] msgid Id of the message received
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int pasps_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct pasps_create_db_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Service Configuration Flag - All attributes have to be added in the database
    uint16_t cfg_flag = PASPS_DB_CFG_FLAG;
    // Database Creation Status
    uint8_t status;
    // Counter
    uint8_t counter;

    // Check if a PAS has already been added in the database
    if (ke_state_get(TASK_PASPS) == PASPS_DISABLED)
    {
        // Check the provided values
        if ((param->alert_status   <= PASP_ALERT_STATUS_VAL_MAX) &&
            (param->ringer_setting <= PASP_RINGER_NORMAL))
        {
            // Add service in the database
            status = attm_svc_create_db(&pasps_env.pass_shdl, (uint8_t *)&cfg_flag, PASS_IDX_NB, NULL,
                                        dest_id, &pasps_att_db[0]);

            // Go to Idle State
            if (status == ATT_ERR_NO_ERROR)
            {
                // Set the value of the Alert Status characteristic
                attmdb_att_set_value(pasps_env.pass_shdl + PASS_IDX_ALERT_STATUS_VAL, sizeof(uint8_t),
                                     (uint8_t *)&param->alert_status);
                // Set the value of the Ringer Setting characteristic
                attmdb_att_set_value(pasps_env.pass_shdl + PASS_IDX_RINGER_SETTING_VAL, sizeof(uint8_t),
                                     (uint8_t *)&param->ringer_setting);

                // Update the ringer state in the environment
                pasps_env.ringer_state = param->ringer_setting;

                // Disable PAS
                attmdb_svc_set_permission(pasps_env.pass_shdl, PERM(SVC, DISABLE));

                for (counter = 0; counter < PASPS_IDX_MAX; counter++)
                {
                    // If we are here, database has been fulfilled with success, go to idle state
                    ke_state_set(KE_BUILD_ID(TASK_PASPS, counter), PASPS_IDLE);
                }
            }
        }
        else
        {
            // One of the provided value in not within the defined range
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    else
    {
        // Request is disallowed, a PAS has already been added
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // Send response to application
    pasps_send_cmp_evt(TASK_PASPS, src_id, GAP_INVALID_CONHDL, PASPS_CREATE_DB_OP_CODE, status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PASPS_ENABLE_REQ message.
 * @param[in] msgid Id of the message received
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int pasps_enable_req_handler(ke_msg_id_t const msgid,
                                   struct pasps_enable_req *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Phone Alert Status Profile Server Role Task Index Environment
    struct pasps_idx_env_tag *pasps_idx_env = NULL;
    // Connection Information structure
    struct prf_con_info con_info;
    // Status
    uint8_t status     = PRF_ERR_OK;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Check if the provided connection handle is valid
    if (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX)
    {
        // Fill the Connection Information structure
        con_info.conidx = gapc_get_conidx(param->conhdl);
        con_info.prf_id = dest_id;
        con_info.appid  = src_id;

        // Add an environment for the provided device
        status = PRF_CLIENT_ENABLE(con_info, param, pasps_idx);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status == PRF_ERR_OK)
    {
        pasps_idx_env = PRF_CLIENT_GET_ENV(dest_id, pasps_idx);

        // Keep the Connection Information structure content
        memcpy(&pasps_idx_env->con_info, &con_info, sizeof(struct prf_con_info));

        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Force the configuration to 0
            param->alert_status_ntf_cfg   = PRF_CLI_STOP_NTFIND;
            param->ringer_setting_ntf_cfg = PRF_CLI_STOP_NTFIND;
        }
        else
        {
            if (param->alert_status_ntf_cfg != PRF_CLI_STOP_NTFIND)
            {
                // Force to PRF_CLI_START_NTF
                param->alert_status_ntf_cfg = PRF_CLI_START_NTF;

                PASPS_ENABLE_NTF(pasps_idx_env, PASPS_FLAG_ALERT_STATUS_CFG);
            }

            if (param->ringer_setting_ntf_cfg != PRF_CLI_STOP_NTFIND)
            {
                // Force to PRF_CLI_START_NTF
                param->ringer_setting_ntf_cfg = PRF_CLI_START_NTF;

                PASPS_ENABLE_NTF(pasps_idx_env, PASPS_FLAG_RINGER_SETTING_CFG);
            }
        }

        // Set the value of the Alert Status Client Characteristic Configuration Descriptor
        attmdb_att_set_value(pasps_env.pass_shdl + PASS_IDX_ALERT_STATUS_CFG, sizeof(uint16_t),
                             (uint8_t *)&param->alert_status_ntf_cfg);
        // Set the value of the Ringer Setting Client Characteristic Configuration Descriptor
        attmdb_att_set_value(pasps_env.pass_shdl + PASS_IDX_RINGER_SETTING_CFG, sizeof(uint16_t),
                             (uint8_t *)&param->ringer_setting_ntf_cfg);

        // Enable PAS
        attmdb_svc_set_permission(pasps_env.pass_shdl, param->sec_lvl);

        // Go to Connected State
        ke_state_set(dest_id, PASPS_CONNECTED);

        // Send response to application
        pasps_send_cmp_evt(dest_id, src_id, param->conhdl, PASPS_ENABLE_OP_CODE, PRF_ERR_OK);
    }
    else if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        msg_status = KE_MSG_NO_FREE;
    }
    else
    {
        // Send response to application
        pasps_send_cmp_evt(dest_id, src_id, param->conhdl, PASPS_ENABLE_OP_CODE, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PASPS_UPDATE_CHAR_VAL_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int pasps_update_char_val_cmd_handler(ke_msg_id_t const msgid,
                                             struct pasps_update_char_val_cmd *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct pasps_idx_env_tag *pasps_idx_env = PRF_CLIENT_GET_ENV(dest_id, pasps_idx);


    if (pasps_idx_env != NULL)
    {
        // Check the provided connection handle value
        if (pasps_idx_env->con_info.conidx != gapc_get_conidx(param->conhdl))
        {
            status = PRF_ERR_INVALID_PARAM;
        }
        else if (ke_state_get(dest_id) == PASPS_BUSY)
        {
            // Keep the message for later
            msg_status = KE_MSG_SAVED;
        }
        else
        {
            // Handle
            uint16_t handle;
            // Notification status flag
            uint8_t flag;

            ASSERT_ERR(ke_state_get(dest_id) == PASPS_CONNECTED);

            switch (param->operation)
            {
                // Alert Status Characteristic
                case (PASPS_UPD_ALERT_STATUS_OP_CODE):
                {
                    // Check the provided value
                    if (param->value <= PASP_ALERT_STATUS_VAL_MAX)
                    {
                        // Set the handle value
                        handle = pasps_env.pass_shdl + PASS_IDX_ALERT_STATUS_VAL;
                        // Set the flag
                        flag   = PASPS_FLAG_ALERT_STATUS_CFG;

                        status = PRF_ERR_OK;
                    }
                    // else status is PRF_ERR_INVALID_PARAM
                } break;

                // Ringer Setting Characteristic
                case (PASPS_UPD_RINGER_SETTING_OP_CODE):
                {
                    // Check the provided value
                    if (param->value <= PASP_RINGER_NORMAL)
                    {
                        // Set the handle value
                        handle = pasps_env.pass_shdl + PASS_IDX_RINGER_SETTING_VAL;
                        // Set the flag
                        flag   = PASPS_FLAG_RINGER_SETTING_CFG;

                        status = PRF_ERR_OK;

                        // Update the ringer state value
                        pasps_env.ringer_state = param->value;
                    }
                    // else status is PRF_ERR_INVALID_PARAM
                } break;

                default:
                {
                    // Nothing more to do, status is PRF_ERR_INVALID_PARAM
                } break;
            }

            if (status == PRF_ERR_OK)
            {
                // Set the value of the characteristic
                attmdb_att_set_value(handle, sizeof(uint8_t), (uint8_t *)&param->value);

                // Check if sending of notifications is enabled for this connection
                if (PASPS_IS_NTF_ENABLED(pasps_idx_env, flag))
                {
                    // Configure the environment for the operation
                    pasps_idx_env->operation = param->operation;
                    // Go to the Busy state
                    ke_state_set(dest_id, PASPS_BUSY);

                    // The notification can be sent, send the notification
                    prf_server_send_event((prf_env_struct *)pasps_idx_env, false, handle);
                }
                else
                {
                    status = PRF_ERR_NTF_DISABLED;
                }
            }
        }

        // If the status is not OK, no notification has been sent
        if (status != PRF_ERR_OK)
        {
            // Send response to application
            pasps_send_cmp_evt(pasps_idx_env->con_info.prf_id, pasps_idx_env->con_info.appid,
                               param->conhdl,
                               param->operation, status);
        }
    }
    else
    {
        // Send response to application
        pasps_send_cmp_evt(dest_id, src_id, param->conhdl, param->operation, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_CMD_IND message.
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
    // Get the conidx
    uint8_t conidx = KE_IDX_GET(src_id);
    // Get the address of the environment
    struct pasps_idx_env_tag *pasps_idx_env = PRF_CLIENT_GET_ENV(KE_BUILD_ID(dest_id, conidx), pasps_idx);

    // Check if the connection exists
    if (pasps_idx_env != NULL)
    {
        /*
         * ---------------------------------------------------------------------------------------------
         * Alert Status Client Characteristic Configuration Descriptor Value - Write
         * ---------------------------------------------------------------------------------------------
         */
        /*
         * ---------------------------------------------------------------------------------------------
         * Ringer Setting Client Characteristic Configuration Descriptor Value - Write
         * ---------------------------------------------------------------------------------------------
         */
        if ((param->handle == (pasps_env.pass_shdl + PASS_IDX_ALERT_STATUS_CFG)) ||
            (param->handle == (pasps_env.pass_shdl + PASS_IDX_RINGER_SETTING_CFG)))
        {
            // Status
            uint8_t status = PRF_ERR_OK;
            // Received configuration value
            uint16_t ntf_cfg = co_read16p(&param->value[0]);

            if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
            {
                // Set the value of the Alert Status Client Characteristic Configuration Descriptor
                attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg);

                struct pasps_written_char_val_ind *ind = KE_MSG_ALLOC(PASPS_WRITTEN_CHAR_VAL_IND,
                                                                      pasps_idx_env->con_info.appid, dest_id,
                                                                      pasps_written_char_val_ind);

                ind->conhdl   = gapc_get_conhdl(pasps_idx_env->con_info.conidx);

                if (param->handle == pasps_env.pass_shdl + PASS_IDX_ALERT_STATUS_CFG)
                {
                    ind->value.alert_status_ntf_cfg = ntf_cfg;
                    ind->att_code = PASPS_ALERT_STATUS_NTF_CFG;

                    if (ntf_cfg == PRF_CLI_STOP_NTFIND)
                    {
                        PASPS_DISABLE_NTF(pasps_idx_env, PASPS_FLAG_ALERT_STATUS_CFG);
                    }
                    else
                    {
                        PASPS_ENABLE_NTF(pasps_idx_env, PASPS_FLAG_ALERT_STATUS_CFG);
                    }
                }
                else
                {
                    ind->value.ringer_setting_ntf_cfg = ntf_cfg;
                    ind->att_code = PASPS_RINGER_SETTING_NTF_CFG;

                    if (ntf_cfg == PRF_CLI_STOP_NTFIND)
                    {
                        PASPS_DISABLE_NTF(pasps_idx_env, PASPS_FLAG_RINGER_SETTING_CFG);
                    }
                    else
                    {
                        PASPS_ENABLE_NTF(pasps_idx_env, PASPS_FLAG_RINGER_SETTING_CFG);
                    }
                }

                ke_msg_send(ind);
            }
            else
            {
                status = PRF_APP_ERROR;
            }

            // Send write response
            atts_write_rsp_send(pasps_idx_env->con_info.conidx, param->handle, status);
        }
        /*
         * ---------------------------------------------------------------------------------------------
         * Ringer Control Point Characteristic Value - Write Without Response
         * ---------------------------------------------------------------------------------------------
         */
        else if (param->handle == (pasps_env.pass_shdl + PASS_IDX_RINGER_CTNL_PT_VAL))
        {
            // Inform the HL ?
            bool inform_hl = false;

            // Check the received value
            switch (param->value[0])
            {
                case (PASP_SILENT_MODE):
                {
                    // Ignore if ringer is already silent
                    if (pasps_env.ringer_state == PASP_RINGER_NORMAL)
                    {
                        inform_hl = true;
                    }
                } break;

                case (PASP_CANCEL_SILENT_MODE):
                {
                    // Ignore if ringer is not silent
                    if (pasps_env.ringer_state == PASP_RINGER_SILENT)
                    {
                        inform_hl = true;
                    }
                } break;

                case (PASP_MUTE_ONCE):
                {
                    inform_hl = true;
                } break;

                // No need to respond with an error (Write Without Response)
                default: break;
            }

            if (inform_hl)
            {
                struct pasps_written_char_val_ind *ind = KE_MSG_ALLOC(PASPS_WRITTEN_CHAR_VAL_IND,
                                                                      pasps_idx_env->con_info.appid, TASK_PASPS,
                                                                      pasps_written_char_val_ind);

                ind->conhdl   = gapc_get_conhdl(pasps_idx_env->con_info.conidx);;
                ind->att_code = PASPS_RINGER_CTNL_PT_CHAR_VAL;
                ind->value.ringer_ctnl_pt = param->value[0];

                ke_msg_send(ind);
            }
        }
        else
        {
            ASSERT_ERR(0);
        }
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT message meaning that a notification or an indication
 * has been correctly sent to peer device (but not confirmed by peer device).
 *
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
    struct pasps_idx_env_tag *pasps_idx_env = PRF_CLIENT_GET_ENV(dest_id, pasps_idx);

    if (pasps_idx_env != NULL)
    {
        // Send a complete event status to the application
        pasps_send_cmp_evt(dest_id, pasps_idx_env->con_info.appid,
                           gapc_get_conhdl(pasps_idx_env->con_info.conidx),
                           pasps_idx_env->operation, param->status);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to PASPS.
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
    struct pasps_idx_env_tag *pasps_idx_env = PRF_CLIENT_GET_ENV(dest_id, pasps_idx);

    // Check if the profile was enabled for this connection
    if (pasps_idx_env != NULL)
    {
        pasps_disable(pasps_idx_env, param->conhdl);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler pasps_default_state[] =
{
    {PASPS_CREATE_DB_REQ,           (ke_msg_func_t)pasps_create_db_req_handler},
    {PASPS_ENABLE_REQ,              (ke_msg_func_t)pasps_enable_req_handler},
    {PASPS_UPDATE_CHAR_VAL_CMD,     (ke_msg_func_t)pasps_update_char_val_cmd_handler},

    {GATTC_WRITE_CMD_IND,           (ke_msg_func_t)gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler pasps_state_handler[PASPS_STATE_MAX] =
{
    [PASPS_DISABLED]    = KE_STATE_HANDLER_NONE,
    [PASPS_IDLE]        = KE_STATE_HANDLER_NONE,
    [PASPS_CONNECTED]   = KE_STATE_HANDLER_NONE,
    [PASPS_BUSY]        = KE_STATE_HANDLER_NONE,
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler pasps_default_handler = KE_STATE_HANDLER(pasps_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t pasps_state[PASPS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_PASS_SERVER)

/// @} PASPSTASK
