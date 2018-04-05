/**
 ****************************************************************************************
 *
 * @file ancc_task.c
 *
 * @brief ANCS Client Task implementation.
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
 * @addtogroup ANCCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "anc_common.h"

#if (BLE_ANC_CLIENT)

#include "gap.h"
#include "attm.h"
#include "ancc.h"
#include "ancc_task.h"
#include "gattm_task.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// ANCS UUID
const struct att_uuid_128 ancc_anc_svc = {{0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4, 0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79}};

/// State machine used to retrieve ANCS characteristics information
const struct prf_char_def_128 ancc_anc_char[ANCC_CHAR_MAX] =
{
    /// Notification Source
    [ANCC_CHAR_NTF_SRC]  =  {{0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C, 0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F},
                            ATT_MANDATORY,
                            ATT_CHAR_PROP_NTF},

    /// Control Point
    [ANCC_CHAR_CTRL_PT]  =  {{0xD9, 0xD9, 0xAA, 0xFD, 0xBD, 0x9B, 0x21, 0x98, 0xA8, 0x49, 0xE1, 0x45, 0xF3, 0xD8, 0xD1, 0x69},
                            ATT_OPTIONAL,
                            ATT_CHAR_PROP_WR},

    /// Data Source
    [ANCC_CHAR_DATA_SRC] =  {{0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE, 0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22},
                            ATT_OPTIONAL,
                            ATT_CHAR_PROP_NTF},

};

/// State machine used to retrieve ANCS characteristic descriptor information
const struct prf_char_desc_def ancc_anc_char_desc[ANCC_DESC_MAX] =
{
    /// Notification Source Char. - Client Characteristic Configuration
    [ANCC_DESC_NTF_SRC_CL_CFG]   = {ATT_DESC_CLIENT_CHAR_CFG,
                                   ATT_MANDATORY,
                                   ANCC_CHAR_NTF_SRC},
    /// Data Source Char. - Client Characteristic Configuration
    [ANCC_DESC_DATA_SRC_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,
                                   ATT_OPTIONAL,
                                   ANCC_CHAR_DATA_SRC},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct ancc_enable_req *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = PRF_ERR_OK;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // ANCS Profile Client Role Task Environment
    struct ancc_env_tag *ancc_env;
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
        status = PRF_CLIENT_ENABLE(con_info, param, ancc);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status == PRF_ERR_OK)
    {
        // Get the newly created environment
        ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

        // Keep the connection info
        memcpy(&ancc_env->con_info, &con_info, sizeof(struct prf_con_info));

        // Start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Configure the environment for a discovery procedure
            ancc_env->last_req  = ATT_DECL_PRIMARY_SERVICE;

            // Force the operation value
            param->operation = ANCC_ENABLE_OP_CODE;

            // Discover ANCS service by 128-bit UUID
            prf_disc_svc_send_128(&(ancc_env->con_info), (uint8_t *)&ancc_anc_svc.uuid);
        }
        // Bond information are provided
        else
        {
            // Keep the provided database content
            memcpy(&ancc_env->anc, &param->anc, sizeof(struct ancc_anc_content));

            // Register in GATT for notifications/indications
            prf_register_atthdl2gatt(&ancc_env->con_info, &ancc_env->anc.svc);

            // Force the operation value
            param->operation = ANCC_ENABLE_OP_CODE;
        }

        // Keep the operation
        ancc_env->operation = (void *)param;
        // Do not free the message
        msg_status = KE_MSG_NO_FREE;

        // Go to BUSY state
        ke_state_set(dest_id, ANCC_BUSY);
    }
    else if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message will be forwarded towards the good task instance
        msg_status = KE_MSG_NO_FREE;
    }
    else
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        ancc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, ANCC_ENABLE_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_read_cmd_handler(ke_msg_id_t const msgid,
                                 struct ancc_read_cmd *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Get the address of the environment
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
    {
        // Attribute Handle
        uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;
        // Status
        uint8_t status     = PRF_ERR_OK;

        // State is Connected or Busy
        ASSERT_ERR(ke_state_get(dest_id) != ANCC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == ancc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == ANCC_BUSY)
            {
                // Keep the request for later, status is PRF_ERR_OK
                msg_status = KE_MSG_SAVED;
            }
            else    // State is ANCC_CONNECTED
            {
                ASSERT_ERR(ancc_env->operation == NULL);

                switch (param->read_code)
                {
                    // Read Notification Source Characteristic Client Char. Cfg. Descriptor Value
                    case (ANCC_RD_WR_NTF_SRC_CFG):
                    {
                        handle = ancc_env->anc.descs[ANCC_DESC_NTF_SRC_CL_CFG].desc_hdl;
                    } break;

                    // Read Data Source Characteristic Client Char. Cfg. Descriptor Value
                    case (ANCC_RD_WR_DATA_SRC_CFG):
                    {
                        handle = ancc_env->anc.descs[ANCC_DESC_DATA_SRC_CL_CFG].desc_hdl;
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
                    param->operation      = ANCC_READ_CL_CFG_OP_CODE;

                    // Store the command structure
                    ancc_env->operation   = param;
                    msg_status = KE_MSG_NO_FREE;

                    // Send the read request
                    prf_read_char_send(&(ancc_env->con_info), ancc_env->anc.svc.shdl,
                                       ancc_env->anc.svc.ehdl, handle);

                    // Go to the Busy state
                    ke_state_set(dest_id, ANCC_BUSY);
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
            ancc_send_cmp_evt(ancc_env, ANCC_READ_CL_CFG_OP_CODE, status);
        }
    }
    else
    {
        // No connection exists
        ancc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, ANCC_READ_CL_CFG_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_WRITE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_write_cmd_handler(ke_msg_id_t const msgid,
                                  struct ancc_write_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Get the address of the environment
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;

        // State is Connected or Busy
        ASSERT_ERR(ke_state_get(dest_id) != ANCC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == ancc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == ANCC_BUSY)
            {
                msg_status = KE_MSG_SAVED;
            }
            else    // state = ANCC_CONNECTED
            {
                // Attribute handle
                uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;

                ASSERT_ERR(ancc_env->operation == NULL);

                switch (param->write_code)
                {
                    // Write Notification Source Characteristic Client Char. Cfg. Descriptor Value
                    case (ANCC_RD_WR_NTF_SRC_CFG):
                    {
                        if (param->value <= PRF_CLI_START_NTF)
                        {
                            handle  = ancc_env->anc.descs[ANCC_DESC_NTF_SRC_CL_CFG].desc_hdl;
                        }
                        else
                        {
                            status = PRF_ERR_INVALID_PARAM;
                        }
                    } break;

                    // Write Data Source Characteristic Client Char. Cfg. Descriptor Value
                    case (ANCC_RD_WR_DATA_SRC_CFG):
                    {
                        if (param->value <= PRF_CLI_START_NTF)
                        {
                            handle  = ancc_env->anc.descs[ANCC_DESC_DATA_SRC_CL_CFG].desc_hdl;
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
                        prf_gatt_write(&(ancc_env->con_info), handle,
                                       (uint8_t*)&param->value, sizeof(uint16_t), GATTC_WRITE);

                        // Force the operation value
                        param->operation      = ANCC_WRITE_CL_CFG_OP_CODE;
                        // Store the command structure
                        ancc_env->operation   = param;

                        msg_status = KE_MSG_NO_FREE;

                        // Go to the Busy state
                        ke_state_set(dest_id, ANCC_BUSY);
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
            ancc_send_cmp_evt(ancc_env, ANCC_WRITE_CL_CFG_OP_CODE, status);
        }
    }
    else
    {
        ancc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, ANCC_WRITE_CL_CFG_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_GET_NTF_ATTS_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_get_ntf_atts_cmd_handler(ke_msg_id_t const msgid,
                                      struct ancc_get_ntf_atts_cmd *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Temporary buffer to store Get Notification Attributes command
    uint8_t buffer[17];

    // Get the address of the environment
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;

        // State is Connected or Busy
        ASSERT_ERR(ke_state_get(dest_id) != ANCC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == ancc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == ANCC_BUSY)
            {
                msg_status = KE_MSG_SAVED;
            }
            else    // state = ANCC_CONNECTED
            {
                // Attribute handle
                uint16_t handle    = ancc_env->anc.chars[ANCC_CHAR_CTRL_PT].val_hdl;
                // Length
                uint8_t length = 0;

                ASSERT_ERR(ancc_env->operation == NULL);

                // Pack command to be written in Control Point 
                            
                buffer[length] = CMD_ID_GET_NTF_ATTS;
                length += 1;
                memcpy( &buffer[length],  &param->ntf_uid, sizeof(uint32_t)) ;
                length += 4;
                if (param->atts & NTF_ATT_ID_APP_ID_PRESENT) { /// App Identifier
                    buffer[length] = NTF_ATT_ID_APP_ID;
                    length += 1;
                }
                if (param->atts & NTF_ATT_ID_TITLE_PRESENT) { /// Title (Needs to be followed by a 2-bytes max length parameter)
                    buffer[length] = NTF_ATT_ID_TITLE;
                    length += 1;
                    co_write16p(&buffer[length], param->title_len);
                    length += 2;
                } 
                if (param->atts & NTF_ATT_ID_SUBTITLE_PRESENT) { /// Subtitle (Needs to be followed by a 2-bytes max length parameter)
                    buffer[length] = NTF_ATT_ID_SUBTITLE;
                    length += 1;
                    co_write16p(&buffer[length], param->subtitle_len);
                    length += 2;
                }
                if (param->atts &  NTF_ATT_ID_MSG_PRESENT) { /// Message (Needs to be followed by a 2-bytes max length parameter)
                    buffer[length] = NTF_ATT_ID_MSG;
                    length += 1;
                    co_write16p(&buffer[length], param->message_len);
                    length += 2;
                }
                if (param->atts &  NTF_ATT_ID_MSG_SIZE_PRESENT) {      /// Message Size
                    buffer[length] = NTF_ATT_ID_MSG_SIZE;
                    length += 1;
                }
                if (param->atts &  NTF_ATT_ID_DATE_PRESENT) {          /// Date
                    buffer[length] = NTF_ATT_ID_DATE;
                    length += 1;
                }
                
                // Check if no attributes have been requested.
                // In this case the packed command will contain the CommmandID and Notification UID fields only.
                if (length == 5)
                {
                    status = PRF_ERR_INVALID_PARAM;
                }

                if (status == PRF_ERR_OK)
                {
                    // Check if handle is viable
                    if (handle != ATT_INVALID_SEARCH_HANDLE)
                    {
                        // Send the write request
                        prf_gatt_write(&(ancc_env->con_info), handle,
                                       buffer, length, GATTC_WRITE);

                        // Force the operation value
                        param->operation      = ANCC_GET_NTF_ATTS_CMD_OP_CODE;
                        // Store the command structure
                        ancc_env->operation   = param;
                        
                        // Initialize the Data Source Notification reception state machine
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_IDLE;

                        msg_status = KE_MSG_NO_FREE;

                        // Go to the Busy state
                        ke_state_set(dest_id, ANCC_BUSY);
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
            /// the procedure is considered completed with error 
            ancc_send_cmp_evt(ancc_env, ANCC_GET_NTF_ATTS_CMD_OP_CODE, status);
        }
    }
    else
    {
        ancc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, ANCC_GET_NTF_ATTS_CMD_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_GET_APP_ATTS_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_get_app_atts_cmd_handler(ke_msg_id_t const msgid,
                                         struct ancc_get_app_atts_cmd *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Temporary buffer to store the Get App Attributes command
    uint8_t buffer[(param->app_id_length) + 3];

    // Get the address of the environment
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;

        // State is Connected or Busy
        ASSERT_ERR(ke_state_get(dest_id) != ANCC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == ancc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == ANCC_BUSY)
            {
                msg_status = KE_MSG_SAVED;
            }
            else    // state = ANCC_CONNECTED
            {
                // Attribute handle
                uint16_t handle    = ancc_env->anc.chars[ANCC_CHAR_CTRL_PT].val_hdl;
                // Length
                uint8_t length = 0;

                ASSERT_ERR(ancc_env->operation == NULL);

                // Pack command to be written in Control Point 
                            
                buffer[length] = CMD_ID_GET_APP_ATTS;
                length += 1;
                memcpy( &buffer[length],  &param->app_id[0], param->app_id_length);
                length += param->app_id_length;
                
                if (param->atts & APP_ATT_ID_DISPLAY_NAME_PRESENT)    // App Identifier
                {
                    buffer[length] = NTF_ATT_ID_APP_ID;
                    length += 1;
                }
                else
                {
                    // No attributes have been requested. 
                    status = PRF_ERR_INVALID_PARAM;
                }
                
                // Append NULL character
                buffer[length++] = NULL;
                
                if (status == PRF_ERR_OK)
                {
                    // Check if handle is viable
                    if (handle != ATT_INVALID_SEARCH_HANDLE)
                    {
                        // Send the write request
                        prf_gatt_write(&(ancc_env->con_info), handle,
                                       buffer, length, GATTC_WRITE);

                        // Force the operation value
                        param->operation      = ANCC_GET_APP_ATTS_CMD_OP_CODE;
                        // Store the command structure
                        ancc_env->operation   = param;

                        // Set Data Source Notification reception initial state
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_IDLE;
                        
                        msg_status = KE_MSG_NO_FREE;

                        // Go to the Busy state
                        ke_state_set(dest_id, ANCC_BUSY);
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
            /// the procedure is considered complete with error 
            ancc_send_cmp_evt(ancc_env, ANCC_GET_APP_ATTS_CMD_OP_CODE, status);
        }
    }
    else
    {
        ancc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, ANCC_GET_APP_ATTS_CMD_OP_CODE);
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
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
    {
        ASSERT_ERR(ancc_env->operation != NULL);
        ASSERT_ERR(((struct ancc_tmp_cmd *)ancc_env->operation)->operation == ANCC_ENABLE_OP_CODE);

        // Keep only one instance of the service
        if (ancc_env->nb_svc == 0)
        {
            // Keep the start handle and the end handle of the service
            ancc_env->anc.svc.shdl = param->start_hdl;
            ancc_env->anc.svc.ehdl = param->end_hdl;
            ancc_env->nb_svc++;
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
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
    {
        ASSERT_ERR(ancc_env->operation != NULL);
        ASSERT_ERR(((struct ancc_tmp_cmd *)ancc_env->operation)->operation == ANCC_ENABLE_OP_CODE);

        // Retrieve ANCS characteristics
        prf_search_chars_128(ancc_env->anc.svc.ehdl, ANCC_CHAR_MAX,
                             &ancc_env->anc.chars[0], &ancc_anc_char[0],
                             param, (uint8_t *)&ancc_env->last_char_code);
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
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
    {
        ASSERT_ERR(ancc_env->operation != NULL);
        ASSERT_ERR(((struct ancc_tmp_cmd *)ancc_env->operation)->operation == ANCC_ENABLE_OP_CODE);

        // Retrieve ANCS descriptors
        prf_search_descs(ANCC_DESC_MAX, &ancc_env->anc.descs[0], &ancc_anc_char_desc[0],
                         param, ancc_env->last_req);
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
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
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
                    ASSERT_ERR(((struct ancc_tmp_cmd *)ancc_env->operation)->operation == ANCC_ENABLE_OP_CODE);

                    /* -------------------------------------------------
                     * SERVICE DISCOVERY -------------------------------
                     * ------------------------------------------------- */
                    if (ancc_env->last_req == ATT_DECL_PRIMARY_SERVICE)
                    {
                        if (ancc_env->nb_svc > 0)
                        {
                            // Check if service handles are OK
                            if ((ancc_env->anc.svc.shdl != ATT_INVALID_HANDLE) &&
                                (ancc_env->anc.svc.ehdl != ATT_INVALID_HANDLE) &&
                                (ancc_env->anc.svc.shdl < ancc_env->anc.svc.ehdl))
                            {
                                // Status is OK
                                status = PRF_ERR_OK;

                                // Discover Notification Source characteristics
                                prf_disc_char_all_send(&(ancc_env->con_info), &(ancc_env->anc.svc));
                                ancc_env->last_req = ATT_DECL_CHARACTERISTIC;
                            }
                            // Handles are not corrects, the ANCS has not been found, stop
                        }
                        // ANCS has not been found, stop discovery
                    }
                    /* -------------------------------------------------
                     * CHARACTERISTICS DISCOVERY -----------------------
                     * ------------------------------------------------- */
                    else if (ancc_env->last_req == ATT_DECL_CHARACTERISTIC)
                    {
                        // Check if mandatory properties have been found and if properties are correct
                        status = prf_check_svc_char_validity_128(ANCC_CHAR_MAX, ancc_env->anc.chars, ancc_anc_char);

                        // Check for characteristic properties.
                        if (status == PRF_ERR_OK)
                        {
                            ancc_env->last_req = ANCC_CHAR_NTF_SRC;

                            // Find the Client Characteristic Configuration Descriptor for the Notification Source characteristic
                            prf_disc_char_desc_send(&(ancc_env->con_info),
                                                    &(ancc_env->anc.chars[ANCC_CHAR_NTF_SRC]));
                        }
                    }
                    /* -------------------------------------------------
                     * DESCRIPTORS DISCOVERY ---------------------------
                     * ------------------------------------------------- */
                    else
                    {
                        if (ancc_env->last_req == ANCC_CHAR_NTF_SRC)
                        {
                            status = PRF_ERR_OK;

                            ancc_env->last_req = ANCC_CHAR_DATA_SRC;


                            // Find the Client Characteristic Configuration Descriptor for the Data Source characteristic
                            prf_disc_char_desc_send(&(ancc_env->con_info),
                                                    &(ancc_env->anc.chars[ANCC_CHAR_DATA_SRC]));
                        }
                        else if (ancc_env->last_req == ANCC_CHAR_DATA_SRC)
                        {
                            status = prf_check_svc_char_desc_validity(ANCC_DESC_MAX,
                                                                      ancc_env->anc.descs,
                                                                      ancc_anc_char_desc,
                                                                      ancc_env->anc.chars);

                            if (status == PRF_ERR_OK)
                            {
                                // Reset number of services
                                ancc_env->nb_svc = 0;

                                // Register in GATT for notifications/indications
                                prf_register_atthdl2gatt(&ancc_env->con_info, &ancc_env->anc.svc);

                                // Send the content of the service to the HL
                                struct ancc_anc_content_ind *ind = KE_MSG_ALLOC(ANCC_ANC_CONTENT_IND,
                                                                                ancc_env->con_info.appid,
                                                                                ancc_env->con_info.prf_id,
                                                                                ancc_anc_content_ind);

                                ind->conhdl = gapc_get_conhdl(ancc_env->con_info.conidx);
                                memcpy(&ind->anc, &ancc_env->anc, sizeof(struct ancc_anc_content));

                                // Send the message
                                ke_msg_send(ind);

                                // ANCC enable process completed
                                ancc_send_cmp_evt(ancc_env, ANCC_ENABLE_OP_CODE, param->status);
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
                    ancc_send_cmp_evt(ancc_env, ANCC_ENABLE_OP_CODE, status);
                }
            } break;

            case (GATTC_WRITE):
            {
                // Retrieve the operation currently performed
                uint8_t operation = ((struct ancc_tmp_cmd *)ancc_env->operation)->operation;

                switch (operation)
                {
                    case (ANCC_WRITE_CL_CFG_OP_CODE):
                    {
                        uint8_t wr_code = ((struct ancc_write_cmd *)ancc_env->operation)->write_code;

                        if ((wr_code == ANCC_RD_WR_NTF_SRC_CFG) ||
                            (wr_code == ANCC_RD_WR_DATA_SRC_CFG))
                        {
                            ancc_send_cmp_evt(ancc_env, ANCC_WRITE_CL_CFG_OP_CODE, param->status);
                        }
                        else
                        {
                            ASSERT_ERR(0);
                        }
                    } break;
                    
                    case ANCC_GET_NTF_ATTS_CMD_OP_CODE:
                    {
                        // If there was an error then command is considered completed.
                        if (param->status != GATT_ERR_NO_ERROR)
                        {
                            // Send ANCC_CMP_EVT with status
                            ancc_send_cmp_evt(ancc_env, ANCC_GET_NTF_ATTS_CMD_OP_CODE, param->status);
                        }
                    } break;
                    
                    case ANCC_GET_APP_ATTS_CMD_OP_CODE:
                    {
                        // If there was an error then command is considered completed.
                        if (param->status != GATT_ERR_NO_ERROR)
                        {
                            // Send ANCC_CMP_EVT with status
                            ancc_send_cmp_evt(ancc_env, ANCC_GET_APP_ATTS_CMD_OP_CODE, param->status);
                        }
                    } break;


                    default:
                    {
                        ASSERT_ERR(0);
                    } break;
                }
            } break;

            case (GATTC_READ):
            {
                switch (((struct ancc_tmp_cmd *)ancc_env->operation)->operation)
                {
                    
                    case (ANCC_READ_CL_CFG_OP_CODE):
                    {
                        // Inform the requester that the read procedure is over
                        ancc_send_cmp_evt(ancc_env, ANCC_READ_CL_CFG_OP_CODE, param->status);
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
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
    {
        ASSERT_ERR(ancc_env->operation != NULL);

        // Prepare the indication to send to the application
        struct ancc_cl_cfg_val_ind *ind = KE_MSG_ALLOC(ANCC_CL_CFG_VAL_IND,
                                                  ancc_env->con_info.appid,
                                                  ancc_env->con_info.prf_id,
                                                  ancc_cl_cfg_val_ind);

        ind->conhdl   = gapc_get_conhdl(ancc_env->con_info.conidx);

        switch (((struct ancc_tmp_cmd *)ancc_env->operation)->operation)
        {
            case (ANCC_READ_CL_CFG_OP_CODE):
            {
                ind->att_code      = ((struct ancc_read_cmd *)ancc_env->operation)->read_code;
                ind->value = co_read16(&param->value[0]);
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
 * @brief Handles reception of response to a Get Notification Attributes command.
 * @param[in] ancc_env Pointer to the profile environment.
 * @param[in] value Pointer to the response value.
 * @param[in] length Length of the response value.
 ****************************************************************************************
 */
static void ancc_get_ntf_atts_rsp_handler(struct ancc_env_tag *ancc_env, uint8_t *value, uint16_t length)
{
    int idx = 0;
    
    while (idx < length)
    {
        switch (ancc_env->data_src.state)
        {
            case ANCC_DATA_SRC_NTF_IDLE:
            {
                idx += 5; // skip cmd id and uid
                
                // Compute the amount of requested attributes
                uint8_t mask  = ((struct ancc_get_ntf_atts_cmd *)ancc_env->operation)->atts;
                
                ancc_env->data_src.nb_req_atts = 0;
                for(int kk=0; kk < 8; ++kk)
                {
                    if (mask & 1)
                        ancc_env->data_src.nb_req_atts += 1;
                    mask >>= 1;
                }
                
                ancc_env->data_src.state = ANCC_DATA_SRC_NTF_NEXT_ATT; // set next state
            }
            break;
            
            case ANCC_DATA_SRC_NTF_NEXT_ATT:
            {
                ancc_env->data_src.curr_att_id = value[idx++];                
                ancc_env->data_src.state = ANCC_DATA_SRC_NTF_ATT_LENGTH; // set next state
            } break;
            
            case ANCC_DATA_SRC_NTF_ATT_LENGTH:
            {
                ancc_env->data_src.curr_att_length = value[idx++];                
                ancc_env->data_src.state = ANCC_DATA_SRC_NTF_ATT_MSB; // set next state
            } break;

            case ANCC_DATA_SRC_NTF_ATT_MSB:
            {
                ancc_env->data_src.curr_att_length |= (value[idx++] << 8);
                ancc_env->data_src.curr_value_idx = 0;
                
                // Allocate the indication
                struct ancc_ntf_att_ind *ind = KE_MSG_ALLOC_DYN(ANCC_NTF_ATT_IND,
                                                                ancc_env->con_info.appid, 
                                                                ancc_env->con_info.prf_id,
                                                                ancc_ntf_att_ind,
                                                                ancc_env->data_src.curr_att_length);
                
                ind->conhdl = gapc_get_conhdl(ancc_env->con_info.conidx);
                ind->ntf_uid    = ((struct ancc_get_ntf_atts_cmd *)ancc_env->operation)->ntf_uid;
                ind->att_id = ancc_env->data_src.curr_att_id;
                ind->length = ancc_env->data_src.curr_att_length;
                ancc_env->data_src.alloc_rsp_ind = ind;
                // Check if length of value is zero
                if (ancc_env->data_src.curr_att_length == 0)
                {
                    ancc_env->data_src.nb_req_atts -= 1;
                    
                    // Send indication to app
                    ke_msg_send(ancc_env->data_src.alloc_rsp_ind);
                    ancc_env->data_src.alloc_rsp_ind = NULL;
                    
                    // Check if all requested attributtes have been received
                    if (ancc_env->data_src.nb_req_atts > 0)
                    {
                        // Advance to the next attribute
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_NEXT_ATT; // set next state
                    }
                    else
                    {
                        // Send ANCC_CMP_EVT
                        ancc_send_cmp_evt(ancc_env, ANCC_GET_NTF_ATTS_CMD_OP_CODE, PRF_ERR_OK);
                        
                        // Reset notification reception state
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_IDLE;
                        
                    }
                }
                else
                {
                    ancc_env->data_src.state = ANCC_DATA_SRC_NTF_ATT_VALUE; // set next state
                }
            } break;
            
            case ANCC_DATA_SRC_NTF_ATT_VALUE:
            {
                ((struct ancc_ntf_att_ind *) ancc_env->data_src.alloc_rsp_ind)->value[ancc_env->data_src.curr_value_idx] = value[idx++];
                ancc_env->data_src.curr_value_idx += 1;
                
                // Check if attribute value has been read
                if (ancc_env->data_src.curr_value_idx == ancc_env->data_src.curr_att_length)
                {
                    ancc_env->data_src.nb_req_atts -= 1;
                    
                    // Send indication to app
                    ke_msg_send(ancc_env->data_src.alloc_rsp_ind);
                    ancc_env->data_src.alloc_rsp_ind = NULL;
                
                    // check if all requested attributtes have been received
                    if (ancc_env->data_src.nb_req_atts > 0)
                    {
                        // Advance to the next attribute
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_NEXT_ATT; // set next state
                    }
                    else
                    {
                        // Send completion event
                        ancc_send_cmp_evt(ancc_env, ANCC_GET_NTF_ATTS_CMD_OP_CODE, PRF_ERR_OK);
                        
                        // Return to idle state
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_IDLE; // set next state
                        
                    }
                }
            } break;

        }
    }
}

/**
 ****************************************************************************************
 * @brief Handles reception of response to a Get App Attributes command.
 * @param[in] ancc_env Pointer to the profile environment.
 * @param[in] value Pointer to the response value.
 * @param[in] length Length of the response value.
 ****************************************************************************************
 */
static void ancc_get_app_atts_rsp_handler(struct ancc_env_tag *ancc_env, uint8_t *value, uint16_t length)
{
    int idx = 0;
    
    while (idx < length)
    {
        switch (ancc_env->data_src.state)
        {
            case ANCC_DATA_SRC_NTF_IDLE:
            {
                // Skip Command ID
                idx += 1;
                
                // Compute the amount of requested attributes
                uint8_t mask  = ((struct ancc_get_app_atts_cmd *)ancc_env->operation)->atts;

                ancc_env->data_src.nb_req_atts = 0;
                for(int kk=0; kk < 8; ++kk)
                {
                    if (mask & 1)
                        ancc_env->data_src.nb_req_atts += 1;
                    mask >>= 1;
                }

                // Initialize index for the buffer that will hold the value
                ancc_env->data_src.curr_value_idx = 0;
                
                // Set next state
                ancc_env->data_src.state = ANCC_DATA_SRC_NTF_APP_ID;
            }
            break;

            case ANCC_DATA_SRC_NTF_APP_ID:
            {
                 if( ((struct ancc_get_app_atts_cmd *) ancc_env->operation)->app_id[ancc_env->data_src.curr_value_idx++]
                    != value[idx++])
                 {
                     ASSERT_ERR(0);
                 };
                
                // Check if app id has been read
                if (ancc_env->data_src.curr_value_idx == ((struct ancc_get_app_atts_cmd *) ancc_env->operation)->app_id_length+1)
                {
                    // Return to idle state
                    ancc_env->data_src.state = ANCC_DATA_SRC_NTF_NEXT_ATT; // set next state
                    
                    // Save current App ID length
                    ancc_env->data_src.curr_app_id_length = ancc_env->data_src.curr_value_idx;
                    
                }
            }
            break;
            
            case ANCC_DATA_SRC_NTF_NEXT_ATT:
            {
                // Get current attribute id
                ancc_env->data_src.curr_att_id = value[idx++];
                // Set next state
                ancc_env->data_src.state = ANCC_DATA_SRC_NTF_ATT_LENGTH; // set next state
            } break;
            
            case ANCC_DATA_SRC_NTF_ATT_LENGTH:
            {
                ancc_env->data_src.curr_att_length = value[idx++];                
                ancc_env->data_src.state = ANCC_DATA_SRC_NTF_ATT_MSB; // set next state
            } break;

            case ANCC_DATA_SRC_NTF_ATT_MSB:
            {
                ancc_env->data_src.curr_att_length |= (value[idx++] << 8);
                
                // Allocate the indication
                struct ancc_app_att_ind *ind = KE_MSG_ALLOC_DYN(ANCC_APP_ATT_IND,
                                               ancc_env->con_info.appid, 
                                               ancc_env->con_info.prf_id,
                                               ancc_app_att_ind,
                                               ancc_env->data_src.curr_app_id_length + ancc_env->data_src.curr_att_length);
                
                ind->conhdl = gapc_get_conhdl(ancc_env->con_info.conidx);
                ind->app_id_length = ancc_env->data_src.curr_app_id_length;
                ind->att_id = ancc_env->data_src.curr_att_id;
                ind->att_length = ancc_env->data_src.curr_att_length;
                /// Write App ID to ind
                memcpy(&ind->values,&(((struct ancc_get_app_atts_cmd *)ancc_env->operation)->app_id),((struct ancc_get_app_atts_cmd *)ancc_env->operation)->app_id_length);
                
                ancc_env->data_src.alloc_rsp_ind = ind;
                // check if length of value is zero
                if (ancc_env->data_src.curr_att_length == 0)
                {
                    ancc_env->data_src.nb_req_atts -= 1;
                    
                    //send indication to app
                    ke_msg_send(ancc_env->data_src.alloc_rsp_ind);
                    ancc_env->data_src.alloc_rsp_ind = NULL;
                    
                    // check if all requested attributtes have been received
                    if (ancc_env->data_src.nb_req_atts > 0)
                    {
                        //advance to the next attr
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_NEXT_ATT; // set next state
                    }
                    else
                    {
                        // send completion event
                        ancc_send_cmp_evt(ancc_env, ANCC_GET_APP_ATTS_CMD_OP_CODE, PRF_ERR_OK);
                        
                        // Return to idle state
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_IDLE; // set next state
                        
                    }
                }
                else
                {
                    ancc_env->data_src.state = ANCC_DATA_SRC_NTF_ATT_VALUE; // set next state
                }
            } break;
            
            case ANCC_DATA_SRC_NTF_ATT_VALUE:
            {
                ((struct ancc_app_att_ind *) ancc_env->data_src.alloc_rsp_ind)->values[ancc_env->data_src.curr_value_idx] = value[idx++];
                ancc_env->data_src.curr_value_idx += 1;
                
                // Check if the attribute value has been read
                if (ancc_env->data_src.curr_value_idx == ancc_env->data_src.curr_att_length 
                                                       + ancc_env->data_src.curr_app_id_length)
                {
                    ancc_env->data_src.nb_req_atts -= 1;
                    
                    //send indication to app
                    ke_msg_send(ancc_env->data_src.alloc_rsp_ind);
                    ancc_env->data_src.alloc_rsp_ind = NULL;
                    
                    // check if all requested attributtes have been received
                    if (ancc_env->data_src.nb_req_atts > 0)
                    {
                        //advance to the next attr
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_NEXT_ATT; // set next state
                    }
                    else
                    {
                        // send completion event
                        ancc_send_cmp_evt(ancc_env, ANCC_GET_APP_ATTS_CMD_OP_CODE, PRF_ERR_OK);
                        
                        // Return to idle state
                        ancc_env->data_src.state = ANCC_DATA_SRC_NTF_IDLE; // set next state
                        
                    }
                }
            } break;

        }
    }
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
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    if (ancc_env != NULL)
    {
        // Notification Source Characteristic Value
        if (param->handle == ancc_env->anc.chars[ANCC_CHAR_NTF_SRC].val_hdl)
        {
            // Check the category ID
            if (param->value[2] < CAT_ID_NB)
            {

                // Send an indication to the application
                struct ancc_ntf_src_ind *ntf_src_ind = KE_MSG_ALLOC(ANCC_NTF_SRC_IND,
                                                                    ancc_env->con_info.appid,
                                                                    ancc_env->con_info.prf_id,
                                                                    ancc_ntf_src_ind);

                ntf_src_ind->conhdl   = gapc_get_conhdl(ancc_env->con_info.conidx);
                
                // Fill the characteristic value
                ntf_src_ind->value.event_id          = param->value[0];
                ntf_src_ind->value.event_flags       = param->value[1];
                ntf_src_ind->value.cat_id            = param->value[2];
                ntf_src_ind->value.cat_cnt           = param->value[3];
                memcpy(&ntf_src_ind->value.ntf_uid, &param->value[4], sizeof(uint32_t));

                // Send the message
                ke_msg_send(ntf_src_ind);
            }
            // else ignore the message
        }
        // Data Source Characteristic Value
        else if (param->handle == ancc_env->anc.chars[ANCC_CHAR_DATA_SRC].val_hdl)
        {
            // For the first notification fragment, store the operation in generic command struct
            uint8_t operation = ((struct ancc_tmp_cmd *)ancc_env->operation)->operation;
           
            // Choose appropriate handler
            switch(operation)
            {
                case ANCC_GET_NTF_ATTS_CMD_OP_CODE: ancc_get_ntf_atts_rsp_handler(ancc_env, (uint8_t *) &param->value, param->length); break;
                case ANCC_GET_APP_ATTS_CMD_OP_CODE: ancc_get_app_atts_rsp_handler(ancc_env, (uint8_t *) &param->value, param->length); break;
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
 * @brief Disconnection indication to ANCC.
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
    struct ancc_env_tag *ancc_env = PRF_CLIENT_GET_ENV(dest_id, ancc);

    ASSERT_ERR(ancc_env != NULL);

    // Free the stored operation if needed
    if (ancc_env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(ancc_env->operation));
        ancc_env->operation = NULL;
    }

    PRF_CLIENT_DISABLE_IND_SEND(ancc_envs, dest_id, ANCC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler ancc_default_state[] =
{
    {ANCC_ENABLE_REQ,               (ke_msg_func_t)ancc_enable_req_handler},
    {ANCC_READ_CMD,                 (ke_msg_func_t)ancc_read_cmd_handler},
    {ANCC_WRITE_CMD,                (ke_msg_func_t)ancc_write_cmd_handler},

    {ANCC_GET_NTF_ATTS_CMD,         (ke_msg_func_t)ancc_get_ntf_atts_cmd_handler},
    {ANCC_GET_APP_ATTS_CMD,         (ke_msg_func_t)ancc_get_app_atts_cmd_handler},

    {GATTC_DISC_SVC_IND,            (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,           (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,      (ke_msg_func_t)gattc_disc_char_desc_ind_handler},

    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler ancc_state_handler[ANCC_STATE_MAX] =
{
    [ANCC_IDLE]        = KE_STATE_HANDLER_NONE,
    [ANCC_CONNECTED]   = KE_STATE_HANDLER_NONE,
    [ANCC_BUSY]        = KE_STATE_HANDLER_NONE,
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler ancc_default_handler = KE_STATE_HANDLER(ancc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t ancc_state[ANCC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_ANC_CLIENT)

/// @} ANCCTASK
