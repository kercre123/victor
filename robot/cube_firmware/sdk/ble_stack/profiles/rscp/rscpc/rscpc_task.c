/**
 ****************************************************************************************
 *
 * @file rscpc_task.c
 *
 * @brief Running Speed and Cadence Profile Collector Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup RSCPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_RSC_COLLECTOR)
#include "rscp_common.h"
#include "gap.h"
#include "attm.h"
#include "rscpc.h"
#include "rscpc_task.h"
#include "gattc_task.h"
#include "prf_utils.h"
#include "attm_cfg.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve Running Speed and Cadence service characteristics information
const struct prf_char_def rscpc_rscs_char[RSCP_RSCS_CHAR_MAX] =
{
    /// RSC Measurement
    [RSCP_RSCS_RSC_MEAS_CHAR]      = {ATT_CHAR_RSC_MEAS,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_NTF},
    /// RSC Feature
    [RSCP_RSCS_RSC_FEAT_CHAR]      = {ATT_CHAR_RSC_FEAT,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_RD},
    /// Sensor Location
    [RSCP_RSCS_SENSOR_LOC_CHAR]    = {ATT_CHAR_SENSOR_LOC,
                                      ATT_OPTIONAL,
                                      ATT_CHAR_PROP_RD},
    /// SC Control Point
    [RSCP_RSCS_SC_CTNL_PT_CHAR]    = {ATT_CHAR_SC_CNTL_PT,
                                      ATT_OPTIONAL,
                                      ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND},
};

/// State machine used to retrieve Running Speed and Cadence service characteristic descriptor information
const struct prf_char_desc_def rscpc_rscs_char_desc[RSCPC_DESC_MAX] =
{
    /// RSC Measurement Char. - Client Characteristic Configuration
    [RSCPC_DESC_RSC_MEAS_CL_CFG]    = {ATT_DESC_CLIENT_CHAR_CFG,
                                       ATT_MANDATORY,
                                       RSCP_RSCS_RSC_MEAS_CHAR},
    /// SC Control Point Char. - Client Characteristic Configuration
    [RSCPC_DESC_SC_CTNL_PT_CL_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG,
                                       ATT_OPTIONAL,
                                       RSCP_RSCS_SC_CTNL_PT_CHAR},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_ENABLE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int rscpc_enable_cmd_handler(ke_msg_id_t const msgid,
                                    struct rscpc_enable_cmd *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = PRF_ERR_OK;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Running Speed and Cadence Profile Collector Role Task Environment
    struct rscpc_env_tag *rscpc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Check if the provided connection handle is valid
    if (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX)
    {
        // Add an environment for the provided connection
        status = PRF_CLIENT_ENABLE(con_info, param, rscpc);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status == PRF_ERR_OK)
    {
        // Get the newly created environment
        rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

        // Keep the connection info
        memcpy(&rscpc_env->con_info, &con_info, sizeof(struct prf_con_info));

        // Start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Configure the environment for a discovery procedure
            rscpc_env->last_req  = ATT_SVC_RUNNING_SPEED_CADENCE;
            // Force the operation value
            param->operation    = RSCPC_ENABLE_OP_CODE;

            prf_disc_svc_send(&(rscpc_env->con_info), ATT_SVC_RUNNING_SPEED_CADENCE);

            // Keep the operation
            rscpc_env->operation = (void *)param;
            msg_status           = KE_MSG_NO_FREE;

            // Go to BUSY state
            ke_state_set(dest_id, RSCPC_BUSY);
        }
        // Bond information are provided
        else
        {
            // Keep the provided database content
            memcpy(&rscpc_env->rscs, &param->rscs, sizeof(struct rscpc_rscs_content));

            // Register in GATT for notifications/indications
            prf_register_atthdl2gatt(&rscpc_env->con_info, &rscpc_env->rscs.svc);

            // Send a complete event status to the application
            rscpc_send_cmp_evt(rscpc_env, RSCPC_ENABLE_OP_CODE, PRF_ERR_OK);
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
        rscps_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, RSCPC_ENABLE_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int rscpc_read_cmd_handler(ke_msg_id_t const msgid,
                                  struct rscpc_read_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    if (rscpc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;
        // Attribute Handle
        uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;

        // State is Connected or Busy
        ASSERT_ERR(ke_state_get(dest_id) != RSCPC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == rscpc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == RSCPC_BUSY)
            {
                // Keep the request for later
                msg_status = KE_MSG_SAVED;
            }
            else    // State is RSCPC_CONNECTED
            {
                ASSERT_ERR(rscpc_env->operation == NULL);

                switch (param->read_code)
                {
                    // Read RSC Feature
                    case (RSCPC_RD_RSC_FEAT):
                    {
                        handle = rscpc_env->rscs.chars[RSCP_RSCS_RSC_FEAT_CHAR].val_hdl;
                    } break;

                    // Read Sensor Location
                    case (RSCPC_RD_SENSOR_LOC):
                    {
                        handle = rscpc_env->rscs.chars[RSCP_RSCS_SENSOR_LOC_CHAR].val_hdl;
                    } break;

                    // Read RSC Measurement Characteristic Client Char. Cfg. Descriptor Value
                    case (RSCPC_RD_WR_RSC_MEAS_CFG):
                    {
                        handle = rscpc_env->rscs.descs[RSCPC_DESC_RSC_MEAS_CL_CFG].desc_hdl;
                    } break;

                    // Read Unread Alert Characteristic Client Char. Cfg. Descriptor Value
                    case (RSCPC_RD_WR_SC_CTNL_PT_CFG):
                    {
                        handle = rscpc_env->rscs.descs[RSCPC_DESC_SC_CTNL_PT_CL_CFG].desc_hdl;
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
                    param->operation       = RSCPC_READ_OP_CODE;

                    // Store the command structure
                    rscpc_env->operation  = (void *)param;
                    msg_status            = KE_MSG_NO_FREE;

                    // Send the read request
                    prf_read_char_send(&(rscpc_env->con_info), rscpc_env->rscs.svc.shdl,
                                       rscpc_env->rscs.svc.ehdl, handle);

                    // Go to the Busy state
                    ke_state_set(dest_id, RSCPC_BUSY);
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
            rscpc_send_cmp_evt(rscpc_env, RSCPC_READ_OP_CODE, status);
        }
    }
    else
    {
        // No connection exists, send the status to the requester
        rscps_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, RSCPC_READ_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_CFG_NTFIND_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int rscpc_cfg_ntfind_cmd_handler(ke_msg_id_t const msgid,
                                        struct rscpc_cfg_ntfind_cmd *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    if (rscpc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;
        // Handle
        uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;

        do
        {
            ASSERT_ERR(ke_state_get(dest_id) != RSCPC_IDLE);

            // Check the provided connection handle
            if (gapc_get_conidx(param->conhdl) != rscpc_env->con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) == RSCPC_BUSY)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is PRF_ERR_OK, no message will be sent to the application
                break;
            }

            // state = RSCPC_CONNECTED
            ASSERT_ERR(rscpc_env->operation == NULL);

            switch(param->desc_code)
            {
                // Write RSC Measurement Characteristic Client Char. Cfg. Descriptor Value
                case (RSCPC_RD_WR_RSC_MEAS_CFG):
                {
                    if (param->ntfind_cfg <= PRF_CLI_START_NTF)
                    {
                        handle = rscpc_env->rscs.descs[RSCPC_DESC_RSC_MEAS_CL_CFG].desc_hdl;

                        // The characteristic is mandatory and the descriptor is mandatory
                        ASSERT_ERR(handle != ATT_INVALID_SEARCH_HANDLE);
                    }
                    else
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    }
                } break;

                // Write SC Control Point Characteristic Client Char. Cfg. Descriptor Value
                case (RSCPC_RD_WR_SC_CTNL_PT_CFG):
                {
                    if ((param->ntfind_cfg == PRF_CLI_STOP_NTFIND) ||
                        (param->ntfind_cfg == PRF_CLI_START_IND))
                    {
                        handle = rscpc_env->rscs.descs[RSCPC_DESC_SC_CTNL_PT_CL_CFG].desc_hdl;

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
            ASSERT_ERR(handle != ATT_INVALID_SEARCH_HANDLE);

            // Set the operation code
            param->operation = RSCPC_CFG_NTF_IND_OP_CODE;

            // Store the command structure
            rscpc_env->operation   = param;
            msg_status = KE_MSG_NO_FREE;

            // Go to the Busy state
            ke_state_set(dest_id, RSCPC_BUSY);

            // Send GATT Write Request
            prf_gatt_write_ntf_ind(&rscpc_env->con_info, handle, param->ntfind_cfg);
        }
        else
        {
            rscpc_send_cmp_evt(rscpc_env, RSCPC_CFG_NTF_IND_OP_CODE, status);
        }
    }
    else
    {
        // No connection
        rscps_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, RSCPC_CFG_NTF_IND_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_CFG_NTFIND_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int rscpc_ctnl_pt_cfg_cmd_handler(ke_msg_id_t const msgid,
                                         struct rscpc_ctnl_pt_cfg_cmd *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    if (rscpc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;

        do
        {
            // State is Connected or Busy
            ASSERT_ERR(ke_state_get(dest_id) > RSCPC_IDLE);

            // Check the provided connection handle
            if (gapc_get_conidx(param->conhdl) != rscpc_env->con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) == RSCPC_BUSY)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is PRF_ERR_OK, no message will be sent to the application
                break;
            }

            // Check if the characteristic has been found
            if (rscpc_env->rscs.descs[RSCPC_DESC_SC_CTNL_PT_CL_CFG].desc_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Packed request
                uint8_t req[RSCP_SC_CNTL_PT_REQ_MAX_LEN];
                // Request Length
                uint8_t req_len = RSCP_SC_CNTL_PT_REQ_MIN_LEN;

                // Set the operation code
                req[0] = param->sc_ctnl_pt.op_code;

                // Fulfill the message according to the operation code
                switch (param->sc_ctnl_pt.op_code)
                {
                    case (RSCP_CTNL_PT_OP_RESERVED):
                    {
                        // Do nothing, used to generate an "Operation Code not supported" error.
                    } break;

                    case (RSCP_CTNL_PT_OP_SET_CUMUL_VAL):
                    {
                        // Set the cumulative value
                        co_write32p(&req[1], param->sc_ctnl_pt.value.cumul_val);
                        // Update length
                        req_len += 4;
                    } break;

                    case (RSCP_CTNL_PT_OP_UPD_LOC):
                    {
                        // Set the sensor location
                        req[1] = param->sc_ctnl_pt.value.sensor_loc;
                        // Update length
                        req_len++;
                    } break;

                    case (RSCP_CTNL_PT_OP_START_CALIB):
                    case (RSCP_CTNL_PT_OP_REQ_SUPP_LOC):
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
                    param->operation = RSCPC_CTNL_PT_CFG_WR_OP_CODE;

                    // Store the command structure
                    rscpc_env->operation   = param;
                    // Store the command information
                    msg_status = KE_MSG_NO_FREE;

                    // Go to the Busy state
                    ke_state_set(dest_id, RSCPC_BUSY);

                    // Send the write request
                    prf_gatt_write(&(rscpc_env->con_info), rscpc_env->rscs.chars[RSCP_RSCS_SC_CTNL_PT_CHAR].val_hdl,
                                   (uint8_t *)&req[0], req_len, GATTC_WRITE);
                }
            }
            else
            {
                // The SC Control Point Characteristic has not been found
                status = PRF_ERR_INEXISTENT_HDL;
            }
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send a complete event status to the application
            rscpc_send_cmp_evt(rscpc_env, RSCPC_CTNL_PT_CFG_WR_OP_CODE, status);
        }
    }
    else
    {
        // No connection
        rscps_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, RSCPC_CTNL_PT_CFG_WR_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_TIMEOUT_TIMER_IND message. This message is
 * received when the peer device doesn't send a SC Control Point indication within 30s
 * after reception of the write response.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int rscpc_timeout_timer_ind_handler(ke_msg_id_t const msgid,
                                           void const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    if (rscpc_env != NULL)
    {
        ASSERT_ERR(rscpc_env->operation != NULL);
        ASSERT_ERR(((struct rscpc_cmd *)rscpc_env->operation)->operation == RSCPC_CTNL_PT_CFG_IND_OP_CODE);

        // Send the complete event message
        rscpc_send_cmp_evt(rscpc_env, RSCPC_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_PROC_TIMEOUT);
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
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    // Check if the environment exists
    if (rscpc_env != NULL)
    {
        ASSERT_ERR(rscpc_env->operation != NULL);
        ASSERT_ERR(((struct rscpc_cmd *)rscpc_env->operation)->operation == RSCPC_ENABLE_OP_CODE);

        // Keep only one instance of the service
        if (rscpc_env->nb_svc == 0)
        {
            // Keep the start handle and the end handle of the service
            rscpc_env->rscs.svc.shdl = param->start_hdl;
            rscpc_env->rscs.svc.ehdl = param->end_hdl;
            rscpc_env->nb_svc++;
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
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    // Check if the environment exists
    if (rscpc_env != NULL)
    {
        ASSERT_ERR(rscpc_env->operation != NULL);
        ASSERT_ERR(((struct rscpc_cmd *)rscpc_env->operation)->operation == RSCPC_ENABLE_OP_CODE);

        // Retrieve RSCS characteristics
        prf_search_chars(rscpc_env->rscs.svc.ehdl, RSCP_RSCS_CHAR_MAX,
                         &rscpc_env->rscs.chars[0], &rscpc_rscs_char[0],
                         param, (uint8_t *)&rscpc_env->last_char_code);
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
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    // Check if the environment exists
    if (rscpc_env != NULL)
    {
        ASSERT_ERR(rscpc_env->operation != NULL);
        ASSERT_ERR(((struct rscpc_cmd *)rscpc_env->operation)->operation == RSCPC_ENABLE_OP_CODE);

        // Retrieve RSCS descriptors
        prf_search_descs(RSCPC_DESC_MAX, &rscpc_env->rscs.descs[0], &rscpc_rscs_char_desc[0],
                         param, rscpc_env->last_req);
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
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    if (rscpc_env != NULL)
    {
        switch (param->req_type)
        {
            case (GATTC_DISC_BY_UUID_SVC):
            case (GATTC_DISC_ALL_CHAR):
            case (GATTC_DISC_DESC_CHAR):
            {
                // Status
                uint8_t status = PRF_ERR_STOP_DISC_CHAR_MISSING;

                ASSERT_ERR(rscpc_env->operation != NULL);
                ASSERT_ERR(((struct rscpc_cmd *)rscpc_env->operation)->operation == RSCPC_ENABLE_OP_CODE);

                if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                    (param->status == ATT_ERR_NO_ERROR))
                {
                    /* -------------------------------------------------
                     * SERVICE DISCOVERY -------------------------------
                     * ------------------------------------------------- */
                    if (rscpc_env->last_req == ATT_SVC_RUNNING_SPEED_CADENCE)
                    {
                        if (rscpc_env->nb_svc > 0)
                        {
                            // Check if service handles are OK
                            if ((rscpc_env->rscs.svc.shdl != ATT_INVALID_HANDLE) &&
                                (rscpc_env->rscs.svc.ehdl != ATT_INVALID_HANDLE) &&
                                (rscpc_env->rscs.svc.shdl < rscpc_env->rscs.svc.ehdl))
                            {
                                // Status is OK
                                status = PRF_ERR_OK;

                                // Discover all RSCS characteristics
                                rscpc_env->last_req = ATT_DECL_CHARACTERISTIC;
                                prf_disc_char_all_send(&(rscpc_env->con_info), &(rscpc_env->rscs.svc));
                            }
                            // Handles are not corrects, the Running Speed and Cadence Service has not been found, stop
                        }
                        // The Running Speed and Cadence Service has not been found, stop discovery
                    }

                    /* -------------------------------------------------
                     * CHARACTERISTICS DISCOVERY -----------------------
                     * ------------------------------------------------- */
                    else if (rscpc_env->last_req == ATT_DECL_CHARACTERISTIC)
                    {
                        // Check if mandatory properties have been found and if properties are correct
                        status = prf_check_svc_char_validity(RSCP_RSCS_CHAR_MAX, rscpc_env->rscs.chars, rscpc_rscs_char);

                        // Check for characteristic properties.
                        if (status == PRF_ERR_OK)
                        {
                            rscpc_env->last_req = RSCP_RSCS_RSC_MEAS_CHAR;

                            // Find the Client Characteristic Configuration Descriptor for the RSC Measurement characteristic
                            prf_disc_char_desc_send(&(rscpc_env->con_info),
                                                    &(rscpc_env->rscs.chars[RSCP_RSCS_RSC_MEAS_CHAR]));
                        }
                    }

                    /* -------------------------------------------------
                     * DESCRIPTORS DISCOVERY ---------------------------
                     * ------------------------------------------------- */
                    else
                    {
                        // Discovery over ?
                        bool disc_over = true;

                        if (rscpc_env->last_req == RSCP_RSCS_RSC_MEAS_CHAR)
                        {
                            // Check if the SC Control Point Characteristic has been found
                            if (rscpc_env->rscs.chars[RSCP_RSCS_SC_CTNL_PT_CHAR].char_hdl != ATT_INVALID_SEARCH_HANDLE)
                            {
                                // Status is OK
                                status = PRF_ERR_OK;

                                // Find the Client Characteristic Configuration Descriptor for the SC Control Point characteristic
                                rscpc_env->last_req = RSCP_RSCS_SC_CTNL_PT_CHAR;
                                prf_disc_char_desc_send(&(rscpc_env->con_info),
                                                        &(rscpc_env->rscs.chars[RSCP_RSCS_SC_CTNL_PT_CHAR]));

                                // Discovery continues
                                disc_over = false;
                            }
                        }
                        else
                        {
                            // Discovery is over
                            ASSERT_ERR(rscpc_env->last_req == RSCP_RSCS_SC_CTNL_PT_CHAR);
                        }

                        if (disc_over)
                        {
                            status = prf_check_svc_char_desc_validity(RSCPC_DESC_MAX,
                                                                      rscpc_env->rscs.descs,
                                                                      rscpc_rscs_char_desc,
                                                                      rscpc_env->rscs.chars);

                            if (status == PRF_ERR_OK)
                            {
                                // Reset number of services
                                rscpc_env->nb_svc = 0;

                                // Register in GATT for notifications/indications
                                prf_register_atthdl2gatt(&(rscpc_env->con_info), &(rscpc_env->rscs.svc));

                                // Send the content of the service to the HL
                                struct rscpc_rscs_content_ind *ind = KE_MSG_ALLOC(RSCPC_RSCS_CONTENT_IND,
                                                                                  rscpc_env->con_info.appid,
                                                                                  rscpc_env->con_info.prf_id,
                                                                                  rscpc_rscs_content_ind);

                                ind->conhdl = gapc_get_conhdl(rscpc_env->con_info.conidx);
                                memcpy(&ind->rscs, &rscpc_env->rscs, sizeof(struct rscpc_rscs_content));

                                // Send the message
                                ke_msg_send(ind);

                                // Stop discovery procedure.
                                rscpc_send_cmp_evt(rscpc_env, RSCPC_ENABLE_OP_CODE, PRF_ERR_OK);
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
                    rscpc_send_cmp_evt(rscpc_env, RSCPC_ENABLE_OP_CODE, status);
                }
            } break;

            case (GATTC_READ):
            {
                // Send the complete event status
                rscpc_send_cmp_evt(rscpc_env, RSCPC_READ_OP_CODE, param->status);
            } break;

            case (GATTC_WRITE):
            {
                uint8_t operation = ((struct rscpc_cmd *)rscpc_env->operation)->operation;

                if (operation == RSCPC_CFG_NTF_IND_OP_CODE)
                {
                    // Send the complete event status
                    rscpc_send_cmp_evt(rscpc_env, operation, param->status);
                }
                else if (operation == RSCPC_CTNL_PT_CFG_WR_OP_CODE)
                {
                    if (param->status == PRF_ERR_OK)
                    {
                        // Start Timeout Procedure
                        ke_timer_set(RSCPC_TIMEOUT_TIMER_IND, dest_id, ATT_TRANS_RTX);

                        // Wait for the response indication
                        ((struct rscpc_cmd *)rscpc_env->operation)->operation = RSCPC_CTNL_PT_CFG_IND_OP_CODE;
                    }
                    else
                    {
                        // Send the complete event status
                        rscpc_send_cmp_evt(rscpc_env, operation, param->status);
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
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    if (rscpc_env != NULL)
    {
        ASSERT_ERR(rscpc_env->operation != NULL);
        ASSERT_ERR(((struct rscpc_cmd *)rscpc_env->operation)->operation == RSCPC_READ_OP_CODE);

        // Send the read value to the HL
        struct rscpc_value_ind *ind = KE_MSG_ALLOC(RSCPC_VALUE_IND,
                                                   rscpc_env->con_info.appid,
                                                   rscpc_env->con_info.prf_id,
                                                   rscpc_value_ind);

        ind->conhdl   = gapc_get_conhdl(rscpc_env->con_info.conidx);

        switch (((struct rscpc_read_cmd *)rscpc_env->operation)->read_code)
        {
            // Read RSC Feature Characteristic value
            case (RSCPC_RD_RSC_FEAT):
            {
                co_write16p(&ind->value.sensor_feat, param->value[0]);

                // Mask the unused bits
                ind->value.sensor_feat &= RSCP_FEAT_ALL_SUPP;
            } break;

            // Read Sensor Location Characteristic value
            case (RSCPC_RD_SENSOR_LOC):
            {
                ind->value.sensor_loc = (param->value[0]< RSCP_LOC_MAX) ? param->value[0] : RSCP_LOC_OTHER;
            } break;

            // Read Client Characteristic Configuration Descriptor value
            case (RSCPC_RD_WR_RSC_MEAS_CFG):
            case (RSCPC_RD_WR_SC_CTNL_PT_CFG):
            {
                co_write16p(&ind->value.ntf_cfg, param->value[0]);
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        ind->att_code = ((struct rscpc_read_cmd *)rscpc_env->operation)->read_code;

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
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    if (rscpc_env != NULL)
    {
        switch (param->type)
        {
            case (GATTC_NOTIFY):
            {
                // Offset
                uint8_t offset = RSCP_RSC_MEAS_MIN_LEN;

                // RSC Measurement value has been received
                struct rscpc_value_ind *ind = KE_MSG_ALLOC(RSCPC_VALUE_IND,
                                                           rscpc_env->con_info.appid,
                                                           rscpc_env->con_info.prf_id,
                                                           rscpc_value_ind);

                // Connection Handle
                ind->conhdl   = gapc_get_conhdl(rscpc_env->con_info.conidx);
                // Attribute code
                ind->att_code = RSCPC_NTF_RSC_MEAS;

                /*----------------------------------------------------
                 * Unpack Measurement --------------------------------
                 *----------------------------------------------------*/

                // Flags - Flag the unused bits
                ind->value.rsc_meas.flags      = (param->value[0] & RSCP_MEAS_ALL_PRESENT);
                // Instantaneous Speed
                ind->value.rsc_meas.inst_speed = co_read16p((uint16_t *)&param->value[1]);
                // Instantaneous Cadence
                ind->value.rsc_meas.inst_cad   = param->value[3];

                // Instantaneous Stride Length
                if ((param->value[0] & RSCP_MEAS_INST_STRIDE_LEN_PRESENT) == RSCP_MEAS_INST_STRIDE_LEN_PRESENT)
                {
                    co_write16p(&ind->value.rsc_meas.inst_stride_len, (uint16_t)param->value[offset]);
                    offset += 2;
                }

                // Total Distance
                if ((param->value[0] & RSCP_MEAS_TOTAL_DST_MEAS_PRESENT) == RSCP_MEAS_TOTAL_DST_MEAS_PRESENT)
                {
                    ind->value.rsc_meas.total_dist = co_read32p(&param->value[offset]);
                }

                ke_msg_send(ind);
            } break;

            case (GATTC_INDICATE):
            {
                // Check if we were waiting for the indication
                if (rscpc_env->operation != NULL)
                {
                    if (((struct rscpc_cmd *)rscpc_env->operation)->operation == RSCPC_CTNL_PT_CFG_IND_OP_CODE)
                    {
                        // Stop the procedure timeout timer
                        ke_timer_clear(RSCPC_TIMEOUT_TIMER_IND, dest_id);

                        // RSC Measurement value has been received
                        struct rscpc_value_ind *ind = KE_MSG_ALLOC(RSCPC_VALUE_IND,
                                                                   rscpc_env->con_info.appid,
                                                                   rscpc_env->con_info.prf_id,
                                                                   rscpc_value_ind);

                        // Connection Handle
                        ind->conhdl   = gapc_get_conhdl(rscpc_env->con_info.conidx);
                        // Attribute code
                        ind->att_code = RSCPC_IND_SC_CTNL_PT;

                        // Requested operation code
                        ind->value.ctnl_pt_rsp.req_op_code = param->value[1];
                        // Response value
                        ind->value.ctnl_pt_rsp.resp_value  = param->value[2];

                        // Get the list of supported sensor locations if needed
                        if ((ind->value.ctnl_pt_rsp.req_op_code == RSCP_CTNL_PT_OP_REQ_SUPP_LOC) &&
                            (ind->value.ctnl_pt_rsp.resp_value == RSCP_CTNL_PT_RESP_SUCCESS) &&
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
                                if (loc < RSCP_LOC_MAX)
                                {
                                    ind->value.ctnl_pt_rsp.supp_loc |= (1 << loc);
                                }
                            }
                        }

                        // Send the message
                        ke_msg_send(ind);

                        // Send the complete event message
                        rscpc_send_cmp_evt(rscpc_env, RSCPC_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_OK);
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
 * @brief Disconnection indication to RSCPC.
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
    struct rscpc_env_tag *rscpc_env = PRF_CLIENT_GET_ENV(dest_id, rscpc);

    ASSERT_ERR(rscpc_env != NULL);

    // Free the stored operation if needed
    if (rscpc_env->operation != NULL)
    {
        // Check if we were waiting for a SC Control Point indication
        if (((struct rscpc_cmd *)rscpc_env->operation)->operation == RSCPC_CTNL_PT_CFG_IND_OP_CODE)
        {
            // Stop the procedure timeout timer
            ke_timer_clear(RSCPC_TIMEOUT_TIMER_IND, dest_id);
        }

        ke_msg_free(ke_param2msg(rscpc_env->operation));
        rscpc_env->operation = NULL;
    }

    PRF_CLIENT_DISABLE_IND_SEND(rscpc_envs, dest_id, RSCPC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler rscpc_default_state[] =
{
    {RSCPC_ENABLE_CMD,               (ke_msg_func_t)rscpc_enable_cmd_handler},
    {RSCPC_READ_CMD,                 (ke_msg_func_t)rscpc_read_cmd_handler},
    {RSCPC_CFG_NTFIND_CMD,           (ke_msg_func_t)rscpc_cfg_ntfind_cmd_handler},
    {RSCPC_CTNL_PT_CFG_CMD,          (ke_msg_func_t)rscpc_ctnl_pt_cfg_cmd_handler},

    {RSCPC_TIMEOUT_TIMER_IND,        (ke_msg_func_t)rscpc_timeout_timer_ind_handler},

    {GATTC_DISC_SVC_IND,             (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,            (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,       (ke_msg_func_t)gattc_disc_char_desc_ind_handler},

    {GATTC_READ_IND,                 (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,                (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                  (ke_msg_func_t)gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler rscpc_state_handler[RSCPC_STATE_MAX] =
{
    [RSCPC_IDLE]         = KE_STATE_HANDLER_NONE,
    [RSCPC_CONNECTED]    = KE_STATE_HANDLER_NONE,
    [RSCPC_BUSY]         = KE_STATE_HANDLER_NONE,
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler rscpc_default_handler = KE_STATE_HANDLER(rscpc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t rscpc_state[RSCPC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_RSC_COLLECTOR)

/// @} RSCPCTASK
