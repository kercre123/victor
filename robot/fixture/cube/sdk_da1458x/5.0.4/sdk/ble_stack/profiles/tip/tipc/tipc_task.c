/**
 ****************************************************************************************
 *
 * @file tipc_task.c
 *
 * @brief Time Profile Client Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup TIPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_TIP_CLIENT)
#include "gap.h"
#include "attm.h"
#include "tipc.h"
#include "tipc_task.h"
#include "gattc_task.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */



/*
 * DEFINES
 ****************************************************************************************
 */

/// State machine used to retrieve Current Time service characteristics information
const struct prf_char_def tipc_cts_char[TIPC_CHAR_CTS_MAX] =
{
    /// Current Time
    [TIPC_CHAR_CTS_CURR_TIME]        = {ATT_CHAR_CT_TIME,
                                        ATT_MANDATORY,
                                        ATT_CHAR_PROP_RD|ATT_CHAR_PROP_NTF},
    /// Local Time Info
    [TIPC_CHAR_CTS_LOCAL_TIME_INFO]  = {ATT_CHAR_LOCAL_TIME_INFO,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_RD},
    /// Reference Time Info
    [TIPC_CHAR_CTS_REF_TIME_INFO]    = {ATT_CHAR_REFERENCE_TIME_INFO,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_RD},
};

/// State machine used to retrieve Current Time service characteristic description information
const struct prf_char_desc_def tipc_cts_char_desc[TIPC_DESC_CTS_MAX] =
{
    /// Current Time client config
    [TIPC_DESC_CTS_CURR_TIME_CLI_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,
                                         ATT_MANDATORY,
                                         TIPC_CHAR_CTS_CURR_TIME},
};

/// State machine used to retrieve Next DST Change service characteristics information
const struct prf_char_def tipc_ndcs_char[TIPC_CHAR_NDCS_MAX] =
{
    /// Current Time
    [TIPC_CHAR_NDCS_TIME_WITH_DST]    = {ATT_CHAR_TIME_WITH_DST,
                                         ATT_MANDATORY,
                                         ATT_CHAR_PROP_RD},
};

