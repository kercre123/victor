/**
 ****************************************************************************************
 *
 * @file hogpbh_task.c
 *
 * @brief HID Over GATT Profile Boot Host Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HOGPBHTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_HID_BOOT_HOST)

#include "gap.h"
#include "attm.h"
#include "hogpbh.h"
#include "hogpbh_task.h"
#include "gattc_task.h"
#include "hogp_common.h"

/*
 * LOCAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/// State machine used to retrieve HID Service characteristics information
const struct prf_char_def hogpbh_hids_char[HOGPBH_CHAR_MAX] =
{
    /// Protocol Mode
    [HOGPBH_CHAR_PROTO_MODE]             = {ATT_CHAR_PROTOCOL_MODE,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP},
    /// Boot Keyboard Input Report
    [HOGPBH_CHAR_BOOT_KB_IN_REPORT]      = {ATT_CHAR_BOOT_KB_IN_REPORT,
                                            ATT_OPTIONAL,
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF},
    /// Boot Keyboard Output Report
    [HOGPBH_CHAR_BOOT_KB_OUT_REPORT]     = {ATT_CHAR_BOOT_KB_OUT_REPORT,
                                            ATT_OPTIONAL,
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    /// Boot Keyboard Output Report
    [HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT]   = {ATT_CHAR_BOOT_MOUSE_IN_REPORT,
                                            ATT_OPTIONAL,
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF},
};

/// State machine used to retrieve HID Service characteristic description information
const struct prf_char_desc_def hogpbh_hids_char_desc[HOGPBH_DESC_MAX] =
{
    /// Boot Keyboard Input Report Client Config
    [HOGPBH_DESC_BOOT_KB_IN_REPORT_CFG]    = {ATT_DESC_CLIENT_CHAR_CFG, ATT_OPTIONAL, HOGPBH_CHAR_BOOT_KB_IN_REPORT},
    /// Boot Mouse Input Report Client Config
    [HOGPBH_DESC_BOOT_MOUSE_IN_REPORT_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_OPTIONAL, HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT},
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPBH_ENABLE_REQ message.
 * The handler enables the HID Over GATT Profile Boot Host Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpbh_enable_req_handler(ke_msg_id_t const msgid,
                                   struct hogpbh_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // HID Over GATT Profile Client Role Task Environment
    struct hogpbh_env_tag *hogpbh_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, hogpbh);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);

        //config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering HIDS on peer
            prf_disc_svc_send(&(hogpbh_env->con_info), ATT_SVC_HID);

            hogpbh_env->last_uuid_req = ATT_SVC_HID;

            // Go to DISCOVERING state
            ke_state_set(dest_id, HOGPBH_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            memcpy(&hogpbh_env->hids[0], &param->hids[0], HOGPBH_NB_HIDS_INST_MAX*sizeof(struct hids_content));
            hogpbh_env->hids_nb = param->hids_nb;

            //send APP confirmation that can start normal connection to TH
            hogpbh_enable_cfm_send(hogpbh_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        hogpbh_enable_cfm_send(NULL, &con_info, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPBH_DISABLE_REQ message.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpbh_disable_req_handler(ke_msg_id_t const msgid,
                                      struct hogpbh_disable_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);
    // Counter
    uint8_t hids_nb;

    for (hids_nb = 0; hids_nb < hogpbh_env->hids_nb; hids_nb++)
    {
        // Unregister the profile in the GATT
        prf_unregister_atthdl2gatt(&hogpbh_env->con_info, &hogpbh_env->hids[hids_nb].svc);
    }

    PRF_CLIENT_DISABLE_IND_SEND(hogpbh_envs, dest_id, HOGPBH, param->conhdl);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPBH_RD_CHAR_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpbh_rd_char_req_handler(ke_msg_id_t const msgid,
                                      struct hogpbh_rd_char_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    // Attribute handle
    uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;
    // Get the address of the environment
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);

    // Save the attribute read code
    hogpbh_env->last_char_code = param->char_code;

    if((param->conhdl == gapc_get_conhdl(hogpbh_env->con_info.conidx)) &&
       (param->hids_nb < HOGPBH_NB_HIDS_INST_MAX))
    {
        // Descriptor
        if(((param->char_code & HOGPBH_DESC_MASK) == HOGPBH_DESC_MASK) &&
           ((param->char_code & ~HOGPBH_DESC_MASK) < HOGPBH_DESC_MASK))
        {
            search_hdl = hogpbh_env->hids[param->hids_nb].descs[param->char_code & ~HOGPBH_DESC_MASK].desc_hdl;
        }
        // Characteristic
        else if (param->char_code < HOGPBH_CHAR_MAX)
        {
            search_hdl = hogpbh_env->hids[param->hids_nb].chars[param->char_code].val_hdl;
        }

        // Check if handle is viable
        if (search_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            prf_read_char_send(&(hogpbh_env->con_info), hogpbh_env->hids[param->hids_nb].svc.shdl,
                               hogpbh_env->hids[param->hids_nb].svc.ehdl, search_hdl);

            // Save the service instance number
            hogpbh_env->last_svc_inst_req  = param->hids_nb;
        }
        else
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;

    }

    if (status != PRF_ERR_OK)
    {
        hogpbh_char_req_rsp_send(hogpbh_env, HOGPBH_RD_CHAR_ERR_RSP, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPBH_CFG_INDNTF_REQ message.
 * It allows configuration of the peer ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpbh_cfg_ntf_req_handler(ke_msg_id_t const msgid,
                                         struct hogpbh_cfg_ntf_req const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Descriptor handle
    uint16_t cfg_hdl;
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;
    // Descriptor Code
    uint8_t desc_code = param->desc_code & ~HOGPBH_DESC_MASK;
    // Get the address of the environment
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);

    // Save the attribute read code
    hogpbh_env->last_char_code = param->desc_code;

    if((param->conhdl == gapc_get_conhdl(hogpbh_env->con_info.conidx)) &&
       (param->hids_nb < HOGPBH_NB_HIDS_INST_MAX) &&
       (desc_code < HOGPBH_DESC_MAX))
    {
        // Check value to write
        if((param->ntf_cfg == PRF_CLI_STOP_NTFIND) || (param->ntf_cfg == PRF_CLI_START_NTF))
        {
            cfg_hdl = hogpbh_env->hids[param->hids_nb].descs[desc_code].desc_hdl;

            // Check if the handle value exists
            if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Send GATT Write Request
                prf_gatt_write_ntf_ind(&hogpbh_env->con_info, cfg_hdl, param->ntf_cfg);

                status = PRF_ERR_OK;
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
    }

    if (status != PRF_ERR_OK)
    {
        hogpbh_char_req_rsp_send(hogpbh_env, HOGPBH_WR_CHAR_RSP, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPBH_BOOT_KB_OUT_REPORT_WR_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpbh_boot_report_wr_req_handler(ke_msg_id_t const msgid,
                                                  struct hogpbh_boot_report_wr_req const *param,
                                                  ke_task_id_t const dest_id,
                                                  ke_task_id_t const src_id)
{
    // Descriptor handle
    uint16_t val_hdl = ATT_INVALID_SEARCH_HANDLE;
    // Status
    uint8_t status = PRF_ERR_OK;
    // Write Type
    uint8_t wr_type = GATTC_WRITE;
    // Get the address of the environment
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);

    // Save the Characteristic Code
    hogpbh_env->last_char_code = param->char_code;

    // Check Connection Handle and HIDS Instance Nb
    if((param->conhdl == gapc_get_conhdl(hogpbh_env->con_info.conidx)) &&
       (param->hids_nb < hogpbh_env->hids_nb) &&
       (param->report_length <= HOGPBH_BOOT_REPORT_MAX_LEN))
    {
        switch (param->char_code)
        {
            case (HOGPBH_CHAR_BOOT_KB_OUT_REPORT):

                // Write Without Response is available for the Boot Keyboard Ouput Report
                if (param->wr_type == GATTC_WRITE_NO_RESPONSE)
    {
                    wr_type = param->wr_type;
    }

                break;

            case (HOGPBH_CHAR_BOOT_KB_IN_REPORT):
            case (HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT):

                // Check if the characteristic is writable
                if ((hogpbh_env->hids[param->hids_nb].chars[param->char_code].prop & ATT_CHAR_PROP_WR)
                                                                                  != ATT_CHAR_PROP_WR)
            {
                status = PRF_ERR_NOT_WRITABLE;
    }

                break;

            default:
                status = PRF_ERR_INVALID_PARAM;
                break;
}

        if (status == PRF_ERR_OK)
{
            val_hdl = hogpbh_env->hids[param->hids_nb].chars[param->char_code].val_hdl;

        // Check if the handle value exists
        if (val_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
                prf_gatt_write(&hogpbh_env->con_info, val_hdl, (uint8_t *)&param->report[0],
                               param->report_length, wr_type);
        }
        else
        {
            status = PRF_ERR_INEXISTENT_HDL;
            }
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        hogpbh_char_req_rsp_send(hogpbh_env, HOGPBH_WR_CHAR_RSP, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPBH_SET_BOOT_PROTO_MODE_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpbh_set_boot_proto_mode_req_handler(ke_msg_id_t const msgid,
                                                  struct hogpbh_set_boot_proto_mode_req const *param,
                                                  ke_task_id_t const dest_id,
                                                  ke_task_id_t const src_id)
{
    // Descriptor handle
    uint16_t val_hdl;
    // Status
    uint8_t status = PRF_ERR_OK;
    // Get the address of the environment
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);
    // Protocol Mode
    uint8_t proto_mode = HOGP_BOOT_PROTOCOL_MODE;

    // Save the Characteristic Code
    hogpbh_env->last_char_code = HOGPBH_RD_WR_HIDS_PROTO_MODE;

    // Check Connection Handle and HIDS Instance Nb
    if((param->conhdl == gapc_get_conhdl(hogpbh_env->con_info.conidx)) &&
       (param->hids_nb < HOGPBH_NB_HIDS_INST_MAX))
    {
        val_hdl = hogpbh_env->hids[param->hids_nb].chars[HOGPBH_CHAR_PROTO_MODE].val_hdl;

        // Check if the handle value exists
        if (val_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            // Send GATT Write Request
            prf_gatt_write(&hogpbh_env->con_info, val_hdl,
                           &proto_mode, sizeof(uint8_t), GATTC_WRITE_NO_RESPONSE);
        }
        else
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        hogpbh_char_req_rsp_send(hogpbh_env, HOGPBH_WR_CHAR_RSP, status);
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
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);

    if (hogpbh_env->hids_nb < HOGPBH_NB_HIDS_INST_MAX)
    {
        hogpbh_env->hids[hogpbh_env->hids_nb].svc.shdl = param->start_hdl;
        hogpbh_env->hids[hogpbh_env->hids_nb].svc.ehdl = param->end_hdl;
    }

    // Update number of found HIDS instances
    hogpbh_env->hids_nb++;

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
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);

    // Retrieve HIDS characteristics
    prf_search_chars(hogpbh_env->hids[hogpbh_env->last_svc_inst_req].svc.ehdl, HOGPBH_CHAR_MAX,
            &hogpbh_env->hids[hogpbh_env->last_svc_inst_req].chars[0], &hogpbh_hids_char[0],
            param, &hogpbh_env->last_char_code);

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
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);

    //Retrieve characteristic descriptor handle using UUID

    if (attm_uuid_match_16((uint8_t*)param->uuid, param->uuid_len, ATT_DESC_CLIENT_CHAR_CFG))
    {
        if (hogpbh_env->last_char_code == HOGPBH_CHAR_BOOT_KB_IN_REPORT)
        {
            if ((hogpbh_env->hids[hogpbh_env->last_svc_inst_req].chars[HOGPBH_CHAR_BOOT_KB_IN_REPORT].char_hdl + 2) == param->attr_hdl)
            {
                hogpbh_env->hids[hogpbh_env->last_svc_inst_req].descs[HOGPBH_DESC_BOOT_KB_IN_REPORT_CFG].desc_hdl = param->attr_hdl;
            }
        }
        else if (hogpbh_env->last_char_code == HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT)
        {
            if ((hogpbh_env->hids[hogpbh_env->last_svc_inst_req].chars[HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT].char_hdl + 2) == param->attr_hdl)
            {
                hogpbh_env->hids[hogpbh_env->last_svc_inst_req].descs[HOGPBH_DESC_BOOT_MOUSE_IN_REPORT_CFG].desc_hdl = param->attr_hdl;
            }
        }
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
    // Status
    uint8_t status;
    // Get the address of the environment
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);

    if(state == HOGPBH_DISCOVERING)
    {

        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                (param->status == ATT_ERR_NO_ERROR))
        {
            /*-------------------------------------------------------------------------------------------------------------
             * SERVICES DISCOVERY
             *-------------------------------------------------------------------------------------------------------------*/
            if (hogpbh_env->last_uuid_req == ATT_SVC_HID)
            {
                // If no HIDS instance has been found
                if (hogpbh_env->hids_nb == 0)
                {
                    // Stop discovery procedure.
                    hogpbh_enable_cfm_send(hogpbh_env, &hogpbh_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                else
                {
                    // Discover all HOGPBH characteristics for first service instance
                    hogpbh_env->last_uuid_req      = ATT_DECL_CHARACTERISTIC;
                    hogpbh_env->last_svc_inst_req  = 0;

                    prf_disc_char_all_send(&(hogpbh_env->con_info), &(hogpbh_env->hids[0].svc));
                }
            }

            /*-------------------------------------------------------------------------------------------------------------
             * CHARACTERISTICS DISCOVERY
             *-------------------------------------------------------------------------------------------------------------*/
            else if (hogpbh_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
            {
                status = prf_check_svc_char_validity(HOGPBH_CHAR_MAX, hogpbh_env->hids[hogpbh_env->last_svc_inst_req].chars,
                        hogpbh_hids_char);

                if (status == PRF_ERR_OK)
                {
                    hogpbh_env->last_uuid_req  = ATT_INVALID_HANDLE;

                    // Check if Boot Keyboard Input Report Char has been found
                    if (HOGPBH_FOUND_CHAR(hogpbh_env, HOGPBH_CHAR_BOOT_KB_IN_REPORT))
                    {
                        hogpbh_env->last_char_code = HOGPBH_CHAR_BOOT_KB_IN_REPORT;
                    }
                    // Check if Boot Mouse Input Report Char has been found
                    else if (HOGPBH_FOUND_CHAR(hogpbh_env, HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT))
                    {
                        hogpbh_env->last_char_code = HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT;
                    }
                    // Error - At least one of these characteristic must be supported
                    else
                    {
                        status = PRF_ERR_STOP_DISC_CHAR_MISSING;
                    }
                }

                if (status != PRF_ERR_OK)
                {
                    //If we are here, stop discovery
                    hogpbh_enable_cfm_send(hogpbh_env, &hogpbh_env->con_info, status);
                }
                else
                {
                    // Send discover descriptors request
                    prf_disc_char_desc_send(&(hogpbh_env->con_info),
                            &(hogpbh_env->hids[hogpbh_env->last_svc_inst_req].chars[hogpbh_env->last_char_code]));
                }
            }

            /*-------------------------------------------------------------------------------------------------------------
             * DESCRIPTORS DISCOVERY
             *-------------------------------------------------------------------------------------------------------------*/
            else
            {
                if (hogpbh_env->last_char_code == HOGPBH_CHAR_BOOT_KB_IN_REPORT)
                {
                    if (HOGPBH_FOUND_CHAR(hogpbh_env, HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT))
                    {
                        hogpbh_env->last_char_code = HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT;

                        // Discover Client Char. Cfg descriptor for Boot Mouse Input Report Char.
                        prf_disc_char_desc_send(&(hogpbh_env->con_info),
                                &(hogpbh_env->hids[hogpbh_env->last_svc_inst_req].chars[hogpbh_env->last_char_code]));
                    }
                    else
                    {
                        if (hogpbh_env->last_svc_inst_req < (hogpbh_env->hids_nb - 1))
                        {
                            // Discover HIDS Characteristics for the next instance
                            hogpbh_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                            hogpbh_env->last_svc_inst_req++;

                            prf_disc_char_all_send(&(hogpbh_env->con_info), &(hogpbh_env->hids[hogpbh_env->last_svc_inst_req].svc));
                        }
                        else
                        {
                            // send app the details about the discovered BAPS DB to save
                            hogpbh_enable_cfm_send(hogpbh_env, &hogpbh_env->con_info, PRF_ERR_OK);
                        }
                    }
                }

                else if (hogpbh_env->last_char_code == HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT)
                {
                    if (hogpbh_env->last_svc_inst_req < (hogpbh_env->hids_nb - 1))
                    {
                        // Discover HIDS Characteristics for the next instance
                        hogpbh_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                        hogpbh_env->last_svc_inst_req++;

                        prf_disc_char_all_send(&(hogpbh_env->con_info), &(hogpbh_env->hids[hogpbh_env->last_svc_inst_req].svc));
                    }
                    else
                    {
                        // send app the details about the discovered BAPS DB to save
                        hogpbh_enable_cfm_send(hogpbh_env, &hogpbh_env->con_info, PRF_ERR_OK);
                    }
                }
            }
        }
    }
    else if (state == HOGPBH_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                // Send the write char. rsp to the application
                hogpbh_char_req_rsp_send(hogpbh_env, HOGPBH_WR_CHAR_RSP, param->status);
            }
            break;

            case GATTC_READ:
            {
                if(param->status != GATT_ERR_NO_ERROR)
                {
                    hogpbh_char_req_rsp_send(hogpbh_env, HOGPBH_RD_CHAR_ERR_RSP, param->status);
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
    uint8_t error_status =PRF_ERR_OK;
    // Get the address of the environment
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);

    switch (hogpbh_env->last_char_code)
    {
    // Boot Keyboard Input Report Read Response
        case (HOGPBH_CHAR_BOOT_KB_IN_REPORT):
        // Boot Mouse Input Report Read Response
        case (HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT):
        // Boot Mouse Input Report Read Response
        case (HOGPBH_CHAR_BOOT_KB_OUT_REPORT):
    {
        // Check report length
            if (param->length <= HOGPBH_BOOT_REPORT_MAX_LEN)
        {
                struct hogpbh_boot_report_ind *ind = KE_MSG_ALLOC_DYN(HOGPBH_BOOT_REPORT_IND,
                    hogpbh_env->con_info.appid, dest_id,
                                                                      hogpbh_boot_report_ind,
                                                                      param->length);

            ind->conhdl     = gapc_get_conhdl(hogpbh_env->con_info.conidx);
            ind->ind_type   = HOGPBH_IND_RD_RSP;
            ind->hids_nb    = hogpbh_env->last_svc_inst_req;
                ind->char_code      = hogpbh_env->last_char_code;
            ind->report_length  = param->length;
            memcpy(&ind->report[0], &param->value[0], param->length);

            ke_msg_send(ind);
        }
        else
        {
            error_status = PRF_ERR_UNEXPECTED_LEN;
        }

            break;
    }

    //Protocol Mode Read Response
        case (HOGPBH_CHAR_PROTO_MODE):
    {
        struct hogpbh_proto_mode_rd_rsp *rsp = KE_MSG_ALLOC(HOGPBH_PROTO_MODE_RD_RSP,
                hogpbh_env->con_info.appid, dest_id,
                hogpbh_proto_mode_rd_rsp);

        rsp->conhdl     = gapc_get_conhdl(hogpbh_env->con_info.conidx);
        rsp->hids_nb    = hogpbh_env->last_svc_inst_req;
        memcpy(&rsp->proto_mode, &param->value[0], sizeof(uint8_t));

        ke_msg_send(rsp);

            break;
    }

    //Client Characteristic Configuration
        case (HOGPBH_RD_WR_HIDS_BOOT_KB_IN_REPORT_CFG):
        case (HOGPBH_RD_WR_HIDS_BOOT_MOUSE_IN_REPORT_CFG):
    {
        struct hogpbh_cfg_ntf_rd_rsp *rsp = KE_MSG_ALLOC(HOGPBH_CFG_NTF_RD_RSP,
                hogpbh_env->con_info.appid, dest_id,
                hogpbh_cfg_ntf_rd_rsp);

        rsp->conhdl     = gapc_get_conhdl(hogpbh_env->con_info.conidx);
        rsp->desc_code  = hogpbh_env->last_char_code;
        rsp->hids_nb    = hogpbh_env->last_svc_inst_req;
        memcpy(&rsp->cfg_val, &param->value[0], sizeof(uint16_t));

        ke_msg_send(rsp);

            break;
        }

        default:
            break;
    }

    if (error_status != PRF_ERR_OK)
    {
        hogpbh_char_req_rsp_send(hogpbh_env, HOGPBH_RD_CHAR_ERR_RSP, error_status);
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
    // HIDS Instance
    uint8_t hids_nb;
    // Get the address of the environment
    struct hogpbh_env_tag *hogpbh_env = PRF_CLIENT_GET_ENV(dest_id, hogpbh);
    // Characteristic Code
    uint8_t char_code = PRF_APP_ERROR;

    if (KE_IDX_GET(src_id) ==hogpbh_env->con_info.conidx)
    {
        // HIDS instance is unknown.
        for (hids_nb = 0; ((hids_nb < hogpbh_env->hids_nb) && (char_code == PRF_APP_ERROR)); hids_nb++)
        {
            //Boot Input Report
            if (param->handle == hogpbh_env->hids[hids_nb].chars[HOGPBH_CHAR_BOOT_KB_IN_REPORT].val_hdl)
            {
                char_code = HOGPBH_CHAR_BOOT_KB_IN_REPORT;
                }
            else if (param->handle == hogpbh_env->hids[hids_nb].chars[HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT].val_hdl)
            {
                char_code = HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT;
            }

            if (char_code != PRF_APP_ERROR)
            {
                // Check Report size
                if (param->length <= HOGPBH_BOOT_REPORT_MAX_LEN)
                {
                    struct hogpbh_boot_report_ind *ind = KE_MSG_ALLOC_DYN(HOGPBH_BOOT_REPORT_IND,
                                                                               hogpbh_env->con_info.appid, dest_id,
                                                                          hogpbh_boot_report_ind,
                                                                          param->length);

                    ind->conhdl = gapc_get_conhdl(hogpbh_env->con_info.conidx);
                    ind->ind_type = HOGPBH_IND_NTF;
                    ind->report_length = param->length;
                    ind->hids_nb = hids_nb;
                    ind->char_code = char_code;
                    memcpy(&ind->report[0], &param->value[0], param->length);

                    ke_msg_send(ind);
                }
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to HOGPBH.
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
    PRF_CLIENT_DISABLE_IND_SEND(hogpbh_envs, dest_id, HOGPBH, param->conhdl);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler hogpbh_connected[] =
{
    {HOGPBH_DISABLE_REQ,                    (ke_msg_func_t)hogpbh_disable_req_handler},
    {HOGPBH_RD_CHAR_REQ,                    (ke_msg_func_t)hogpbh_rd_char_req_handler},
    {HOGPBH_CFG_NTF_REQ,                    (ke_msg_func_t)hogpbh_cfg_ntf_req_handler},
    {HOGPBH_SET_BOOT_PROTO_MODE_REQ,        (ke_msg_func_t)hogpbh_set_boot_proto_mode_req_handler},
    {HOGPBH_BOOT_REPORT_WR_REQ,             (ke_msg_func_t)hogpbh_boot_report_wr_req_handler},
    {GATTC_READ_IND,                        (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,                       (ke_msg_func_t)gattc_event_ind_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler hogpbh_discovering[] =
{
    {GATTC_DISC_SVC_IND,                 (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,                (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,           (ke_msg_func_t)gattc_disc_char_desc_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler hogpbh_default_state[] =
{
    {HOGPBH_ENABLE_REQ,                     (ke_msg_func_t)hogpbh_enable_req_handler},
    {GAPC_DISCONNECT_IND,                   (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                         (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler hogpbh_state_handler[HOGPBH_STATE_MAX] =
{
    [HOGPBH_IDLE]        = KE_STATE_HANDLER_NONE,
    [HOGPBH_CONNECTED]   = KE_STATE_HANDLER(hogpbh_connected),
    [HOGPBH_DISCOVERING] = KE_STATE_HANDLER(hogpbh_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler hogpbh_default_handler = KE_STATE_HANDLER(hogpbh_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t hogpbh_state[HOGPBH_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* (BLE_HOG_BOOT_HOST) */
/// @} HOGPBHTASK
