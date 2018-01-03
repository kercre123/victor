/**
 ****************************************************************************************
 *
 * @file blpc_task.c
 *
 * @brief Blood Pressure Profile Collector Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup BLPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BP_COLLECTOR)
#include "gap.h"
#include "attm.h"
#include "blpc.h"
#include "blpc_task.h"
#include "gattc_task.h"
#include "smpc_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// State machine used to retrieve blood pressure service characteristics information
const struct prf_char_def blpc_bps_char[BLPC_CHAR_MAX] =
{
    /// Blood Pressure Measurement
    [BLPC_CHAR_BP_MEAS]            = {ATT_CHAR_BLOOD_PRESSURE_MEAS,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_IND},
    /// Intermediate Cuff pressure
    [BLPC_CHAR_CP_MEAS]            = {ATT_CHAR_INTERMEDIATE_CUFF_PRESSURE,
                                   ATT_OPTIONAL,
                                   ATT_CHAR_PROP_NTF},
    /// Blood Pressure Feature
    [BLPC_CHAR_BP_FEATURE]         = {ATT_CHAR_BLOOD_PRESSURE_FEATURE,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_RD},
};

/// State machine used to retrieve blood pressure service characteristic description information
const struct prf_char_desc_def blpc_bps_char_desc[BLPC_DESC_MAX] =
{
    /// Blood Pressure Measurement client config
    [BLPC_DESC_BP_MEAS_CLI_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, BLPC_CHAR_BP_MEAS},
    /// Intermediate Cuff pressure client config
    [BLPC_DESC_IC_MEAS_CLI_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, BLPC_CHAR_CP_MEAS},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BLPC_ENABLE_REQ message.
 * The handler enables the Blood Pressure Profile Collector Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int blpc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct blpc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Battery Service Client Role Task Environment
    struct blpc_env_tag *blpc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status = PRF_CLIENT_ENABLE(con_info, param, blpc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        blpc_env = PRF_CLIENT_GET_ENV(dest_id, blpc);

        // Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering BPS on peer
            prf_disc_svc_send(&(blpc_env->con_info), ATT_SVC_BLOOD_PRESSURE);

            blpc_env->last_uuid_req = ATT_SVC_BLOOD_PRESSURE;

            // Go to DISCOVERING state
            ke_state_set(dest_id, BLPC_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            blpc_env->bps = param->bps;

            blpc_enable_cfm_send(blpc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        blpc_enable_cfm_send(NULL, &con_info, status);
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
    struct blpc_env_tag *blpc_env = PRF_CLIENT_GET_ENV(dest_id, blpc);

    //even if we get multiple responses we only store 1 range
    if (blpc_env->last_uuid_req == ATT_SVC_BLOOD_PRESSURE)
    {
        blpc_env->bps.svc.shdl = param->start_hdl;
        blpc_env->bps.svc.ehdl = param->end_hdl;
        blpc_env->nb_svc++;
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
    struct blpc_env_tag *blpc_env = PRF_CLIENT_GET_ENV(dest_id, blpc);

    // Retrieve BPS characteristics
    prf_search_chars(blpc_env->bps.svc.ehdl, BLPC_CHAR_MAX,
            &blpc_env->bps.chars[0], &blpc_bps_char[0],
            param, &blpc_env->last_char_code);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_DESC_IND message.
 * This event can be received several times
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_disc_char_desc_evt_handler(ke_msg_id_t const msgid,
                                           struct gattc_disc_char_desc_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct blpc_env_tag *blpc_env = PRF_CLIENT_GET_ENV(dest_id, blpc);

    // Retrieve BPS descriptors
    prf_search_descs(BLPC_DESC_MAX, &blpc_env->bps.descs[0], &blpc_bps_char_desc[0],
                     param, blpc_env->last_char_code);

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
    // Get the address of the environment
    struct blpc_env_tag *blpc_env = PRF_CLIENT_GET_ENV(dest_id, blpc);

    if(state == BLPC_DISCOVERING)
    {
        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
                (param->status == ATT_ERR_NO_ERROR))
        {
            // service start/end handles has been received
            if(blpc_env->last_uuid_req == ATT_SVC_BLOOD_PRESSURE)
            {
                // check if service handles are not ok
                if(blpc_env->bps.svc.shdl == ATT_INVALID_HANDLE)
                {
                    // stop discovery procedure.
                    blpc_enable_cfm_send(blpc_env, &blpc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                //too many services found only one such service should exist
                else if(blpc_env->nb_svc != 1)
                {
                    // stop discovery procedure.
                    blpc_enable_cfm_send(blpc_env, &blpc_env->con_info, PRF_ERR_MULTIPLE_SVC);
                }
                else
                {
                    //discover all BPS characteristics
                    prf_disc_char_all_send(&(blpc_env->con_info), &(blpc_env->bps.svc));
                    blpc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                }
            }
            else if(blpc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
            {
                status = prf_check_svc_char_validity(BLPC_CHAR_MAX, blpc_env->bps.chars,
                        blpc_bps_char);

                // Check for characteristic properties.
                if(status == PRF_ERR_OK)
                {
                    blpc_env->last_uuid_req = ATT_INVALID_HANDLE;
                    blpc_env->last_char_code = blpc_bps_char_desc[BLPC_DESC_BP_MEAS_CLI_CFG].char_code;

                    //Discover Blood Pressure Measurement Char. Descriptor - Mandatory
                    prf_disc_char_desc_send(&(blpc_env->con_info),
                            &(blpc_env->bps.chars[blpc_env->last_char_code]));
                }
                else
                {
                    // Stop discovery procedure.
                    blpc_enable_cfm_send(blpc_env, &blpc_env->con_info, status);
                }
            }
            else
            {
                //If Intermediate Cuff Pressure Char. found, discover its descriptor
                if ((blpc_env->bps.chars[BLPC_CHAR_CP_MEAS].char_hdl != ATT_INVALID_HANDLE) &&
                        (blpc_env->last_char_code == BLPC_CHAR_BP_MEAS))
                {
                    blpc_env->last_char_code = BLPC_CHAR_CP_MEAS;
                    //Discover Intermediate Cuff Pressure Char. Descriptor - Mandatory
                    prf_disc_char_desc_send(&(blpc_env->con_info),
                            &(blpc_env->bps.chars[blpc_env->last_char_code]));
                }
                else
                {
                    status = prf_check_svc_char_desc_validity(BLPC_DESC_MAX,
                            blpc_env->bps.descs,
                            blpc_bps_char_desc,
                            blpc_env->bps.chars);

                    blpc_enable_cfm_send(blpc_env, &blpc_env->con_info, status);
                }
            }
        }
    }
    else if(state == BLPC_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                struct blpc_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(BLPC_WR_CHAR_RSP,
                                                               blpc_env->con_info.appid, dest_id,
                                                               blpc_wr_char_rsp);
                wr_cfm->conhdl    = gapc_get_conhdl(blpc_env->con_info.conidx);
                //it will be a GATT status code
                wr_cfm->status    = param->status;
                // send the message
                ke_msg_send(wr_cfm);
            }
            break;

            case GATTC_READ:
            {
                if(param->status != GATT_ERR_NO_ERROR)
                {
                    prf_client_att_info_rsp((prf_env_struct*) blpc_env, BLPC_RD_CHAR_RSP,
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
 * @brief Handles reception of the @ref BLPC_RD_DATETIME_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int blpc_rd_char_req_handler(ke_msg_id_t const msgid,
                                        struct blpc_rd_char_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint16_t search_hdl = 0x0000;
    uint16_t shdl = 0x0000;
    uint16_t ehdl = 0x0000;
    // Get the address of the environment
    struct blpc_env_tag *blpc_env = PRF_CLIENT_GET_ENV(dest_id, blpc);

    if(param->conhdl == gapc_get_conhdl(blpc_env->con_info.conidx))
    {
        if((param->char_code & BLPC_DESC_MASK) == BLPC_DESC_MASK)
        {
            search_hdl = blpc_env->bps.descs[param->char_code & ~BLPC_DESC_MASK].desc_hdl;
            shdl = blpc_env->bps.svc.shdl;
            ehdl = blpc_env->bps.svc.ehdl;
        }
        else
        {
            search_hdl = blpc_env->bps.chars[param->char_code].val_hdl;
            shdl = blpc_env->bps.svc.shdl;
            ehdl = blpc_env->bps.svc.ehdl;
        }

        //check if handle is viable
        if (search_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            prf_read_char_send(&(blpc_env->con_info), shdl, ehdl, search_hdl);
        }
        else
        {
            prf_client_att_info_rsp((prf_env_struct*) blpc_env, BLPC_RD_CHAR_RSP,
                    PRF_ERR_INEXISTENT_HDL, NULL);
        }
    }
    else
    {
        prf_client_att_info_rsp((prf_env_struct*) blpc_env, BLPC_RD_CHAR_RSP,
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
    struct blpc_env_tag *blpc_env = PRF_CLIENT_GET_ENV(dest_id, blpc);

    prf_client_att_info_rsp((prf_env_struct*) blpc_env, BLPC_RD_CHAR_RSP,
            GATT_ERR_NO_ERROR, param);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BLPC_CFG_INDNTF_REQ message.
 * It allows configuration of the peer ind/ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int blpc_cfg_indntf_req_handler(ke_msg_id_t const msgid,
                                       struct blpc_cfg_indntf_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    uint16_t cfg_hdl = 0x0000;
    // Get the address of the environment
    struct blpc_env_tag *blpc_env = PRF_CLIENT_GET_ENV(dest_id, blpc);

    if(param->conhdl == gapc_get_conhdl(blpc_env->con_info.conidx))
    {
        //get handle of the configuration characteristic to set and check if value matches property
        switch(param->char_code)
        {
            case BPS_BP_MEAS_CODE://can only IND
                cfg_hdl = blpc_env->bps.descs[BLPC_DESC_BP_MEAS_CLI_CFG].desc_hdl;
                if(!((param->cfg_val == PRF_CLI_STOP_NTFIND)||
                     (param->cfg_val == PRF_CLI_START_IND)))
                {
                    blpc_error_ind_send(blpc_env, PRF_ERR_INVALID_PARAM);
                }
                break;

            case BPS_INTERM_CP_CODE://can only NTF
                cfg_hdl = blpc_env->bps.descs[BLPC_DESC_IC_MEAS_CLI_CFG].desc_hdl;
                if(!((param->cfg_val == PRF_CLI_STOP_NTFIND)||
                     (param->cfg_val == PRF_CLI_START_NTF)))
                {
                    blpc_error_ind_send(blpc_env, PRF_ERR_INVALID_PARAM);
                }
                break;

            default:
                //let app know that one of the request params is incorrect
                blpc_error_ind_send(blpc_env, PRF_ERR_INVALID_PARAM);
                return (KE_MSG_CONSUMED);
        }

        //if we get here, then the APP request looks OK

        //check if the handle value exists
        if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            // Send GATT Write Request
            prf_gatt_write_ntf_ind(&blpc_env->con_info, cfg_hdl, param->cfg_val);
        }
        else
        {
            blpc_error_ind_send(blpc_env, PRF_ERR_INEXISTENT_HDL);
        }
    }
    else
    {
        blpc_error_ind_send(blpc_env, PRF_ERR_INVALID_PARAM);
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
    // Get the address of the environment
    struct blpc_env_tag *blpc_env = PRF_CLIENT_GET_ENV(dest_id, blpc);

    if(KE_IDX_GET(src_id) == blpc_env->con_info.conidx)
    {
        if((param->handle == blpc_env->bps.chars[BLPC_CHAR_CP_MEAS].val_hdl)
            || ((param->handle == blpc_env->bps.chars[BLPC_CHAR_BP_MEAS].val_hdl)))
        {
            //build a BLPC_BP_MEAD_IND message with stable blood pressure value code.
            struct blpc_meas_ind * ind = KE_MSG_ALLOC(BLPC_BP_MEAS_IND,
                                                      blpc_env->con_info.appid, dest_id,
                                                      blpc_meas_ind);
            // retrieve connection handle
            ind->conhdl = gapc_get_conhdl(blpc_env->con_info.conidx);

            // Intermediate cuff pressure value
            ind->flag_interm_cp = ((param->type == GATTC_NOTIFY) ? 0x01 : 0x00);

            // unpack blood pressure measurement.
            blpc_unpack_meas_value(&(ind->meas_val), (uint8_t*) param->value);

            ke_msg_send(ind);
        }
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to BLPC.
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
    PRF_CLIENT_DISABLE_IND_SEND(blpc_envs, dest_id, BLPC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler blpc_connected[] =
{
    {BLPC_RD_CHAR_REQ,              (ke_msg_func_t)blpc_rd_char_req_handler},
    {BLPC_CFG_INDNTF_REQ,           (ke_msg_func_t)blpc_cfg_indntf_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler blpc_discovering[] =
{
    {GATTC_DISC_SVC_IND, (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,    (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,   (ke_msg_func_t)gatt_disc_char_desc_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler blpc_default_state[] =
{
    {BLPC_ENABLE_REQ,               (ke_msg_func_t)blpc_enable_req_handler},
    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler blpc_state_handler[BLPC_STATE_MAX] =
{
    [BLPC_IDLE]        = KE_STATE_HANDLER_NONE,
    [BLPC_CONNECTED]   = KE_STATE_HANDLER(blpc_connected),
    [BLPC_DISCOVERING] = KE_STATE_HANDLER(blpc_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler blpc_default_handler = KE_STATE_HANDLER(blpc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t blpc_state[BLPC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* (BLE_BP_COLLECTOR) */
/// @} BLPCTASK
