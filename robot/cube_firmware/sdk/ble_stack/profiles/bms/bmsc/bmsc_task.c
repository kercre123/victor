/**
 ****************************************************************************************
 *
 * @file bmsc_task.c
 *
 * @brief C file - Bond Management Service Client Role Task Implementation.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BMSCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BM_CLIENT)
#include "gap.h"
#include "bmsc.h"
#include "bmsc_task.h"
#include "gattc_task.h"
#include "prf_utils.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/// State machine used to retrieve Bond Management service characteristics information
const struct prf_char_def bmsc_bms_char[BMSC_CHAR_MAX] =
{
    /// Control point
    [BMSC_CHAR_CP]              = {ATT_CHAR_BM_CTRL_PT,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_WR},       // TODO: add also AUTH

    /// Feature
    [BMSC_CHAR_FEATURE]         = {ATT_CHAR_BM_FEAT,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_RD},       // TODO: add also AUTH
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BMSC_ENABLE_REQ message.
 * The handler enables the Bond Management Profile Client Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bmsc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct bmsc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Bond Management Profile Client Role Task Environment
    struct bmsc_env_tag *bmsc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, bmsc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        bmsc_env = PRF_CLIENT_GET_ENV(dest_id, bmsc);

        // Config connection, start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering BMS on peer
            prf_disc_svc_send(&(bmsc_env->con_info), ATT_SVC_BOND_MANAGEMENT);

            bmsc_env->last_uuid_req = ATT_SVC_BOND_MANAGEMENT;

            // Go to DISCOVERING state
            ke_state_set(dest_id, BMSC_DISCOVERING);
        }
        // normal connection, get saved att details
        else
        {
            bmsc_env->bms = param->bms;

            // send APP confirmation that can start normal connection to TH
            bmsc_enable_cfm_send(bmsc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        bmsc_enable_cfm_send(NULL, &con_info, status);
    }

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
    struct bmsc_env_tag *bmsc_env = PRF_CLIENT_GET_ENV(dest_id, bmsc);

    // even if we get multiple responses we only store 1 range
    if (bmsc_env->last_uuid_req == ATT_SVC_BOND_MANAGEMENT)
    {
        bmsc_env->bms.svc.shdl = param->start_hdl;
        bmsc_env->bms.svc.ehdl = param->end_hdl;
        bmsc_env->nb_svc++;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_IND message.
 * Characteristics for the currently desired service handle range are obtained and kept.
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
    struct bmsc_env_tag *bmsc_env = PRF_CLIENT_GET_ENV(dest_id, bmsc);

    // Retrieve BMS characteristics
    prf_search_chars(bmsc_env->bms.svc.ehdl, BMSC_CHAR_MAX,
                     &bmsc_env->bms.chars[0], &bmsc_bms_char[0],
                     param, &bmsc_env->last_char_code);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * This generic event is received for different requests, so need to keep track.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gattc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    // Get the address of the environment
    struct bmsc_env_tag *bmsc_env = PRF_CLIENT_GET_ENV(dest_id, bmsc);
    uint8_t status;

    if (state == BMSC_DISCOVERING)
    {
        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
                (param->status == ATT_ERR_NO_ERROR))
        {
            // service start/end handles has been received
            if (bmsc_env->last_uuid_req == ATT_SVC_BOND_MANAGEMENT)
            {
                // check if service handles are not ok
                if (bmsc_env->bms.svc.shdl == ATT_INVALID_HANDLE)
                {
                    // stop discovery procedure.
                    bmsc_enable_cfm_send(bmsc_env, &bmsc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                // too many services found only one such service should exist
                else if (bmsc_env->nb_svc != 1)
                {
                    // stop discovery procedure.
                    bmsc_enable_cfm_send(bmsc_env, &bmsc_env->con_info, PRF_ERR_MULTIPLE_SVC);
                }
                // check if service handles are ok
                else
                {
                    // discover all BMS characteristics
                    bmsc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    prf_disc_char_all_send(&(bmsc_env->con_info), &(bmsc_env->bms.svc));
                }
            }
            else // Characteristics
            {
                status = prf_check_svc_char_validity(BMSC_CHAR_MAX, bmsc_env->bms.chars,
                        bmsc_bms_char);

                bmsc_enable_cfm_send(bmsc_env, &bmsc_env->con_info, status);
            }
        }
        else
        {
            bmsc_enable_cfm_send(bmsc_env, &bmsc_env->con_info, param->status);
        }
    }
    else if (state == BMSC_CONNECTED)
    {
        switch (param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                struct bmsc_wr_ctrl_point_rsp *wr_cfm = KE_MSG_ALLOC(BMSC_WR_CTRL_POINT_RSP,
                                                                     bmsc_env->con_info.appid,
                                                                     dest_id,
                                                                     bmsc_wr_ctrl_point_rsp);

                wr_cfm->conhdl = gapc_get_conhdl(bmsc_env->con_info.conidx);
                // it will be a GATT status code
                wr_cfm->status = param->status;
                // send the message
                ke_msg_send(wr_cfm);
            } break;

            case GATTC_READ:
            {
                if(param->status != GATT_ERR_NO_ERROR)
                {
                    // an error occurs while reading peer device attribute
                    struct bmsc_rd_features_rsp *rsp = KE_MSG_ALLOC(BMSC_RD_FEATURES_RSP,
                                                        bmsc_env->con_info.appid,
                                                        dest_id,
                                                        bmsc_rd_features_rsp);

                    rsp->conhdl = gapc_get_conhdl(bmsc_env->con_info.conidx);
                    rsp->status = param->status;
                    rsp->length = 0;

                    ke_msg_send(rsp);
                }
            } break;

            default: break;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BMSC_RD_FEATURES_REQ message.
 * Send by application task, it's used to retrieve Bond Management Service features.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bmsc_rd_features_req_handler(ke_msg_id_t const msgid,
                                          struct bmsc_rd_features_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct bmsc_env_tag *bmsc_env = PRF_CLIENT_GET_ENV(dest_id, bmsc);

    uint8_t status = bmsc_validate_request(bmsc_env, param->conhdl, BMSC_CHAR_FEATURE);

    // request can be performed
    if (status == PRF_ERR_OK)
    {
        // read Bond Management server feature
        prf_read_char_send(&(bmsc_env->con_info), bmsc_env->bms.svc.shdl,
                            bmsc_env->bms.svc.ehdl,
                            bmsc_env->bms.chars[BMSC_CHAR_FEATURE].val_hdl);
    }
    // send command response with error code
    else
    {
        struct bmsc_rd_features_rsp *rsp = KE_MSG_ALLOC(BMSC_RD_FEATURES_RSP,
                                                        bmsc_env->con_info.appid,
                                                        dest_id,
                                                        bmsc_rd_features_rsp);
        // set error status
        rsp->conhdl = param->conhdl;
        rsp->status = status;

        ke_msg_send(rsp);
    }

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
    struct bmsc_env_tag *bmsc_env = PRF_CLIENT_GET_ENV(dest_id, bmsc);

    struct bmsc_rd_features_rsp *rsp = KE_MSG_ALLOC_DYN(BMSC_RD_FEATURES_RSP,
                                                        bmsc_env->con_info.appid,
                                                        dest_id,
                                                        bmsc_rd_features_rsp,
                                                        param->length);

    // set connection handle
    rsp->conhdl = gapc_get_conhdl(bmsc_env->con_info.conidx);
    // set error status
    rsp->status = ATT_ERR_NO_ERROR;
    // value length
    rsp->length = param->length;
    // extract features
    memcpy(&rsp->features[0], &(param->value[0]), param->length);

    ke_msg_send(rsp);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BMSC_WR_CTRL_POINT_REQ message.
 * When receiving this request, Bond Management client sends an operation code to
 * Bond Management server.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bmsc_wr_ctrl_point_req_handler(ke_msg_id_t const msgid,
                                          struct bmsc_wr_ctrl_point_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct bmsc_env_tag *bmsc_env = PRF_CLIENT_GET_ENV(dest_id, bmsc);
    uint8_t status = bmsc_validate_request(bmsc_env, param->conhdl, BMSC_CHAR_CP);

    if (status != PRF_ERR_OK)
    {
        bmsc_error_ind_send(bmsc_env, status);
        return (KE_MSG_CONSUMED);
    }
    else
    {
        struct gattc_write_cmd *wr_char = KE_MSG_ALLOC_DYN(GATTC_WRITE_CMD,
                                                    KE_BUILD_ID(TASK_GATTC,
                                                    bmsc_env->con_info.conidx),
                                                    bmsc_env->con_info.prf_id,
                                                    gattc_write_cmd,
                                                    param->operation.operand_length + 1);

        // Offset
        wr_char->offset         = 0x0000;
        // Cursor always 0
        wr_char->cursor         = 0x0000;
        // Write Type
        wr_char->req_type       = GATTC_WRITE;
        // Characteristic Value attribute handle
        wr_char->handle         = bmsc_env->bms.chars[BMSC_CHAR_CP].val_hdl;
        // Value Length
        wr_char->length         = param->operation.operand_length + 1;
        // Auto Execute
        wr_char->auto_execute   = true;

        // Operation Code
        wr_char->value[0] = param->operation.op_code;
        // Operand
        memcpy(&wr_char->value[1], &(param->operation.operand),
                param->operation.operand_length);

        // Send the message
        ke_msg_send(wr_char);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GAPC_DISCONNECT_IND message. Disconnection
 * indication to BMSC.
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
    PRF_CLIENT_DISABLE_IND_SEND(bmsc_envs, dest_id, BMSC, param->conhdl);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler bmsc_connected[] =
{
    {BMSC_WR_CTRL_POINT_REQ,        (ke_msg_func_t) bmsc_wr_ctrl_point_req_handler},
    {BMSC_RD_FEATURES_REQ,          (ke_msg_func_t) bmsc_rd_features_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t) gattc_read_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler bmsc_discovering[] =
{
    {GATTC_DISC_SVC_IND,            (ke_msg_func_t) gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,           (ke_msg_func_t) gattc_disc_char_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler bmsc_default_state[] =
{
    {BMSC_ENABLE_REQ,               (ke_msg_func_t) bmsc_enable_req_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},
    {GAPC_DISCONNECT_IND,           (ke_msg_func_t) gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler bmsc_state_handler[BMSC_STATE_MAX] =
{
    [BMSC_IDLE]        = KE_STATE_HANDLER_NONE,
    [BMSC_CONNECTED]   = KE_STATE_HANDLER(bmsc_connected),
    [BMSC_DISCOVERING] = KE_STATE_HANDLER(bmsc_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler bmsc_default_handler = KE_STATE_HANDLER(bmsc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t bmsc_state[BMSC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif // (BLE_BM_CLIENT)

/// @} BMSCTASK
