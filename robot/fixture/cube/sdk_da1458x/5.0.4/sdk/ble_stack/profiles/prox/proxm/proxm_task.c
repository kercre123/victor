/**
 ****************************************************************************************
 *
 * @file proxm_task.c
 *
 * @brief Proximity Monitor Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup PROXMTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_PROX_MONITOR)

#include "gap.h"
#include "gapc.h"
#include "proxm_task.h"
#include "proxm.h"
#include "gattc_task.h"
#include "prf_types.h"
#include "prf_utils.h"

/// State machine used to retrieve Link Loss service characteristics information
const struct prf_char_def proxm_lls_char     = {ATT_CHAR_ALERT_LEVEL,
                                               ATT_MANDATORY,
                                               ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR};

/// State machine used to retrieve Immediate Alert service characteristics information
const struct prf_char_def proxm_ias_char     = {ATT_CHAR_ALERT_LEVEL,
                                               ATT_MANDATORY,
                                               ATT_CHAR_PROP_WR_NO_RESP};

/// State machine used to retrieve TX Power Level service characteristics information
const struct prf_char_def proxm_txps_char     = {ATT_CHAR_TX_POWER_LEVEL,
                                               ATT_MANDATORY,
                                               ATT_CHAR_PROP_RD};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Enable the Proximity Monitor role, used after connection.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxm_enable_req_handler(ke_msg_id_t const msgid,
                                    struct proxm_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Device Information Service Client Role Task Environment
    struct proxm_env_tag *proxm_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, proxm);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        proxm_env = PRF_CLIENT_GET_ENV(dest_id, proxm);

        // Discovery connection
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Start discovering LLS on peer
            prf_disc_svc_send(&proxm_env->con_info, ATT_SVC_LINK_LOSS);

            proxm_env->last_uuid_req     = ATT_SVC_LINK_LOSS;
            proxm_env->last_svc_req      = ATT_SVC_LINK_LOSS;

            // Set state to discovering
            ke_state_set(dest_id, PROXM_DISCOVERING);
        }
        else
        {
            //copy over data that has been stored
            proxm_env->ias     = param->ias;
            proxm_env->lls     = param->lls;
            proxm_env->txps    = param->txps;

            //send confirmation of enable request to application
            proxm_enable_cfm_send(proxm_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        proxm_enable_cfm_send(NULL, &con_info, status);
    }

    // message is consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PROXM_RD_ALERT_LVL_REQ message.
 * Request to read the LLS alert level.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxm_rd_alert_lvl_req_handler(ke_msg_id_t const msgid,
                                          struct proxm_rd_alert_lvl_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct proxm_env_tag *proxm_env = PRF_CLIENT_GET_ENV(dest_id, proxm);

    // Save the read characteristic code
    proxm_env->last_char_code = PROXM_RD_LL_ALERT_LVL;

    if(param->conhdl == gapc_get_conhdl(proxm_env->con_info.conidx))
    {
        if(proxm_env->lls.charact.val_hdl != ATT_INVALID_HANDLE)
        {
            // Send Read Char. request
            prf_read_char_send(&proxm_env->con_info,
                               proxm_env->lls.svc.shdl, proxm_env->lls.svc.ehdl,
                               proxm_env->lls.charact.val_hdl);
        }
        else
        {
            prf_client_att_info_rsp((prf_env_struct*) proxm_env, PROXM_RD_CHAR_RSP,
                    PRF_ERR_INEXISTENT_HDL, NULL);
        }
    }
    else
    {
        // Wrong connection handle
        prf_client_att_info_rsp((prf_env_struct*) proxm_env, PROXM_RD_CHAR_RSP,
                PRF_ERR_INVALID_PARAM, NULL);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PROXM_RD_TXPW_LVL_REQ message.
 * Will send request to read this characteristic value on peer ATT DB.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxm_rd_txpw_lvl_req_handler(ke_msg_id_t const msgid,
                                         struct proxm_rd_txpw_lvl_req const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct proxm_env_tag *proxm_env = PRF_CLIENT_GET_ENV(dest_id, proxm);

    // Save the read characteristic code
    proxm_env->last_char_code = PROXM_RD_TX_POWER_LVL;

    if(param->conhdl == gapc_get_conhdl(proxm_env->con_info.conidx))
    {
        if(proxm_env->txps.charact.val_hdl != ATT_INVALID_HANDLE)
        {
            // Send Read Char. request
            prf_read_char_send(&proxm_env->con_info,
                               proxm_env->txps.svc.shdl, proxm_env->txps.svc.ehdl,
                               proxm_env->txps.charact.val_hdl);
        }
        else
        {
            // TX Power Level Char. has not been found
            prf_client_att_info_rsp((prf_env_struct*) proxm_env, PROXM_RD_CHAR_RSP,
                    PRF_ERR_INEXISTENT_HDL, NULL);
        }
    }
    else
    {
        // Wrong connection handle
        prf_client_att_info_rsp((prf_env_struct*) proxm_env, PROXM_RD_CHAR_RSP,
                PRF_ERR_INVALID_PARAM, NULL);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PROXM_SET_ALERT_LVL_REQ message.
 * Request to write either the LLS or IAS alert levels.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxm_wr_alert_lvl_req_handler(ke_msg_id_t const msgid,
                                          struct proxm_wr_alert_lvl_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    uint8_t  req_type = 0x00;
    uint16_t val_hdl  = 0x0000;
    // Get the address of the environment
    struct proxm_env_tag *proxm_env = PRF_CLIENT_GET_ENV(dest_id, proxm);

    if(param->conhdl == gapc_get_conhdl(proxm_env->con_info.conidx))
    {
        // Alert level only has 3 values, other not useful
        if(param->lvl <= PROXM_ALERT_HIGH)
        {
            // Get the request type and handle value for this type of write
            switch(param->svc_code)
            {
                case PROXM_SET_LK_LOSS_ALERT:
                    req_type = GATTC_WRITE;
                    val_hdl  = proxm_env->lls.charact.val_hdl;
                    break;
                case PROXM_SET_IMMDT_ALERT:
                    req_type = GATTC_WRITE_NO_RESPONSE;
                    val_hdl  = proxm_env->ias.charact.val_hdl;
                    break;
                default:
                    proxm_write_char_rsp_send(proxm_env, PRF_ERR_INVALID_PARAM);
                    return (KE_MSG_CONSUMED);
            }

            // Check if attribute handle is OK
            if (val_hdl == ATT_INVALID_HANDLE)
            {
                proxm_write_char_rsp_send(proxm_env, PRF_ERR_INEXISTENT_HDL);
            }
            else
            {
                // Send GATT Write Request
                prf_gatt_write(&proxm_env->con_info, val_hdl,
                               (uint8_t *)&param->lvl, sizeof(uint8_t), req_type);
            }
        }
        else
        {
            proxm_write_char_rsp_send(proxm_env, PRF_ERR_INVALID_PARAM);
        }
    }
    else
    {
        // Wrong Connection Handle
        proxm_write_char_rsp_send(proxm_env, PRF_ERR_INVALID_PARAM);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_SVC_IND message.
 * The handler disables the Health Thermometer Profile Collector Role.
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
    struct proxm_env_tag *proxm_env = PRF_CLIENT_GET_ENV(dest_id, proxm);

    // Even if we get multiple responses we only store 1 range
    if (proxm_env->last_uuid_req == ATT_SVC_LINK_LOSS)
    {
        proxm_env->lls.svc.shdl = param->start_hdl;
        proxm_env->lls.svc.ehdl = param->end_hdl;
        proxm_env->nb_svc++;
    }
    else if (proxm_env->last_uuid_req == ATT_SVC_IMMEDIATE_ALERT)
    {
        proxm_env->ias.svc.shdl = param->start_hdl;
        proxm_env->ias.svc.ehdl = param->end_hdl;
        proxm_env->nb_svc++;
    }
    else if (proxm_env->last_uuid_req == ATT_SVC_TX_POWER)
    {
        proxm_env->txps.svc.shdl = param->start_hdl;
        proxm_env->txps.svc.ehdl = param->end_hdl;
        proxm_env->nb_svc++;
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
    struct proxm_env_tag *proxm_env = PRF_CLIENT_GET_ENV(dest_id, proxm);

    if(proxm_env->last_svc_req == ATT_SVC_LINK_LOSS)
    {
        // Retrieve LLS characteristics - Only 1 Char.
        prf_search_chars(proxm_env->lls.svc.ehdl, 1,
                &proxm_env->lls.charact, &proxm_lls_char,
                param, &proxm_env->last_char_code);
    }
    else if(proxm_env->last_svc_req == ATT_SVC_IMMEDIATE_ALERT)
    {
        // Retrieve IAS characteristics - Only 1 Char.
        prf_search_chars(proxm_env->ias.svc.ehdl, 1,
                &proxm_env->ias.charact, &proxm_ias_char,
                param, &proxm_env->last_char_code);
    }
    else if(proxm_env->last_svc_req == ATT_SVC_TX_POWER)
    {
        // Retrieve TXPS characteristics - Only 1 Char.
        prf_search_chars(proxm_env->txps.svc.ehdl, 1,
                &proxm_env->txps.charact, &proxm_txps_char,
                param, &proxm_env->last_char_code);
    }

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
    uint8_t status = 0;
    // Get the address of the environment
    struct proxm_env_tag *proxm_env = PRF_CLIENT_GET_ENV(dest_id, proxm);

    if(state == PROXM_DISCOVERING)
    {
        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
            (param->status == ATT_ERR_NO_ERROR))
        {
            //Currently discovering Link Loss Service
            if (proxm_env->last_svc_req == ATT_SVC_LINK_LOSS)
            {
                if (proxm_env->last_uuid_req == ATT_SVC_LINK_LOSS)
                {
                    if (proxm_env->lls.svc.shdl == ATT_INVALID_HANDLE)
                    {
                        proxm_enable_cfm_send(proxm_env, &proxm_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                    }
                    // Too many services found only one such service should exist
                    else if(proxm_env->nb_svc > 1)
                    {
                        // stop discovery procedure.
                        proxm_enable_cfm_send(proxm_env, &proxm_env->con_info, PRF_ERR_MULTIPLE_SVC);
                    }
                    else
                    {
                        // Discover LLS characteristics
                        prf_disc_char_all_send(&(proxm_env->con_info), &(proxm_env->lls.svc));

                        // Keep last UUID requested and for which service in env
                        proxm_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    }
                }
                else if (proxm_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
                {
                    status = prf_check_svc_char_validity(1, &proxm_env->lls.charact,
                                                         &proxm_lls_char);

                    if (status == PRF_ERR_OK)
                    {
                        proxm_env->nb_svc = 0;

                        // Start discovering TXPS on peer
                        prf_disc_svc_send(&(proxm_env->con_info), ATT_SVC_TX_POWER);

                        proxm_env->last_uuid_req = ATT_SVC_TX_POWER;
                        proxm_env->last_svc_req = ATT_SVC_TX_POWER;
                    }
                    else
                    {
                        // Stop discovery procedure.
                        proxm_enable_cfm_send(proxm_env, &proxm_env->con_info, status);
                    }

                }
            }

            //Currently discovering TX Power Service
            else if (proxm_env->last_svc_req == ATT_SVC_TX_POWER)
            {
                if (proxm_env->last_uuid_req == ATT_SVC_TX_POWER)
                {
                    // TXPS is optional
                    if (proxm_env->txps.svc.shdl == ATT_INVALID_HANDLE)
                    {
                        proxm_env->nb_svc = 0;

                        // Start discovering IAS on peer
                        prf_disc_svc_send(&(proxm_env->con_info), ATT_SVC_IMMEDIATE_ALERT);

                        proxm_env->last_uuid_req = ATT_SVC_IMMEDIATE_ALERT;
                        proxm_env->last_svc_req = ATT_SVC_IMMEDIATE_ALERT;
                    }
                    else
                    {
                        // Discover TXPS characteristics
                        prf_disc_char_all_send(&(proxm_env->con_info), &(proxm_env->txps.svc));

                        // Keep last UUID requested and for which service in env
                        proxm_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    }
                }
                else if (proxm_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
                {
                    status = prf_check_svc_char_validity(1, &proxm_env->txps.charact,
                                                         &proxm_txps_char);

                    if (status == PRF_ERR_OK)
                    {
                        proxm_env->nb_svc = 0;

                        // Start discovering IAS on peer
                        prf_disc_svc_send(&(proxm_env->con_info), ATT_SVC_IMMEDIATE_ALERT);

                        proxm_env->last_uuid_req = ATT_SVC_IMMEDIATE_ALERT;
                        proxm_env->last_svc_req = ATT_SVC_IMMEDIATE_ALERT;
                    }
                    else
                    {
                        // Stop discovery procedure.
                        proxm_enable_cfm_send(proxm_env, &proxm_env->con_info, status);
                    }

                }
            }

            //Currently discovering Immediate Alert Service
            else if (proxm_env->last_svc_req == ATT_SVC_IMMEDIATE_ALERT)
            {
                if (proxm_env->last_uuid_req == ATT_SVC_IMMEDIATE_ALERT)
                {
                    // IAS is optional
                    if (proxm_env->ias.svc.shdl == ATT_INVALID_HANDLE)
                    {
                        proxm_enable_cfm_send(proxm_env, &proxm_env->con_info, PRF_ERR_OK);
                    }
                    else
                    {
                        // Discover IAS characteristics
                        prf_disc_char_all_send(&(proxm_env->con_info), &(proxm_env->ias.svc));

                        // Keep last UUID requested and for which service in env
                        proxm_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    }
                }
                else if (proxm_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
                {
                    status = prf_check_svc_char_validity(1, &proxm_env->txps.charact,
                                                         &proxm_txps_char);

                    proxm_enable_cfm_send(proxm_env, &proxm_env->con_info, status);
                }
            }
        }
    }
    else if (state == PROXM_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                proxm_write_char_rsp_send(proxm_env, param->status);
            }
            break;

            case GATTC_READ:
            {
                if(param->status != GATT_ERR_NO_ERROR)
                {
                    // an error occurs while reading peer device attribute
                    prf_client_att_info_rsp((prf_env_struct*) proxm_env, PROXM_RD_CHAR_RSP,
                            param->status, NULL);
                }
            }
            break;
            default: break;
        }
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
    struct proxm_env_tag *proxm_env = PRF_CLIENT_GET_ENV(dest_id, proxm);

    prf_client_att_info_rsp((prf_env_struct*) proxm_env, PROXM_RD_CHAR_RSP,
            GATT_ERR_NO_ERROR, param);

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Disconnection indication to proximity monitor
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
    PRF_CLIENT_DISABLE_IND_SEND(proxm_envs, dest_id, PROXM, param->conhdl);

    // message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// CONNECTED State handlers definition.
const struct ke_msg_handler proxm_connected[] =
{
    {PROXM_RD_ALERT_LVL_REQ,      (ke_msg_func_t)proxm_rd_alert_lvl_req_handler},
    {PROXM_WR_ALERT_LVL_REQ,      (ke_msg_func_t)proxm_wr_alert_lvl_req_handler},
    {PROXM_RD_TXPW_LVL_REQ,       (ke_msg_func_t)proxm_rd_txpw_lvl_req_handler},
    {GATTC_READ_IND,              (ke_msg_func_t)gattc_read_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler proxm_discovering[] =
{
    {GATTC_DISC_SVC_IND,          (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,         (ke_msg_func_t)gattc_disc_char_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler proxm_default_state[] =
{
    {PROXM_ENABLE_REQ,            (ke_msg_func_t)proxm_enable_req_handler},
    {GAPC_DISCONNECT_IND,         (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,               (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler proxm_state_handler[PROXM_STATE_MAX] =
{
    [PROXM_IDLE]        = KE_STATE_HANDLER_NONE,
    [PROXM_CONNECTED]   = KE_STATE_HANDLER(proxm_connected),
    [PROXM_DISCOVERING] = KE_STATE_HANDLER(proxm_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler proxm_default_handler = KE_STATE_HANDLER(proxm_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t proxm_state[PROXM_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_PROX_MONITOR

/// @} PROXMTASK
