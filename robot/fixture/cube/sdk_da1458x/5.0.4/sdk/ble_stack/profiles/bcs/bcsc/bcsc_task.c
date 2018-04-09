/**
 ****************************************************************************************
 *
 * @file bcsc_task.c
 *
 * @brief Body Composition Profile Client Task implementation.
 *
 * Copyright (C) 2015 Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup BCSCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BCS_CLIENT)
#include "gap.h"
#include "gapc.h"
#include "attm.h"
#include "bcsc.h"
#include "bcsc_task.h"
#include "gattc_task.h"
#include "smpc_task.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define BCS_FEATURE_MASK     0x0003FFFF

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve blood pressure service characteristics information
const struct prf_char_def bcsc_bcs_char[BCSC_CHAR_MAX] =
{
    /// Body Composition Feature
    [BCSC_CHAR_BC_FEATURE]           = {ATT_CHAR_BCS_FEATURE,
                                        ATT_MANDATORY,
                                        ATT_CHAR_PROP_RD},
    /// Body Composition Measurement
    [BCSC_CHAR_BC_MEAS]              = {ATT_CHAR_BCS_MEAS,
                                        ATT_MANDATORY,
                                        ATT_CHAR_PROP_IND},
};

/// State machine used to retrieve blood pressure service characteristic description information
const struct prf_char_desc_def bcsc_bcs_char_desc[BCSC_DESC_MAX] =
{
    /// Body Composition Measurement client config
    [BCSC_DESC_MEAS_CLI_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, BCSC_CHAR_BC_MEAS},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BCSC_ENABLE_REQ message.
 * The handler enables the Body Composition Profile Collector Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bcsc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct bcsc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Body Composition Service Client Role Task Environment
    struct bcsc_env_tag *bcsc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status = PRF_CLIENT_ENABLE(con_info, param, bcsc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);

        // Config connection, start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Start discovering included service on peer
            if (param->bcs.is_secondary)
            {
                struct prf_svc svc = param->bcs.svc;

                if (svc.shdl == 0)
                    svc.shdl = 1;

                if (svc.ehdl <= svc.shdl)
                    svc.ehdl = 0xFFFF;

                prf_disc_incl_svc_send(&(bcsc_env->con_info), &svc);
            }
            // Start discovering primary service on peer
            else
            {
                prf_disc_svc_send(&(bcsc_env->con_info), ATT_SVC_BODY_COMPOSITION);
            }

            bcsc_env->last_uuid_req = ATT_SVC_BODY_COMPOSITION;

            // Go to DISCOVERING state
            ke_state_set(dest_id, BCSC_DISCOVERING);
        }
        // Normal connection, get saved att details
        else
        {
            bcsc_env->bcs = param->bcs;
            // Send APP confirmation that can start normal connection
            bcsc_enable_cfm_send(bcsc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        bcsc_enable_cfm_send(NULL, &con_info, status);
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
    struct bcsc_env_tag *bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);

    // Even if we get multiple responses we only store 1 range
    if (bcsc_env->last_uuid_req == ATT_SVC_BODY_COMPOSITION)
    {
        bcsc_env->bcs.svc.shdl = param->start_hdl;
        bcsc_env->bcs.svc.ehdl = param->end_hdl;
        bcsc_env->nb_svc++;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_SVC_INCL_IND message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_svc_incl_ind_handler(ke_msg_id_t const msgid,
                                             struct gattc_disc_svc_incl_ind const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct bcsc_env_tag *bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);

    // If included service search is performed
    if((bcsc_env->bcs.is_secondary) && (param->uuid_len == ATT_UUID_16_LEN))
    {
        // Even if we get multiple responses we only store 1 range
        if (!memcmp(&param->uuid[0], &bcsc_env->last_uuid_req, ATT_UUID_16_LEN))
        {
            bcsc_env->bcs.svc.shdl = param->start_hdl;
            bcsc_env->bcs.svc.ehdl = param->end_hdl;
            bcsc_env->nb_svc++;
        }
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
    struct bcsc_env_tag *bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);

    // Retrieve BCS characteristics
    prf_search_chars(bcsc_env->bcs.svc.ehdl, BCSC_CHAR_MAX,
                                    &bcsc_env->bcs.chars[0], &bcsc_bcs_char[0],
                                    param, &bcsc_env->last_char_code);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_DESC_IND message.
 * Descriptors for the currently desired characteristic handle range are obtained and kept.
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
    struct bcsc_env_tag *bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);

    // Retrieve BCS descriptors
    prf_search_descs(BCSC_DESC_MAX, &bcsc_env->bcs.descs[0],
                                            &bcsc_bcs_char_desc[0],
                                            param, bcsc_env->last_char_code);

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
    struct bcsc_env_tag *bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);

    // Handle discovering events
    if(state == BCSC_DISCOVERING)
    {
        // Only when done with discovering some attribute types check what was found
        if ((param->status != ATT_ERR_ATTRIBUTE_NOT_FOUND) &&
                                                (param->status != ATT_ERR_NO_ERROR))
            goto done;

        // If Service was found
        if (bcsc_env->last_uuid_req == ATT_SVC_BODY_COMPOSITION)
        {
            // Check if service handles are not ok
            if(bcsc_env->bcs.svc.shdl == ATT_INVALID_HANDLE)
            {
                // Try looking for include in case no primary service is found
                if (!bcsc_env->bcs.is_secondary)
                {
                    struct prf_svc svc;

                    bcsc_env->bcs.is_secondary = true;
                    svc.shdl = 0x0001;
                    svc.ehdl = 0xFFFF;

                    prf_disc_incl_svc_send(&(bcsc_env->con_info), &svc);
                }
                else
                {
                    // Stop discovery procedure.
                    bcsc_enable_cfm_send(bcsc_env, &bcsc_env->con_info,
                                                    PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
            }
            // Too many services found only one such service should exist
            else if(bcsc_env->nb_svc != 1)
            {
                // Stop discovery procedure.
                bcsc_enable_cfm_send(bcsc_env, &bcsc_env->con_info,
                                                        PRF_ERR_MULTIPLE_SVC);
            }
            else
            {
                // Discover all BCS characteristics
                prf_disc_char_all_send(&(bcsc_env->con_info), &(bcsc_env->bcs.svc));
                bcsc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
            }
        }
        // If characteristics declaration was found
        else if (bcsc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
        {
            status = prf_check_svc_char_validity(BCSC_CHAR_MAX, bcsc_env->bcs.chars,
                                                                    bcsc_bcs_char);

            if (status == PRF_ERR_OK)
            {
                bcsc_env->last_uuid_req = ATT_INVALID_HANDLE;
                bcsc_env->last_char_code = bcsc_bcs_char_desc[BCSC_DESC_MEAS_CLI_CFG].char_code;

                // Discover Body Composition Measurement Char. Descriptor - Mandatory
                prf_disc_char_desc_send(&(bcsc_env->con_info),
                                &(bcsc_env->bcs.chars[bcsc_env->last_char_code]));
            }
            else
            {
                // Stop the discovery procedure
                bcsc_enable_cfm_send(bcsc_env, &bcsc_env->con_info, status);
            }
        }
        else
        {
            status = prf_check_svc_char_desc_validity(BCSC_DESC_MAX,
                                                            bcsc_env->bcs.descs,
                                                            bcsc_bcs_char_desc,
                                                            bcsc_env->bcs.chars);

            bcsc_enable_cfm_send(bcsc_env, &bcsc_env->con_info, status);
        }
    }
    // Handle Connected events
    else if(state == BCSC_CONNECTED)
    {
        if(param->req_type == GATTC_WRITE || param->req_type == GATTC_WRITE_NO_RESPONSE)
        {
            if (bcsc_env->last_uuid_req == BCSC_DESC_MEAS_CLI_CFG)
            {
                struct bcsc_register_cfm *cfm = KE_MSG_ALLOC(BCSC_REGISTER_CFM,
                                                      bcsc_env->con_info.appid,
                                                      dest_id, bcsc_register_cfm);

                if (cfm)
                {
                    // Set error status
                    cfm->conhdl = gapc_get_conhdl(bcsc_env->con_info.conidx);
                    cfm->status = param->status;

                    ke_msg_send(cfm);
                }
            }
        }
    }

done:
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Helper function to check if request is possible for the given characteristic
 * @param[in] bcsc_env Body Composition Service client environment variable.
 * @param[in] conhdl Connection handle.
 * @param[in] char_code Attribute handle.
 * @return PRF_ERR_OK if request can be performed, error code else.
 ****************************************************************************************
 */
