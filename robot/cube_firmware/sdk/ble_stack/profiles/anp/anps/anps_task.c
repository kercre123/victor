/**
 ****************************************************************************************
 *
 * @file anps_task.c
 *
 * @brief Alert Notification Profile Server Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev: $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup ANPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_AN_SERVER)

#include "gapm.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "anps.h"
#include "anps_task.h"

#include "prf_utils.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define ANPS_DB_CONFIG_MASK         (0x1FFF)

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPS_CREATE_DB_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int anps_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct anps_create_db_req *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Service Configuration Flag - All attributes have to be added in the database
    uint16_t cfg_flag = ANPS_DB_CONFIG_MASK;
    // Database Creation Status
    uint8_t status;
    // Counter
    uint8_t counter;

    // Check if an ANS has already been added in the database
    if (ke_state_get(TASK_ANPS) == ANPS_DISABLED)
    {
        // Check the provided values (Supported New Alert Category Char value - shall not be 0)
        if ((param->supp_new_alert_cat.cat_id_mask_0 != 0) ||
            (param->supp_new_alert_cat.cat_id_mask_1 != 0))
        {
            // Add service in the database
            status = attm_svc_create_db(&anps_env.ans_shdl, (uint8_t *)&cfg_flag, ANS_IDX_NB, NULL,
                                        dest_id, &ans_att_db[0]);

            // Go to Idle State
            if (status == ATT_ERR_NO_ERROR)
            {
                uint8_t length = sizeof(uint8_t);

                // Mask the Category ID Bit Mask 1 values to have a good value
                param->supp_new_alert_cat.cat_id_mask_1    &= ANP_CAT_ID_1_MASK;
                param->supp_unread_alert_cat.cat_id_mask_1 &= ANP_CAT_ID_1_MASK;

                // The second part of a supported category value is optional is set to 0, remove it
                if (param->supp_new_alert_cat.cat_id_mask_1 != 0x00)
                {
                    length *= 2;
                }

                // Set the value of the Supported New Alert Category characteristic
                attmdb_att_set_value(anps_env.ans_shdl + ANS_IDX_SUPP_NEW_ALERT_CAT_VAL, length,
                                     (uint8_t *)&param->supp_new_alert_cat);

                length = sizeof(uint8_t);

                // The second part of a supported category value is optional is set to 0, remove it
                if (param->supp_unread_alert_cat.cat_id_mask_1 != 0x00)
                {
                    length *= 2;
                }

                // Set the value of the Supported Unread Alert Category characteristic
                attmdb_att_set_value(anps_env.ans_shdl + ANS_IDX_SUPP_UNREAD_ALERT_CAT_VAL, length,
                                     (uint8_t *)&param->supp_unread_alert_cat);

                // Keep the supported categories in the environment
                anps_env.supp_new_alert_cat    = ((param->supp_new_alert_cat.cat_id_mask_0) |
                                                  (param->supp_new_alert_cat.cat_id_mask_1 << 8));
                anps_env.supp_unread_alert_cat = ((param->supp_unread_alert_cat.cat_id_mask_0) |
                                                  (param->supp_unread_alert_cat.cat_id_mask_1 << 8));

                // Disable ANS
                attmdb_svc_set_permission(anps_env.ans_shdl, PERM(SVC, DISABLE));

                for (counter = 0; counter < ANPS_IDX_MAX; counter++)
                {
                    // If we are here, database has been fulfilled with success, go to idle state
                    ke_state_set(KE_BUILD_ID(TASK_ANPS, counter), ANPS_IDLE);
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
        // Request is disallowed, an ANS has already been added
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // Send response to application
    anps_send_cmp_evt(TASK_ANPS, src_id, GAP_INVALID_CONHDL, ANPS_CREATE_DB_OP_CODE, status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPS_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int anps_enable_req_handler(ke_msg_id_t const msgid,
                                   struct anps_enable_req *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Alert Notification Profile Server Role Task Index Environment
    struct anps_idx_env_tag *anps_idx_env = NULL;
    // Connection Information structure
    struct prf_con_info con_info;
    // Status
    uint8_t status = PRF_ERR_OK;
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
        status = PRF_CLIENT_ENABLE(con_info, param, anps_idx);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status == PRF_ERR_OK)
    {
        // Get the newly created environment
        anps_idx_env = PRF_CLIENT_GET_ENV(dest_id, anps_idx);

        // Keep the Connection Information structure content
        memcpy(&anps_idx_env->con_info, &con_info, sizeof(struct prf_con_info));

        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Force the configuration to 0
            param->new_alert_ntf_cfg           = PRF_CLI_STOP_NTFIND;
            param->unread_alert_status_ntf_cfg = PRF_CLI_STOP_NTFIND;
        }
        else        // PRF_CON_NORMAL (Bonded peer device)
        {
            // Update the state in the environment
            if (param->new_alert_ntf_cfg != PRF_CLI_STOP_NTFIND)
            {
                // Force to PRF_CLI_START_NTF
                param->new_alert_ntf_cfg = PRF_CLI_START_NTF;

                ANPS_ENABLE_ALERT(anps_idx_env, ANP_NEW_ALERT);
            }

            if (param->unread_alert_status_ntf_cfg != PRF_CLI_STOP_NTFIND)
            {
                // Force to PRF_CLI_START_NTF
                param->unread_alert_status_ntf_cfg = PRF_CLI_START_NTF;

                ANPS_ENABLE_ALERT(anps_idx_env, ANP_UNREAD_ALERT);
            }
        }

        // Set the value of the New Alert Client Characteristic Configuration Descriptor
        attmdb_att_set_value(anps_env.ans_shdl + ANS_IDX_NEW_ALERT_CFG, sizeof(uint16_t),
                             (uint8_t *)&param->new_alert_ntf_cfg);
        // Set the value of the Unread Alert Status Client Characteristic Configuration Descriptor
        attmdb_att_set_value(anps_env.ans_shdl + ANS_IDX_UNREAD_ALERT_STATUS_CFG, sizeof(uint16_t),
                             (uint8_t *)&param->unread_alert_status_ntf_cfg);

        // Enable ANS
        attmdb_svc_set_permission(anps_env.ans_shdl, param->sec_lvl);

        // Go to Connected State
        ke_state_set(dest_id, ANPS_CONNECTED);

        // Send a complete event status to the application
        anps_send_cmp_evt(anps_idx_env->con_info.prf_id, anps_idx_env->con_info.appid,
                          gapc_get_conhdl(anps_idx_env->con_info.conidx),
                          ANPS_ENABLE_OP_CODE, PRF_ERR_OK);
    }
    else if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        msg_status = KE_MSG_NO_FREE;
    }
    else
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        anps_send_cmp_evt(dest_id, src_id, param->conhdl, ANPS_ENABLE_OP_CODE, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPS_NTF_ALERT_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int anps_ntf_alert_cmd_handler(ke_msg_id_t const msgid,
                                      struct anps_ntf_alert_cmd const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{

    // Message Status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Task environment
    struct anps_idx_env_tag *anps_idx_env = PRF_CLIENT_GET_ENV(dest_id, anps_idx);

    if (anps_idx_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;
        // Category ID
        uint8_t cat_id;
        // Is the category supported ?
        bool cat_supported;
        // Handle
        uint16_t handle;

        do
        {
            // Check the provided connection handle value
            if (anps_idx_env->con_info.conidx != gapc_get_conidx(param->conhdl))
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) == ANPS_BUSY)
            {
                // The message will be handled once the current operation is over
                msg_status = KE_MSG_NO_FREE;
                break;
            }

            ASSERT_ERR(ke_state_get(dest_id) == ANPS_CONNECTED);

            // Check the operation code, get the category ID.
            if (param->operation == ANPS_UPD_NEW_ALERT_OP_CODE)
            {
                // Check the length of the string info value
                if (param->value.new_alert.info_str_len > ANS_NEW_ALERT_STRING_INFO_MAX_LEN)
                {
                    status = PRF_ERR_INVALID_PARAM;
                    break;
                }

                // Get the category ID
                cat_id = param->value.new_alert.cat_id;
                // Check if the category is supported
                cat_supported = ANPS_IS_NEW_ALERT_CATEGORY_SUPPORTED(cat_id);
            }
            else if (param->operation == ANPS_UPD_UNREAD_ALERT_STATUS_OP_CODE)
            {
                // Get the category ID
                cat_id = param->value.unread_alert_status.cat_id;
                // Check if the category is supported
                cat_supported = ANPS_IS_UNREAD_ALERT_CATEGORY_SUPPORTED(cat_id);
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            // Check the category ID
            if ((cat_id >= CAT_ID_NB) || (!cat_supported))
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (param->operation == ANPS_UPD_NEW_ALERT_OP_CODE)
            {
                // Check if sending of notification is enabled for the provided category
                if (ANPS_IS_NEW_ALERT_CATEGORY_ENABLED(cat_id, anps_idx_env) &&
                    ANPS_IS_ALERT_ENABLED(anps_idx_env, ANP_NEW_ALERT))
                {
                    handle = anps_env.ans_shdl + ANS_IDX_NEW_ALERT_VAL;

                    // Notification can be sent, set the value in the database
                    attmdb_att_set_value(handle, 2 + param->value.new_alert.info_str_len,
                                         (uint8_t *)&param->value.new_alert.cat_id);
                }
                else
                {
                    status = PRF_ERR_NTF_DISABLED;
                    break;
                }
            }
            else
            {
                if (ANPS_IS_UNREAD_ALERT_CATEGORY_ENABLED(cat_id, anps_idx_env) &&
                    ANPS_IS_ALERT_ENABLED(anps_idx_env, ANP_UNREAD_ALERT))
                {
                    handle = anps_env.ans_shdl + ANS_IDX_UNREAD_ALERT_STATUS_VAL;

                    attmdb_att_set_value(handle, sizeof(struct anp_unread_alert),
                                         (uint8_t *)&param->value.unread_alert_status);
                }
                else
                {
                    status = PRF_ERR_NTF_DISABLED;
                    break;
                }
            }

            // Configure the environment
            anps_idx_env->operation = param->operation;

            // The notification can be sent, send the notification
            prf_server_send_event((prf_env_struct *)anps_idx_env, false, handle);

            // Go to Busy state
            ke_state_set(dest_id, ANPS_BUSY);
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send response to application
            anps_send_cmp_evt(anps_idx_env->con_info.prf_id, anps_idx_env->con_info.appid,
                              param->conhdl,
                              param->operation, status);
        }
    }
    else
    {
        // Send response to application
        anps_send_cmp_evt(dest_id, src_id, param->conhdl, param->operation, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
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
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct anps_idx_env_tag *anps_idx_env = PRF_CLIENT_GET_ENV(dest_id, anps_idx);

    if (anps_idx_env != NULL)
    {
        // Send a complete event status to the application
        anps_send_cmp_evt(anps_idx_env->con_info.prf_id, anps_idx_env->con_info.appid,
                          gapc_get_conhdl(anps_idx_env->con_info.conidx),
                          anps_idx_env->operation, param->status);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
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
    uint8_t conidx = KE_IDX_GET(src_id);
    // Get the address of the environment
    struct anps_idx_env_tag *anps_idx_env = PRF_CLIENT_GET_ENV(KE_BUILD_ID(dest_id, conidx), anps_idx);

    // Check if the connection exists
    if (anps_idx_env != NULL)
    {
        // Status
        uint8_t status = PRF_ERR_OK;

        /*
         * ---------------------------------------------------------------------------------------------
         * New Alert Client Characteristic Configuration Descriptor Value - Write
         * ---------------------------------------------------------------------------------------------
         */
        /*
         * ---------------------------------------------------------------------------------------------
         * Unread Status Alert Client Characteristic Configuration Descriptor Value - Write
         * ---------------------------------------------------------------------------------------------
         */
        if ((param->handle == (anps_env.ans_shdl + ANS_IDX_NEW_ALERT_CFG)) ||
            (param->handle == (anps_env.ans_shdl + ANS_IDX_UNREAD_ALERT_STATUS_CFG)))
        {
            // Received configuration value
            uint16_t ntf_cfg = co_read16p(&param->value[0]);

            if (ntf_cfg <= PRF_CLI_START_NTF)
            {
                // Alert type
                uint8_t alert_type = (param->handle == (anps_env.ans_shdl + ANS_IDX_NEW_ALERT_CFG))
                                     ? ANP_NEW_ALERT : ANP_UNREAD_ALERT;

                // Set the value of the Alert Status Client Characteristic Configuration Descriptor (Readable)
                attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg);

                // Update the status in the environment
                if (ntf_cfg == PRF_CLI_START_NTF)
                {
                    ANPS_ENABLE_ALERT(anps_idx_env, alert_type);
                }
                else
                {
                    ANPS_DISABLE_ALERT(anps_idx_env, alert_type);
                }

                // Inform the HL that the notification configuration status has been written
                anps_send_ntf_status_update_ind(anps_idx_env, alert_type);
            }
            else
            {
                status = PRF_APP_ERROR;
            }
        }
        /*
         * ---------------------------------------------------------------------------------------------
         * Alert Notification Control Point Characteristic Value - Write
         * ---------------------------------------------------------------------------------------------
         */
        else if (param->handle == (anps_env.ans_shdl + ANS_IDX_ALERT_NTF_CTNL_PT_VAL))
        {
            do
            {
                // Check the command ID value
                if (param->value[0] >= CMD_ID_NB)
                {
                    status = ANP_CMD_NOT_SUPPORTED;
                    break;
                }

                // Check the category ID value
                if ((param->value[1] >= CAT_ID_NB) &&
                    (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT))
                {
                    status = ANP_CAT_NOT_SUPPORTED;
                    break;
                }

                if (param->value[1] < CAT_ID_NB)
                {
                    // New Alert
                    if ((param->value[0] % 2) == 0)
                    {
                        // Check if the category is supported
                        if (!ANPS_IS_NEW_ALERT_CATEGORY_SUPPORTED(param->value[1]))
                        {
                            status = ANP_CAT_NOT_SUPPORTED;
                            break;
                        }
                    }
                    // Unread Alert Status
                    else
                    {
                        // Check if the category is supported
                        if (!ANPS_IS_UNREAD_ALERT_CATEGORY_SUPPORTED(param->value[1]))
                        {
                            status = ANP_CAT_NOT_SUPPORTED;
                            break;
                        }
                    }
                }

                // React according to the received command id value
                switch (param->value[0])
                {
                    // Enable New Alert Notification
                    case (CMD_ID_EN_NEW_IN_ALERT_NTF):
                    {
                        if (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT)
                        {
                            // Enable sending of new alert notification for the specified category
                            ANPS_ENABLE_NEW_ALERT_CATEGORY(param->value[1], anps_idx_env);
                        }
                        else
                        {
                            // Enable sending of new alert notification for all supported category
                            anps_idx_env->ntf_new_alert_cfg |= anps_env.supp_new_alert_cat;
                        }

                        anps_send_ntf_status_update_ind(anps_idx_env, ANP_NEW_ALERT);
                    } break;

                    // Enable Unread Alert Status Notification
                    case (CMD_ID_EN_UNREAD_CAT_STATUS_NTF):
                    {
                        if (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT)
                        {
                            // Enable sending of unread alert notification for the specified category
                            ANPS_ENABLE_UNREAD_ALERT_CATEGORY(param->value[1], anps_idx_env);
                        }
                        else
                        {
                            // Enable sending of unread alert notification for all supported category
                            anps_idx_env->ntf_unread_alert_cfg |= anps_env.supp_unread_alert_cat;
                        }

                        anps_send_ntf_status_update_ind(anps_idx_env, ANP_UNREAD_ALERT);
                    } break;

                    // Disable New Alert Notification
                    case (CMD_ID_DIS_NEW_IN_ALERT_NTF):
                    {
                        if (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT)
                        {
                            // Disable sending of new alert notification for the specified category
                            ANPS_DISABLE_NEW_ALERT_CATEGORY(param->value[1], anps_idx_env);
                        }
                        else
                        {
                            // Disable sending of new alert notification for all supported category
                            anps_idx_env->ntf_new_alert_cfg &= ~anps_env.supp_new_alert_cat;
                        }

                        anps_send_ntf_status_update_ind(anps_idx_env, ANP_NEW_ALERT);
                    } break;

                    // Disable Unread Alert Status Notification
                    case (CMD_ID_DIS_UNREAD_CAT_STATUS_NTF):
                    {
                        if (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT)
                        {
                            // Disable sending of unread alert notification for the specified category
                            ANPS_DISABLE_UNREAD_ALERT_CATEGORY(param->value[1], anps_idx_env);
                        }
                        else
                        {
                            // Enable sending of unread alert notification for all supported category
                            anps_idx_env->ntf_unread_alert_cfg &= ~anps_env.supp_unread_alert_cat;
                        }

                        anps_send_ntf_status_update_ind(anps_idx_env, ANP_UNREAD_ALERT);
                    } break;

                    // Notify New Alert immediately
                    case (CMD_ID_NTF_NEW_IN_ALERT_IMM):
                    {
                        // Check if sending of notification is enabled
                        if (ANPS_IS_ALERT_ENABLED(anps_idx_env, ANP_NEW_ALERT))
                        {
                            if (param->value[1] == CAT_ID_ALL_SUPPORTED_CAT)
                            {
                                // Check if at least one category can be notified
                                if (anps_idx_env->ntf_new_alert_cfg != 0)
                                {
                                    anps_send_ntf_immediate_req_ind(anps_idx_env, ANP_NEW_ALERT,
                                                                    CAT_ID_ALL_SUPPORTED_CAT);
                                }
                            }
                            else
                            {
                                // Check if sending of notifications has been enabled for the specified category.
                                if (ANPS_IS_NEW_ALERT_CATEGORY_ENABLED(param->value[1], anps_idx_env))
                                {
                                    anps_send_ntf_immediate_req_ind(anps_idx_env, ANP_NEW_ALERT,
                                                                    param->value[1]);
                                }
                            }
                        }
                    } break;

                    // Notify Unread Alert Status immediately
                    case (CMD_ID_NTF_UNREAD_CAT_STATUS_IMM):
                    {
                        if (ANPS_IS_ALERT_ENABLED(anps_idx_env, ANP_UNREAD_ALERT))
                        {
                            // Check if sending of notification is enabled
                            if (ANPS_IS_ALERT_ENABLED(anps_idx_env, ANP_UNREAD_ALERT))
                            {
                                if (param->value[1] == CAT_ID_ALL_SUPPORTED_CAT)
                                {
                                    // Check if at least one category can be notified
                                    if (anps_idx_env->ntf_unread_alert_cfg != 0)
                                    {
                                        anps_send_ntf_immediate_req_ind(anps_idx_env, ANP_UNREAD_ALERT,
                                                                        CAT_ID_ALL_SUPPORTED_CAT);
                                    }
                                }
                                else
                                {
                                    // Check if sending of notifications has been enabled for the specified category.
                                    if (ANPS_IS_UNREAD_ALERT_CATEGORY_ENABLED(param->value[1], anps_idx_env))
                                    {
                                        anps_send_ntf_immediate_req_ind(anps_idx_env, ANP_UNREAD_ALERT,
                                                                        param->value[1]);
                                    }
                                }
                            }
                        }
                    } break;

                    default:
                    {
                        ASSERT_ERR(0);
                    } break;
                }
            } while (0);
        }
        else
        {
            ASSERT_ERR(0);
        }

        // Send write response
        atts_write_rsp_send(anps_idx_env->con_info.conidx, param->handle, status);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to ANPS.
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
    struct anps_idx_env_tag *anps_idx_env = PRF_CLIENT_GET_ENV(dest_id, anps_idx);

    // Check if the profile was enabled for this connection
    if (anps_idx_env != NULL)
    {
        anps_disable(anps_idx_env, param->conhdl);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler anps_default_state[] =
{
    {ANPS_CREATE_DB_REQ,           (ke_msg_func_t)anps_create_db_req_handler},
    {ANPS_ENABLE_REQ,              (ke_msg_func_t)anps_enable_req_handler},

    {ANPS_NTF_ALERT_CMD,           (ke_msg_func_t)anps_ntf_alert_cmd_handler},

    {GATTC_WRITE_CMD_IND,           (ke_msg_func_t)gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler anps_state_handler[ANPS_STATE_MAX] =
{
    [ANPS_DISABLED]    = KE_STATE_HANDLER_NONE,
    [ANPS_IDLE]        = KE_STATE_HANDLER_NONE,
    [ANPS_CONNECTED]   = KE_STATE_HANDLER_NONE,
    [ANPS_BUSY]        = KE_STATE_HANDLER_NONE,
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler anps_default_handler = KE_STATE_HANDLER(anps_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t anps_state[ANPS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_AN_SERVER)

/// @} ANPSTASK
