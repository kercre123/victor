/**
 ****************************************************************************************
 *
 * @file anpc_task.c
 *
 * @brief Alert Notification Profile Profile Client Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup ANPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_AN_CLIENT)
#include "anp_common.h"
#include "gap.h"
#include "attm.h"
#include "anpc.h"
#include "anpc_task.h"
#include "gattm_task.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve Alert Notification service characteristics information
const struct prf_char_def anpc_ans_char[ANPC_CHAR_MAX] =
{
    /// Supported New Alert Category
    [ANPC_CHAR_SUP_NEW_ALERT_CAT]      = {ATT_CHAR_SUP_NEW_ALERT_CAT,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_RD},
    /// New Alert
    [ANPC_CHAR_NEW_ALERT]              = {ATT_CHAR_NEW_ALERT,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_NTF},
    /// Supported Unread Alert Category
    [ANPC_CHAR_SUP_UNREAD_ALERT_CAT]   = {ATT_CHAR_SUP_UNREAD_ALERT_CAT,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_RD},
    /// Unread Alert Status
    [ANPC_CHAR_UNREAD_ALERT_STATUS]    = {ATT_CHAR_UNREAD_ALERT_STATUS,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_NTF},
    /// Alert Notification Control Point
    [ANPC_CHAR_ALERT_NTF_CTNL_PT]      = {ATT_CHAR_ALERT_NTF_CTNL_PT,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_WR},
};

/// State machine used to retrieve Phone Alert Status service characteristic descriptor information
const struct prf_char_desc_def anpc_ans_char_desc[ANPC_DESC_MAX] =
{
    /// New Alert Char. - Client Characteristic Configuration
    [ANPC_DESC_NEW_ALERT_CL_CFG]            = {ATT_DESC_CLIENT_CHAR_CFG,
                                               ATT_MANDATORY,
                                               ANPC_CHAR_NEW_ALERT},
    /// Unread Alert Status Char. - Client Characteristic Configuration
    [ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG,
                                               ATT_MANDATORY,
                                               ANPC_CHAR_UNREAD_ALERT_STATUS},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPC_ENABLE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int anpc_enable_cmd_handler(ke_msg_id_t const msgid,
                                   struct anpc_enable_cmd *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = PRF_ERR_OK;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Alert Notification Profile Client Role Task Environment
    struct anpc_env_tag *anpc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Check if the provided connection handle is valid
    if (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX)
    {
        // Fill the Connection Information structure
        con_info.conidx = gapc_get_conidx(param->conhdl);
        con_info.prf_id = dest_id;
        con_info.appid  = src_id;

        // Add an environment for the provided connection
        status = PRF_CLIENT_ENABLE(con_info, param, anpc);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status == PRF_ERR_OK)
    {
        // Get the newly created environment
        anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

        // Keep the connection info
        memcpy(&anpc_env->con_info, &con_info, sizeof(struct prf_con_info));

        // Start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Configure the environment for a discovery procedure
            anpc_env->last_req  = ATT_SVC_ALERT_NTF;
            // Force the operation value
            param->operation    = ANPC_ENABLE_OP_CODE;

            prf_disc_svc_send(&(anpc_env->con_info), ATT_SVC_ALERT_NTF);
        }
        // Bond information are provided
        else
        {
            // Keep the provided database content
            memcpy(&anpc_env->ans, &param->ans, sizeof(struct anpc_ans_content));

            // Register in GATT for notifications/indications
            prf_register_atthdl2gatt(&anpc_env->con_info, &anpc_env->ans.svc);

            // Force the operation value
            param->operation = ANPC_ENABLE_RD_NEW_ALERT_OP_CODE;

            // Check Supported New Alert Category
            prf_read_char_send(&(anpc_env->con_info), anpc_env->ans.svc.shdl,
                               anpc_env->ans.svc.ehdl, anpc_env->ans.chars[ANPC_CHAR_SUP_NEW_ALERT_CAT].val_hdl);
        }

        // Keep the operation
        anpc_env->operation = (void *)param;
        // Do not free the message
        msg_status = KE_MSG_NO_FREE;

        // Go to BUSY state
        ke_state_set(dest_id, ANPC_BUSY);
    }
    else if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message will be forwarded towards the good task instance
        msg_status = KE_MSG_NO_FREE;
    }
    else
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        anpc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, ANPC_ENABLE_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int anpc_read_cmd_handler(ke_msg_id_t const msgid,
                                  struct anpc_read_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

    if (anpc_env != NULL)
    {
        // Attribute Handle
        uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;
        // Status
        uint8_t status     = PRF_ERR_OK;

        // State is Connected or Busy
        ASSERT_ERR(ke_state_get(dest_id) != ANPC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == anpc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == ANPC_BUSY)
            {
                // Keep the request for later, status is PRF_ERR_OK
                msg_status = KE_MSG_SAVED;
            }
            else    // State is ANPC_CONNECTED
            {
                ASSERT_ERR(anpc_env->operation == NULL);

                switch (param->read_code)
                {
                    // Read New Alert Characteristic Client Char. Cfg. Descriptor Value
                    case (ANPC_RD_WR_NEW_ALERT_CFG):
                    {
                        handle = anpc_env->ans.descs[ANPC_DESC_NEW_ALERT_CL_CFG].desc_hdl;
                    } break;

                    // Read Unread Alert Characteristic Client Char. Cfg. Descriptor Value
                    case (ANPC_RD_WR_UNREAD_ALERT_STATUS_CFG):
                    {
                        handle = anpc_env->ans.descs[ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG].desc_hdl;
                    } break;

                    default:
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    } break;
                }

                // Check if handle is viable
                if ((handle != ATT_INVALID_SEARCH_HANDLE) &&
                    (status == PRF_ERR_OK))
                {
                    // Force the operation value
                    param->operation      = ANPC_READ_OP_CODE;

                    // Store the command structure
                    anpc_env->operation   = param;
                    msg_status = KE_MSG_NO_FREE;

                    // Send the read request
                    prf_read_char_send(&(anpc_env->con_info), anpc_env->ans.svc.shdl,
                                       anpc_env->ans.svc.ehdl, handle);

                    // Go to the Busy state
                    ke_state_set(dest_id, ANPC_BUSY);
                }
                else
                {
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }


        if (status != PRF_ERR_OK)
        {
            anpc_send_cmp_evt(anpc_env, ANPC_READ_OP_CODE, status);
        }
    }
    else
    {
        // No connection exists
        anpc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, ANPC_READ_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPC_WRITE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int anpc_write_cmd_handler(ke_msg_id_t const msgid,
                                   struct anpc_write_cmd *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

    if (anpc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;

        // State is Connected or Busy
        ASSERT_ERR(ke_state_get(dest_id) != ANPC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == anpc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == ANPC_BUSY)
            {
                msg_status = KE_MSG_SAVED;
            }
            else    // state = ANPC_CONNECTED
            {
                // Attribute handle
                uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;
                // Length
                uint8_t length;

                ASSERT_ERR(anpc_env->operation == NULL);

                switch (param->write_code)
                {
                    // Write Alert Notification Control Point Characteristic Value
                    case (ANPC_WR_ALERT_NTF_CTNL_PT):
                    {
                        // Check the Category ID and the Command ID
                        if ((param->value.ctnl_pt.cmd_id < CMD_ID_NB) &&
                            ((param->value.ctnl_pt.cat_id < CAT_ID_NB) || (param->value.ctnl_pt.cat_id == CAT_ID_ALL_SUPPORTED_CAT)))
                        {
                            handle  = anpc_env->ans.chars[ANPC_CHAR_ALERT_NTF_CTNL_PT].val_hdl;
                            length  = sizeof(struct anp_ctnl_pt);
                        }
                        else
                        {
                            status = PRF_ERR_INVALID_PARAM;
                        }
                    } break;

                    // Write New Alert Characteristic Client Char. Cfg. Descriptor Value
                    case (ANPC_RD_WR_NEW_ALERT_CFG):
                    {
                        if (param->value.new_alert_ntf_cfg <= PRF_CLI_START_NTF)
                        {
                            handle  = anpc_env->ans.descs[ANPC_DESC_NEW_ALERT_CL_CFG].desc_hdl;
                            length  = sizeof(uint16_t);
                        }
                        else
                        {
                            status = PRF_ERR_INVALID_PARAM;
                        }
                    } break;

                    // Write Unread Alert Status Characteristic Client Char. Cfg. Descriptor Value
                    case (ANPC_RD_WR_UNREAD_ALERT_STATUS_CFG):
                    {
                        if (param->value.unread_alert_status_ntf_cfg <= PRF_CLI_START_NTF)
                        {
                            handle  = anpc_env->ans.descs[ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG].desc_hdl;
                            length  = sizeof(uint16_t);
                        }
                        else
                        {
                            status = PRF_ERR_INVALID_PARAM;
                        }
                    } break;

                    default:
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    } break;
                }

                if (status == PRF_ERR_OK)
                {
                    // Check if handle is viable
                    if (handle != ATT_INVALID_SEARCH_HANDLE)
                    {
                        // Send the write request
                        prf_gatt_write(&(anpc_env->con_info), handle,
                                       (uint8_t *)&param->value, length, GATTC_WRITE);

                        // Force the operation value
                        param->operation      = ANPC_WRITE_OP_CODE;
                        // Store the command structure
                        anpc_env->operation   = param;

                        msg_status = KE_MSG_NO_FREE;

                        // Go to the Busy state
                        ke_state_set(dest_id, ANPC_BUSY);
                    }
                    else
                    {
                        status = PRF_ERR_INEXISTENT_HDL;
                    }
                }
            }
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }

        if (status != PRF_ERR_OK)
        {
            anpc_send_cmp_evt(anpc_env, ANPC_WRITE_OP_CODE, status);
        }
    }
    else
    {
        anpc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, ANPC_WRITE_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_SVC_IND message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_svc_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_disc_svc_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

    if (anpc_env != NULL)
    {
        ASSERT_ERR(anpc_env->operation != NULL);
        ASSERT_ERR(((struct anpc_cmd *)anpc_env->operation)->operation == ANPC_ENABLE_OP_CODE);

        // Keep only one instance of the service
        if (anpc_env->nb_svc == 0)
        {
            // Keep the start handle and the end handle of the service
            anpc_env->ans.svc.shdl = param->start_hdl;
            anpc_env->ans.svc.ehdl = param->end_hdl;
            anpc_env->nb_svc++;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_char_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_disc_char_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

    if (anpc_env != NULL)
    {
        ASSERT_ERR(anpc_env->operation != NULL);
        ASSERT_ERR(((struct anpc_cmd *)anpc_env->operation)->operation == ANPC_ENABLE_OP_CODE);

        // Retrieve ANPS characteristics
        prf_search_chars(anpc_env->ans.svc.ehdl, ANPC_CHAR_MAX,
                         &anpc_env->ans.chars[0], &anpc_ans_char[0],
                         param, (uint8_t *)&anpc_env->last_char_code);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_DESC_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_char_desc_ind_handler(ke_msg_id_t const msgid,
                                           struct gattc_disc_char_desc_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

    if (anpc_env != NULL)
    {
        ASSERT_ERR(anpc_env->operation != NULL);
        ASSERT_ERR(((struct anpc_cmd *)anpc_env->operation)->operation == ANPC_ENABLE_OP_CODE);

        // Retrieve ANPS descriptors
        prf_search_descs(ANPC_DESC_MAX, &anpc_env->ans.descs[0], &anpc_ans_char_desc[0],
                         param, anpc_env->last_req);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

    if (anpc_env != NULL)
    {
        switch (param->req_type)
        {
            case (GATTC_DISC_BY_UUID_SVC):
            case (GATTC_DISC_ALL_CHAR):
            case (GATTC_DISC_DESC_CHAR):
            {
                // Status
                uint8_t status = PRF_ERR_STOP_DISC_CHAR_MISSING;

                if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                    (param->status == ATT_ERR_NO_ERROR))
                {
                    ASSERT_ERR(((struct anpc_cmd *)anpc_env->operation)->operation == ANPC_ENABLE_OP_CODE);

                    /* -------------------------------------------------
                     * SERVICE DISCOVERY -------------------------------
                     * ------------------------------------------------- */
                    if (anpc_env->last_req == ATT_SVC_ALERT_NTF)
                    {
                        if (anpc_env->nb_svc > 0)
                        {
                            // Check if service handles are OK
                            if ((anpc_env->ans.svc.shdl != ATT_INVALID_HANDLE) &&
                                (anpc_env->ans.svc.ehdl != ATT_INVALID_HANDLE) &&
                                (anpc_env->ans.svc.shdl < anpc_env->ans.svc.ehdl))
                            {
                                // Status is OK
                                status = PRF_ERR_OK;

                                // Discover all ANS characteristics
                                prf_disc_char_all_send(&(anpc_env->con_info), &(anpc_env->ans.svc));
                                anpc_env->last_req = ATT_DECL_CHARACTERISTIC;
                            }
                            // Handles are not corrects, the Phone Alert Status Service has not been found, stop
                        }
                        // The Phone Alert Status Service has not been found, stop discovery
                    }
                    /* -------------------------------------------------
                     * CHARACTERISTICS DISCOVERY -----------------------
                     * ------------------------------------------------- */
                    else if (anpc_env->last_req == ATT_DECL_CHARACTERISTIC)
                    {
                        // Check if mandatory properties have been found and if properties are correct
                        status = prf_check_svc_char_validity(ANPC_CHAR_MAX, anpc_env->ans.chars, anpc_ans_char);

                        // Check for characteristic properties.
                        if (status == PRF_ERR_OK)
                        {
                            anpc_env->last_req = ANPC_CHAR_NEW_ALERT;

                            // Find the Client Characteristic Configuration Descriptor for the New Alert characteristic
                            prf_disc_char_desc_send(&(anpc_env->con_info),
                                                    &(anpc_env->ans.chars[ANPC_CHAR_NEW_ALERT]));
                        }
                    }
                    /* -------------------------------------------------
                     * DESCRIPTORS DISCOVERY ---------------------------
                     * ------------------------------------------------- */
                    else
                    {
                        if (anpc_env->last_req == ANPC_CHAR_NEW_ALERT)
                        {
                            status = PRF_ERR_OK;

                            anpc_env->last_req = ANPC_CHAR_UNREAD_ALERT_STATUS;

                            // Find the Client Characteristic Configuration Descriptor for the Unread Alert Status characteristic
                            prf_disc_char_desc_send(&(anpc_env->con_info),
                                                    &(anpc_env->ans.chars[ANPC_CHAR_UNREAD_ALERT_STATUS]));
                        }
                        else if (anpc_env->last_req == ANPC_CHAR_UNREAD_ALERT_STATUS)
                        {
                            status = prf_check_svc_char_desc_validity(ANPC_DESC_MAX,
                                                                      anpc_env->ans.descs,
                                                                      anpc_ans_char_desc,
                                                                      anpc_env->ans.chars);

                            if (status == PRF_ERR_OK)
                            {
                                // Reset number of services
                                anpc_env->nb_svc = 0;

                                // Register in GATT for notifications/indications
                                prf_register_atthdl2gatt(&anpc_env->con_info, &anpc_env->ans.svc);

                                // Send the content of the service to the HL
                                struct anpc_ans_content_ind *ind = KE_MSG_ALLOC(ANPC_ANS_CONTENT_IND,
                                                                                anpc_env->con_info.appid,
                                                                                anpc_env->con_info.prf_id,
                                                                                anpc_ans_content_ind);

                                ind->conhdl = gapc_get_conhdl(anpc_env->con_info.conidx);
                                memcpy(&ind->ans, &anpc_env->ans, sizeof(struct anpc_ans_content));

                                // Send the message
                                ke_msg_send(ind);

                                // Force the operation value
                                ((struct anpc_enable_cmd *)anpc_env->operation)->operation = ANPC_ENABLE_RD_NEW_ALERT_OP_CODE;
                                // Check Supported New Alert Categories
                                prf_read_char_send(&(anpc_env->con_info), anpc_env->ans.svc.shdl,
                                                   anpc_env->ans.svc.ehdl, anpc_env->ans.chars[ANPC_CHAR_SUP_NEW_ALERT_CAT].val_hdl);
                            }
                        }
                        else
                        {
                            ASSERT_ERR(0);
                        }
                    }
                }
                else
                {
                    status = param->status;
                }

                if (status != PRF_ERR_OK)
                {
                    // Stop discovery procedure.
                    anpc_send_cmp_evt(anpc_env, ANPC_ENABLE_OP_CODE, status);
                }
            } break;

            case (GATTC_WRITE):
            {
                // Retrieve the operation currently performed
                uint8_t operation = ((struct anpc_cmd *)anpc_env->operation)->operation;

                switch (operation)
                {
                    case (ANPC_WRITE_OP_CODE):
                    {
                        uint8_t wr_code = ((struct anpc_write_cmd *)anpc_env->operation)->write_code;

                        if ((wr_code == ANPC_RD_WR_NEW_ALERT_CFG) ||
                            (wr_code == ANPC_RD_WR_UNREAD_ALERT_STATUS_CFG) ||
                            (wr_code == ANPC_WR_ALERT_NTF_CTNL_PT))
                        {
                            anpc_send_cmp_evt(anpc_env, ANPC_WRITE_OP_CODE, param->status);
                        }
                        else
                        {
                            ASSERT_ERR(0);
                        }
                    } break;

                    case (ANPC_ENABLE_WR_NEW_ALERT_OP_CODE):
                    {
                        // Look for the next category to enable
                        if (anpc_found_next_alert_cat(anpc_env, ((struct anpc_enable_cmd *)anpc_env->operation)->new_alert_enable))
                        {
                            // Enable sending of notifications for the found category ID
                            anpc_write_alert_ntf_ctnl_pt(anpc_env, CMD_ID_EN_NEW_IN_ALERT_NTF, anpc_env->last_req - 1);
                        }
                        else
                        {
                            // Force the operation value
                            ((struct anpc_cmd *)anpc_env->operation)->operation = ANPC_ENABLE_WR_NTF_NEW_ALERT_OP_CODE;
                            // Send a "Notify New Alert Immediately" command with the Category ID field set to 0xFF
                            anpc_write_alert_ntf_ctnl_pt(anpc_env, CMD_ID_NTF_NEW_IN_ALERT_IMM, CAT_ID_ALL_SUPPORTED_CAT);
                        }
                    } break;

                    case (ANPC_ENABLE_WR_NTF_NEW_ALERT_OP_CODE):
                    {
                        // Reset the environment
                        anpc_env->last_req = CAT_ID_SPL_ALERT;

                        if (anpc_found_next_alert_cat(anpc_env, ((struct anpc_enable_cmd *)anpc_env->operation)->unread_alert_enable))
                        {
                            // Force the operation value
                            ((struct anpc_cmd *)anpc_env->operation)->operation = ANPC_ENABLE_WR_UNREAD_ALERT_OP_CODE;
                            // Enable sending of notifications for the found category ID
                            anpc_write_alert_ntf_ctnl_pt(anpc_env, CMD_ID_EN_UNREAD_CAT_STATUS_NTF, anpc_env->last_req - 1);
                        }
                        else
                        {
                            anpc_send_cmp_evt(anpc_env, ANPC_ENABLE_OP_CODE, PRF_ERR_OK);
                        }
                    } break;

                    case (ANPC_ENABLE_WR_UNREAD_ALERT_OP_CODE):
                    {
                        // Look for the next category to enable
                        if (anpc_found_next_alert_cat(anpc_env, ((struct anpc_enable_cmd *)anpc_env->operation)->unread_alert_enable))
                        {
                            // Enable sending of notifications for the found category ID
                            anpc_write_alert_ntf_ctnl_pt(anpc_env, CMD_ID_EN_UNREAD_CAT_STATUS_NTF, anpc_env->last_req - 1);
                        }
                        else
                        {
                            // Force the operation value
                            ((struct anpc_cmd *)anpc_env->operation)->operation = ANPC_ENABLE_WR_NTF_UNREAD_ALERT_OP_CODE;
                            // Send a "Notify New Alert Immediately" command with the Category ID field set to 0xFF
                            anpc_write_alert_ntf_ctnl_pt(anpc_env, CMD_ID_NTF_UNREAD_CAT_STATUS_IMM, CAT_ID_ALL_SUPPORTED_CAT);
                        }
                    } break;

                    case (ANPC_ENABLE_WR_NTF_UNREAD_ALERT_OP_CODE):
                    {
                        // The discovery procedure is over
                        anpc_send_cmp_evt(anpc_env, ANPC_ENABLE_OP_CODE, param->status);
                    } break;

                    default:
                    {
                        ASSERT_ERR(0);
                    } break;
                }
            } break;

            case (GATTC_READ):
            {
                switch (((struct anpc_cmd *)anpc_env->operation)->operation)
                {
                    // Read Supported New Alert Category Characteristic value
                    case (ANPC_ENABLE_RD_NEW_ALERT_OP_CODE):
                    {
                        if (param->status == GATT_ERR_NO_ERROR)
                        {
                            // Force the operation value
                            ((struct anpc_cmd *)anpc_env->operation)->operation = ANPC_ENABLE_RD_UNREAD_ALERT_OP_CODE;
                            // Check Supported Unread Alert Category
                            prf_read_char_send(&(anpc_env->con_info),
                                               anpc_env->ans.svc.shdl, anpc_env->ans.svc.ehdl,
                                               anpc_env->ans.chars[ANPC_CHAR_SUP_UNREAD_ALERT_CAT].val_hdl);
                        }
                        else
                        {
                            anpc_send_cmp_evt(anpc_env, ANPC_ENABLE_OP_CODE, param->status);
                        }
                    } break;

                    case (ANPC_ENABLE_RD_UNREAD_ALERT_OP_CODE):
                    {
                        if (param->status == GATT_ERR_NO_ERROR)
                        {
                            // If peer device was unknown, stop the procedure
                            if (((struct anpc_enable_cmd *)anpc_env->operation)->con_type == PRF_CON_DISCOVERY)
                            {
                                anpc_send_cmp_evt(anpc_env, ANPC_ENABLE_OP_CODE, PRF_ERR_OK);
                            }
                            // If peer device was already known (bonded), start Recovery from Connection Loss procedure
                            else
                            {
                                // Reset the environment
                                anpc_env->last_req = CAT_ID_SPL_ALERT;

                                if (anpc_found_next_alert_cat(anpc_env, ((struct anpc_enable_cmd *)anpc_env->operation)->new_alert_enable))
                                {
                                    // Force the operation value
                                    ((struct anpc_cmd *)anpc_env->operation)->operation = ANPC_ENABLE_WR_NEW_ALERT_OP_CODE;
                                    // Enable sending of notifications for the found category ID
                                    anpc_write_alert_ntf_ctnl_pt(anpc_env, CMD_ID_EN_NEW_IN_ALERT_NTF, anpc_env->last_req - 1);
                                }
                                else
                                {
                                    // Reset the environment
                                    anpc_env->last_req = CAT_ID_SPL_ALERT;

                                    if (anpc_found_next_alert_cat(anpc_env, ((struct anpc_enable_cmd *)anpc_env->operation)->unread_alert_enable))
                                    {
                                        // Force the operation value
                                        ((struct anpc_cmd *)anpc_env->operation)->operation = ANPC_ENABLE_WR_UNREAD_ALERT_OP_CODE;
                                        // Enable sending of notifications for the found category ID
                                        anpc_write_alert_ntf_ctnl_pt(anpc_env, CMD_ID_EN_UNREAD_CAT_STATUS_NTF, anpc_env->last_req - 1);
                                    }
                                    else
                                    {
                                        anpc_send_cmp_evt(anpc_env, ANPC_ENABLE_OP_CODE, PRF_ERR_OK);
                                    }
                                }
                            }
                        }
                        else
                        {
                            anpc_send_cmp_evt(anpc_env, ANPC_ENABLE_OP_CODE, param->status);
                        }
                    } break;

                    case (ANPC_READ_OP_CODE):
                    {
                        // Inform the requester that the read procedure is over
                        anpc_send_cmp_evt(anpc_env, ANPC_READ_OP_CODE, param->status);
                    } break;

                    default:
                    {
                        ASSERT_ERR(0);
                    }
                }
            } break;

            case (GATTC_REGISTER):
            case (GATTC_UNREGISTER):
            {
                // Do nothing
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_READ_IND message.
 * Generic event received after every simple read command sent to peer server.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_read_ind_handler(ke_msg_id_t const msgid,
                                    struct gattc_read_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

    if (anpc_env != NULL)
    {
        ASSERT_ERR(anpc_env->operation != NULL);

        // Prepare the indication to send to the application
        struct anpc_value_ind *ind = KE_MSG_ALLOC(ANPC_VALUE_IND,
                                                  anpc_env->con_info.appid,
                                                  anpc_env->con_info.prf_id,
                                                  anpc_value_ind);

        ind->conhdl   = gapc_get_conhdl(anpc_env->con_info.conidx);

        switch (((struct anpc_cmd *)anpc_env->operation)->operation)
        {
            // Read Supported New Alert Category Characteristic value
            case (ANPC_ENABLE_RD_NEW_ALERT_OP_CODE):
            {
                ind->att_code = ANPC_RD_SUP_NEW_ALERT_CAT;
                ind->value.supp_cat.cat_id_mask_0 = param->value[0];
                // If cat_id_mask_1 not present, shall be considered as 0
                ind->value.supp_cat.cat_id_mask_1 = (param->length > 1)
                                                    ? param->value[1] : 0x00;
            } break;

            case (ANPC_ENABLE_RD_UNREAD_ALERT_OP_CODE):
            {
                ind->att_code = ANPC_RD_SUP_UNREAD_ALERT_CAT;
                ind->value.supp_cat.cat_id_mask_0 = param->value[0];
                // If cat_id_mask_1 not present, shall be considered as 0
                ind->value.supp_cat.cat_id_mask_1 = (param->length > 1)
                                                    ? param->value[1] : 0;
            } break;

            case (ANPC_READ_OP_CODE):
            {
                ind->att_code      = ((struct anpc_read_cmd *)anpc_env->operation)->read_code;
                ind->value.ntf_cfg = co_read16(&param->value[0]);
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        // Send the indication
        ke_msg_send(ind);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_EVENT_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_event_ind_handler(ke_msg_id_t const msgid,
                                   struct gattc_event_ind const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

    if (anpc_env != NULL)
    {
        // Check the category ID
        if (param->value[0] < CAT_ID_NB)
        {
            // New Alert Characteristic Value
            if (param->handle == anpc_env->ans.chars[ANPC_CHAR_NEW_ALERT].val_hdl)
            {
                // Text String information Length
                uint8_t length = param->length - 2;

                if (length > ANS_NEW_ALERT_STRING_INFO_MAX_LEN)
                {
                    length = ANS_NEW_ALERT_STRING_INFO_MAX_LEN;
                }

                // Send an indication to the application
                struct anpc_value_ind *new_alert_ind = KE_MSG_ALLOC_DYN(ANPC_VALUE_IND,
                                                                        anpc_env->con_info.appid,
                                                                        anpc_env->con_info.prf_id,
                                                                        anpc_value_ind,
                                                                        length);

                new_alert_ind->att_code = ANPC_NTF_NEW_ALERT;
                new_alert_ind->conhdl   = gapc_get_conhdl(anpc_env->con_info.conidx);

                // Fulfill the characteristic value
                new_alert_ind->value.new_alert.cat_id          = param->value[0];
                new_alert_ind->value.new_alert.nb_new_alert    = param->value[1];
                new_alert_ind->value.new_alert.info_str_len    = length;
                memcpy(&new_alert_ind->value.new_alert.str_info[0], &param->value[2], length);

                // Send the message
                ke_msg_send(new_alert_ind);
            }
            // Unread Alert Status Characteristic Value
            else if (param->handle == anpc_env->ans.chars[ANPC_CHAR_UNREAD_ALERT_STATUS].val_hdl)
            {
                // Send an indication to the application
                struct anpc_value_ind *unrd_alert_ind = KE_MSG_ALLOC(ANPC_VALUE_IND,
                                                                     anpc_env->con_info.appid,
                                                                     anpc_env->con_info.prf_id,
                                                                     anpc_value_ind);

                unrd_alert_ind->att_code = ANPC_NTF_UNREAD_ALERT;
                unrd_alert_ind->conhdl   = gapc_get_conhdl(anpc_env->con_info.conidx);

                // Fulfill the characteristic value
                unrd_alert_ind->value.unread_alert.cat_id          = param->value[0];
                unrd_alert_ind->value.unread_alert.nb_unread_alert = param->value[1];

                // Send the message
                ke_msg_send(unrd_alert_ind);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        // else ignore the message
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to ANPC.
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
    struct anpc_env_tag *anpc_env = PRF_CLIENT_GET_ENV(dest_id, anpc);

    ASSERT_ERR(anpc_env != NULL);

    // Free the stored operation if needed
    if (anpc_env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(anpc_env->operation));
        anpc_env->operation = NULL;
    }

    PRF_CLIENT_DISABLE_IND_SEND(anpc_envs, dest_id, ANPC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler anpc_default_state[] =
{
    {ANPC_ENABLE_CMD,               (ke_msg_func_t)anpc_enable_cmd_handler},
    {ANPC_READ_CMD,                 (ke_msg_func_t)anpc_read_cmd_handler},
    {ANPC_WRITE_CMD,                (ke_msg_func_t)anpc_write_cmd_handler},

    {GATTC_DISC_SVC_IND,            (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,           (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,      (ke_msg_func_t)gattc_disc_char_desc_ind_handler},

    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler anpc_state_handler[ANPC_STATE_MAX] =
{
    [ANPC_IDLE]        = KE_STATE_HANDLER_NONE,
    [ANPC_CONNECTED]   = KE_STATE_HANDLER_NONE,
    [ANPC_BUSY]        = KE_STATE_HANDLER_NONE,
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler anpc_default_handler = KE_STATE_HANDLER(anpc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t anpc_state[ANPC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_AN_CLIENT)

/// @} ANPCTASK
