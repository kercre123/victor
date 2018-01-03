/**
 ****************************************************************************************
 *
 * @file disc_task.c
 *
 * @brief Device Information Service Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup DISCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_DIS_CLIENT)
#include "gap.h"
#include "attm.h"
#include "disc.h"
#include "disc_task.h"
#include "gattc_task.h"
#include "prf_utils.h"

/// State machine used to retrieve Device Information Service characteristics information
const struct prf_char_def disc_dis_char[DISC_CHAR_MAX] =
{
    /// Manufacturer Name
    [DISC_MANUFACTURER_NAME_CHAR]       = {ATT_CHAR_MANUF_NAME,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
    /// Model Number String
    [DISC_MODEL_NB_STR_CHAR]            = {ATT_CHAR_MODEL_NB,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
    /// Serial Number String
    [DISC_SERIAL_NB_STR_CHAR]           = {ATT_CHAR_SERIAL_NB,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
    /// Hardware Revision String
    [DISC_HARD_REV_STR_CHAR]            = {ATT_CHAR_HW_REV,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
    /// Firmware Revision String
    [DISC_FIRM_REV_STR_CHAR]            = {ATT_CHAR_FW_REV,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
    /// TSoftware Revision String
    [DISC_SW_REV_STR_CHAR]              = {ATT_CHAR_SW_REV,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
    /// System ID
    [DISC_SYSTEM_ID_CHAR]               = {ATT_CHAR_SYS_ID,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
    /// IEEE
    [DISC_IEEE_CHAR]                    = {ATT_CHAR_IEEE_CERTIF,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
    /// PnP ID
    [DISC_PNP_ID_CHAR]                  = {ATT_CHAR_PNP_ID,
                                           ATT_OPTIONAL,
                                           ATT_CHAR_PROP_RD},
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref DISC_ENABLE_REQ message.
 * The handler enables the Device Information Service Client Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int disc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct disc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Device Information Service Client Role Task Environment
    struct disc_env_tag *disc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, disc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        disc_env = PRF_CLIENT_GET_ENV(dest_id, disc);

        //Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering HTS on peer
            prf_disc_svc_send(&disc_env->con_info, ATT_SVC_DEVICE_INFO);

            disc_env->last_uuid_req = ATT_SVC_DEVICE_INFO;

            // Go to DISCOVERING state
            ke_state_set(dest_id, DISC_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            disc_env->dis = param->dis;

            //send APP confirmation that can start normal connection to TH
            disc_enable_cfm_send(disc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        disc_enable_cfm_send(NULL, &con_info, status);
    }


    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_SVC_IND_HANDLER message.
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
    struct disc_env_tag *disc_env = PRF_CLIENT_GET_ENV(dest_id, disc);

    //Even if we get multiple responses we only store 1 range
    disc_env->dis.svc.shdl = param->start_hdl;
    disc_env->dis.svc.ehdl = param->end_hdl;
    disc_env->nb_svc++;

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
    struct disc_env_tag *disc_env = PRF_CLIENT_GET_ENV(dest_id, disc);

    // Retrieve DIS characteristics
    prf_search_chars(disc_env->dis.svc.ehdl, DISC_CHAR_MAX,
            &disc_env->dis.chars[0], &disc_dis_char[0],
            param, &disc_env->last_char_code);


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
    struct disc_env_tag *disc_env = PRF_CLIENT_GET_ENV(dest_id, disc);
    uint8_t status;

    if(state == DISC_DISCOVERING)
    {
        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
                (param->status == ATT_ERR_NO_ERROR))
        {
            // service start/end handles has been received
            if(disc_env->last_uuid_req == ATT_SVC_DEVICE_INFO)
            {
                // check if service handles are not ok
                if (disc_env->dis.svc.shdl == ATT_INVALID_HANDLE)
                {
                    // stop discovery procedure.
                    disc_enable_cfm_send(disc_env, &disc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                //too many services found only one such service should exist
                else if(disc_env->nb_svc > 1)
                {
                    // stop discovery procedure.
                    disc_enable_cfm_send(disc_env, &disc_env->con_info, PRF_ERR_MULTIPLE_SVC);
                }
                // check if service handles are ok
                else
                {
                    //discover all HTS characteristics
                    disc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    prf_disc_char_all_send(&(disc_env->con_info), &(disc_env->dis.svc));
                }
            }
            else //Characteristics
            {
                status = prf_check_svc_char_validity(DISC_CHAR_MAX, disc_env->dis.chars,
                        disc_dis_char);

                disc_enable_cfm_send(disc_env, &disc_env->con_info, status);
            }
        }
        else
        {
            disc_enable_cfm_send(disc_env, &disc_env->con_info, param->status);
        }
    }
    else if (state == DISC_CONNECTED)
    {
        // an error occurs while performing read request
        if((param->req_type == GATTC_READ) && (param->status != GATT_ERR_NO_ERROR))
        {
            prf_client_att_info_rsp((prf_env_struct*)disc_env, DISC_RD_CHAR_RSP,
                    param->status, NULL);
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
static int disc_rd_char_req_handler(ke_msg_id_t const msgid,
                                    struct disc_rd_char_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct disc_env_tag *disc_env = PRF_CLIENT_GET_ENV(dest_id, disc);

    uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;

    if(param->conhdl == gapc_get_conhdl(disc_env->con_info.conidx))
    {
        if (param->char_code < DISC_CHAR_MAX)
        {
            search_hdl = disc_env->dis.chars[param->char_code].val_hdl;
        }

        //Check if handle is viable
        if (search_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            disc_env->last_char_code = param->char_code;
            prf_read_char_send(&(disc_env->con_info), disc_env->dis.svc.shdl,
                               disc_env->dis.svc.ehdl, search_hdl);
        }
        else
        {
            prf_client_att_info_rsp((prf_env_struct*)disc_env, DISC_RD_CHAR_RSP,
                    PRF_ERR_INEXISTENT_HDL, NULL);
        }
    }
    else
    {
        prf_client_att_info_rsp((prf_env_struct*)disc_env, DISC_RD_CHAR_RSP,
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
    struct disc_env_tag *disc_env = PRF_CLIENT_GET_ENV(dest_id, disc);

    prf_client_att_info_rsp((prf_env_struct*)disc_env, DISC_RD_CHAR_RSP,
                            GATT_ERR_NO_ERROR, param);

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
    PRF_CLIENT_DISABLE_IND_SEND(disc_envs, dest_id, DISC, param->conhdl);

    // message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler disc_connected[] =
{
    {DISC_RD_CHAR_REQ,              (ke_msg_func_t)disc_rd_char_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
};

// Specifies the Discovering state message handlers
const struct ke_msg_handler disc_discovering[] =
{
    {GATTC_DISC_SVC_IND,     (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,    (ke_msg_func_t)gattc_disc_char_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler disc_default_state[] =
{
    {DISC_ENABLE_REQ,               (ke_msg_func_t)disc_enable_req_handler},
    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler disc_state_handler[DISC_STATE_MAX] =
{
    [DISC_IDLE]        = KE_STATE_HANDLER_NONE,
    [DISC_DISCOVERING] = KE_STATE_HANDLER(disc_discovering),
    [DISC_CONNECTED]   = KE_STATE_HANDLER(disc_connected),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler disc_default_handler = KE_STATE_HANDLER(disc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t disc_state[DISC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_DIS_CLIENT

/// @} DISCTASK
