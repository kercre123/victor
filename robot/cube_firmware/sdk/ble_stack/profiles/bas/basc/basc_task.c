/**
 ****************************************************************************************
 *
 * @file basc_task.c
 *
 * @brief Battery Service Client Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BASCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BAS_CLIENT)

#include "gap.h"
#include "attm.h"
#include "basc.h"
#include "basc_task.h"
#include "gattc_task.h"
#include "prf_utils.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// State machine used to retrieve Battery Service characteristics information
const struct prf_char_def basc_bas_char[BAS_CHAR_MAX] =
{
    /// Battery Level
    [BAS_CHAR_BATT_LEVEL]  = {ATT_CHAR_BATTERY_LEVEL,
                              ATT_MANDATORY,
                              ATT_CHAR_PROP_RD},
};

/// State machine used to retrieve Battery Service characteristic description information
const struct prf_char_desc_def basc_bas_char_desc[BAS_DESC_MAX] =
{
    /// Battery Level Characteristic Presentation Format
    [BAS_DESC_BATT_LEVEL_PRES_FORMAT]   = {ATT_DESC_CHAR_PRES_FORMAT,  ATT_OPTIONAL, BAS_CHAR_BATT_LEVEL},
    /// Battery Level Client Config
    [BAS_DESC_BATT_LEVEL_CFG]           = {ATT_DESC_CLIENT_CHAR_CFG,   ATT_OPTIONAL, BAS_CHAR_BATT_LEVEL},

};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BASC_ENABLE_REQ message.
 * The handler enables the Battery Service Client Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int basc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct basc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Battery Service Client Role Task Environment
    struct basc_env_tag *basc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status = PRF_CLIENT_ENABLE(con_info, param, basc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        basc_env = PRF_CLIENT_GET_ENV(dest_id, basc);

        // Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            // Start discovering BAS on peer
            prf_disc_svc_send(&(basc_env->con_info), ATT_SVC_BATTERY_SERVICE);

            basc_env->last_uuid_req = ATT_SVC_BATTERY_SERVICE;

            // Go to DISCOVERING state
            ke_state_set(dest_id, BASC_DISCOVERING);
        }
        // Normal connection, get saved att details
        else
        {
            memcpy(&basc_env->bas[0], &param->bas[0], BASC_NB_BAS_INSTANCES_MAX*sizeof(struct bas_content));
            basc_env->bas_nb = param->bas_nb;

            // Send APP confirmation that can start normal connection to TH
            basc_enable_cfm_send(basc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        // An error has been raised during the process, env is NULL and won't be handled
        basc_enable_cfm_send(NULL, &con_info, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BASC_CHAR_RD_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int basc_char_rd_req_handler(ke_msg_id_t const msgid,
                                               struct basc_char_rd_req const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
    uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;
    // Get the address of the environment
    struct basc_env_tag *basc_env = PRF_CLIENT_GET_ENV(dest_id, basc);

    // Check Connection Handle, BAS Instance
    if((param->conhdl == gapc_get_conhdl(basc_env->con_info.conidx)) &&
       (param->bas_nb < BASC_NB_BAS_INSTANCES_MAX))
    {
        //Battery Level Service Descriptor
        if(((param->char_code & BAS_DESC_MASK) == BAS_DESC_MASK) &&
           ((param->char_code & ~BAS_DESC_MASK) < BAS_DESC_MASK))
        {
            search_hdl = basc_env->bas[param->bas_nb].descs[param->char_code & ~BAS_DESC_MASK].desc_hdl;
        }
        //Battery Service Characteristic
        else if (param->char_code < BAS_CHAR_MAX)
        {
            search_hdl = basc_env->bas[param->bas_nb].chars[param->char_code].val_hdl;
        }

        // Check if handle is viable
        if (search_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            prf_read_char_send(&(basc_env->con_info), basc_env->bas[param->bas_nb].svc.shdl,
                               basc_env->bas[param->bas_nb].svc.ehdl, search_hdl);
            // Save the service instance number
            basc_env->last_svc_inst_req  = param->bas_nb;
            // Save the attribute read code
            basc_env->last_char_code     = param->char_code;
        }
        else
        {
            basc_error_ind_send(basc_env, PRF_ERR_INEXISTENT_HDL);
        }
    }
    else
    {
        basc_error_ind_send(basc_env, PRF_ERR_INVALID_PARAM);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BASC_BATT_LEVEL_NTF_CFG_REQ message.
 * It allows configuration of the peer ntf/stop characteristic for Battery Level Characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int basc_batt_level_ntf_cfg_req_handler(ke_msg_id_t const msgid,
                                               struct basc_batt_level_ntf_cfg_req const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
    // Descriptor handle
    uint16_t cfg_hdl;
    // Get the address of the environment
    struct basc_env_tag *basc_env = PRF_CLIENT_GET_ENV(dest_id, basc);

    if((param->conhdl == gapc_get_conhdl(basc_env->con_info.conidx)) &&
       (param->bas_nb < BASC_NB_BAS_INSTANCES_MAX))
    {
        cfg_hdl = basc_env->bas[param->bas_nb].descs[BAS_DESC_BATT_LEVEL_CFG].desc_hdl;

        // Check value to write
        if(!((param->ntf_cfg == PRF_CLI_STOP_NTFIND) || (param->ntf_cfg == PRF_CLI_START_NTF)))
        {
            basc_error_ind_send(basc_env, PRF_ERR_INVALID_PARAM);
        }
        else
        {
            // Check if the handle value exists
            if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Send GATT Write Request
                prf_gatt_write_ntf_ind(&basc_env->con_info, cfg_hdl, param->ntf_cfg);
            }
            else
            {
                basc_error_ind_send(basc_env, PRF_ERR_INEXISTENT_HDL);
            }
        }
    }
    else
    {
        basc_error_ind_send(basc_env, PRF_ERR_INVALID_PARAM);
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
    struct basc_env_tag *basc_env = PRF_CLIENT_GET_ENV(dest_id, basc);

    if (basc_env->bas_nb < BASC_NB_BAS_INSTANCES_MAX)
    {
        basc_env->bas[basc_env->bas_nb].svc.shdl = param->start_hdl;
        basc_env->bas[basc_env->bas_nb].svc.ehdl = param->end_hdl;
    }

    // Update number of found BAS instances
    basc_env->bas_nb++;

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
    struct basc_env_tag *basc_env = PRF_CLIENT_GET_ENV(dest_id, basc);

    // Retrieve BAS characteristics
    prf_search_chars(basc_env->bas[basc_env->last_svc_inst_req].svc.ehdl, BAS_CHAR_MAX,
            &basc_env->bas[basc_env->last_svc_inst_req].chars[0], &basc_bas_char[0],
            param, &basc_env->last_char_code);

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
    struct basc_env_tag *basc_env = PRF_CLIENT_GET_ENV(dest_id, basc);

    // Retrieve BAS descriptors
    prf_search_descs(BAS_DESC_MAX, &basc_env->bas[basc_env->last_svc_inst_req].descs[0],
                     &basc_bas_char_desc[0], param, basc_env->last_char_code);

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
    struct basc_env_tag *basc_env = PRF_CLIENT_GET_ENV(dest_id, basc);

    if(state == BASC_DISCOVERING)
    {
        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                (param->status == ATT_ERR_NO_ERROR))
        {
            // SERVICES
            if (basc_env->last_uuid_req == ATT_SVC_BATTERY_SERVICE)
            {
                // If no BAS instance has been found
                if (basc_env->bas_nb == 0)
                {
                    // Stop discovery procedure.
                    basc_enable_cfm_send(basc_env, &basc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                else
                {
                    // Discover all BAS characteristics for first service instance
                    basc_env->last_uuid_req      = ATT_DECL_CHARACTERISTIC;
                    basc_env->last_svc_inst_req  = 0;

                    prf_disc_char_all_send(&(basc_env->con_info), &(basc_env->bas[0].svc));
                }
            }

            // CHARACTERISTICS
            else if (basc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
            {
                status = prf_check_svc_char_validity(BAS_CHAR_MAX, basc_env->bas[basc_env->last_svc_inst_req].chars,
                        basc_bas_char);

                if (status == PRF_ERR_OK)
                {
                    basc_env->last_uuid_req  = ATT_INVALID_HANDLE;
                    basc_env->last_char_code = basc_bas_char_desc[BAS_DESC_BATT_LEVEL_CFG].char_code;

                    // Request all service characteristic descriptor for BAS
                    prf_disc_char_desc_send(&(basc_env->con_info), &(basc_env->bas[basc_env->last_svc_inst_req].chars[BAS_CHAR_BATT_LEVEL]));
                }
                else
                {
                    // Stop discovery procedure.
                    basc_enable_cfm_send(basc_env, &basc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
            }

            // DESCRIPTORS
            else
            {
                //Check if Characteristic Presentation Format is present
                if (basc_env->bas_nb > 1)
                {
                    // The Characteristic Presentation Format descriptor must be present if several BAS instances are present
                    if (basc_env->bas[basc_env->last_svc_inst_req].descs[BAS_DESC_BATT_LEVEL_PRES_FORMAT].desc_hdl == ATT_INVALID_HANDLE)
                    {
                        // stop discovery procedure.
                        basc_enable_cfm_send(basc_env, &basc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                        return (KE_MSG_CONSUMED);
                    }
                }

                //Check if Client Characteristic Configuration is present
                if ((basc_env->bas[basc_env->last_svc_inst_req].chars[BAS_CHAR_BATT_LEVEL].prop & ATT_CHAR_PROP_NTF)
                        == ATT_CHAR_PROP_NTF)
                {
                    // The Client Characteristic Configuration descriptor must be present if the characteristic supports NTFs
                    if (basc_env->bas[basc_env->last_svc_inst_req].descs[BAS_DESC_BATT_LEVEL_CFG].desc_hdl == ATT_INVALID_HANDLE)
                    {
                        // Stop discovery procedure.
                        basc_enable_cfm_send(basc_env, &basc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                        return (KE_MSG_CONSUMED);
                    }
                }

                if (basc_env->last_svc_inst_req < (basc_env->bas_nb-1))
                {
                    // Discover BAS Characteristics for the next instance
                    basc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    basc_env->last_svc_inst_req++;

                    prf_disc_char_all_send(&(basc_env->con_info), &(basc_env->bas[basc_env->last_svc_inst_req].svc));
                }
                else
                {
                    // send app the details about the discovered BAPS DB to save
                    basc_enable_cfm_send(basc_env, &basc_env->con_info, PRF_ERR_OK);
                }

            }
        }
    }
    else if(state == BASC_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                struct basc_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(BASC_WR_CHAR_RSP,
                        basc_env->con_info.appid, dest_id,
                        basc_wr_char_rsp);

                wr_cfm->conhdl    = gapc_get_conhdl(basc_env->con_info.conidx);
                // It will be a GATT status code
                wr_cfm->status    = param->status;

                // Send the message
                ke_msg_send(wr_cfm);
            }
            break;

            case GATTC_READ:
            {
                if(param->status != GATT_ERR_NO_ERROR)
                {
                    basc_error_ind_send(basc_env, param->status);
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
    // Status
    uint8_t error_status = PRF_ERR_OK;
    // Get the address of the environment
    struct basc_env_tag *basc_env = PRF_CLIENT_GET_ENV(dest_id, basc);

    // Battery Level Value
    if (basc_env->last_char_code == BASC_RD_BAS_BATT_LEVEL)
    {
        struct basc_batt_level_ind *ind = KE_MSG_ALLOC(BASC_BATT_LEVEL_IND,
                basc_env->con_info.appid, dest_id,
                basc_batt_level_ind);

        ind->conhdl     = gapc_get_conhdl(basc_env->con_info.conidx);
        ind->ind_type   = BASC_BATT_LEVEL_RD_RSP;
        ind->batt_level = param->value[0];
        ind->bas_nb     = basc_env->last_svc_inst_req;

        ke_msg_send(ind);
    }

    // Battery Level Client Characteristic Configuration Descriptor value
    else if (basc_env->last_char_code == BASC_RD_BAS_BATT_LEVEL_CLI_CFG)
    {
        struct basc_batt_level_ntf_cfg_rd_rsp *rsp = KE_MSG_ALLOC(BASC_BATT_LEVEL_NTF_CFG_RD_RSP,
                basc_env->con_info.appid, dest_id,
                basc_batt_level_ntf_cfg_rd_rsp);

        rsp->conhdl     = gapc_get_conhdl(basc_env->con_info.conidx);
        rsp->bas_nb     = basc_env->last_svc_inst_req;
        rsp->ntf_cfg    = co_read16p(&param->value[0]);

        ke_msg_send(rsp);
    }

    // Battery Level Client Presentation Format Descriptor value
    else if (basc_env->last_char_code == BASC_RD_BAS_BATT_LEVEL_PRES_FORMAT)
    {
        struct basc_batt_level_pres_format_rd_rsp *rsp = KE_MSG_ALLOC(BASC_BATT_LEVEL_PRES_FORMAT_RD_RSP,
                basc_env->con_info.appid, dest_id,
                basc_batt_level_pres_format_rd_rsp);

        rsp->conhdl     = gapc_get_conhdl(basc_env->con_info.conidx);
        rsp->bas_nb     = basc_env->last_svc_inst_req;

        prf_unpack_char_pres_fmt(&param->value[0], &rsp->char_pres_format);

        ke_msg_send(rsp);
    }

    //Unsupported Characteristic
    else
    {
        error_status = PRF_ERR_INEXISTENT_HDL;
    }

    // Send Error indication to APP if needed
    if (error_status != PRF_ERR_OK)
    {
        basc_error_ind_send(basc_env, error_status);
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
    // BAS Instance
    uint8_t bas_nb;
    // Attribute found
    uint8_t found = PRF_APP_ERROR;
    // Get the address of the environment
    struct basc_env_tag *basc_env = PRF_CLIENT_GET_ENV(dest_id, basc);

    if (KE_IDX_GET(src_id) == basc_env->con_info.conidx)
    {
        //Battery Level - BAS instance is unknown.
        for (bas_nb = 0; ((bas_nb < basc_env->bas_nb) && (found != PRF_ERR_OK)); bas_nb++)
        {
            if (param->handle == basc_env->bas[bas_nb].chars[BAS_CHAR_BATT_LEVEL].val_hdl)
            {
                found = PRF_ERR_OK;

                struct basc_batt_level_ind * ind = KE_MSG_ALLOC(BASC_BATT_LEVEL_IND,
                                                                basc_env->con_info.appid, dest_id,
                                                                basc_batt_level_ind);

                ind->conhdl     = gapc_get_conhdl(basc_env->con_info.conidx);
                ind->ind_type   = BASC_BATT_LEVEL_NTF;
                ind->batt_level = param->value[0];
                ind->bas_nb     = bas_nb;

                //Send Battery Level value to APP
                ke_msg_send(ind);
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to BAPC.
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
    PRF_CLIENT_DISABLE_IND_SEND(basc_envs, dest_id, BASC, param->conhdl);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler basc_connected[] =
{
    {BASC_CHAR_RD_REQ,              (ke_msg_func_t)basc_char_rd_req_handler},
    {BASC_BATT_LEVEL_NTF_CFG_REQ,   (ke_msg_func_t)basc_batt_level_ntf_cfg_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler basc_discovering[] =
{
    {GATTC_DISC_SVC_IND,            (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,           (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,      (ke_msg_func_t)gatt_disc_char_desc_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler basc_default_state[] =
{
    {BASC_ENABLE_REQ,               (ke_msg_func_t)basc_enable_req_handler},
    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler basc_state_handler[BASC_STATE_MAX] =
{
    [BASC_IDLE]        = KE_STATE_HANDLER_NONE,
    [BASC_CONNECTED]   = KE_STATE_HANDLER(basc_connected),
    [BASC_DISCOVERING] = KE_STATE_HANDLER(basc_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler basc_default_handler = KE_STATE_HANDLER(basc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t basc_state[BASC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* (BLE_BAS_CLIENT) */

/// @} BASCTASK
