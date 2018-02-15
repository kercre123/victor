/**
 ****************************************************************************************
 *
 * @file htpc_task.c
 *
 * @brief Health Thermometer Profile Collector Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HTPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HT_COLLECTOR)
#include "gap.h"
#include "attm.h"
#include "htpc.h"
#include "htpc_task.h"
#include "gapc.h"
#include "gapc_task.h"
#include "prf_utils.h"

/// State machine used to retrieve Health Thermometer service characteristics information
const struct prf_char_def htpc_hts_char[HTPC_CHAR_HTS_MAX] =
{
    /// Temperature Measurement
    [HTPC_CHAR_HTS_TEMP_MEAS]        = {ATT_CHAR_TEMPERATURE_MEAS,
                                        ATT_MANDATORY,
                                        ATT_CHAR_PROP_IND},
    /// Temperature Type
    [HTPC_CHAR_HTS_TEMP_TYPE]        = {ATT_CHAR_TEMPERATURE_TYPE,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_RD},
    /// Intermediate Temperature
    [HTPC_CHAR_HTS_INTM_TEMP]        = {ATT_CHAR_INTERMED_TEMPERATURE,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_NTF},
    /// Measurement Interval
    [HTPC_CHAR_HTS_MEAS_INTV]        = {ATT_CHAR_MEAS_INTERVAL,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_RD},
};

/// State machine used to retrieve Current Time service characteristic description information
const struct prf_char_desc_def htpc_hts_char_desc[HTPC_DESC_HTS_MAX] =
{
    /// Temperature Measurement Char. - Client Char. Configuration
    [HTPC_DESC_HTS_TEMP_MEAS_CLI_CFG]   = {ATT_DESC_CLIENT_CHAR_CFG,     ATT_MANDATORY,   HTPC_CHAR_HTS_TEMP_MEAS},
    /// Intermediate Temperature Char. - Client Char. Configuration
    [HTPC_DESC_HTS_INTM_MEAS_CLI_CFG]   = {ATT_DESC_CLIENT_CHAR_CFG,     ATT_MANDATORY,   HTPC_CHAR_HTS_INTM_TEMP},
    /// Measurement Interval Char. - Client Char. Configuration
    [HTPC_DESC_HTS_MEAS_INTV_CLI_CFG]   = {ATT_DESC_CLIENT_CHAR_CFG,     ATT_OPTIONAL,    HTPC_CHAR_HTS_MEAS_INTV},
    /// Measurement Interval Char. - Valid Range
    [HTPC_DESC_HTS_MEAS_INTV_VAL_RGE]   = {ATT_DESC_VALID_RANGE,         ATT_OPTIONAL,    HTPC_CHAR_HTS_MEAS_INTV},
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HTPC_ENABLE_REQ message.
 * The handler enables the Health Thermometer Profile Collector Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int htpc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct htpc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Health Thermometer Profile Collector Role Task Environment
    struct htpc_env_tag *htpc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status = PRF_CLIENT_ENABLE(con_info, param, htpc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

        //Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            htpc_env->last_uuid_req = ATT_SVC_HEALTH_THERMOM;

            // Start discovering HTS on peer
            prf_disc_svc_send(&htpc_env->con_info, ATT_SVC_HEALTH_THERMOM);

            // Go to DISCOVERING state
            ke_state_set(dest_id, HTPC_DISCOVERING);
        }
        // Normal connection, get saved att details
        else
        {
            htpc_env->hts = param->hts;

            //send APP confirmation that can start normal connection to TH
            htpc_enable_cfm_send(htpc_env, &htpc_env->con_info, PRF_ERR_OK);
        }
    }
    else
    {
        // An error has been raised during the process, hts is NULL and won't be handled
        htpc_enable_cfm_send(NULL, &con_info, status);
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
    struct htpc_env_tag *htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

    //Even if we get multiple responses we only store 1 range
    htpc_env->hts.svc.shdl = param->start_hdl;
    htpc_env->hts.svc.ehdl = param->end_hdl;
    htpc_env->nb_svc++;

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
    struct htpc_env_tag *htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

    // Retrieve Health Thermometer Service Characteristics
    prf_search_chars(htpc_env->hts.svc.ehdl, HTPC_CHAR_HTS_MAX,
            &htpc_env->hts.chars[0], &htpc_hts_char[0],
            param, &htpc_env->last_char_code);

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
    // Get the address of the environment
    struct htpc_env_tag *htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

    // Retrieve HTS descriptors
    prf_search_descs(HTPC_DESC_HTS_MAX, &htpc_env->hts.descs[0], &htpc_hts_char_desc[0],
                     param, htpc_env->last_char_code);

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
    // Status
    uint8_t status;
    // Get the address of the environment
    struct htpc_env_tag *htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

    if(state == HTPC_DISCOVERING)
    {
        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
                (param->status == ATT_ERR_NO_ERROR))
        {
            // Service start/end handles has been received
            if(htpc_env->last_uuid_req == ATT_SVC_HEALTH_THERMOM)
            {
                // check if service handles are not ok
                if (htpc_env->hts.svc.shdl == ATT_INVALID_HANDLE)
                {
                    // stop discovery procedure.
                    htpc_enable_cfm_send(htpc_env, &htpc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                //too many services found only one such service should exist
                else if(htpc_env->nb_svc > 1)
                {
                    // stop discovery procedure.
                    htpc_enable_cfm_send(htpc_env, &htpc_env->con_info, PRF_ERR_MULTIPLE_SVC);
                }
                // check if service handles are ok
                else
                {
                    //discover all HTS characteristics
                    htpc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    prf_disc_char_all_send(&(htpc_env->con_info), &(htpc_env->hts.svc));
                }
            }
            else if(htpc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
            {
                status = prf_check_svc_char_validity(HTPC_CHAR_HTS_MAX, htpc_env->hts.chars,
                        htpc_hts_char);

                // Check for characteristic properties.
                if(status == PRF_ERR_OK)
                {
                    htpc_env->last_uuid_req = ATT_INVALID_HANDLE;
                    htpc_env->last_char_code = htpc_hts_char_desc[HTPC_DESC_HTS_TEMP_MEAS_CLI_CFG].char_code;
                    //Discover Temperature Measurement Char. Descriptor - Mandatory
                    prf_disc_char_desc_send(&(htpc_env->con_info),
                            &(htpc_env->hts.chars[htpc_env->last_char_code]));
                }
                else
                {
                    // Stop discovery procedure.
                    htpc_enable_cfm_send(htpc_env, &htpc_env->con_info, status);
                }
            }
            else //Descriptors
            {
                //Get code of next char. having descriptors
                htpc_env->last_char_code = htpc_get_next_desc_char_code(htpc_env, &htpc_hts_char_desc[0]);
                if (htpc_env->last_char_code != HTPC_DESC_HTS_MAX)
                {
                    prf_disc_char_desc_send(&(htpc_env->con_info),
                            &(htpc_env->hts.chars[htpc_env->last_char_code]));
                }
                else
                {
                    status = prf_check_svc_char_desc_validity(HTPC_DESC_HTS_MAX,
                            htpc_env->hts.descs,
                            htpc_hts_char_desc,
                            htpc_env->hts.chars);

                    htpc_enable_cfm_send(htpc_env, &htpc_env->con_info, status);
                }
            }
        }
        else
        {
            htpc_enable_cfm_send(htpc_env, &htpc_env->con_info, param->status);
        }
    }
    else if(state == HTPC_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                // Get the address of the environment
                struct htpc_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(HTPC_WR_CHAR_RSP,
                        htpc_env->con_info.appid, dest_id,
                        htpc_wr_char_rsp);

                wr_cfm->conhdl    = gapc_get_conhdl(htpc_env->con_info.conidx);
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
                    prf_client_att_info_rsp((prf_env_struct*) htpc_env, HTPC_RD_CHAR_RSP,
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
 * @brief Handles reception of the @ref HTPC_RD_DATETIME_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int htpc_rd_char_req_handler(ke_msg_id_t const msgid,
                                        struct htpc_rd_char_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Attribute handle
    uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;
    // Get the address of the environment
    struct htpc_env_tag *htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

    if(param->conhdl == gapc_get_conhdl(htpc_env->con_info.conidx))
    {
        // Descriptors
        if(((param->char_code & HTPC_DESC_HTS_MASK) == HTPC_DESC_HTS_MASK) &&
           ((param->char_code & ~HTPC_DESC_HTS_MASK) < HTPC_DESC_HTS_MAX))
        {
            search_hdl = htpc_env->hts.descs[param->char_code & ~HTPC_DESC_HTS_MASK].desc_hdl;
        }
        // Characteristic
        else if (param->char_code < HTPC_CHAR_HTS_MAX)
        {
            search_hdl = htpc_env->hts.chars[param->char_code].val_hdl;
        }

        // Check if handle is viable
        if (search_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            htpc_env->last_char_code = param->char_code;
            prf_read_char_send(&(htpc_env->con_info), htpc_env->hts.svc.shdl,
                               htpc_env->hts.svc.ehdl, search_hdl);
        }
        else
        {
            prf_client_att_info_rsp((prf_env_struct*) htpc_env, HTPC_RD_CHAR_RSP,
                    PRF_ERR_INEXISTENT_HDL, NULL);

        }
    }
    else
    {
        prf_client_att_info_rsp((prf_env_struct*) htpc_env, HTPC_RD_CHAR_RSP,
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
    struct htpc_env_tag *htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

    // send attribute information
    prf_client_att_info_rsp((prf_env_struct*) htpc_env, HTPC_RD_CHAR_RSP,
            GATT_ERR_NO_ERROR, param);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HTPC_CFG_INDNTF_REQ message.
 * It allows configuration of the peer ind/ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int htpc_cfg_indntf_req_handler(ke_msg_id_t const msgid,
                                       struct htpc_cfg_indntf_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Client Characteristic Configuration Descriptor Handle
    uint16_t cfg_hdl = 0x0000;
    // Status
    uint8_t status = PRF_ERR_OK;
    // Get the address of the environment
    struct htpc_env_tag *htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

    if(param->conhdl == gapc_get_conhdl(htpc_env->con_info.conidx))
    {
        switch(param->char_code)
        {
            case HTPC_CHAR_HTS_TEMP_MEAS://can only IND
                cfg_hdl = htpc_env->hts.descs[HTPC_DESC_HTS_TEMP_MEAS_CLI_CFG].desc_hdl;
                if(!((param->cfg_val == PRF_CLI_STOP_NTFIND)||
                     (param->cfg_val == PRF_CLI_START_IND)))
                {
                    status = PRF_ERR_INVALID_PARAM;
                }
                break;

            case HTPC_CHAR_HTS_MEAS_INTV://can only IND
                cfg_hdl = htpc_env->hts.descs[HTPC_DESC_HTS_MEAS_INTV_CLI_CFG].desc_hdl;
                if(!((param->cfg_val == PRF_CLI_STOP_NTFIND)||
                     (param->cfg_val == PRF_CLI_START_IND)))
                {
                    status = PRF_ERR_INVALID_PARAM;
                }
                break;

            case HTPC_CHAR_HTS_INTM_TEMP://can only NTF
                cfg_hdl = htpc_env->hts.descs[HTPC_DESC_HTS_INTM_MEAS_CLI_CFG].desc_hdl;
                if(!((param->cfg_val == PRF_CLI_STOP_NTFIND)||
                     (param->cfg_val == PRF_CLI_START_NTF)))
                {
                    status = PRF_ERR_INVALID_PARAM;
                }
                break;

            default:
                //Let app know that one of the request params is incorrect
                status = PRF_ERR_INVALID_PARAM;
                break;
        }

        if (status == PRF_ERR_OK)
        {
            //Check if the handle value exists
            if ((cfg_hdl == ATT_INVALID_SEARCH_HANDLE))
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
            else
            {
                // Send GATT Write Request
                prf_gatt_write_ntf_ind(&htpc_env->con_info, cfg_hdl, param->cfg_val);
            }
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        htpc_error_ind_send(htpc_env, status);
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HTPC_WR_MEAS_INTV_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int htpc_wr_meas_intv_req_handler(ke_msg_id_t const msgid,
                                        struct htpc_wr_meas_intv_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct htpc_env_tag *htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

    if(param->conhdl == gapc_get_conhdl(htpc_env->con_info.conidx))
    {
        if (htpc_env->hts.chars[HTPC_CHAR_HTS_MEAS_INTV].char_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            if ((htpc_env->hts.chars[HTPC_CHAR_HTS_MEAS_INTV].prop & ATT_CHAR_PROP_WR)
                                  == ATT_CHAR_PROP_WR)
            {
                // Send GATT Write Request
                prf_gatt_write(&htpc_env->con_info, htpc_env->hts.chars[HTPC_CHAR_HTS_MEAS_INTV].val_hdl,
                               (uint8_t *)&param->intv, sizeof(uint16_t), GATTC_WRITE);
            }
            //Write not allowed, so no point in continuing
            else
            {
                htpc_error_ind_send(htpc_env, PRF_ERR_NOT_WRITABLE);
            }
        }
        //Send app error indication for inexistent handle for this characteristic
        else
        {
            htpc_error_ind_send(htpc_env, PRF_ERR_INEXISTENT_HDL);
        }
    }
    else
    {
        htpc_error_ind_send(htpc_env, PRF_ERR_INVALID_PARAM);
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
    struct htpc_env_tag *htpc_env = PRF_CLIENT_GET_ENV(dest_id, htpc);

    // Temperature Measurement Char.
    if((param->handle == htpc_env->hts.chars[HTPC_CHAR_HTS_TEMP_MEAS].val_hdl)
        || (param->handle == htpc_env->hts.chars[HTPC_CHAR_HTS_INTM_TEMP].val_hdl))
    {
        htpc_unpack_temp(htpc_env, (uint8_t *)&(param->value), param->length,
                ((param->type == GATTC_NOTIFY) ? HTPC_TEMP_MEAS_INTM : HTPC_TEMP_MEAS_STABLE));
    }
    // Measurement Interval Char.
    else if (param->handle == htpc_env->hts.chars[HTPC_CHAR_HTS_MEAS_INTV].val_hdl)
    {
        struct htpc_meas_intv_ind * ind = KE_MSG_ALLOC(HTPC_MEAS_INTV_IND,
                htpc_env->con_info.appid, dest_id,
                htpc_meas_intv_ind);

        ind->conhdl = gapc_get_conhdl(htpc_env->con_info.conidx);
        memcpy(&ind->intv, &param->value[0], sizeof(uint16_t));

        ke_msg_send(ind);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to HTPC.
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
    PRF_CLIENT_DISABLE_IND_SEND(htpc_envs, dest_id, HTPC, param->conhdl);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler htpc_connected[] =
{
    {HTPC_RD_CHAR_REQ,              (ke_msg_func_t)htpc_rd_char_req_handler},
    {HTPC_CFG_INDNTF_REQ,           (ke_msg_func_t)htpc_cfg_indntf_req_handler},
    {HTPC_WR_MEAS_INTV_REQ,         (ke_msg_func_t)htpc_wr_meas_intv_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler htpc_discovering[] =
{
    {GATTC_DISC_SVC_IND,         (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,        (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,   (ke_msg_func_t)gattc_disc_char_desc_ind_handler},
};

/* Default State handlers definition. */
const struct ke_msg_handler htpc_default_state[] =
{
    {HTPC_ENABLE_REQ,               (ke_msg_func_t)htpc_enable_req_handler},
    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler htpc_state_handler[HTPC_STATE_MAX] =
{
    [HTPC_IDLE]        = KE_STATE_HANDLER_NONE,
    [HTPC_DISCOVERING] = KE_STATE_HANDLER(htpc_discovering),
    [HTPC_CONNECTED]   = KE_STATE_HANDLER(htpc_connected),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler htpc_default_handler = KE_STATE_HANDLER(htpc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t htpc_state[HTPC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_HT_COLLECTOR

/// @} HTPCTASK
