/**
 ****************************************************************************************
 *
 * @file hrpc_task.c
 *
 * @brief Heart Rate Profile Collector Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HRPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_HR_COLLECTOR)
#include "gap.h"
#include "attm.h"
#include "hrpc.h"
#include "hrpc_task.h"
#include "gattc_task.h"
#include "smpc_task.h"
#include "hrp_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// State machine used to retrieve heart rate service characteristics information
const struct prf_char_def hrpc_hrs_char[HRPC_CHAR_MAX] =
{
    /// Heart Rate Measurement
    [HRPC_CHAR_HR_MEAS]              = {ATT_CHAR_HEART_RATE_MEAS,
                                        ATT_MANDATORY,
                                        ATT_CHAR_PROP_NTF},
    /// Body Sensor Location
    [HRPC_CHAR_BODY_SENSOR_LOCATION] = {ATT_CHAR_BODY_SENSOR_LOCATION,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_RD},
    /// Heart Rate Control Point
    [HRPC_CHAR_HR_CNTL_POINT]        = {ATT_CHAR_HEART_RATE_CNTL_POINT,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_WR},
};

/// State machine used to retrieve heart rate service characteristic description information
const struct prf_char_desc_def hrpc_hrs_char_desc[HRPC_DESC_MAX] =
{
    /// Heart Rate Measurement client config
    [HRPC_DESC_HR_MEAS_CLI_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, HRPC_CHAR_HR_MEAS},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPC_ENABLE_REQ message.
 * The handler enables the Heart Rate Profile Collector Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hrpc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct hrpc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Heart Rate Profile Client Role Task Environment
    struct hrpc_env_tag *hrpc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status = PRF_CLIENT_ENABLE(con_info, param, hrpc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

        // Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering HRS on peer
            prf_disc_svc_send(&(hrpc_env->con_info), ATT_SVC_HEART_RATE);

            hrpc_env->last_uuid_req = ATT_SVC_HEART_RATE;

            // Go to DISCOVERING state
            ke_state_set(dest_id, HRPC_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            hrpc_env->hrs = param->hrs;
            //send APP confirmation that can start normal connection to TH
            hrpc_enable_cfm_send(hrpc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        hrpc_enable_cfm_send(NULL, &con_info, status);
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
    struct hrpc_env_tag *hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

    //even if we get multiple responses we only store 1 range
    hrpc_env->hrs.svc.shdl = param->start_hdl;
    hrpc_env->hrs.svc.ehdl = param->end_hdl;
    hrpc_env->nb_svc++;

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
    struct hrpc_env_tag *hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

    // Retrieve HRS characteristics
    prf_search_chars(hrpc_env->hrs.svc.ehdl, HRPC_CHAR_MAX,
            &hrpc_env->hrs.chars[0], &hrpc_hrs_char[0],
            param, &hrpc_env->last_char_code);

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
    struct hrpc_env_tag *hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

    // Retrieve HRS descriptors
    prf_search_descs(HRPC_DESC_MAX, &hrpc_env->hrs.descs[0], &hrpc_hrs_char_desc[0],
                     param, hrpc_env->last_char_code);

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
    struct hrpc_env_tag *hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

    if(state == HRPC_DISCOVERING)
    {
    if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
        (param->status == ATT_ERR_NO_ERROR))
    {
        // service start/end handles has been received
        if(hrpc_env->last_uuid_req == ATT_SVC_HEART_RATE)
        {
            // check if service handles are not ok
            if(hrpc_env->hrs.svc.shdl == ATT_INVALID_HANDLE)
            {
                // stop discovery procedure.
                hrpc_enable_cfm_send(hrpc_env, &hrpc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
            }
            //too many services found only one such service should exist
            else if(hrpc_env->nb_svc != 1)
            {
                // stop discovery procedure.
                hrpc_enable_cfm_send(hrpc_env, &hrpc_env->con_info, PRF_ERR_MULTIPLE_SVC);
            }
            else
            {
                //discover all HRS characteristics
                prf_disc_char_all_send(&(hrpc_env->con_info), &(hrpc_env->hrs.svc));
                hrpc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
            }
        }
        else if(hrpc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
        {
            status = prf_check_svc_char_validity(HRPC_CHAR_MAX, hrpc_env->hrs.chars,
                                                 hrpc_hrs_char);

            // Check for characteristic properties.
            if(status == PRF_ERR_OK)
            {
                hrpc_env->last_uuid_req = ATT_INVALID_HANDLE;
                hrpc_env->last_char_code = hrpc_hrs_char_desc[HRPC_DESC_HR_MEAS_CLI_CFG].char_code;

                //Discover Heart Rate Measurement Char. Descriptor - Mandatory
                prf_disc_char_desc_send(&(hrpc_env->con_info),
                                        &(hrpc_env->hrs.chars[hrpc_env->last_char_code]));
            }
            else
            {
                // Stop discovery procedure.
                hrpc_enable_cfm_send(hrpc_env, &hrpc_env->con_info, status);
            }
        }
        else //Descriptors
        {
            status = prf_check_svc_char_desc_validity(HRPC_DESC_MAX,
                                                      hrpc_env->hrs.descs,
                                                      hrpc_hrs_char_desc,
                                                      hrpc_env->hrs.chars);

            hrpc_enable_cfm_send(hrpc_env, &hrpc_env->con_info, status);
        }
    }
    }
    else if(state == HRPC_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                struct hrpc_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(HRPC_WR_CHAR_RSP,
                        hrpc_env->con_info.appid, dest_id,
                        hrpc_wr_char_rsp);
                wr_cfm->conhdl    = gapc_get_conhdl(hrpc_env->con_info.conidx);
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
                    // an error occurs while reading peer device attribute
                    prf_client_att_info_rsp((prf_env_struct*) hrpc_env, HRPC_RD_CHAR_RSP,
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
 * @brief Handles reception of the @ref HRPC_RD_DATETIME_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hrpc_rd_char_req_handler(ke_msg_id_t const msgid,
                                        struct hrpc_rd_char_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

    if(param->conhdl == gapc_get_conhdl(hrpc_env->con_info.conidx))
    {
        if(((param->char_code & HRPC_DESC_MASK) == HRPC_DESC_MASK) &&
           ((param->char_code & ~HRPC_DESC_MASK) < HRPC_DESC_MAX))
        {
            search_hdl = hrpc_env->hrs.descs[param->char_code & ~HRPC_DESC_MASK].desc_hdl;
        }
        else if (param->char_code < HRPC_CHAR_MAX)
        {
            search_hdl = hrpc_env->hrs.chars[param->char_code].val_hdl;
        }

        //check if handle is viable
        if (search_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            hrpc_env->last_char_code = param->char_code;
            prf_read_char_send(&(hrpc_env->con_info), hrpc_env->hrs.svc.shdl, hrpc_env->hrs.svc.ehdl, search_hdl);
        }
        else
        {
            prf_client_att_info_rsp((prf_env_struct*) hrpc_env, HRPC_RD_CHAR_RSP,
                    PRF_ERR_INEXISTENT_HDL, NULL);
        }
    }
    else
    {
        prf_client_att_info_rsp((prf_env_struct*) hrpc_env, HRPC_RD_CHAR_RSP,
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
    struct hrpc_env_tag *hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

    prf_client_att_info_rsp((prf_env_struct*) hrpc_env, HRPC_RD_CHAR_RSP,
            PRF_ERR_OK, param);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPC_CFG_INDNTF_REQ message.
 * It allows configuration of the peer ind/ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hrpc_cfg_indntf_req_handler(ke_msg_id_t const msgid,
                                       struct hrpc_cfg_indntf_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    uint16_t cfg_hdl = 0x0000;
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

    if(param->conhdl == gapc_get_conhdl(hrpc_env->con_info.conidx))
    {
        //get handle of the configuration characteristic to set and check if value matches property
        cfg_hdl = hrpc_env->hrs.descs[HRPC_DESC_HR_MEAS_CLI_CFG].desc_hdl;
        if(!((param->cfg_val == PRF_CLI_STOP_NTFIND)||
                (param->cfg_val == PRF_CLI_START_NTF)))
        {
            hrpc_error_ind_send(hrpc_env, PRF_ERR_INVALID_PARAM);
        }

        //if we get here, then the APP request looks OK

        //check if the handle value exists
        if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            // Send GATT Write Request
            prf_gatt_write_ntf_ind(&hrpc_env->con_info, cfg_hdl, param->cfg_val);
        }
        else
        {
            hrpc_error_ind_send(hrpc_env, PRF_ERR_INEXISTENT_HDL);
        }
    }
    else
    {
        hrpc_error_ind_send(hrpc_env, PRF_ERR_INVALID_PARAM);
    }

    return (KE_MSG_CONSUMED);
}

/*
****************************************************************************************
* @brief Handles reception of the @ref HRPC_WR_CNTL_POINT_REQ message.
* Check if the handle exists in profile(already discovered) and send request, otherwise
* error to APP.
* @param[in] msgid Id of the message received (probably unused).
* @param[in] param Pointer to the parameters of the message.
* @param[in] dest_id ID of the receiving task instance (probably unused).
* @param[in] src_id ID of the sending task instance.
* @return If the message was consumed or not.
****************************************************************************************
*/
static int hrpc_wr_cntl_point_req_handler(ke_msg_id_t const msgid,
                                       struct htpc_wr_cntl_point_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

   //this is mandatory readable if it is included in the peer's DB
   if(param->conhdl == gapc_get_conhdl(hrpc_env->con_info.conidx))
   {
       //this is mandatory readable if it is included in the peer's DB
       if (hrpc_env->hrs.chars[HRPC_CHAR_HR_CNTL_POINT].char_hdl != ATT_INVALID_SEARCH_HANDLE)
       {
           if ((hrpc_env->hrs.chars[HRPC_CHAR_HR_CNTL_POINT].prop & ATT_CHAR_PROP_WR) == ATT_CHAR_PROP_WR)
           {
               // Send GATT Write Request
               prf_gatt_write(&hrpc_env->con_info, hrpc_env->hrs.chars[HRPC_CHAR_HR_CNTL_POINT].val_hdl,
                              (uint8_t *)&param->val, sizeof(uint8_t), GATTC_WRITE);
           }
           //write not allowed, so no point in continuing
           else
           {
               hrpc_error_ind_send(hrpc_env, PRF_ERR_NOT_WRITABLE);
           }
       }
       //send app error indication for inexistent handle for this characteristic
       else
       {
           hrpc_error_ind_send(hrpc_env, PRF_ERR_INEXISTENT_HDL);
       }
   }
   else
   {
       hrpc_error_ind_send(hrpc_env, PRF_ERR_INVALID_PARAM);
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
    struct hrpc_env_tag *hrpc_env = PRF_CLIENT_GET_ENV(dest_id, hrpc);

    if(KE_IDX_GET(src_id) == hrpc_env->con_info.conidx)
    {
        if(param->handle == hrpc_env->hrs.chars[HRPC_CHAR_HR_MEAS].val_hdl)
        {
            //build a HRPC_HR_MEAS_IND message with stable heart rate value code.
            struct hrpc_meas_ind * ind = KE_MSG_ALLOC(HRPC_HR_MEAS_IND,
                                                      hrpc_env->con_info.appid, dest_id,
                                                      hrpc_meas_ind);
            // retrieve connection handle
            ind->conhdl = gapc_get_conhdl(hrpc_env->con_info.conidx);

            // unpack heart rate measurement.
            hrpc_unpack_meas_value(&(ind->meas_val), (uint8_t*) param->value, param->length);

            ke_msg_send(ind);
        }
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to HRPC.
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
    PRF_CLIENT_DISABLE_IND_SEND(hrpc_envs, dest_id, HRPC, param->conhdl);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler hrpc_connected[] =
{
    {HRPC_RD_CHAR_REQ,       (ke_msg_func_t)hrpc_rd_char_req_handler},
    {GATTC_READ_IND,         (ke_msg_func_t)gattc_read_ind_handler},
    {HRPC_CFG_INDNTF_REQ,    (ke_msg_func_t)hrpc_cfg_indntf_req_handler},
    {HRPC_WR_CNTL_POINT_REQ, (ke_msg_func_t)hrpc_wr_cntl_point_req_handler},
    {GATTC_EVENT_IND,        (ke_msg_func_t)gattc_event_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler hrpc_discovering[] =
{
    {GATTC_DISC_SVC_IND,         (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,        (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,   (ke_msg_func_t)gattc_disc_char_desc_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler hrpc_default_state[] =
{
    {HRPC_ENABLE_REQ,        (ke_msg_func_t)hrpc_enable_req_handler},
    {GAPC_DISCONNECT_IND,    (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,          (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler hrpc_state_handler[HRPC_STATE_MAX] =
{
    [HRPC_IDLE]        = KE_STATE_HANDLER_NONE,
    [HRPC_CONNECTED]   = KE_STATE_HANDLER(hrpc_connected),
    [HRPC_DISCOVERING] = KE_STATE_HANDLER(hrpc_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler hrpc_default_handler = KE_STATE_HANDLER(hrpc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t hrpc_state[HRPC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* (BLE_HR_COLLECTOR) */
/// @} HRPCTASK