static uint8_t bcsc_validate_char_req(struct bcsc_env_tag *bcsc_env, uint16_t conhdl,
                                                                        uint8_t char_code)
{
    uint8_t status = PRF_ERR_OK;

    // Check if collector role enabled
    if (conhdl == gapc_get_conhdl(bcsc_env->con_info.conidx))
    {
        // Check if feature val characteristic exists
        if (bcsc_env->bcs.chars[char_code].val_hdl == ATT_INVALID_HANDLE)
            status = PRF_ERR_INEXISTENT_HDL;
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief Helper function to check if request is possible for the given descriptor
 * @param[in] bcsc_env Body Composition Service client environment variable.
 * @param[in] conhdl Connection handle.
 * @param[in] desc_code Attribute handle.
 * @return PRF_ERR_OK if request can be performed, error code else.
 ****************************************************************************************
 */
static uint8_t bcsc_validate_desc_req(struct bcsc_env_tag *bcsc_env, uint16_t conhdl,
                                                                        uint8_t desc_code)
{
    uint8_t status = PRF_ERR_OK;

    // Check if collector role enabled
    if (conhdl == gapc_get_conhdl(bcsc_env->con_info.conidx))
    {
        // Check if feature val characteristic exists
        if (bcsc_env->bcs.descs[desc_code].desc_hdl == ATT_INVALID_HANDLE)
            status = PRF_ERR_INEXISTENT_HDL;
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BCSC_READ_FEATURES_REQ message.
 * Check if the handle exists in profile(already discovered) and read features,
 * otherwise error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bcsc_read_features_req_handler(ke_msg_id_t const msgid,
                                    struct bcsc_read_features_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct bcsc_env_tag *bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);

    uint8_t status = bcsc_validate_char_req(bcsc_env, param->conhdl,
                                                        BCSC_CHAR_BC_FEATURE);

    // Request can be performed
    if(status == PRF_ERR_OK)
    {
        // Read body composition feature
        prf_read_char_send(&(bcsc_env->con_info), bcsc_env->bcs.svc.shdl,
                            bcsc_env->bcs.svc.ehdl,
                            bcsc_env->bcs.chars[BCSC_CHAR_BC_FEATURE].val_hdl);
    }
    // Send command response with error code
    else
    {    struct bcsc_read_features_cfm *cfm =
                                KE_MSG_ALLOC(BCSC_READ_FEATURES_CFM,
                                                bcsc_env->con_info.appid,
                                                dest_id, bcsc_read_features_cfm);
        // Set error status
        cfm->conhdl = param->conhdl;
        cfm->status = status;

        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BCSC_REGISTER_REQ message.
 * It allows configuration of the peer ind/ntf/stop for the measurement characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bcsc_register_req_handler(ke_msg_id_t const msgid,
                                        struct bcsc_register_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint8_t status;
    // Get the address of the environment
    struct bcsc_env_tag *bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);

    // Only indication or stop is allowed
    if ((param->cfg_val != PRF_CLI_STOP_NTFIND) &&
                                        (param->cfg_val != PRF_CLI_START_IND))
        status = PRF_ERR_INVALID_PARAM;
    else
        status = bcsc_validate_desc_req(bcsc_env, param->conhdl,
                                                        BCSC_DESC_MEAS_CLI_CFG);

    if(status == PRF_ERR_OK)
    {
        // Register to notification
        prf_gatt_write_ntf_ind(&(bcsc_env->con_info),
                                 bcsc_env->bcs.descs[BCSC_DESC_MEAS_CLI_CFG].desc_hdl,
                                 PRF_CLI_START_IND);
        bcsc_env->last_uuid_req = BCSC_DESC_MEAS_CLI_CFG;
    }
    // Inform application that request cannot be performed
    else
    {
        struct bcsc_register_cfm *cfm = KE_MSG_ALLOC(BCSC_REGISTER_CFM,
                                                      bcsc_env->con_info.appid,
                                                      dest_id, bcsc_register_cfm);
        // Set error status
        cfm->conhdl = param->conhdl;
        cfm->status = status;

        ke_msg_send(cfm);
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
    struct bcsc_env_tag *bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);
    struct bcsc_meas_val_ind *ind;

    // Check if collector is running
    if(KE_IDX_GET(src_id) != bcsc_env->con_info.conidx)
        goto done;

    // Only indications are allowed
    if (param->type != GATTC_INDICATE)
        goto done;

    // Check if it is the correct handle
    if (bcsc_env->bcs.chars[BCSC_CHAR_BC_MEAS].val_hdl != param->handle)
        goto done;

    ind = KE_MSG_ALLOC(BCSC_MEAS_VAL_IND, bcsc_env->con_info.appid, dest_id,
                                                            bcsc_meas_val_ind);

    ind->conhdl = gapc_get_conhdl(bcsc_env->con_info.conidx);
    bcsc_unpack_meas_value(&ind->meas, &ind->flags, (uint8_t*) param->value);

    ke_msg_send(ind);

done:
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
    struct bcsc_env_tag *bcsc_env = PRF_CLIENT_GET_ENV(dest_id, bcsc);

    if (bcsc_env != NULL)
    {
        // Prepare the confirmation to send to the application
        struct bcsc_read_features_cfm *cfm = KE_MSG_ALLOC(BCSC_READ_FEATURES_CFM,
                                                  bcsc_env->con_info.appid,
                                                  bcsc_env->con_info.prf_id,
                                                  bcsc_read_features_cfm);

        cfm->conhdl = gapc_get_conhdl(bcsc_env->con_info.conidx);
        cfm->status = (param->length >= sizeof(uint32_t)) ? PRF_ERR_OK :
                                                        PRF_ERR_UNEXPECTED_LEN;

        if(cfm->status == PRF_ERR_OK)
        {
            // Copy data only when length is valid
            cfm->features = co_read32p(&param->value[0]) & BCS_FEATURE_MASK;
        }
        // Send the confirmation
        ke_msg_send(cfm);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to BCSC.
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
    PRF_CLIENT_DISABLE_IND_SEND(bcsc_envs, dest_id, BCSC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler bcsc_connected[] =
{
    {BCSC_READ_FEATURES_REQ,        (ke_msg_func_t)bcsc_read_features_req_handler},
    {BCSC_REGISTER_REQ,             (ke_msg_func_t)bcsc_register_req_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler bcsc_discovering[] =
{
    {GATTC_DISC_SVC_IND,            (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_SVC_INCL_IND,       (ke_msg_func_t)gattc_disc_svc_incl_ind_handler},
    {GATTC_DISC_CHAR_IND,           (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,      (ke_msg_func_t)gattc_disc_char_desc_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler bcsc_default_state[] =
{
    {BCSC_ENABLE_REQ,               (ke_msg_func_t)bcsc_enable_req_handler},
    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler bcsc_state_handler[BCSC_STATE_MAX] =
{
    [BCSC_IDLE]        = KE_STATE_HANDLER_NONE,
    [BCSC_CONNECTED]   = KE_STATE_HANDLER(bcsc_connected),
    [BCSC_DISCOVERING] = KE_STATE_HANDLER(bcsc_discovering),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler bcsc_default_handler = KE_STATE_HANDLER(bcsc_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t bcsc_state[BCSC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* (BLE_BCS_CLIENT) */
/// @} BCSCTASK