/// State machine used to retrieve Reference Time Update service characteristics information
const struct prf_char_def tipc_rtus_char[TIPC_CHAR_RTUS_MAX] =
{
    /// Time Update Control Point
    [TIPC_CHAR_RTUS_TIME_UPD_CTNL_PT]    = {ATT_CHAR_TIME_UPDATE_CNTL_POINT,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_WR_NO_RESP},
    /// Time Update State
    [TIPC_CHAR_RTUS_TIME_UPD_STATE]      = {ATT_CHAR_TIME_UPDATE_STATE,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_RD},
};


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPC_ENABLE_REQ message.
 * The handler enables the Time Profile Client Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tipc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct tipc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Battery Service Client Role Task Environment
    struct tipc_env_tag *tipc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status = PRF_CLIENT_ENABLE(con_info, param, tipc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);

        //config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering CTS on peer
            prf_disc_svc_send(&(tipc_env->con_info), ATT_SVC_CURRENT_TIME);

            tipc_env->last_uuid_req = ATT_SVC_CURRENT_TIME;
            tipc_env->last_svc_req  = ATT_SVC_CURRENT_TIME;

            // Go to DISCOVERING state
            ke_state_set(dest_id, TIPC_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            tipc_env->cts = param->cts;
            tipc_env->ndcs = param->ndcs;
            tipc_env->rtus = param->rtus;

            //send APP confirmation that can start normal connection
            tipc_enable_cfm_send(tipc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        // Send confirmation that can start normal connection
        tipc_enable_cfm_send(NULL, &con_info, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPC_RD_CHAR_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tipc_rd_char_req_handler(ke_msg_id_t const msgid,
                                        struct tipc_rd_char_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Attribute Handle
    uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;
    // Service
    struct prf_svc *svc;

    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);

    if(param->conhdl == gapc_get_conhdl(tipc_env->con_info.conidx))
    {
        //Next DST Change Service Characteristic
        if (((param->char_code & TIPC_CHAR_NDCS_MASK) == TIPC_CHAR_NDCS_MASK) &&
            ((param->char_code & ~TIPC_CHAR_NDCS_MASK) < TIPC_CHAR_NDCS_MAX))
        {
            svc = &tipc_env->ndcs.svc;
            search_hdl = tipc_env->ndcs.chars[param->char_code & ~TIPC_CHAR_NDCS_MASK].val_hdl;
        }
        //Reference Time Update Service Characteristic
        else if (((param->char_code & TIPC_CHAR_RTUS_MASK) == TIPC_CHAR_RTUS_MASK) &&
                 ((param->char_code & ~TIPC_CHAR_RTUS_MASK) < TIPC_CHAR_RTUS_MAX))
        {
            svc = &tipc_env->rtus.svc;
            search_hdl = tipc_env->rtus.chars[param->char_code & ~TIPC_CHAR_RTUS_MASK].val_hdl;
        }
        else
        {
            svc = &tipc_env->cts.svc;

            //Current Time Characteristic Descriptor
            if (((param->char_code & TIPC_DESC_CTS_MASK) == TIPC_DESC_CTS_MASK) &&
                ((param->char_code & ~TIPC_DESC_CTS_MASK) < TIPC_DESC_CTS_MAX))
            {
                search_hdl = tipc_env->cts.descs[param->char_code & ~TIPC_DESC_CTS_MASK].desc_hdl;
            }
            //Current Time Service Characteristic
            else if (param->char_code < TIPC_CHAR_CTS_MAX)
            {
                search_hdl = tipc_env->cts.chars[param->char_code].val_hdl;
            }
        }

        // Check if handle is viable
        if (search_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            tipc_env->last_char_code = param->char_code;
            prf_read_char_send(&(tipc_env->con_info), svc->shdl, svc->ehdl, search_hdl);
        }
        else
        {
            tipc_error_ind_send(tipc_env, PRF_ERR_INEXISTENT_HDL);
        }
    }
    else
    {
        tipc_error_ind_send(tipc_env, PRF_ERR_INVALID_PARAM);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPC_CT_NTF_CFG_REQ message.
 * It allows configuration of the peer ind/ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tipc_ct_ntf_cfg_req_handler(ke_msg_id_t const msgid,
                                       struct tipc_ct_ntf_cfg_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    uint16_t cfg_hdl = 0x0000;
    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);

    if(param->conhdl == gapc_get_conhdl(tipc_env->con_info.conidx))
    {
        cfg_hdl = tipc_env->cts.descs[TIPC_DESC_CTS_CURR_TIME_CLI_CFG].desc_hdl;

        //Only NTF
        if(!((param->cfg_val == PRF_CLI_STOP_NTFIND)||(param->cfg_val == PRF_CLI_START_NTF)))
        {
            tipc_error_ind_send(tipc_env, PRF_ERR_INVALID_PARAM);
        }
        else
        {
            //check if the handle value exists
            if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Send GATT Write Request
                prf_gatt_write_ntf_ind(&tipc_env->con_info, cfg_hdl, param->cfg_val);
            }
            else
            {
                tipc_error_ind_send(tipc_env, PRF_ERR_INEXISTENT_HDL);
            }
        }
    }
    else
    {
        tipc_error_ind_send(tipc_env, PRF_ERR_INVALID_PARAM);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPC_WR_TIME_UPD_CTNL_PT_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int tipc_wr_time_upd_ctnl_pt_req_handler(ke_msg_id_t const msgid,
                                                struct tipc_wr_time_udp_ctnl_pt_req const *param,
                                                ke_task_id_t const dest_id,
                                                ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);

    // Check provided parameters
    if ((param->conhdl == gapc_get_conhdl(tipc_env->con_info.conidx)) &&
        ((param->value == TIPS_TIME_UPD_CTNL_PT_GET) || (param->value == TIPS_TIME_UPD_CTNL_PT_CANCEL)))
    {
        if (tipc_env->cts.chars[TIPC_CHAR_RTUS_TIME_UPD_CTNL_PT].char_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            // Send GATT Write Request
            prf_gatt_write(&tipc_env->con_info, tipc_env->rtus.chars[TIPC_CHAR_RTUS_TIME_UPD_CTNL_PT].val_hdl,
                           (uint8_t *)&param->value, sizeof(uint8_t), GATTC_WRITE_NO_RESPONSE);
        }
        //send app error indication for inexistent handle for this characteristic
        else
        {
            tipc_error_ind_send(tipc_env, PRF_ERR_INEXISTENT_HDL);
        }
    }
    else
    {
        tipc_error_ind_send(tipc_env, PRF_ERR_INVALID_PARAM);
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
static int gattc_disc_svc_ind_handler(ke_msg_id_t const msgid, struct gattc_disc_svc_ind const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);

    //even if we get multiple responses we only store 1 range
    if (tipc_env->last_uuid_req == ATT_SVC_CURRENT_TIME)
    {
        tipc_env->cts.svc.shdl = param->start_hdl;
        tipc_env->cts.svc.ehdl = param->end_hdl;
        tipc_env->nb_svc++;
    }
    //should have just one range for this service - use first one only
    else if (tipc_env->last_uuid_req == ATT_SVC_NEXT_DST_CHANGE)
    {
        tipc_env->ndcs.svc.shdl = param->start_hdl;
        tipc_env->ndcs.svc.ehdl = param->end_hdl;
        tipc_env->nb_svc++;
    }
    //should have just one range for this service - use first one only
    else if (tipc_env->last_uuid_req == ATT_SVC_REF_TIME_UPDATE)
    {
        tipc_env->rtus.svc.shdl = param->start_hdl;
        tipc_env->rtus.svc.ehdl = param->end_hdl;
        tipc_env->nb_svc++;
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
    struct tipc_env_tag *tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);

    if(tipc_env->last_svc_req == ATT_SVC_CURRENT_TIME)
    {
        // Retrieve CTS characteristics
        prf_search_chars(tipc_env->cts.svc.ehdl, TIPC_CHAR_CTS_MAX,
                         &tipc_env->cts.chars[0], &tipc_cts_char[0],
                         param, &tipc_env->last_char_code);
    }
    else if(tipc_env->last_svc_req == ATT_SVC_NEXT_DST_CHANGE)
    {
        // Retrieve NDCS characteristics
        prf_search_chars(tipc_env->ndcs.svc.ehdl, TIPC_CHAR_NDCS_MAX,
                         &tipc_env->ndcs.chars[0], &tipc_ndcs_char[0],
                         param, &tipc_env->last_char_code);
    }
    else if(tipc_env->last_svc_req == ATT_SVC_REF_TIME_UPDATE)
    {
        // Retrieve RTUS characteristics
        prf_search_chars(tipc_env->rtus.svc.ehdl, TIPC_CHAR_RTUS_MAX,
                         &tipc_env->rtus.chars[0], &tipc_rtus_char[0],
                         param, &tipc_env->last_char_code);
    }

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
static int gattc_disc_char_desc_ind_handler(ke_msg_id_t const msgid,
                                           struct gattc_disc_char_desc_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);

    // Retrieve CTS descriptors
    prf_search_descs(TIPC_DESC_CTS_MAX, &tipc_env->cts.descs[0], &tipc_cts_char_desc[0],
                     param, tipc_env->last_char_code);

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
    struct tipc_env_tag *tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);
    if(state == TIPC_DISCOVERING)
    {
        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
                (param->status == ATT_ERR_NO_ERROR))
        {
            // Currently Discovering Current Time service characteristics.
            if(tipc_env->last_svc_req  == ATT_SVC_CURRENT_TIME)
            {
                // service start/end handles has been received
                if(tipc_env->last_uuid_req == ATT_SVC_CURRENT_TIME)
                {
                    // check if service handles are not ok
                    if (tipc_env->cts.svc.shdl == ATT_INVALID_HANDLE)
                    {
                        // stop discovery procedure.
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                    }
                    //too many services found only one such service should exist
                    else if(tipc_env->nb_svc != 1)
                    {
                        // stop discovery procedure.
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, PRF_ERR_MULTIPLE_SVC);
                    }
                    // check if service handles are ok
                    else
                    {
                        //discover all CTS characteristics
                        prf_disc_char_all_send(&(tipc_env->con_info), &(tipc_env->cts.svc));
                        tipc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    }

                }
                else if(tipc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
                {
                    status = prf_check_svc_char_validity(TIPC_CHAR_CTS_MAX, tipc_env->cts.chars,
                            tipc_cts_char);

                    //  check for characteristic properties.
                    if(status == PRF_ERR_OK)
                    {
                        tipc_env->last_uuid_req = ATT_INVALID_HANDLE;
                        tipc_env->last_char_code = TIPC_CHAR_CTS_CURR_TIME;
                        //request all service characteristic description for CTS
                        prf_disc_char_desc_send(&(tipc_env->con_info), &(tipc_env->cts.chars[TIPC_CHAR_CTS_CURR_TIME]));
                    }
                    else
                    {
                        // stop discovery procedure.
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, status);
                    }
                }
                else
                {
                    status = prf_check_svc_char_desc_validity(TIPC_DESC_CTS_MAX,
                            tipc_env->cts.descs,
                            tipc_cts_char_desc,
                            tipc_env->cts.chars);

                    if(status == PRF_ERR_OK)
                    {
                        // reset number of services
                        tipc_env->nb_svc = 0;

                        //start discovering NDCS on peer
                        prf_disc_svc_send(&(tipc_env->con_info), ATT_SVC_NEXT_DST_CHANGE);

                        tipc_env->last_uuid_req = ATT_SVC_NEXT_DST_CHANGE;
                        tipc_env->last_svc_req  = ATT_SVC_NEXT_DST_CHANGE;
                    }
                    else
                    {
                        // stop discovery procedure.
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, status);
                    }
                }
            }

            // Currently Next DST Change service characteristics.
            else if(tipc_env->last_svc_req  == ATT_SVC_NEXT_DST_CHANGE)
            {
                // service start/end handles has been received
                if(tipc_env->last_uuid_req == ATT_SVC_NEXT_DST_CHANGE)
                {
                    if (tipc_env->ndcs.svc.shdl == ATT_INVALID_HANDLE)
                    {
                        // reset number of services
                        tipc_env->nb_svc = 0;

                        //start discovering RTUS on peer
                        prf_disc_svc_send(&(tipc_env->con_info), ATT_SVC_REF_TIME_UPDATE);

                        tipc_env->last_uuid_req = ATT_SVC_REF_TIME_UPDATE;
                        tipc_env->last_svc_req  = ATT_SVC_REF_TIME_UPDATE;
                    }
                    //too many services found only one such service should exist
                    else if(tipc_env->nb_svc != 1)
                    {
                        // stop discovery procedure.
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, PRF_ERR_MULTIPLE_SVC);
                    }
                    // check if service handles are ok
                    else
                    {
                        //discover all NDCS characteristics
                        prf_disc_char_all_send(&(tipc_env->con_info), &(tipc_env->ndcs.svc));
                        tipc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    }
                }
                else if(tipc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
                {
                    status = prf_check_svc_char_validity(TIPC_CHAR_NDCS_MAX, tipc_env->ndcs.chars,
                            tipc_ndcs_char);

                    //  check for characteristic properties.
                    if(status == PRF_ERR_OK)
                    {
                        // reset number of services
                        tipc_env->nb_svc = 0;

                        //start discovering RTUS on peer
                        prf_disc_svc_send(&(tipc_env->con_info), ATT_SVC_REF_TIME_UPDATE);

                        tipc_env->last_uuid_req = ATT_SVC_REF_TIME_UPDATE;
                        tipc_env->last_svc_req  = ATT_SVC_REF_TIME_UPDATE;
                    }
                    else
                    {
                        // stop discovery procedure.
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, status);
                    }
                }
            }

            // Currently Reference Time Update service characteristics.
            else if(tipc_env->last_svc_req  == ATT_SVC_REF_TIME_UPDATE)
            {
                // service start/end handles has been received
                if(tipc_env->last_uuid_req == ATT_SVC_REF_TIME_UPDATE)
                {
                    if (tipc_env->rtus.svc.shdl == ATT_INVALID_HANDLE)
                    {
                        // send app the details about the discovered TIPS DB to save
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, PRF_ERR_OK);
                    }
                    //too many services found only one such service should exist
                    else if(tipc_env->nb_svc != 1)
                    {
                        // stop discovery procedure.
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, PRF_ERR_MULTIPLE_SVC);
                    }
                    // check if service handles are ok
                    else
                    {
                        //discover all RTUS characteristics
                        prf_disc_char_all_send(&(tipc_env->con_info), &(tipc_env->rtus.svc));
                        tipc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    }
                }
                else if(tipc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
                {
                    status = prf_check_svc_char_validity(TIPC_CHAR_RTUS_MAX, tipc_env->rtus.chars,
                            tipc_rtus_char);

                    //  check for characteristic properties.
                    if(status == PRF_ERR_OK)
                    {
                        // send app the details about the discovered TIPS DB to save
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, PRF_ERR_OK);
                    }
                    else
                    {
                        // stop discovery procedure.
                        tipc_enable_cfm_send(tipc_env, &tipc_env->con_info, status);
                    }
                }
            }
        }
    }
    else if(state == TIPC_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                struct tipc_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(TIPC_WR_CHAR_RSP,
                        tipc_env->con_info.appid, dest_id, tipc_wr_char_rsp);

                wr_cfm->conhdl    = gapc_get_conhdl(tipc_env->con_info.conidx);
                //it will be a GATT status code
                wr_cfm->status    = param->status;
                // send the message
                ke_msg_send(wr_cfm);
            }
            break;
            case GATTC_READ:
            {
                // an error occurs while reading peer attribute, inform app
                if(param->status != GATT_ERR_NO_ERROR)
                {
                    tipc_error_ind_send(tipc_env, param->status);
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
    struct tipc_env_tag *tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);

    //Current Time Characteristic
    if (tipc_env->last_char_code == TIPC_RD_CTS_CURR_TIME)
    {
        //Build a TIPC_CT_IND message
        struct tipc_ct_ind * ind = KE_MSG_ALLOC(TIPC_CT_IND,
                                                tipc_env->con_info.appid, dest_id,
                                                tipc_ct_ind);

        // retrieve connection handle
        ind->conhdl = gapc_get_conhdl(tipc_env->con_info.conidx);
        // Unpack Current Time Value.
        tipc_unpack_curr_time_value(&(ind->ct_val), (uint8_t*) &(param->value[0]));
        //Indication Type
        ind->ind_type = TIP_RD_RSP,

        ke_msg_send(ind);
    }
    //Local Time Information Characteristic
    else if (tipc_env->last_char_code == TIPC_RD_CTS_LOCAL_TIME_INFO)
    {
        //Build a TIPC_LTI_RD_RSP message
        struct tipc_lti_rd_rsp * rsp = KE_MSG_ALLOC(TIPC_LTI_RD_RSP,
                                                    tipc_env->con_info.appid, dest_id,
                                                    tipc_lti_rd_rsp);

        // Retrieve Connection handle
        rsp->conhdl = gapc_get_conhdl(tipc_env->con_info.conidx);
        // Local Time Information Value
        rsp->lti_val.time_zone         = param->value[0];
        rsp->lti_val.dst_offset         = param->value[1];

        ke_msg_send(rsp);
    }
    //Reference Time Information Characteristic
    else if (tipc_env->last_char_code == TIPC_RD_CTS_REF_TIME_INFO)
    {
        //Build a TIPC_RTI_RD_RSP message
        struct tipc_rti_rd_rsp * rsp = KE_MSG_ALLOC(TIPC_RTI_RD_RSP,
                                                    tipc_env->con_info.appid, dest_id,
                                                    tipc_rti_rd_rsp);

        // Retrieve Connection handle
        rsp->conhdl = gapc_get_conhdl(tipc_env->con_info.conidx);
        // Reference Time Information Value
        rsp->rti_val.time_source      = param->value[0];
        rsp->rti_val.time_accuracy    = param->value[1];
        rsp->rti_val.days_update      = param->value[2];
        rsp->rti_val.hours_update     = param->value[3];

        ke_msg_send(rsp);
    }
    //Time with DST Characteristic
    else if (tipc_env->last_char_code == TIPC_RD_NDCS_TIME_WITH_DST)
    {
        //Build a TIPC_TDST_RD_RSP message
        struct tipc_tdst_rd_rsp * rsp = KE_MSG_ALLOC(TIPC_TDST_RD_RSP,
                                                     tipc_env->con_info.appid, dest_id,
                                                     tipc_tdst_rd_rsp);

        // Retrieve Connection handle
        rsp->conhdl = gapc_get_conhdl(tipc_env->con_info.conidx);
        // Time with DST Value
        prf_unpack_date_time((uint8_t*) &(param->value[0]), &(rsp->tdst_val.date_time));
        rsp->tdst_val.dst_offset        = param->value[7];

        ke_msg_send(rsp);
    }
    //Time Update State Characteristic
    else if (tipc_env->last_char_code == TIPC_RD_RTUS_TIME_UPD_STATE)
    {
        //Build a TIPC_TUS_RD_RSP message
        struct tipc_tus_rd_rsp * rsp = KE_MSG_ALLOC(TIPC_TUS_RD_RSP,
                                                    tipc_env->con_info.appid, dest_id,
                                                    tipc_tus_rd_rsp);

        // Retrieve Connection handle
        rsp->conhdl = gapc_get_conhdl(tipc_env->con_info.conidx);
        // Reference Time Information Value
        rsp->tus_val.current_state     = param->value[0];
        rsp->tus_val.result            = param->value[1];

        ke_msg_send(rsp);
    }
    //Current Time Characteristic - Client Characteristic Configuration Descriptor
    else if (tipc_env->last_char_code == TIPC_RD_CTS_CURR_TIME_CLI_CFG)
    {
        //Build a TIPC_CT_NTF_CFG_RD_RSP message
        struct tipc_ct_ntf_cfg_rd_rsp * rsp = KE_MSG_ALLOC(TIPC_CT_NTF_CFG_RD_RSP,
                                                           tipc_env->con_info.appid, dest_id,
                                                           tipc_ct_ntf_cfg_rd_rsp);

        // Retrieve Connection handle
        rsp->conhdl = gapc_get_conhdl(tipc_env->con_info.conidx);
        // Notification Configuration
        memcpy(&rsp->ntf_cfg, &param->value[0], sizeof(rsp->ntf_cfg));

        ke_msg_send(rsp);
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
    struct tipc_env_tag *tipc_env = PRF_CLIENT_GET_ENV(dest_id, tipc);

    if(KE_IDX_GET(src_id) == tipc_env->con_info.conidx)
    {
        if(param->handle == tipc_env->cts.chars[TIPC_CHAR_CTS_CURR_TIME].val_hdl)
        {
            //Build a TIPC_CT_IND message
            struct tipc_ct_ind * ind = KE_MSG_ALLOC(TIPC_CT_IND,
                                                    tipc_env->con_info.appid, dest_id,
                                                    tipc_ct_ind);
            // retrieve connection handle
            ind->conhdl = gapc_get_conhdl(tipc_env->con_info.conidx);

            // Unpack Current Time Value.
            tipc_unpack_curr_time_value(&(ind->ct_val), (uint8_t*) param->value);

            // Indication Type
            ind->ind_type = TIP_NTF;

            ke_msg_send(ind);
        }
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to TIPC.
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
    PRF_CLIENT_DISABLE_IND_SEND(tipc_envs, dest_id, TIPC, param->conhdl);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler tipc_connected[] =
{
    {TIPC_RD_CHAR_REQ,                  (ke_msg_func_t)tipc_rd_char_req_handler},
    {TIPC_CT_NTF_CFG_REQ,               (ke_msg_func_t)tipc_ct_ntf_cfg_req_handler},
    {TIPC_WR_TIME_UPD_CTNL_PT_REQ,      (ke_msg_func_t)tipc_wr_time_upd_ctnl_pt_req_handler},
    {GATTC_READ_IND,                    (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,                   (ke_msg_func_t)gattc_event_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler tipc_discovering[] =
{
    {GATTC_DISC_SVC_IND,            (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,           (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,      (ke_msg_func_t)gattc_disc_char_desc_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler tipc_default_state[] =
{
    {TIPC_ENABLE_REQ,        (ke_msg_func_t)tipc_enable_req_handler},
    {GAPC_DISCONNECT_IND,    (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,          (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler tipc_state_handler[TIPC_STATE_MAX] =
{
    [TIPC_IDLE]        = KE_STATE_HANDLER_NONE,
    [TIPC_CONNECTED]   = KE_STATE_HANDLER(tipc_connected),
    [TIPC_DISCOVERING] = KE_STATE_HANDLER(tipc_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler tipc_default_handler = KE_STATE_HANDLER(tipc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t tipc_state[TIPC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* (BLE_TIP_CLIENT) */
/// @} TIPCTASK
