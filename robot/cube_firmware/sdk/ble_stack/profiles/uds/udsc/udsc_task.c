/**
 ****************************************************************************************
 *
 * @file udsc_task.c
 *
 * @brief User Data Service Client Task implementation.
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
 * @addtogroup UDSCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_UDS_CLIENT)
#include "gap.h"
#include "gapc.h"
#include "attm.h"
#include "udsc.h"
#include "udsc_task.h"
#include "gattc_task.h"
#include "smpc_task.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Length of the maximum User Control Point value to write
#define UDS_UCP_REQ_MAX_LEN     4

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Array of characteristics and its properties used to verify peer's attributes
const struct prf_char_def udsc_uds_char[UDSC_CHAR_MAX] =
{
    [UDSC_FIRST_NAME_CHAR]                      = {ATT_CHAR_UDS_USER_FIRST_NAME,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_LAST_NAME_CHAR]                       = {ATT_CHAR_UDS_USER_LAST_NAME,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_EMAIL_ADDRESS_CHAR]                   = {ATT_CHAR_UDS_USER_EMAIL_ADDR,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_AGE_CHAR]                             = {ATT_CHAR_UDS_USER_AGE,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_DATE_OF_BIRTH_CHAR]                   = {ATT_CHAR_UDS_USER_DATE_OF_BIRTH,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_GENDER_CHAR]                          = {ATT_CHAR_UDS_USER_GENDER,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_WEIGHT_CHAR]                          = {ATT_CHAR_UDS_USER_WEIGHT,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_HEIGHT_CHAR]                          = {ATT_CHAR_UDS_USER_HEIGHT,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_VO2_MAX_CHAR]                         = {ATT_CHAR_UDS_USER_VO2_MAX,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_HEART_RATE_MAX_CHAR]                  = {ATT_CHAR_UDS_USER_HEART_RATE_MAX,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_RESTING_HEART_RATE_CHAR]              = {ATT_CHAR_UDS_USER_RESTING_HEART_RATE,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_MAX_RECOMMENDED_HEART_RATE_CHAR]      = {ATT_CHAR_UDS_USER_MAX_REC_HEART_RATE,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_AEROBIC_THRESHOLD_CHAR]               = {ATT_CHAR_UDS_USER_AEROBIC_THRESHOLD,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_ANAEROBIC_THRESHOLD_CHAR]             = {ATT_CHAR_UDS_USER_ANAEROBIC_THRESHOLD,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_SPORT_TYPE_CHAR]                      = {ATT_CHAR_UDS_USER_THRESHOLDS_SPORT_TYPE,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_DATE_OF_THRESHOLD_ASSESSMENT_CHAR]    = {ATT_CHAR_UDS_USER_DATE_OF_THRESHOLD_ASS,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_WAIST_CIRCUMFERENCE_CHAR]             = {ATT_CHAR_UDS_USER_WAIST_CIRCUMFERENCE,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_HIP_CIRCUMFERENCE_CHAR]               = {ATT_CHAR_UDS_USER_HIP_CIRCUMFERENCE,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_FAT_BURN_HEART_RATE_LOW_LIMIT_CHAR]   = {ATT_CHAR_UDS_USER_FAT_BURN_HR_LOW_LIM,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_FAT_BURN_HEART_RATE_UP_LIMIT_CHAR]    = {ATT_CHAR_UDS_USER_FAT_BURN_HR_UP_LIM,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_AEROBIC_HEART_RATE_LOW_LIMIT_CHAR]    = {ATT_CHAR_UDS_USER_AEROBIC_HR_LOW_LIM,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_AEROBIC_HEART_RATE_UP_LIMIT_CHAR]     = {ATT_CHAR_UDS_USER_AEROBIC_HR_UP_LIM,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_ANEROBIC_HEART_RATE_LOW_LIMIT_CHAR]   = {ATT_CHAR_UDS_USER_ANAEROBIC_HR_LOW_LIM,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_ANEROBIC_HEART_RATE_UP_LIMIT_CHAR]    = {ATT_CHAR_UDS_USER_ANAEROBIC_HR_UP_LIM,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_FIVE_ZONE_HEART_RATE_LIMITS_CHAR]     = {ATT_CHAR_UDS_USER_5ZONE_HR_LIM,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_THREE_ZONE_HEART_RATE_LIMITS_CHAR]    = {ATT_CHAR_UDS_USER_3ZONE_HR_LIM,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_TWO_ZONE_HEART_RATE_LIMIT_CHAR]       = {ATT_CHAR_UDS_USER_2ZONE_HR_LIM,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_LANGUAGE_CHAR]                        = {ATT_CHAR_UDS_USER_LANGUAGE,
                                                    ATT_OPTIONAL,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_DB_CHANGE_INCR_CHAR]                  = {ATT_CHAR_UDS_USER_DB_CHANGE_INCR,
                                                    ATT_MANDATORY,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    [UDSC_INDEX_CHAR]                           = {ATT_CHAR_UDS_USER_INDEX,
                                                    ATT_MANDATORY,
                                                    ATT_CHAR_PROP_RD},
    [UDSC_CTRL_POINT_CHAR]                      = {ATT_CHAR_UDS_USER_CTRL_PT,
                                                    ATT_MANDATORY,
                                                    ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND},
};

/// Array of descriptors and its properties used to verify peer's attributes
const struct prf_char_desc_def udsc_uds_char_desc[UDSC_DESC_MAX] =
{
    /// User Data Measurement client config
    [UDSC_USER_DBC_INC_CFG]    = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, UDSC_DB_CHANGE_INCR_CHAR},
    [UDSC_DESC_UCP_CFG]        = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, UDSC_CTRL_POINT_CHAR},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSC_ENABLE_REQ message.
 * The handler enables the User Data Profile Collector Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udsc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct udsc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Device Information Service Client Role Task Environment
    struct udsc_env_tag *udsc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, udsc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

        // Discovery connection
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Start discovering UDS on peer
            prf_disc_svc_send(&udsc_env->con_info, ATT_SVC_USER_DATA);

            udsc_env->last_uuid_req = ATT_SVC_USER_DATA;

            // Set state to discovering
            ke_state_set(dest_id, UDSC_DISCOVERING);
        }
        else
        {
            // Copy over data that has been stored
            udsc_env->uds = param->uds;

            // Send confirmation of enable request to application
            udsc_enable_cfm_send(udsc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        udsc_enable_cfm_send(NULL, &con_info, status);
    }

    // Message is consumed
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
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    // Even if we get multiple responses we only store 1 range
    udsc_env->uds.svc.shdl = param->start_hdl;
    udsc_env->uds.svc.ehdl = param->end_hdl;
    udsc_env->nb_svc++;

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
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    // Retrieve UDSS characteristics
    prf_search_chars(udsc_env->uds.svc.ehdl, UDSC_CHAR_MAX,
                                    &udsc_env->uds.chars[0], &udsc_uds_char[0],
                                    param, &udsc_env->last_char_code);

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
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    // Retrieve UDSS descriptors
    prf_search_descs(UDSC_DESC_MAX, &udsc_env->uds.descs[0],
                                            &udsc_uds_char_desc[0],
                                            param, udsc_env->last_char_code);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Helper function for the discovery process
 * @param[in] udsc_env User Data Service Client environment variable
 * @return Discovery status
 ****************************************************************************************
 */
