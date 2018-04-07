/**
 ****************************************************************************************
 *
 * @file findl_task.c
 *
 * @brief Find Me Locator Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup FINDLTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_FINDME_LOCATOR)
#include "findl.h"
#include "findl_task.h"
#include "gattc_task.h"
#include "prf_utils.h"
#include "gap.h"
#include "gapc.h"
#include "gapc_task.h"

/// State machine used to retrieve heart rate service characteristics information
const struct prf_char_def findl_ias_char = {ATT_CHAR_ALERT_LEVEL,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_WR_NO_RESP};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref FINDL_ENABLE_REQ message.
 * The handler enables the Find Me profile - Locator Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int findl_enable_req_handler(ke_msg_id_t const msgid,
                                    struct findl_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Find Me Profile Client Role Task Environment
    struct findl_env_tag *findl_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, findl);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        findl_env = PRF_CLIENT_GET_ENV(dest_id, findl);

        // If it's a discovery connection, start discovery
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering HRS on peer
            prf_disc_svc_send(&(findl_env->con_info), ATT_SVC_IMMEDIATE_ALERT);

            //set what we are discovering in environment
            findl_env->last_uuid_req = ATT_SVC_IMMEDIATE_ALERT;

            // Go to connected state
            ke_state_set(dest_id, FINDL_DISCOVERING);
        }
        // Confirm to app that everything is OK
        else
        {
            //copy over the info app has kept
            findl_env->ias = param->ias;

            findl_enable_cfm_send(findl_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        findl_enable_cfm_send(NULL, &con_info, status);
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
    struct findl_env_tag *findl_env = PRF_CLIENT_GET_ENV(dest_id, findl);

    // Even if we get multiple responses we only store 1 range
    findl_env->ias.svc.shdl = param->start_hdl;
    findl_env->ias.svc.ehdl = param->end_hdl;
    findl_env->nb_svc++;


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
    struct findl_env_tag *findl_env = PRF_CLIENT_GET_ENV(dest_id, findl);

    // Retrieve IAS characteristics - Only 1 Char.
    prf_search_chars(findl_env->ias.svc.ehdl, 1,
            &findl_env->ias.alert_lvl_char, &findl_ias_char,
            param, &findl_env->last_char_code);


    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref FINDL_SET_ALERT_REQ message.
 * The handler disables the Find Me profile - Target Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int findl_set_alert_req_handler(ke_msg_id_t const msgid,
                                       struct findl_set_alert_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct findl_env_tag *findl_env = PRF_CLIENT_GET_ENV(dest_id, findl);
    // Status
    uint8_t status = PRF_ERR_OK;

    if(param->conhdl == gapc_get_conhdl(findl_env->con_info.conidx))
    {
        //Check we're sending a correct value
        if ((param->alert_lvl == FINDL_ALERT_NONE)   ||
            (param->alert_lvl == FINDL_ALERT_MILD)   ||
            (param->alert_lvl == FINDL_ALERT_HIGH))
        {
            if(findl_env->ias.alert_lvl_char.char_hdl != 0x0000)
            {
                // Send GATT Write Request
                prf_gatt_write(&findl_env->con_info,
                               findl_env->ias.alert_lvl_char.val_hdl,
                               (uint8_t *)&param->alert_lvl, sizeof(uint8_t), GATTC_WRITE_NO_RESPONSE);
            }
            else
            {
                //cannot set alert level because have no char handle for it
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
        else
        {
            //wrong level - not one of the possible 3
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    else
    {
        //wrong connection handle
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        // Send an error indication for the application
        findl_error_ind_send(findl_env, status);
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
    struct findl_env_tag *findl_env = PRF_CLIENT_GET_ENV(dest_id, findl);

    if(state == FINDL_DISCOVERING)
    {
        if((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
                (param->status == ATT_ERR_NO_ERROR))
        {
            //IAS search is finished
            if (findl_env->last_uuid_req == ATT_SVC_IMMEDIATE_ALERT)
            {
                if (findl_env->ias.svc.shdl == ATT_INVALID_HANDLE)
                {
                    findl_enable_cfm_send(findl_env, &findl_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                // Too many services found only one such service should exist
                else if(findl_env->nb_svc > 1)
                {
                    // stop discovery procedure.
                    findl_enable_cfm_send(findl_env, &findl_env->con_info, PRF_ERR_MULTIPLE_SVC);
                }
                else
                {
                    // Discover all IAS characteristics
                    prf_disc_char_all_send(&(findl_env->con_info), &(findl_env->ias.svc));

                    // Keep last UUID requested and for which service in env
                    findl_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                }
            }
            //the search for alert char is finished
            else if (findl_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
            {
                status = prf_check_svc_char_validity(1, &findl_env->ias.alert_lvl_char,
                        &findl_ias_char);

                // Stop discovery procedure.
                findl_enable_cfm_send(findl_env, &findl_env->con_info, status);

            }
        }
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to FINDL.
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
    PRF_CLIENT_DISABLE_IND_SEND(findl_envs, dest_id, FINDL, param->conhdl);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// CONNECTED State handlers definition.
const struct ke_msg_handler findl_connected[] =
{
    {FINDL_SET_ALERT_REQ,            (ke_msg_func_t)findl_set_alert_req_handler},
};

/// DISCOVERING State handlers definition.
const struct ke_msg_handler findl_discovering[] =
{
    {GATTC_DISC_SVC_IND,            (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,           (ke_msg_func_t)gattc_disc_char_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler findl_default_state[] =
{
    {FINDL_ENABLE_REQ,              (ke_msg_func_t)findl_enable_req_handler},
    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler findl_state_handler[FINDL_STATE_MAX] =
{
    [FINDL_IDLE]        = KE_STATE_HANDLER_NONE,
    [FINDL_DISCOVERING] = KE_STATE_HANDLER(findl_discovering),
    [FINDL_CONNECTED]   = KE_STATE_HANDLER(findl_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler findl_default_handler = KE_STATE_HANDLER(findl_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t findl_state[FINDL_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_FINDME_LOCATOR

/// @} FINDLTASK
