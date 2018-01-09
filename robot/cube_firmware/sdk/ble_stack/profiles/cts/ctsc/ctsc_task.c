/**
 ****************************************************************************************
 *
 * @file ctsc_task.c
 *
 * @brief Current Time Service Client Task implementation.
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
 * @addtogroup CTSCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_CTS_CLIENT)
#include "ctsc.h"
#include "ctsc_task.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

const struct prf_char_def ctsc_cts_char[CTSC_CHAR_MAX] =
{
    // Current Time
    [CTSC_CHAR_CTS_CURRENT_TIME]        = {ATT_CHAR_CT_TIME,
                                           ATT_MANDATORY,
                                           ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF},
    // Local Time Information
    [CTSC_CHAR_CTS_LOC_TIME_INFO]       = {ATT_CHAR_LOCAL_TIME_INFO,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
    // Reference Time Information
    [CTSC_CHAR_CTS_REF_TIME_INFO]       = {ATT_CHAR_REFERENCE_TIME_INFO,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
};

const struct prf_char_desc_def ctsc_cts_char_desc[CTSC_DESC_MAX] =
{
    [CTSC_DESC_CTS_CT_CLI_CFG]          = {ATT_DESC_CLIENT_CHAR_CFG,
                                           ATT_MANDATORY,
                                           CTSC_CHAR_CTS_CURRENT_TIME},
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CTSC_ENABLE_REQ message.
 * The handler enables the Current Time Service Client Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ctsc_enable_req_handler(ke_msg_id_t const msgid,
                                    struct ctsc_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{

    // Status
    uint8_t status;
    // CTS client role tesk enviroment
    struct ctsc_env_tag *ctsc_env;
    // Connection info
    struct prf_con_info con_info;

    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid = src_id;

    status = PRF_CLIENT_ENABLE(con_info,param,ctsc);

    if(status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        return (KE_MSG_NO_FREE);
    }
    else if(status == PRF_ERR_OK)
    {
        ctsc_env = PRF_CLIENT_GET_ENV(dest_id, ctsc);

        if(param->con_type == PRF_CON_DISCOVERY)
        {
            ctsc_env->last_uuid_req = ATT_SVC_CURRENT_TIME;

            // Start discovering
            prf_disc_svc_send(&ctsc_env->con_info, ATT_SVC_CURRENT_TIME);

            ke_state_set(dest_id, CTSC_DISCOVERING);
        }
        else
        {
            ctsc_env->cts = param->cts;
            ctsc_enable_cfm_send(ctsc_env,&ctsc_env->con_info,PRF_ERR_OK);
        }
    }
    else
    {
        ctsc_enable_cfm_send(NULL, &con_info, status);
    }

    return (KE_MSG_CONSUMED);
};

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
    struct ctsc_env_tag *ctsc_env = PRF_CLIENT_GET_ENV(dest_id, ctsc);

    if(ctsc_env != NULL)
    {
        ctsc_env->cts.svc.shdl = param->start_hdl;
        ctsc_env->cts.svc.ehdl = param->end_hdl;
        ctsc_env->nb_svc++;
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

    struct ctsc_env_tag *ctsc_env = PRF_CLIENT_GET_ENV(dest_id, ctsc);

    if(ctsc_env != NULL)
    {
        // Retrieve Weight Scale Service Characteristics
        prf_search_chars(ctsc_env->cts.svc.ehdl,
                         CTSC_CHAR_MAX,
                         &ctsc_env->cts.chars[0],
                         &ctsc_cts_char[0],
                         param,
                         &ctsc_env->last_char_code);
    }

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
    struct ctsc_env_tag *ctsc_env = PRF_CLIENT_GET_ENV(dest_id, ctsc);

    if(ctsc_env != NULL)
    {
        prf_search_descs(CTSC_DESC_MAX,
                         &ctsc_env->cts.desc[0],
                         &ctsc_cts_char_desc[0],
                         param,
                         ctsc_env->last_char_code);
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
    uint8_t status;

    struct ctsc_env_tag *ctsc_env = PRF_CLIENT_GET_ENV(dest_id, ctsc);

    if(ctsc_env == NULL)
    {
        return (KE_MSG_CONSUMED);
    }

    if(state == CTSC_DISCOVERING)
    {
        if((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
           (param->status == ATT_ERR_NO_ERROR))
        {
            if(ctsc_env->last_uuid_req == ATT_SVC_CURRENT_TIME)
            {
                if(ctsc_env->cts.svc.shdl == ATT_INVALID_HANDLE)
                {
                    // Stop discovery
                    ctsc_enable_cfm_send(ctsc_env, &ctsc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                else if(ctsc_env->nb_svc > 1)
                {
                    ctsc_enable_cfm_send(ctsc_env,&ctsc_env->con_info,PRF_ERR_MULTIPLE_SVC);
                }
                else
                {
                    // Discover all CTS characteristics
                    prf_disc_char_all_send(&(ctsc_env->con_info), &(ctsc_env->cts.svc));
                    ctsc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                }
            }
            else if(ctsc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
            {
                status = prf_check_svc_char_validity(CTSC_CHAR_MAX,
                                                    ctsc_env->cts.chars,
                                                    ctsc_cts_char);

                if(status == PRF_ERR_OK)
                {
                    ctsc_env->last_uuid_req = ATT_INVALID_HANDLE;
                    ctsc_env->last_char_code = ctsc_cts_char_desc[CTSC_DESC_CTS_CT_CLI_CFG].char_code;

                    // Discover CT descriptor
                    prf_disc_char_desc_send(&ctsc_env->con_info,
                                            &(ctsc_env->cts.chars[ctsc_env->last_char_code]));
                }
                else
                { // Stop discovery
                    ctsc_enable_cfm_send(ctsc_env,&ctsc_env->con_info, status);
                }
            }
            else // Descriptor
            {
                status = prf_check_svc_char_desc_validity(CTSC_DESC_MAX,
                                                          ctsc_env->cts.desc,
                                                          ctsc_cts_char_desc,
                                                          ctsc_env->cts.chars);

                ctsc_enable_cfm_send(ctsc_env, &ctsc_env->con_info, status);
            }
        }
    }
    else if(state == CTSC_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                struct ctsc_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(CTSC_WR_CHAR_RSP,
                                                               ctsc_env->con_info.appid,
                                                               dest_id,
                                                               ctsc_wr_char_rsp);

                wr_cfm->conhdl = gapc_get_conhdl(ctsc_env->con_info.conidx);
                wr_cfm->status = param->status;

                ke_msg_send(wr_cfm);
            }
            break;

            case GATTC_READ:
            {
                // Error while reading attribute, inform app
                if(param->status != GATT_ERR_NO_ERROR)
                {
                    ctsc_error_ind_send(ctsc_env, param->status);
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
 * @brief Handles reception of the @ref CTSC_RD_CHAR_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ctsc_rd_char_req_handler(ke_msg_id_t const msgid,
                                    struct ctsc_rd_char_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;

    struct ctsc_env_tag *ctsc_env = PRF_CLIENT_GET_ENV(dest_id, ctsc);

    if(ctsc_env == NULL)
    {
        return (KE_MSG_CONSUMED);
    }

    if(param->conhdl == gapc_get_conhdl(ctsc_env->con_info.conidx))
    {

        // Current Time Characteristic Descriptor
        if (((param->char_code & CTSC_DESC_MASK) == CTSC_DESC_MASK) &&
            ((param->char_code & ~CTSC_DESC_MASK) < CTSC_DESC_MAX))
        {
            search_hdl = ctsc_env->cts.desc[param->char_code & ~CTSC_DESC_MASK].desc_hdl;
        }
        // Current Time Service Characteristic
        else if (param->char_code < CTSC_CHAR_MAX)
        {
            search_hdl = ctsc_env->cts.chars[param->char_code].val_hdl;
        }

        // Check handle
        if(search_hdl != ATT_INVALID_HANDLE)
        {
            ctsc_env->last_char_code = param->char_code;
            prf_read_char_send(&(ctsc_env->con_info),ctsc_env->cts.svc.shdl,ctsc_env->cts.svc.ehdl,search_hdl);
        }
        else
        {
            ctsc_error_ind_send(ctsc_env, PRF_ERR_INEXISTENT_HDL);
        }
    }
    else
    {
        ctsc_error_ind_send(ctsc_env, PRF_ERR_INVALID_PARAM);
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
    struct ctsc_env_tag *ctsc_env = PRF_CLIENT_GET_ENV(dest_id, ctsc);

    if(ctsc_env == NULL)
    {
        return (KE_MSG_CONSUMED);
    }

    // Current Time
    if(ctsc_env->last_char_code == CTSC_RD_CURRENT_TIME)
    {
        struct ctsc_ct_ind *rsp = KE_MSG_ALLOC(CTSC_CT_IND,
                                               ctsc_env->con_info.appid,
                                               dest_id,
                                               ctsc_ct_ind);

        // Connection handle
        rsp->conhdl = gapc_get_conhdl(ctsc_env->con_info.conidx);
        // Indication type
        rsp->ind_type = CTSC_TYPE_RSP;
        // Current Time value
        ctsc_unpack_curr_time_value(&(rsp->ct_val), (uint8_t*) param->value);

        ke_msg_send(rsp);
    }
    // Local Time Information
    else if(ctsc_env->last_char_code == CTSC_RD_LOC_TIME_INFO)
    {
        struct ctsc_lti_rd_rsp *rsp = KE_MSG_ALLOC(CTSC_LTI_RD_RSP,
                                                   ctsc_env->con_info.appid,
                                                   dest_id,
                                                   ctsc_lti_rd_rsp);

        // Connection handle
        rsp->conhdl = gapc_get_conhdl(ctsc_env->con_info.conidx);
        // Local Time Information
        rsp->lti_val.time_zone  = param->value[0];
        rsp->lti_val.dst_offset = param->value[1];

        ke_msg_send(rsp);
    }
    // Reference Time Information
    else if(ctsc_env->last_char_code == CTSC_RD_REF_TIME_INFO)
    {
        struct ctsc_rti_rd_rsp *rsp = KE_MSG_ALLOC(CTSC_RTI_RD_RSP,
                                                   ctsc_env->con_info.appid,
                                                   dest_id,
                                                   ctsc_rti_rd_rsp);

        // Connection handle
        rsp->conhdl = gapc_get_conhdl(ctsc_env->con_info.conidx);
        // Reference Time Information
        rsp->rti_val.time_source    = param->value[0];
        rsp->rti_val.time_accuracy  = param->value[1];
        rsp->rti_val.days_update    = param->value[2];
        rsp->rti_val.hours_update   = param->value[3];

        ke_msg_send(rsp);
    }
    // Current Time Configuration Descriptor
    else if(ctsc_env->last_char_code == CTSC_RD_CT_CLI_CFG)
    {
        struct ctsc_ct_ntf_cfg_rd_rsp *rsp = KE_MSG_ALLOC(CTSC_RTI_RD_RSP,
                                                   ctsc_env->con_info.appid,
                                                   dest_id,
                                                   ctsc_ct_ntf_cfg_rd_rsp);

        // Connection handle
        rsp->conhdl = gapc_get_conhdl(ctsc_env->con_info.conidx);
        // Notification Configuration
        memcpy(&rsp->ntf_cfg, param->value, sizeof(rsp->ntf_cfg));

        ke_msg_send(rsp);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CTSC_CFG_INDNTF_REQ message.
 * It allows configuration of the peer ind/ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ctsc_cfg_indntf_req_handler(ke_msg_id_t const msgid,
                                       struct ctsc_cfg_indntf_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{

    uint16_t cfg_hdl = 0x0000;

    struct ctsc_env_tag *ctsc_env = PRF_CLIENT_GET_ENV(dest_id, ctsc);

    if(ctsc_env != NULL)
    {
        if(param->conhdl == gapc_get_conhdl(ctsc_env->con_info.conidx))
        {
            cfg_hdl = ctsc_env->cts.desc[CTSC_DESC_CTS_CT_CLI_CFG].desc_hdl;

            if(!((param->cfg_val == PRF_CLI_STOP_NTFIND)||(param->cfg_val == PRF_CLI_START_NTF)))
            {
                ctsc_error_ind_send(ctsc_env, PRF_ERR_INVALID_PARAM);
            }
            else
            {
                if(cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
                {
                    // Send GATT write req
                    prf_gatt_write_ntf_ind(&ctsc_env->con_info, cfg_hdl, param->cfg_val);
                }
                else
                {
                    ctsc_error_ind_send(ctsc_env, PRF_ERR_INEXISTENT_HDL);
                }
            }
        }
        else
        {
            ctsc_error_ind_send(ctsc_env, PRF_ERR_INVALID_PARAM);
        }
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

    struct ctsc_env_tag *ctsc_env = PRF_CLIENT_GET_ENV(dest_id, ctsc);

    if(ctsc_env != NULL)
    {
        if(param->handle == ctsc_env->cts.chars[CTSC_CHAR_CTS_CURRENT_TIME].val_hdl)
        {
            struct ctsc_ct_ind *ind = KE_MSG_ALLOC(CTSC_CT_IND,
                                                   ctsc_env->con_info.appid,
                                                   dest_id,
                                                   ctsc_ct_ind);

            ind->conhdl = gapc_get_conhdl(ctsc_env->con_info.conidx);
            ind->ind_type = CTSC_TYPE_NTF;
            // Unpack current time value
            ctsc_unpack_curr_time_value(&(ind->ct_val), (uint8_t*) param->value);

            ke_msg_send(ind);
        }
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to CTSC.
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
    PRF_CLIENT_DISABLE_IND_SEND(ctsc_envs, dest_id, CTSC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies state message handlers for connected state
const struct ke_msg_handler ctsc_connected[] =
{
    {GATTC_READ_IND,             (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,            (ke_msg_func_t)gattc_event_ind_handler},
    {CTSC_CFG_INDNTF_REQ,        (ke_msg_func_t)ctsc_cfg_indntf_req_handler},
    {CTSC_RD_CHAR_REQ,           (ke_msg_func_t)ctsc_rd_char_req_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler ctsc_discovering[] =
{
    {GATTC_DISC_SVC_IND,         (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,        (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,   (ke_msg_func_t)gattc_disc_char_desc_ind_handler},
};


/// Specified state message handlers for default state
const struct ke_msg_handler ctsc_default_state[] =
{
    {CTSC_ENABLE_REQ,             (ke_msg_func_t)ctsc_enable_req_handler},
    {GAPC_DISCONNECT_IND,         (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,               (ke_msg_func_t)gattc_cmp_evt_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler ctsc_state_handler[CTSC_STATE_MAX] =
{
    [CTSC_IDLE]        = KE_STATE_HANDLER_NONE,
    [CTSC_CONNECTED]   = KE_STATE_HANDLER(ctsc_connected),
    [CTSC_DISCOVERING] = KE_STATE_HANDLER(ctsc_discovering),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler ctsc_default_handler = KE_STATE_HANDLER(ctsc_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t ctsc_state[CTSC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_WSS_COLLECTOR
/// @} WSSSTASK