static uint8_t udsc_trigger_discovery(struct udsc_env_tag *udsc_env)
{
    uint8_t status = PRF_ERR_STOP_DISC_CHAR_MISSING;

    // Service Discovery
    if(udsc_env->last_uuid_req == ATT_SVC_USER_DATA)
    {
        // Check if service handles are not ok
        if(udsc_env->uds.svc.shdl == ATT_INVALID_HANDLE)
        {
            // Stop discovery procedure.
            status = PRF_ERR_STOP_DISC_CHAR_MISSING;
        }
        // Too many services found only one such service should exist
        else if(udsc_env->nb_svc != 1)
        {
            // Stop discovery procedure.
            status = PRF_ERR_MULTIPLE_SVC;
        }
        else
        {
            // Discover all UDS characteristics
            prf_disc_char_all_send(&(udsc_env->con_info), &(udsc_env->uds.svc));
            udsc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
            status = PRF_ERR_OK;
        }
    }
    // Characteristic Discovery
    else if(udsc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
    {
        status = prf_check_svc_char_validity(UDSC_CHAR_MAX, udsc_env->uds.chars,
                                             udsc_uds_char);

        // Check for characteristic properties.
        if(status == PRF_ERR_OK)
        {
            udsc_env->last_uuid_req = ATT_CHAR_UDS_USER_DB_CHANGE_INCR;
            udsc_env->last_char_code = udsc_uds_char_desc[UDSC_USER_DBC_INC_CFG].char_code;

            // Discover DB Change Increment Descriptor - Mandatory
            prf_disc_char_desc_send(&(udsc_env->con_info),
                                    &(udsc_env->uds.chars[UDSC_DB_CHANGE_INCR_CHAR]));
        }
    }
    // Descriptors Discovery
    else
    {
        if (udsc_env->last_uuid_req == ATT_CHAR_UDS_USER_DB_CHANGE_INCR)
        {
            udsc_env->last_uuid_req = ATT_CHAR_UDS_USER_CTRL_PT;
            udsc_env->last_char_code = udsc_uds_char_desc[UDSC_DESC_UCP_CFG].char_code;

            // Discover User Control Point Descriptor - Mandatory
            prf_disc_char_desc_send(&(udsc_env->con_info),
                                    &(udsc_env->uds.chars[UDSC_CTRL_POINT_CHAR]));
            status = PRF_ERR_OK;
        }
        else
        {
            // Discovery should be over after the Ctrl. Point Char. Desc.
            ASSERT_ERR(udsc_env->last_uuid_req == ATT_CHAR_UDS_USER_CTRL_PT);

            udsc_env->last_uuid_req = ATT_LAST;

            status = prf_check_svc_char_desc_validity(UDSC_DESC_MAX,
                                                        udsc_env->uds.descs,
                                                        udsc_uds_char_desc,
                                                        udsc_env->uds.chars);

        }
    }

    return status;
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
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    if (udsc_env == NULL)
        return(KE_MSG_CONSUMED);

    switch(param->req_type)
    {
        case GATTC_DISC_BY_UUID_SVC:
        case GATTC_DISC_ALL_CHAR:
        case GATTC_DISC_DESC_CHAR:
        {
            if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                                            (param->status == ATT_ERR_NO_ERROR))
            {
                status = udsc_trigger_discovery(udsc_env);
            }
            else
            {
                status = param->status;
            }

            // Notify in case of successful discovery or premature failure
            if ((udsc_env->last_uuid_req == ATT_LAST) ||
                                                    (status != PRF_ERR_OK))
            {
                udsc_enable_cfm_send(udsc_env, &udsc_env->con_info, status);
            }
        } break;

        case GATTC_READ:
        {
            if(param->status != GATT_ERR_NO_ERROR)
            {
                // An error occurs while reading peer device attribute
                udsc_read_char_val_rsp_send(udsc_env, param->status, UDSC_CHAR_MAX,
                                                                        0, NULL);
            }
            // Do nothing on success as GATTC_READ_IND handler will be called
        } break;

        case GATTC_WRITE:
        case GATTC_WRITE_NO_RESPONSE:
        {
            if (udsc_env->op_code != UDS_NO_OP)
            {
                // Notify right away in case of failure
                if (param->status != GATT_ERR_NO_ERROR)
                {
                    struct udsc_ucp_op_cfm *cfm = KE_MSG_ALLOC(UDSC_UCP_OP_CFM,
                                                        udsc_env->con_info.appid,
                                                        udsc_env->con_info.prf_id,
                                                        udsc_ucp_op_cfm);

                    cfm->cfm.req_op_code = udsc_env->op_code;
                    cfm->cfm.op_code = UDS_RSP_OPERATION_FAILED;

                    ke_msg_send(cfm);

                    // Clear ongoing operation indicator on failure
                    udsc_env->op_code = UDS_NO_OP;
                }
                else
                {
                    // On UCP write start timeout procedure
                    ke_timer_set(UDSC_TIMEOUT_TIMER_IND, dest_id, ATT_TRANS_RTX);
                }
            }
            // Regular characteristic or DB Change Increment char. write confirmation
            else if (udsc_env->db_update)
            {
                // Notify all write requests but not the internal db change incr. write
                if (udsc_env->last_char_code != UDSC_DB_CHANGE_INCR_CHAR)
                {
                    udsc_set_char_val_cfm_send(udsc_env, param->status);
                }

                udsc_env->db_update = false;
            }
            // It is descriptor write confirmation
            else
            {
                udsc_cmp_evt_ind_send(udsc_env, param->status);
            }
        } break;

        case GATTC_REGISTER:
        case GATTC_UNREGISTER:
        {
            // Do nothing
        } break;

        default:
        {
            ASSERT_ERR(0);
        } break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Helper function for packing UCP request structure.
 * @param[in] req Request from the App.
 * @param[in] buf Output buffer.
 * @return length
 ****************************************************************************************
 */
static uint8_t uds_pack_ucp_req(struct uds_ucp_req const *req, uint8_t *buf)
{
    uint8_t length = 1;

    buf[0] = req->op_code;

    switch (req->op_code)
    {
        case UDS_REQ_REG_NEW_USER:
        {
            co_write16p(&buf[1], req->parameter.reg_new_user.consent_code);
            length += 2;

            break;
        }
        case UDS_REQ_CONSENT:
        {
            buf[1] = req->parameter.consent.user_idx;
            co_write16p(&buf[2], req->parameter.consent.consent_code);
            length += 3;

            break;
        }
        case UDS_REQ_DEL_USER_DATA:
        default:
            break;
    }

    return length;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSC_UCP_OP_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udsc_ucp_op_req_handler(ke_msg_id_t const msgid,
                                    struct udsc_ucp_op_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    if (udsc_env == NULL)
        return (KE_MSG_CONSUMED);

    if (udsc_env->op_code != UDS_NO_OP)
    {
        udsc_cmp_evt_ind_send(udsc_env, PRF_ERR_REQ_DISALLOWED);
    }
    else
    {
        uint8_t req[UDS_UCP_REQ_MAX_LEN];
        uint8_t nb;

        // Store the opcode for UCP operation
        udsc_env->op_code = param->req.op_code;

        nb = uds_pack_ucp_req(&param->req, &req[0]);

        // Send the write request
        prf_gatt_write(&(udsc_env->con_info),
                            udsc_env->uds.chars[UDSC_CTRL_POINT_CHAR].val_hdl,
                            (uint8_t *)&req[0], nb, GATTC_WRITE);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSC_NTFIND_CFG_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udsc_ntfind_cfg_req_handler(ke_msg_id_t const msgid,
                                        struct udsc_ntfind_cfg_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint16_t cfg_hdl = 0x0000;
    uint16_t allowed_cfg_val = 0x0000;
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    if (udsc_env == NULL)
        return (KE_MSG_CONSUMED);

    if((param->conhdl != gapc_get_conhdl(udsc_env->con_info.conidx)) ||
                                            param->desc_code >= UDSC_DESC_MAX)
    {
        udsc_cmp_evt_ind_send(udsc_env, PRF_ERR_INVALID_PARAM);
        return (KE_MSG_CONSUMED);
    }

    cfg_hdl = udsc_env->uds.descs[param->desc_code].desc_hdl;

    if (param->desc_code == UDSC_USER_DBC_INC_CFG)
    {
        allowed_cfg_val = PRF_CLI_START_NTF;
    }
    else if (param->desc_code == UDSC_DESC_UCP_CFG)
    {
        allowed_cfg_val = PRF_CLI_START_IND;
    }
    else
    {
        udsc_cmp_evt_ind_send(udsc_env, PRF_ERR_INVALID_PARAM);
    }

    if(!((param->cfg_val == PRF_CLI_STOP_NTFIND) ||
                                        (param->cfg_val == allowed_cfg_val)))
    {
        udsc_cmp_evt_ind_send(udsc_env, PRF_ERR_INVALID_PARAM);
    }
    else if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
    {
        prf_gatt_write_ntf_ind(&udsc_env->con_info, cfg_hdl, param->cfg_val);
        // On success - GATTC_CMP_EVT will come
    }
    else
    {
        udsc_cmp_evt_ind_send(udsc_env, PRF_ERR_INEXISTENT_HDL);
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
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    if (udsc_env == NULL)
    {
        return (KE_MSG_CONSUMED);
    }

    // Store db_change value
    if ((udsc_env->last_char_code == UDSC_DB_CHANGE_INCR_CHAR) &&
                            (param->length >= sizeof(udsc_env->db_change_incr)))
    {
            memcpy(&udsc_env->db_change_incr, param->value,
                                            sizeof(udsc_env->db_change_incr));
    }

    udsc_read_char_val_rsp_send(udsc_env, PRF_ERR_OK, udsc_env->last_char_code,
                                                param->length, param->value);

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
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    if (udsc_env == NULL)
    {
        return (KE_MSG_CONSUMED);
    }

    switch (param->type)
    {
        case GATTC_NOTIFY:
        {
            // Handle DB Change Characteristic value notification
            if (param->length >= sizeof(udsc_env->db_change_incr))
            {
                struct udsc_db_change_ntf *ntf;

                memcpy(&udsc_env->db_change_incr, param->value,
                                            sizeof(udsc_env->db_change_incr));

                ntf = KE_MSG_ALLOC(UDSC_DB_CHANGE_NTF, udsc_env->con_info.appid,
                                                    udsc_env->con_info.prf_id,
                                                    udsc_db_change_ntf);

                ntf->conhdl = gapc_get_conhdl(udsc_env->con_info.conidx);
                ntf->incr_val = udsc_env->db_change_incr;

                ke_msg_send(ntf);
            }
        } break;

        case GATTC_INDICATE:
        {
            // Stop the UCP operation timeout timer
            if (udsc_env->op_code != UDS_NO_OP)
            {
                struct udsc_ucp_op_cfm *cfm;

                // At least resp_op_code, req_op_code & resp_val are required
                if (param->length >= 3)
                {
                    cfm = KE_MSG_ALLOC(UDSC_UCP_OP_CFM, udsc_env->con_info.appid,
                                                        udsc_env->con_info.prf_id,
                                                        udsc_ucp_op_cfm);

                    cfm->cfm.op_code = param->value[0];
                    cfm->cfm.req_op_code = param->value[1];
                    cfm->cfm.rsp_val = param->value[2];

                    // The only expected parameter (if any) is the user index byte
                    if ((param->length > 3) &&
                            (cfm->cfm.req_op_code == UDS_REQ_REG_NEW_USER) &&
                            (cfm->cfm.rsp_val == UDS_RSP_SUCCESS))
                    {
                        cfm->cfm.parameter.reg_new_user.user_idx = param->value[3];
                    }

                    ke_msg_send(cfm);
                }

                ke_timer_clear(UDSC_TIMEOUT_TIMER_IND, dest_id);
                udsc_env->op_code = UDS_NO_OP;
            }
        } break;

        default:
        {
            ASSERT_ERR(0);
        } break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to UDSC.
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
    // Get the address of the Environment
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    if (udsc_env == NULL)
        return (KE_MSG_CONSUMED);

    // Stop the UCP operation timeout timer
    if (udsc_env->op_code != UDS_NO_OP)
    {
        ke_timer_clear(UDSC_TIMEOUT_TIMER_IND, dest_id);
        udsc_env->op_code = UDS_NO_OP;
    }

    PRF_CLIENT_DISABLE_IND_SEND(udsc_envs, dest_id, UDSC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSC_READ_CHAR_VAL_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udsc_read_char_val_req_handler(ke_msg_id_t const msgid,
                                struct udsc_read_char_val_req const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    if (udsc_env == NULL)
        return (KE_MSG_CONSUMED);

    // This function should not be called for reading UCP
    if((param->conhdl != gapc_get_conhdl(udsc_env->con_info.conidx)) ||
                                    (param->char_code >= UDSC_CTRL_POINT_CHAR))
    {
        udsc_read_char_val_rsp_send(udsc_env, PRF_ERR_INVALID_PARAM,
                                                        UDSC_CHAR_MAX, 0, NULL);
        return (KE_MSG_CONSUMED);
    }

    if (udsc_env->uds.chars[param->char_code].val_hdl ==
                                                    ATT_INVALID_SEARCH_HANDLE)
    {
        udsc_read_char_val_rsp_send(udsc_env, PRF_ERR_INEXISTENT_HDL,
                                                        UDSC_CHAR_MAX, 0, NULL);
    }
    else
    {
        udsc_env->last_char_code = param->char_code;

        // Send GATT Read Request
        prf_read_char_send(&udsc_env->con_info,
                               udsc_env->uds.svc.shdl, udsc_env->uds.svc.ehdl,
                               udsc_env->uds.chars[param->char_code].val_hdl);
        // On failure - GATTC_CMP_EVT will come with req_type == GATTC_READ
        // On success - GATTC_READ_IND will come
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSC_READ_CHAR_VAL_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udsc_set_char_val_req_handler(ke_msg_id_t const msgid,
                                struct udsc_set_char_val_req const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    if (udsc_env == NULL)
        return (KE_MSG_CONSUMED);

    // If its the first write - we need to know the db_change_incr value.
    // It is stored on read or when notified (the App needs to register for it)
    if (udsc_env->db_change_incr == 0)
    {
        udsc_set_char_val_cfm_send(udsc_env, UDS_ERROR_IMPROPERLY_CONFIGURED);
        return (KE_MSG_CONSUMED);
    }

    // This function should not be called for writing opcodes to UCP nor for
    // incrementing db change increment characteristic as it is done automatically
    if((param->conhdl != gapc_get_conhdl(udsc_env->con_info.conidx)) ||
                                            (param->char_code >= UDSC_CTRL_POINT_CHAR) ||
                                            (param->char_code == UDSC_DB_CHANGE_INCR_CHAR))
    {
        udsc_set_char_val_cfm_send(udsc_env, PRF_ERR_INVALID_PARAM);
        return (KE_MSG_CONSUMED);
    }

    if (udsc_env->uds.chars[param->char_code].val_hdl ==
                                                    ATT_INVALID_SEARCH_HANDLE)
    {
        udsc_set_char_val_cfm_send(udsc_env, PRF_ERR_INEXISTENT_HDL);
    }
    else
    {
        udsc_env->db_update = true;
        udsc_env->last_char_code = param->char_code;

        // Send GATT Write Request
        prf_gatt_write(&udsc_env->con_info, udsc_env->uds.chars[param->char_code].val_hdl,
                        (uint8_t *)&param->val, param->val_len, GATTC_WRITE);
        // GATTC_CMP_EVT will come with req_type == GATTC_WRITE
    }

    // If its the last write we should write db change increment
    if (param->finalize)
    {
        udsc_env->last_char_code = UDSC_DB_CHANGE_INCR_CHAR;
        udsc_env->db_change_incr++;

        prf_gatt_write(&udsc_env->con_info, udsc_env->uds.chars[UDSC_DB_CHANGE_INCR_CHAR].val_hdl,
                        (uint8_t *)&udsc_env->db_change_incr, sizeof(udsc_env->db_change_incr),
                                                                                    GATTC_WRITE);
        // GATTC_CMP_EVT will come with req_type == GATTC_WRITE
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSC_TIMEOUT_TIMER_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udsc_timeout_timer_ind_handler(ke_msg_id_t const msgid,
                                                    void const *param,
                                                    ke_task_id_t const dest_id,
                                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct udsc_env_tag *udsc_env = PRF_CLIENT_GET_ENV(dest_id, udsc);

    if (udsc_env != NULL)
    {
        ASSERT_ERR(udsc_env->op_code != UDS_NO_OP);
        udsc_env->op_code = UDS_NO_OP;

        // Send the complete event message with error
        udsc_cmp_evt_ind_send(udsc_env, PRF_ERR_PROC_TIMEOUT);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the message handlers for the connected state
const struct ke_msg_handler udsc_connected[] =
{
    {UDSC_UCP_OP_REQ,               (ke_msg_func_t)udsc_ucp_op_req_handler},
    {UDSC_NTFIND_CFG_REQ,           (ke_msg_func_t)udsc_ntfind_cfg_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {UDSC_READ_CHAR_VAL_REQ,        (ke_msg_func_t)udsc_read_char_val_req_handler},
    {UDSC_SET_CHAR_VAL_REQ,         (ke_msg_func_t)udsc_set_char_val_req_handler},
    {UDSC_TIMEOUT_TIMER_IND,        (ke_msg_func_t)udsc_timeout_timer_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler udsc_discovering[] =
{
    {GATTC_DISC_SVC_IND,            (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,           (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,      (ke_msg_func_t)gattc_disc_char_desc_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler udsc_default_state[] =
{
    {UDSC_ENABLE_REQ,               (ke_msg_func_t)udsc_enable_req_handler},
    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler udsc_state_handler[UDSC_STATE_MAX] =
{
    [UDSC_IDLE]        = KE_STATE_HANDLER_NONE,
    [UDSC_CONNECTED]   = KE_STATE_HANDLER(udsc_connected),
    [UDSC_DISCOVERING] = KE_STATE_HANDLER(udsc_discovering),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler udsc_default_handler = KE_STATE_HANDLER(udsc_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t udsc_state[UDSC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* (BLE_UDS_CLIENT) */
/// @} UDSCTASK
