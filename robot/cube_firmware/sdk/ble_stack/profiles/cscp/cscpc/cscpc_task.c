/**
 ****************************************************************************************
 *
 * @file cscpc_task.c
 *
 * @brief Cycling Speed and Cadence Profile Collector Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup CSCPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"   

#if (BLE_CSC_COLLECTOR)
#include "cscp_common.h"
#include "gap.h"
#include "attm.h"
#include "cscpc.h"
#include "cscpc_task.h"
#include "gattc_task.h"
#include "attm_cfg.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve Cycling Speed and Cadence service characteristics information
const struct prf_char_def cscpc_cscs_char[CSCP_CSCS_CHAR_MAX] =
{
    /// CSC Measurement
    [CSCP_CSCS_CSC_MEAS_CHAR]      = {ATT_CHAR_CSC_MEAS,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_NTF},
    /// CSC Feature
    [CSCP_CSCS_CSC_FEAT_CHAR]      = {ATT_CHAR_CSC_FEAT,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_RD},
    /// Sensor Location
    [CSCP_CSCS_SENSOR_LOC_CHAR]    = {ATT_CHAR_SENSOR_LOC,
                                      ATT_OPTIONAL,
                                      ATT_CHAR_PROP_RD},
    /// SC Control Point
    [CSCP_CSCS_SC_CTNL_PT_CHAR]    = {ATT_CHAR_SC_CNTL_PT,
                                      ATT_OPTIONAL,
                                      ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND},
};

/// State machine used to retrieve Cycling Speed and Cadence service characteristic descriptor information
const struct prf_char_desc_def cscpc_cscs_char_desc[CSCPC_DESC_MAX] =
{
    /// CSC Measurement Char. - Client Characteristic Configuration
    [CSCPC_DESC_CSC_MEAS_CL_CFG]    = {ATT_DESC_CLIENT_CHAR_CFG,
                                       ATT_MANDATORY,
                                       CSCP_CSCS_CSC_MEAS_CHAR},
    /// SC Control Point Char. - Client Characteristic Configuration
    [CSCPC_DESC_SC_CTNL_PT_CL_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG,
                                       ATT_OPTIONAL,
                                       CSCP_CSCS_SC_CTNL_PT_CHAR},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CSCPC_ENABLE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cscpc_enable_cmd_handler(ke_msg_id_t const msgid,
                                    struct cscpc_enable_cmd *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = PRF_ERR_OK;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Cycling Speed and Cadence Profile Collector Role Task Environment
    struct cscpc_env_tag *cscpc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Check if the provided conhdl is valid
    if (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX)
    {
        // Fill the Connection Information structure
        con_info.conidx = gapc_get_conidx(param->conhdl);
        con_info.prf_id = dest_id;
        con_info.appid  = src_id;

        // Add an environment for the provided connection
        status = PRF_CLIENT_ENABLE(con_info, param, cscpc);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status == PRF_ERR_OK)
    {
        // Get the newly created environment
        cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

        // Keep the connection info
        memcpy(&cscpc_env->con_info, &con_info, sizeof(struct prf_con_info));

        // Start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Configure the environment for a discovery procedure
            cscpc_env->last_req  = ATT_SVC_CYCLING_SPEED_CADENCE;
            // Force the operation value
            param->operation     = CSCPC_ENABLE_OP_CODE;

            prf_disc_svc_send(&(cscpc_env->con_info), ATT_SVC_CYCLING_SPEED_CADENCE);

            // Keep the operation
            cscpc_env->operation = (void *)param;
            // Do not free the message
            msg_status = KE_MSG_NO_FREE;

            // Go to BUSY state
            ke_state_set(dest_id, CSCPC_BUSY);
        }
        // Bond information are provided
        else
        {
            // Keep the provided database content
            memcpy(&cscpc_env->cscs, &param->cscs, sizeof(struct cscpc_cscs_content));

            // Register in GATT for notifications/indications
            prf_register_atthdl2gatt(&cscpc_env->con_info, &cscpc_env->cscs.svc);

            // Send a complete event status to the application
            cscpc_send_cmp_evt(cscpc_env, CSCPC_ENABLE_OP_CODE, PRF_ERR_OK);
        }
    }
    else if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message will be forwarded towards the good task instance
        msg_status = KE_MSG_NO_FREE;
    }
    else
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        cscpc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, CSCPC_ENABLE_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CSCPC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cscpc_read_cmd_handler(ke_msg_id_t const msgid,
                                  struct cscpc_read_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Get the address of the environment
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    if (cscpc_env != NULL)
    {
        // Attribute Handle
        uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;
        // Status
        uint8_t status     = PRF_ERR_OK;

        ASSERT_ERR(ke_state_get(dest_id) != CSCPC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == cscpc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == CSCPC_BUSY)
            {
                // Keep the request for later
                msg_status = KE_MSG_SAVED;
            }
            else    // State is CSCPC_CONNECTED
            {
                ASSERT_ERR(cscpc_env->operation == NULL);

                switch (param->read_code)
                {
                    // Read CSC Feature
                    case (CSCPC_RD_CSC_FEAT):
                    {
                        handle = cscpc_env->cscs.chars[CSCP_CSCS_CSC_FEAT_CHAR].val_hdl;
                    } break;

                    // Read Sensor Location
                    case (CSCPC_RD_SENSOR_LOC):
                    {
                        handle = cscpc_env->cscs.chars[CSCP_CSCS_SENSOR_LOC_CHAR].val_hdl;
                    } break;

                    // Read CSC Measurement Characteristic Client Char. Cfg. Descriptor Value
                    case (CSCPC_RD_WR_CSC_MEAS_CFG):
                    {
                        handle = cscpc_env->cscs.descs[CSCPC_DESC_CSC_MEAS_CL_CFG].desc_hdl;
                    } break;

                    // Read Unread Alert Characteristic Client Char. Cfg. Descriptor Value
                    case (CSCPC_RD_WR_SC_CTNL_PT_CFG):
                    {
                        handle = cscpc_env->cscs.descs[CSCPC_DESC_SC_CTNL_PT_CL_CFG].desc_hdl;
                    } break;

                    default:
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    } break;
                }

                // Check if handle is viable
                if (handle != ATT_INVALID_SEARCH_HANDLE)
                {
                    // Force the operation value
                    param->operation       = CSCPC_READ_OP_CODE;

                    // Store the command structure
                    cscpc_env->operation   = param;
                    msg_status             = KE_MSG_NO_FREE;

                    // Send the read request
                    prf_read_char_send(&(cscpc_env->con_info), cscpc_env->cscs.svc.shdl,
                                       cscpc_env->cscs.svc.ehdl, handle);

                    // Go to the Busy state
                    ke_state_set(dest_id, CSCPC_BUSY);
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
            // Send the complete event message to the task id stored in the environment
            cscpc_send_cmp_evt(cscpc_env, CSCPC_READ_OP_CODE, status);
        }
    }
    else
    {
        // No connection exists, send the status to the requester
        cscpc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, CSCPC_READ_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CSCPC_CFG_NTFIND_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cscpc_cfg_ntfind_cmd_handler(ke_msg_id_t const msgid,
                                        struct cscpc_cfg_ntfind_cmd *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    if (cscpc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;
        // Handle
        uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;

        do
        {
            // Check the provided connection handle
            if (gapc_get_conidx(param->conhdl) != cscpc_env->con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) == CSCPC_BUSY)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is PRF_ERR_OK, no message will be sent to the application
                break;
            }

            // state = CSCPC_CONNECTED
            ASSERT_ERR(cscpc_env->operation == NULL);

            switch(param->desc_code)
            {
                // Write CSC Measurement Characteristic Client Char. Cfg. Descriptor Value
                case (CSCPC_RD_WR_CSC_MEAS_CFG):
                {
                    if (param->ntfind_cfg <= PRF_CLI_START_NTF)
                    {
                        handle = cscpc_env->cscs.descs[CSCPC_DESC_CSC_MEAS_CL_CFG].desc_hdl;

                        // The descriptor is mandatory
                        ASSERT_ERR(handle != ATT_INVALID_SEARCH_HANDLE);
                    }
                    else
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    }
                } break;

                // Write SC Control Point Characteristic Client Char. Cfg. Descriptor Value
                case (CSCPC_RD_WR_SC_CTNL_PT_CFG):
                {
                    if ((param->ntfind_cfg == PRF_CLI_STOP_NTFIND) ||
                        (param->ntfind_cfg == PRF_CLI_START_IND))
                    {
                        handle = cscpc_env->cscs.descs[CSCPC_DESC_SC_CTNL_PT_CL_CFG].desc_hdl;

                        if (handle == ATT_INVALID_SEARCH_HANDLE)
                        {
                            // The descriptor has not been found.
                            status = PRF_ERR_INEXISTENT_HDL;
                        }
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
        } while (0);

        if (status == PRF_ERR_OK)
        {
            // Set the operation code
            param->operation = CSCPC_CFG_NTF_IND_OP_CODE;

            // Store the command structure
            cscpc_env->operation   = param;
            msg_status = KE_MSG_NO_FREE;

            // Go to the Busy state
            ke_state_set(dest_id, CSCPC_BUSY);

            // Send GATT Write Request
            prf_gatt_write_ntf_ind(&cscpc_env->con_info, handle, param->ntfind_cfg);
        }
        else
        {
            cscpc_send_cmp_evt(cscpc_env, CSCPC_CFG_NTF_IND_OP_CODE, status);
        }
    }
    else
    {
        cscpc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, CSCPC_CFG_NTF_IND_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CSCPC_CFG_NTFIND_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cscpc_ctnl_pt_cfg_cmd_handler(ke_msg_id_t const msgid,
                                         struct cscpc_ctnl_pt_cfg_cmd *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    if (cscpc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;

        do
        {
            // State is Connected or Busy
            ASSERT_ERR(ke_state_get(dest_id) > CSCPC_IDLE);

            // Check the provided connection handle
            if (gapc_get_conidx(param->conhdl) != cscpc_env->con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) == CSCPC_BUSY)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is PRF_ERR_OK, no message will be sent to the application
                break;
            }

            // Check if the characteristic has been found
            if (cscpc_env->cscs.descs[CSCPC_DESC_SC_CTNL_PT_CL_CFG].desc_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Packed request
                uint8_t req[CSCP_SC_CNTL_PT_REQ_MAX_LEN];
                // Request Length
                uint8_t req_len = CSCP_SC_CNTL_PT_REQ_MIN_LEN;

                // Set the operation code
                req[0] = param->sc_ctnl_pt.op_code;

                // Fulfill the message according to the operation code
                switch (param->sc_ctnl_pt.op_code)
                {
                    case (CSCP_CTNL_PT_OP_SET_CUMUL_VAL):
                    {
                        // Set the cumulative value
                        co_write32p(&req[1], param->sc_ctnl_pt.value.cumul_val);
                        // Update length
                        req_len += 4;
                    } break;

                    case (CSCP_CTNL_PT_OP_UPD_LOC):
                    {
                        // Set the sensor location
                        req[1] = param->sc_ctnl_pt.value.sensor_loc;
                        // Update length
                        req_len++;
                    } break;

                    case (CSCP_CTNL_PT_OP_RESERVED):
                    case (CSCP_CTNL_PT_OP_START_CALIB):
                    case (CSCP_CTNL_PT_OP_REQ_SUPP_LOC):
                    {
                        // Nothing more to do
                    } break;

                    default:
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    } break;
                }

                if (status == PRF_ERR_OK)
                {
                    // Set the operation code
                    param->operation = CSCPC_CTNL_PT_CFG_WR_OP_CODE;

                    // Store the command structure
                    cscpc_env->operation   = param;
                    // Store the command information
                    msg_status = KE_MSG_NO_FREE;

                    // Go to the Busy state
                    ke_state_set(dest_id, CSCPC_BUSY);

                    // Send the write request
                    prf_gatt_write(&(cscpc_env->con_info), cscpc_env->cscs.chars[CSCP_CSCS_SC_CTNL_PT_CHAR].val_hdl,
                                   (uint8_t *)&req[0], req_len, GATTC_WRITE);
                }
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send a complete event status to the application
            cscpc_send_cmp_evt(cscpc_env, CSCPC_CTNL_PT_CFG_WR_OP_CODE, status);
        }
    }
    else
    {
        // No connection
        cscpc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, CSCPC_CTNL_PT_CFG_WR_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CSCPC_TIMEOUT_TIMER_IND message. This message is
 * received when the peer device doesn't send a SC Control Point indication within 30s
 * after reception of the write response.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cscpc_timeout_timer_ind_handler(ke_msg_id_t const msgid,
                                           void const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    if (cscpc_env != NULL)
    {
        ASSERT_ERR(cscpc_env->operation != NULL);
        ASSERT_ERR(((struct cscpc_cmd *)cscpc_env->operation)->operation == CSCPC_CTNL_PT_CFG_IND_OP_CODE);

        // Send the complete event message
        cscpc_send_cmp_evt(cscpc_env, CSCPC_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_PROC_TIMEOUT);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
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
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    // Check if the environment exists
    if (cscpc_env != NULL)
    {
        ASSERT_ERR(cscpc_env->operation != NULL);
        ASSERT_ERR(((struct cscpc_cmd *)cscpc_env->operation)->operation == CSCPC_ENABLE_OP_CODE);

        // Keep only one instance of the service
        if (cscpc_env->nb_svc == 0)
        {
            // Keep the start handle and the end handle of the service
            cscpc_env->cscs.svc.shdl = param->start_hdl;
            cscpc_env->cscs.svc.ehdl = param->end_hdl;
            cscpc_env->nb_svc++;
        }
    }
    // else drop the message

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
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    // Check if the environment exists
    if (cscpc_env != NULL)
    {
        ASSERT_ERR(cscpc_env->operation != NULL);
        ASSERT_ERR(((struct cscpc_cmd *)cscpc_env->operation)->operation == CSCPC_ENABLE_OP_CODE);

        // Retrieve CSCS characteristics
        prf_search_chars(cscpc_env->cscs.svc.ehdl, CSCP_CSCS_CHAR_MAX,
                         &cscpc_env->cscs.chars[0], &cscpc_cscs_char[0],
                         param, (uint8_t *)&cscpc_env->last_char_code);
    }
    // else drop the message

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
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    // Check if the environment exists
    if (cscpc_env != NULL)
    {
        ASSERT_ERR(cscpc_env->operation != NULL);
        ASSERT_ERR(((struct cscpc_cmd *)cscpc_env->operation)->operation == CSCPC_ENABLE_OP_CODE);

        // Retrieve CSCS descriptors
        prf_search_descs(CSCPC_DESC_MAX, &cscpc_env->cscs.descs[0], &cscpc_cscs_char_desc[0],
                         param, cscpc_env->last_req);
    }
    // else drop the message

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
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    if (cscpc_env != NULL)
    {
        switch (param->req_type)
        {
            case (GATTC_DISC_BY_UUID_SVC):
            case (GATTC_DISC_ALL_CHAR):
            case (GATTC_DISC_DESC_CHAR):
            {
                // Status
                uint8_t status = PRF_ERR_STOP_DISC_CHAR_MISSING;

                ASSERT_ERR(cscpc_env->operation != NULL);
                ASSERT_ERR(((struct cscpc_cmd *)cscpc_env->operation)->operation == CSCPC_ENABLE_OP_CODE);

                if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                    (param->status == ATT_ERR_NO_ERROR))
                {
                    /* -------------------------------------------------
                     * SERVICE DISCOVERY -------------------------------
                     * ------------------------------------------------- */
                    if (cscpc_env->last_req == ATT_SVC_CYCLING_SPEED_CADENCE)
                    {
                        if (cscpc_env->nb_svc > 0)
                        {
                            // Check if service handles are OK
                            if ((cscpc_env->cscs.svc.shdl != ATT_INVALID_HANDLE) &&
                                (cscpc_env->cscs.svc.ehdl != ATT_INVALID_HANDLE) &&
                                (cscpc_env->cscs.svc.shdl < cscpc_env->cscs.svc.ehdl))
                            {
                                // Status is OK
                                status = PRF_ERR_OK;

                                // Discover all CSCS characteristics
                                cscpc_env->last_req = ATT_DECL_CHARACTERISTIC;
                                prf_disc_char_all_send(&(cscpc_env->con_info), &(cscpc_env->cscs.svc));
                            }
                            // Handles are not corrects, the Cycling Speed and Cadence Service has not been found, stop
                        }
                        // The Cycling Speed and Cadence Service has not been found, stop discovery
                    }

                    /* -------------------------------------------------
                     * CHARACTERISTICS DISCOVERY -----------------------
                     * ------------------------------------------------- */
                    else if (cscpc_env->last_req == ATT_DECL_CHARACTERISTIC)
                    {
                        // Check if mandatory properties have been found and if properties are correct
                        status = prf_check_svc_char_validity(CSCP_CSCS_CHAR_MAX, cscpc_env->cscs.chars, cscpc_cscs_char);

                        // Check for characteristic properties.
                        if (status == PRF_ERR_OK)
                        {
                            cscpc_env->last_req = CSCP_CSCS_CSC_MEAS_CHAR;

                            // Find the Client Characteristic Configuration Descriptor for the CSC Measurement characteristic
                            prf_disc_char_desc_send(&(cscpc_env->con_info),
                                                    &(cscpc_env->cscs.chars[CSCP_CSCS_CSC_MEAS_CHAR]));
                        }
                    }

                    /* -------------------------------------------------
                     * DESCRIPTORS DISCOVERY ---------------------------
                     * ------------------------------------------------- */
                    else
                    {
                        // Discovery over ?
                        bool disc_over = true;

                        if (cscpc_env->last_req == CSCP_CSCS_CSC_MEAS_CHAR)
                        {
                            // Check if the SC Control Point Characteristic has been found
                            if (cscpc_env->cscs.chars[CSCP_CSCS_SC_CTNL_PT_CHAR].char_hdl != ATT_INVALID_SEARCH_HANDLE)
                            {
                                // Status is OK
                                status = PRF_ERR_OK;

                                // Find the Client Characteristic Configuration Descriptor for the SC Control Point characteristic
                                cscpc_env->last_req = CSCP_CSCS_SC_CTNL_PT_CHAR;
                                prf_disc_char_desc_send(&(cscpc_env->con_info),
                                                        &(cscpc_env->cscs.chars[CSCP_CSCS_SC_CTNL_PT_CHAR]));

                                // Discovery continues
                                disc_over = false;
                            }
                        }
                        else
                        {
                            // Discovery is over
                            ASSERT_ERR(cscpc_env->last_req == CSCP_CSCS_SC_CTNL_PT_CHAR);
                        }

                        if (disc_over)
                        {
                            status = prf_check_svc_char_desc_validity(CSCPC_DESC_MAX,
                                                                      cscpc_env->cscs.descs,
                                                                      cscpc_cscs_char_desc,
                                                                      cscpc_env->cscs.chars);

                            if (status == PRF_ERR_OK)
                            {
                                // Reset number of services
                                cscpc_env->nb_svc = 0;

                                // Register in GATT for notifications/indications
                                prf_register_atthdl2gatt(&cscpc_env->con_info, &cscpc_env->cscs.svc);

                                // Send the content of the service to the HL
                                struct cscpc_cscs_content_ind *ind = KE_MSG_ALLOC(CSCPC_CSCS_CONTENT_IND,
                                                                                  cscpc_env->con_info.appid,
                                                                                  cscpc_env->con_info.prf_id,
                                                                                  cscpc_cscs_content_ind);

                                ind->conhdl = gapc_get_conhdl(cscpc_env->con_info.conidx);
                                memcpy(&ind->cscs, &cscpc_env->cscs, sizeof(struct cscpc_cscs_content));

                                // Send the message
                                ke_msg_send(ind);

                                // Stop discovery procedure.
                                cscpc_send_cmp_evt(cscpc_env, CSCPC_ENABLE_OP_CODE, status);
                            }
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
                    cscpc_send_cmp_evt(cscpc_env, CSCPC_ENABLE_OP_CODE, status);
                }
            } break;

            case (GATTC_READ):
            {
                // Send the complete event status
                cscpc_send_cmp_evt(cscpc_env, CSCPC_READ_OP_CODE, param->status);
            } break;

            case (GATTC_WRITE):
            {
                uint8_t operation = ((struct cscpc_cmd *)cscpc_env->operation)->operation;

                if (operation == CSCPC_CFG_NTF_IND_OP_CODE)
                {
                    // Send the complete event status
                    cscpc_send_cmp_evt(cscpc_env, operation, param->status);
                }
                else if (operation == CSCPC_CTNL_PT_CFG_WR_OP_CODE)
                {
                    if (param->status == PRF_ERR_OK)
                    {
                        // Start Timeout Procedure
                        ke_timer_set(CSCPC_TIMEOUT_TIMER_IND, dest_id, ATT_TRANS_RTX);

                        // Wait for the response indication
                        ((struct cscpc_cmd *)cscpc_env->operation)->operation = CSCPC_CTNL_PT_CFG_IND_OP_CODE;
                    }
                    else
                    {
                        // Send the complete event status
                        cscpc_send_cmp_evt(cscpc_env, operation, param->status);
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
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    if (cscpc_env != NULL)
    {
        ASSERT_ERR(cscpc_env->operation != NULL);
        ASSERT_ERR(((struct cscpc_cmd *)cscpc_env->operation)->operation == CSCPC_READ_OP_CODE);

        // Send the read value to the HL
        struct cscpc_value_ind *ind = KE_MSG_ALLOC(CSCPC_VALUE_IND,
                                                   cscpc_env->con_info.appid,
                                                   cscpc_env->con_info.prf_id,
                                                   cscpc_value_ind);

        ind->conhdl   = gapc_get_conhdl(cscpc_env->con_info.conidx);

        switch (((struct cscpc_read_cmd *)cscpc_env->operation)->read_code)
        {
            // Read CSC Feature Characteristic value
            case (CSCPC_RD_CSC_FEAT):
            {
                co_write16p(&ind->value.sensor_feat, param->value[0]);

                // Mask the reserved bits
                ind->value.sensor_feat &= CSCP_FEAT_ALL_SUPP;
            } break;

            // Read Sensor Location Characteristic value
            case (CSCPC_RD_SENSOR_LOC):
            {
                ind->value.sensor_loc = (param->value[0] < CSCP_LOC_MAX) ? param->value[0] : CSCP_LOC_OTHER;
            } break;

            // Read Client Characteristic Configuration Descriptor value
            case (CSCPC_RD_WR_CSC_MEAS_CFG):
            case (CSCPC_RD_WR_SC_CTNL_PT_CFG):
            {
                co_write16p(&ind->value.ntf_cfg, param->value[0]);
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        ind->att_code = ((struct cscpc_read_cmd *)cscpc_env->operation)->read_code;

        // Send the message to the application
        ke_msg_send(ind);
    }
    // else drop the message

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
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    if (cscpc_env != NULL)
    {
        switch (param->type)
        {
            case (GATTC_NOTIFY):
            {
                // Offset
                uint8_t offset = CSCP_CSC_MEAS_MIN_LEN;

                // CSC Measurement value has been received
                struct cscpc_value_ind *ind = KE_MSG_ALLOC(CSCPC_VALUE_IND,
                                                           cscpc_env->con_info.appid,
                                                           cscpc_env->con_info.prf_id,
                                                           cscpc_value_ind);

                // Connection Handle
                ind->conhdl   = gapc_get_conhdl(cscpc_env->con_info.conidx);
                // Attribute code
                ind->att_code = CSCPC_NTF_CSC_MEAS;

                /*----------------------------------------------------
                 * Unpack Measurement --------------------------------
                 *----------------------------------------------------*/

                // Flags
                ind->value.csc_meas.flags = param->value[0];

                // Cumulative Wheel Revolutions
                // Last Wheel Event Time
                if ((param->value[0] & CSCP_MEAS_WHEEL_REV_DATA_PRESENT) == CSCP_MEAS_WHEEL_REV_DATA_PRESENT)
                {
                    // Cumulative Wheel Revolutions
                    ind->value.csc_meas.cumul_wheel_rev = co_read32p(&param->value[offset]);
                    offset += 4;

                    // Last Wheel Event Time
                    ind->value.csc_meas.last_wheel_evt_time = co_read16p(&param->value[offset]);
                    offset += 2;
                }

                // Cumulative Crank Revolutions
                // Last Crank Event Time
                if ((param->value[0] & CSCP_MEAS_CRANK_REV_DATA_PRESENT) == CSCP_MEAS_CRANK_REV_DATA_PRESENT)
                {
                    // Cumulative Crank Revolutions
                    ind->value.csc_meas.cumul_crank_rev = co_read16p(&param->value[offset]);
                    offset += 2;

                    // Last Crank Event Time
                    ind->value.csc_meas.last_crank_evt_time = co_read16p(&param->value[offset]);
                }

                ke_msg_send(ind);
            } break;

            case (GATTC_INDICATE):
            {
                // Check if we were waiting for the indication
                if (cscpc_env->operation != NULL)
                {
                    if (((struct cscpc_cmd *)cscpc_env->operation)->operation == CSCPC_CTNL_PT_CFG_IND_OP_CODE)
                    {
                        // Stop the procedure timeout timer
                        ke_timer_clear(CSCPC_TIMEOUT_TIMER_IND, dest_id);

                        // CSC Measurement value has been received
                        struct cscpc_value_ind *ind = KE_MSG_ALLOC(CSCPC_VALUE_IND,
                                                                   cscpc_env->con_info.appid,
                                                                   cscpc_env->con_info.prf_id,
                                                                   cscpc_value_ind);

                        // Connection Handle
                        ind->conhdl   = gapc_get_conhdl(cscpc_env->con_info.conidx);
                        // Attribute code
                        ind->att_code = CSCPC_IND_SC_CTNL_PT;

                        // Requested operation code
                        ind->value.ctnl_pt_rsp.req_op_code = param->value[1];
                        // Response value
                        ind->value.ctnl_pt_rsp.resp_value  = param->value[2];

                        // Get the list of supported sensor locations if needed
                        if ((ind->value.ctnl_pt_rsp.req_op_code == CSCP_CTNL_PT_OP_REQ_SUPP_LOC) &&
                            (ind->value.ctnl_pt_rsp.resp_value == CSCP_CTNL_PT_RESP_SUCCESS)     &&
                            (param->length > 3))
                        {
                            // Get the number of supported locations that have been received
                            uint8_t nb_supp_loc = (param->length - 3);
                            // Counter
                            uint8_t counter;
                            // Location
                            uint8_t loc;

                            for (counter = 0; counter < nb_supp_loc; counter++)
                            {
                                loc = param->value[counter + 3];

                                // Check if valid
                                if (loc < CSCP_LOC_MAX)
                                {
                                    ind->value.ctnl_pt_rsp.supp_loc |= (1 << loc);
                                }
                            }
                        }

                        // Send the message
                        ke_msg_send(ind);

                        // Send the complete event message
                        cscpc_send_cmp_evt(cscpc_env, CSCPC_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_OK);
                    }
                    // else drop the message
                }
                // else drop the message
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to CSCPC.
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
    struct cscpc_env_tag *cscpc_env = PRF_CLIENT_GET_ENV(dest_id, cscpc);

    ASSERT_ERR(cscpc_env != NULL);

    // Free the stored operation if needed
    if (cscpc_env->operation != NULL)
    {
        // Check if we were waiting for a SC Control Point indication
        if (((struct cscpc_cmd *)cscpc_env->operation)->operation == CSCPC_CTNL_PT_CFG_IND_OP_CODE)
        {
            // Stop the procedure timeout timer
            ke_timer_clear(CSCPC_TIMEOUT_TIMER_IND, dest_id);
        }

        ke_msg_free(ke_param2msg(cscpc_env->operation));
        cscpc_env->operation = NULL;
    }

    PRF_CLIENT_DISABLE_IND_SEND(cscpc_envs, dest_id, CSCPC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler cscpc_default_state[] =
{
    {CSCPC_ENABLE_CMD,               (ke_msg_func_t)cscpc_enable_cmd_handler},
    {CSCPC_READ_CMD,                 (ke_msg_func_t)cscpc_read_cmd_handler},
    {CSCPC_CFG_NTFIND_CMD,           (ke_msg_func_t)cscpc_cfg_ntfind_cmd_handler},
    {CSCPC_CTNL_PT_CFG_CMD,          (ke_msg_func_t)cscpc_ctnl_pt_cfg_cmd_handler},

    {CSCPC_TIMEOUT_TIMER_IND,        (ke_msg_func_t)cscpc_timeout_timer_ind_handler},

    {GATTC_DISC_SVC_IND,             (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,            (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,       (ke_msg_func_t)gattc_disc_char_desc_ind_handler},

    {GATTC_READ_IND,                 (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,                (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                  (ke_msg_func_t)gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler cscpc_state_handler[CSCPC_STATE_MAX] =
{
    [CSCPC_IDLE]        = KE_STATE_HANDLER_NONE,
    [CSCPC_CONNECTED]   = KE_STATE_HANDLER_NONE,
    [CSCPC_BUSY]        = KE_STATE_HANDLER_NONE,
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler cscpc_default_handler = KE_STATE_HANDLER(cscpc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t cscpc_state[CSCPC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_CSC_COLLECTOR)

/// @} CSCPCTASK
