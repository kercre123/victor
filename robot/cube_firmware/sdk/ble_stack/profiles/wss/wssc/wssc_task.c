/**
 ****************************************************************************************
 *
 * @file wssc_task.c
 *
 * @brief Weight Scale Service Collector Task implementation.
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
 * @addtogroup WSSCTASK
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
#include "prf_types.h"
#include "prf_utils.h"

const struct prf_char_def wssc_wss_char[WSSC_CHAR_MAX] =
{
    // Weight Scale Feature
    [WSSC_CHAR_WSS_WS_FEAT]        = {ATT_CHAR_WS_FEAT,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_RD},
    // Weight Measurement
    [WSSC_CHAR_WSS_WEIGHT_MEAS]    = {ATT_CHAR_WS_MEAS,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_IND},
};

const struct prf_char_desc_def wssc_wss_char_desc[WSSC_DESC_MAX] = 
{
    [WSSC_DESC_WSS_WM_CLI_CFG]     = {ATT_DESC_CLIENT_CHAR_CFG,
                                      ATT_MANDATORY,
                                      WSSC_CHAR_WSS_WEIGHT_MEAS},
};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref WSSC_ENABLE_REQ message.
 * The handler enables the Weight Scale Service Collector Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wssc_enable_req_handler(ke_msg_id_t const msgid,
                                    struct wssc_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{

    // Status
    uint8_t status;
    // WSS collector role tesk enviroment
    struct wssc_env_tag *wssc_env;
    // Connection info
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status = PRF_CLIENT_ENABLE(con_info, param, wssc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if(status == PRF_ERR_OK)
    {
        wssc_env = PRF_CLIENT_GET_ENV(dest_id,wssc);

        // Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            wssc_env->last_uuid_req = ATT_SVC_WEIGHT_SCALE;

            // Start discovering
            prf_disc_svc_send(&wssc_env->con_info,ATT_SVC_WEIGHT_SCALE);

            ke_state_set(dest_id, WSSC_DISCOVERING);
        }
        // Normal connection, get saved att detailes
        else
        {
            wssc_env->wss = param->wss;

            wssc_enable_cfm_send(wssc_env,&wssc_env->con_info,PRF_ERR_OK);
        }
    }
    else
    {
        wssc_enable_cfm_send(NULL, &con_info, status);
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

    struct wssc_env_tag *wssc_env = PRF_CLIENT_GET_ENV(dest_id, wssc);

    wssc_env->wss.svc.shdl = param->start_hdl;
    wssc_env->wss.svc.ehdl = param->end_hdl;
    wssc_env->nb_svc++;

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

    struct wssc_env_tag *wssc_env = PRF_CLIENT_GET_ENV(dest_id, wssc);

    // Retrieve Weight Scale Service Characteristics
    prf_search_chars(wssc_env->wss.svc.ehdl,
                     WSSC_CHAR_MAX,
                     &wssc_env->wss.chars[0],
                     &wssc_wss_char[0],
                     param,
                     &wssc_env->last_char_code);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_DESC_IND message.
 * This event can be received 2-4 times, depending if measurement interval has seevral properties.
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

    struct wssc_env_tag *wssc_env = PRF_CLIENT_GET_ENV(dest_id, wssc);

    prf_search_descs(WSSC_DESC_MAX,
                     &wssc_env->wss.desc[0],
                     &wssc_wss_char_desc[0],
                     param,
                     wssc_env->last_char_code);

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
    uint8_t status;

    struct wssc_env_tag *wssc_env = PRF_CLIENT_GET_ENV(dest_id, wssc);

    if(state == WSSC_DISCOVERING)
    {
        if((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
           (param->status == ATT_ERR_NO_ERROR))
        {
            if(wssc_env->last_uuid_req == ATT_SVC_WEIGHT_SCALE)
            {
                // Check if service handles are not ok
                if(wssc_env->wss.svc.shdl == ATT_INVALID_HANDLE)
                { // Stop discovery
                    wssc_enable_cfm_send(wssc_env, &wssc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                // Too many services found only one such service should exist
                else if(wssc_env->nb_svc > 1)
                {// Stop discovery
                    wssc_enable_cfm_send(wssc_env,&wssc_env->con_info,PRF_ERR_MULTIPLE_SVC);
                }
                else
                {
                    // Discover all WSS characteristics
                    prf_disc_char_all_send(&(wssc_env->con_info), &(wssc_env->wss.svc));
                    wssc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                }
            }
            else if(wssc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
            {
                status = prf_check_svc_char_validity(WSSC_CHAR_MAX,
                                                     wssc_env->wss.chars,
                                                     wssc_wss_char);

                if(status == PRF_ERR_OK)
                {
                    wssc_env->last_uuid_req = ATT_INVALID_HANDLE;
                    wssc_env->last_char_code = wssc_wss_char_desc[WSSC_DESC_WSS_WM_CLI_CFG].char_code;

                  // Discover Weight Measurement Char. Descriptor
                    prf_disc_char_desc_send(&(wssc_env->con_info),
                                          &(wssc_env->wss.chars[wssc_env->last_char_code]));
                }
                else
                { // Stop discovery
                    wssc_enable_cfm_send(wssc_env,&wssc_env->con_info, status);
                }
            }
            else // Descriptors
            {
                status = prf_check_svc_char_desc_validity(WSSC_DESC_MAX,
                                                          wssc_env->wss.desc,
                                                          wssc_wss_char_desc,
                                                          wssc_env->wss.chars);

                wssc_enable_cfm_send(wssc_env,&wssc_env->con_info,status);
            }
        }
    }
    else if(state == WSSC_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                struct wssc_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(WSSC_WR_CHAR_RSP,
                                                               wssc_env->con_info.appid,
                                                               dest_id,
                                                               wssc_wr_char_rsp);

                wr_cfm->conhdl = gapc_get_conhdl(wssc_env->con_info.conidx);
                wr_cfm->status = param->status;

                ke_msg_send(wr_cfm);
            }
            break;

            case GATTC_READ:
            {
                if(param->status != GATT_ERR_NO_ERROR)
                {
                    prf_client_att_info_rsp((prf_env_struct*) wssc_env,
                                            WSSC_RD_CHAR_RSP,
                                            param->status,
                                            NULL);
                }
            }
            break;
            default:
            break;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref WSSC_RD_CHAR_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wssc_rd_char_req_handler(ke_msg_id_t const msgid,
                                    struct wssc_rd_char_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{

    // Attribute handle
    uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;
    // Get the address of the environment
    struct wssc_env_tag *wssc_env = PRF_CLIENT_GET_ENV(dest_id, wssc);

    if(param->conhdl == gapc_get_conhdl(wssc_env->con_info.conidx))
    {
        // WS Feat characteristicgi
        if(param->char_code == WSSC_RD_WSS_WS_FEAT)
        {
            search_hdl = wssc_env->wss.chars[param->char_code].val_hdl;
        }
        else if(param->char_code == WSSC_RD_WSS_WM_CLI_CFG)
        {
            search_hdl = wssc_env->wss.desc[param->char_code & ~WSSC_DESC_MASK].desc_hdl;
        }

        if(search_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            wssc_env->last_char_code = param->char_code;
            prf_read_char_send(&(wssc_env->con_info), wssc_env->wss.svc.shdl,
                               wssc_env->wss.svc.ehdl, search_hdl);
        }
        else
        {
            prf_client_att_info_rsp((prf_env_struct*) wssc_env, WSSC_RD_CHAR_RSP,
                    PRF_ERR_INEXISTENT_HDL, NULL);
        }
    }
    else
    {
        prf_client_att_info_rsp((prf_env_struct*) wssc_env, WSSC_RD_CHAR_RSP,
                PRF_ERR_INVALID_PARAM, NULL);
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
    struct wssc_env_tag *wssc_env = PRF_CLIENT_GET_ENV(dest_id, wssc);

    prf_client_att_info_rsp((prf_env_struct*) wssc_env, WSSC_RD_CHAR_RSP,
            GATT_ERR_NO_ERROR, param);

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref WSSC_CFG_INDNTF_REQ message.
 * It allows configuration of the peer ind/ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wssc_cfg_indntf_req_handler(ke_msg_id_t const msgid,
                                       struct wssc_cfg_indntf_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{

    uint16_t cfg_hdl = 0x0000;
    // Get the address of the environment
    struct wssc_env_tag *wssc_env = PRF_CLIENT_GET_ENV(dest_id, wssc);

    if(param->conhdl == gapc_get_conhdl(wssc_env->con_info.conidx))
    {
        // Get handle of the configuration characteristic to set and check if value matches property
        cfg_hdl = wssc_env->wss.desc[WSSC_DESC_WSS_WM_CLI_CFG].desc_hdl;
        if(!((param->cfg_val == PRF_CLI_STOP_NTFIND) ||
             (param->cfg_val == PRF_CLI_START_IND)))
        {
            wssc_error_ind_send(wssc_env, PRF_ERR_INVALID_PARAM);
        }
        else
        {
            if(cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Send GATT write req
                prf_gatt_write_ntf_ind(&wssc_env->con_info,cfg_hdl, param->cfg_val);
            }
            else
            {
                wssc_error_ind_send(wssc_env, PRF_ERR_INEXISTENT_HDL);
            }
        }
    }
    else
    {
        wssc_error_ind_send(wssc_env, PRF_ERR_INVALID_PARAM);
    }

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

    struct wssc_env_tag *wssc_env = PRF_CLIENT_GET_ENV(dest_id,wssc);

    if(param->handle == wssc_env->wss.chars[WSSC_CHAR_WSS_WEIGHT_MEAS].val_hdl)
    {
        wssc_unpack_meas_value(wssc_env, (uint8_t*)&(param->value), param->length);
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Disconnection indication to WSSC.
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
    PRF_CLIENT_DISABLE_IND_SEND(wssc_envs, dest_id, WSSC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


// Specifies state message handlers for connected state
const struct ke_msg_handler wssc_connected[] =
{
    {WSSC_RD_CHAR_REQ,           (ke_msg_func_t)wssc_rd_char_req_handler},
    {GATTC_READ_IND,             (ke_msg_func_t)gattc_read_ind_handler},
    {WSSC_CFG_INDNTF_REQ,        (ke_msg_func_t)wssc_cfg_indntf_req_handler},
    {GATTC_EVENT_IND,            (ke_msg_func_t)gattc_event_ind_handler}
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler wssc_discovering[] =
{
    {GATTC_DISC_SVC_IND,         (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,        (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,   (ke_msg_func_t)gattc_disc_char_desc_ind_handler},
};


/// Specified state message handlers for discovering state
const struct ke_msg_handler wssc_default_state[] =
{
    {WSSC_ENABLE_REQ,             (ke_msg_func_t)wssc_enable_req_handler},
    {GAPC_DISCONNECT_IND,         (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,               (ke_msg_func_t)gattc_cmp_evt_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler wssc_state_handler[WSSC_STATE_MAX] =
{
    [WSSC_IDLE]        = KE_STATE_HANDLER_NONE,
    [WSSC_CONNECTED]   = KE_STATE_HANDLER(wssc_connected),
    [WSSC_DISCOVERING] = KE_STATE_HANDLER(wssc_discovering),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler wssc_default_handler = KE_STATE_HANDLER(wssc_default_state);


/// Defines the place holder for the states of all the task instances.
ke_state_t wssc_state[WSSC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY


#endif //BLE_WSS_COLLECTOR
/// @} WSSSTASK
