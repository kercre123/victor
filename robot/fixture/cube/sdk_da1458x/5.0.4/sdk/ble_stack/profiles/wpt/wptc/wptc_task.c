/**
 ****************************************************************************************
 *
 * @file wptc_task.c
 *
 * @brief Wireless Transfer Client Task implementation.
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup WPTCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_WPT_CLIENT)

#include "gap.h"
#include "gapc.h"
#include "wptc_task.h"
#include "wptc.h"
#include "wpt_common.h"
#include "gattc_task.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "prf_utils_128.h"
#include "wpt_common.h"
#include "gattc.h"

/// State machine used to write PRU Charging service characteristics information
const struct prf_char_def_128 wptc_pru_charging_control = {WPTS_PRU_CHARGING_CTRL_UUID_128,
                                                    ATT_MANDATORY,
                                                    ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR};

/// State machine used to write PRU Static Parameter service characteristics information
const struct prf_char_def_128 wptc_ptu_static_param = {WPTS_PTU_STATIC_UUID_128,
                                               ATT_MANDATORY,
                                               ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR};

/// State machine used to retrieve PRU Alert Notification service characteristics information
const struct prf_char_def_128 wptc_pru_alert_notification = {WPTS_PRU_ALERT_UUID_128,
                                                            ATT_MANDATORY,
                                                            ATT_CHAR_PROP_RD|ATT_CHAR_PROP_NTF};

/// State machine used to retrieve PRU Static Parameter service characteristics information
const struct prf_char_def_128 wptc_pru_static_param = {WPTS_PRU_STATIC_UUID_128,
                                                       ATT_MANDATORY,
                                                       ATT_CHAR_PROP_RD};

/// State machine used to retrieve PRU Dynamic Parameter service characteristics information
const struct prf_char_def_128 wptc_pru_dynamic_param = {WPTS_PRU_DYNAMIC_UUID_128,
                                                       ATT_MANDATORY,
                                                       ATT_CHAR_PROP_RD};
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Enable the Wireless Power Tranfser Client (PTU) role, used after connection.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wptc_enable_req_handler(ke_msg_id_t const msgid,
                                    struct wptc_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Device Information Service Client Role Task Environment
    struct wptc_env_tag *wptc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, wptc);
    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

        // Discovery connection
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Start discovering WPTS on peer
            prf_disc_svc_send(&wptc_env->con_info, WPTS_SERVICE_UUID_16);
            wptc_env->discovery_mode = WPTC_DISCOVERY_MODE_PRIMARY_SERVICE;

            // Set state to discovering
            ke_state_set(dest_id, WPTC_DISCOVERING);
        }
        else
        {
            //copy over data that has been stored
            memcpy(&wptc_env->wpts, &param->wpts, sizeof(struct svc_content_wpts));

            //send confirmation of enable request to application
            wptc_enable_cfm_send(wptc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        wptc_enable_cfm_send(NULL, &con_info, status);
    }

    // message is consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Enable the Wireless Power Tranfser Client (PTU) role without using GATT Discover Services procedures,
 * Initialization of profile is based on handle value of begin WPT Service (this is delivered in advertising packet from PRU side)
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wptc_offline_enable_req_handler(ke_msg_id_t const msgid,
                                    struct wptc_offline_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Device Information Service Client Role Task Environment
    struct wptc_env_tag *wptc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, wptc);
    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

        wptc_env->nb_svc = 1;
        wptc_env->wpts.svc.shdl = param->wpt_uuid_start;
        wptc_env->wpts.svc.ehdl = param->wpt_uuid_start + 11;

        // PRU Control characteristic
        wptc_env->wpts.pru_control_charact.char_hdl = param->wpt_uuid_start + 1;
        wptc_env->wpts.pru_control_charact.val_hdl = param->wpt_uuid_start + 2;
        wptc_env->wpts.pru_control_charact.char_ehdl_off = 2;
        wptc_env->wpts.pru_control_charact.prop = ATT_CHAR_PROP_WR_NO_RESP|ATT_CHAR_PROP_RD;

        // PTU Static Parameter characteristic
        wptc_env->wpts.ptu_static_charact.char_hdl = param->wpt_uuid_start + 3;
        wptc_env->wpts.ptu_static_charact.val_hdl = param->wpt_uuid_start + 4;
        wptc_env->wpts.ptu_static_charact.char_ehdl_off = 2;
        wptc_env->wpts.ptu_static_charact.prop = ATT_CHAR_PROP_WR_NO_RESP|ATT_CHAR_PROP_RD;

        // PRU Alert characteristic
        wptc_env->wpts.pru_alert_charact.char_hdl = param->wpt_uuid_start + 5;
        wptc_env->wpts.pru_alert_charact.val_hdl = param->wpt_uuid_start + 6;
        wptc_env->wpts.pru_alert_charact.char_ehdl_off = 3;
        wptc_env->wpts.pru_alert_charact.prop = ATT_CHAR_PROP_RD|ATT_CHAR_PROP_NTF|ATT_CHAR_PROP_IND;

        // PRU Alert Contifutation descripto
        wptc_env->wpts.alert_conf_desc.desc_hdl = param->wpt_uuid_start + 7;

        // PRU Static Parameter characteristic
        wptc_env->wpts.pru_static_charact.char_hdl = param->wpt_uuid_start + 8;
        wptc_env->wpts.pru_static_charact.val_hdl = param->wpt_uuid_start + 9;
        wptc_env->wpts.pru_static_charact.char_ehdl_off = 2;
        wptc_env->wpts.pru_static_charact.prop = ATT_CHAR_PROP_RD;

        // PRU Dynamic Parameter characteristic
        wptc_env->wpts.pru_dynamic_charact.char_hdl = param->wpt_uuid_start + 10;
        wptc_env->wpts.pru_dynamic_charact.val_hdl = param->wpt_uuid_start + 11;
        wptc_env->wpts.pru_dynamic_charact.char_ehdl_off = 2;
        wptc_env->wpts.pru_dynamic_charact.prop = ATT_CHAR_PROP_RD;

        //send confirmation of enable request to application
        ke_state_set(dest_id, WPTC_CONNECTED);
        wptc_enable_cfm_send(wptc_env, &con_info, PRF_ERR_OK);
    }
    else
    {
        wptc_enable_cfm_send(NULL, &con_info, status);
    }

    // message is consumed
    return (KE_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Handles reception of the @ref WPTC_RD_PRU_DYNAMIC_PARAMETER_REQ message.
 * Request to read the PRU Dynamic Parameter value.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wptc_rd_pru_dynamic_parameter_req_handler(ke_msg_id_t const msgid,
                                                    struct wptc_rd_pru_dynamic_param_req const *param,
                                                    ke_task_id_t const dest_id,
                                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    if(param->conhdl == gapc_get_conhdl(wptc_env->con_info.conidx))
    {
        if(wptc_env->wpts.pru_dynamic_charact.val_hdl != ATT_INVALID_HANDLE)
        {
            // Send Read Char. request
            prf_read_char_send(&wptc_env->con_info,
                               wptc_env->wpts.svc.shdl,
                               wptc_env->wpts.svc.ehdl,
                               wptc_env->wpts.pru_dynamic_charact.val_hdl);
        }
        else
        {
            prf_client_att_info_rsp((prf_env_struct*) wptc_env, WPTC_RD_CHAR_RSP,
                    PRF_ERR_INEXISTENT_HDL, NULL);
        }
    }
    else
    {
        // Wrong connection handle
        prf_client_att_info_rsp((prf_env_struct*) wptc_env, WPTC_RD_CHAR_RSP,
                PRF_ERR_INVALID_PARAM, NULL);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref WPTC_RD_PRU_STATIC_PARAMETER_REQ message.
 * Request to read the PRU Static Parameter value.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wptc_rd_pru_static_parameter_req_handler(ke_msg_id_t const msgid,
                                                    struct wptc_rd_pru_static_param_req const *param,
                                                    ke_task_id_t const dest_id,
                                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    if(param->conhdl == gapc_get_conhdl(wptc_env->con_info.conidx))
    {
        if(wptc_env->wpts.pru_static_charact.val_hdl != ATT_INVALID_HANDLE)
        {
            // Send Read Char. request
            prf_read_char_send(&wptc_env->con_info,
                               wptc_env->wpts.svc.shdl,
                               wptc_env->wpts.svc.ehdl,
                               wptc_env->wpts.pru_static_charact.val_hdl);
        }
        else
        {
            prf_client_att_info_rsp((prf_env_struct*) wptc_env, WPTC_RD_CHAR_RSP,
                    PRF_ERR_INEXISTENT_HDL, NULL);
        }
    }
    else
    {
        // Wrong connection handle
        prf_client_att_info_rsp((prf_env_struct*) wptc_env, WPTC_RD_CHAR_RSP,
                PRF_ERR_INVALID_PARAM, NULL);
    }

    return (KE_MSG_CONSUMED);
}

//WPTC_SEND_ALERT_CFG_INDNTF_REQ
static int wptc_send_alert_cfg_indntf_req_handler(ke_msg_id_t const msgid,
                                                  struct wptc_alert_cfg_indntf_req const *param,
                                                  ke_task_id_t const dest_id,
                                                  ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    uint8_t  req_type = GATTC_WRITE_NO_RESPONSE; // A4WP demands write without response, A4WP @6.3.6
    // Descriptor handle
    uint16_t val_hdl;
    
    // Get the address of the environment
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    if(param->conhdl == gapc_get_conhdl(wptc_env->con_info.conidx))
    {
        if (param->cfg_val == PRF_CLI_STOP_NTFIND || param->cfg_val == PRF_CLI_START_NTF||
                param->cfg_val == PRF_CLI_START_IND ||
                param->cfg_val == (PRF_CLI_START_NTF|PRF_CLI_START_IND)) // bot indication and notification at once
        {
            // Get handle value for this type of write
            val_hdl = wptc_env->wpts.alert_conf_desc.desc_hdl;

            // Check if attribute handle is OK
            if (val_hdl == ATT_INVALID_HANDLE)
            {
                wptc_write_char_rsp_send(wptc_env, PRF_ERR_INEXISTENT_HDL);
            }
            else
            {
                // Send GATT Write Request
                prf_gatt_write(&wptc_env->con_info, val_hdl,
                               (uint8_t *) &param->cfg_val,
                               WPTS_PRU_ALERT_CONFIR_CHAR_LEN, req_type);
            }
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }
    
    if (PRF_ERR_OK != status)
    {
        // Wrong Connection Handle
        wptc_write_char_rsp_send(wptc_env, status);
    }
    
    return (KE_MSG_CONSUMED);
};

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref WPTC_WR_PRU_CONTROL_REQ message.
 * Request to write PRU Control Value
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wptc_wr_pru_control_req_handler(ke_msg_id_t const msgid,
                                          struct wptc_wr_pru_control_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    uint8_t  req_type = 0x00;
    uint16_t val_hdl  = 0x0000;
    // Get the address of the environment
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    if(param->conhdl == gapc_get_conhdl(wptc_env->con_info.conidx))
    {
        // Get the request type and handle value for this type of write
        req_type = GATTC_WRITE_NO_RESPONSE; // A4WP demands write without response, A4WP @6.3.6
        val_hdl = wptc_env->wpts.pru_control_charact.val_hdl;

        // Check if attribute handle is OK
        if (val_hdl == ATT_INVALID_HANDLE)
        {
            wptc_write_char_rsp_send(wptc_env, PRF_ERR_INEXISTENT_HDL);
        } else
        {
            // Send GATT Write Request
            prf_gatt_write(&wptc_env->con_info, val_hdl,
                            (uint8_t *) &param->pru_control_val,
                            WPTS_PRU_CHARGING_CTRL_CHAR_LEN, req_type);
        }
    }
    else
    {
        // Wrong Connection Handle
        wptc_write_char_rsp_send(wptc_env, PRF_ERR_INVALID_PARAM);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref WPTC_WR_PRU_CONTROL_REQ message.
 * Request to write either PRU Control Value
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wptc_wr_ptu_static_req_handler(ke_msg_id_t const msgid,
                                          struct wptc_wr_ptu_static_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    uint8_t  req_type = 0x00;
    uint16_t val_hdl  = 0x0000;
    // Get the address of the environment
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    if(param->conhdl == gapc_get_conhdl(wptc_env->con_info.conidx))
    {
        // Get the request type and handle value for this type of write
        req_type = GATTC_WRITE_NO_RESPONSE; // A4WP demands write without response A4WP @6.3.6
        val_hdl = wptc_env->wpts.ptu_static_charact.val_hdl;

        // Check if attribute handle is OK
        if (val_hdl == ATT_INVALID_HANDLE)
        {
            wptc_write_char_rsp_send(wptc_env, PRF_ERR_INEXISTENT_HDL);
        } else
        {
            // Send GATT Write Request
            prf_gatt_write(&wptc_env->con_info, val_hdl,
                    (uint8_t *) &param->ptu_static_val,
                    WPTS_PTU_STATIC_CHAR_LEN, req_type);
        }
    }
    else
    {
        // Wrong Connection Handle
        wptc_write_char_rsp_send(wptc_env, PRF_ERR_INVALID_PARAM);
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
static int gattc_disc_char_desc_ind_handler(ke_msg_id_t const msgid, struct gattc_disc_char_desc_ind const *param,
                                       ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    //Retrieve characteristic descriptor handle using UUID

    if (wptc_env->discovery_mode == WPTC_DISCOVERY_MODE_DESCRIPTORS)
    {

        if (attm_uuid_match_16((uint8_t*)param->uuid, param->uuid_len, ATT_DESC_CLIENT_CHAR_CFG))
        {
            wptc_env->wpts.alert_conf_desc.desc_hdl = param->attr_hdl;
        }
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
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    if (KE_IDX_GET(src_id) == wptc_env->con_info.conidx)
    {
        // Report Char.
        if (param->handle == wptc_env->wpts.pru_alert_charact.val_hdl)
        {
            if (param->length == WPTS_PRU_ALERT_CHAR_LEN)
            {
                struct wptc_alert_report_ntf *ntf = KE_MSG_ALLOC(
                        WPTC_PRU_ALERT_REPORT_NTF, wptc_env->con_info.appid,
                        dest_id, wptc_alert_report_ntf);


                ntf->conhdl = gapc_get_conhdl(wptc_env->con_info.conidx);

                ntf->alert_val = param->value[0];
                memcpy(ntf->device_address, &(param->value[1]), WPTS_BD_ADDR_LEN);

                ke_msg_send(ntf);
            } // ignoring notification msg with wrong length
        }
    }// ignore message with wrong conn id

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_SVC_IND message.
 * The handler disables the Health Thermometer Profile Collector Role.
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
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    // NOTE: RJ, we only discovering one service - (Wireless Power Transfer Service)
    //       So not need to check found service UUID here
    wptc_env->wpts.svc.shdl = param->start_hdl;
    wptc_env->wpts.svc.ehdl = param->end_hdl;
    wptc_env->nb_svc++;

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
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);
    uint8_t* last_found_char = NULL; // param unused WPTC but passed because of func declaration

    // NOTE: RJ, we only discovering one service - (Wireless Power Transfer Service)
    //       So not need to check found service UUID here

    // Retrieve PRU Charging Control characteristic
    prf_search_chars_128(wptc_env->wpts.svc.shdl, 1,
                        &wptc_env->wpts.pru_control_charact,
                        &wptc_pru_charging_control,
                        param, last_found_char);

    // Retrieve PTU Static Parameter characteristic
    prf_search_chars_128(wptc_env->wpts.svc.shdl, 1,
                        &wptc_env->wpts.ptu_static_charact,
                        &wptc_ptu_static_param,
                        param, last_found_char);

    // Retrieve PRU Alert Notification characteristic
    prf_search_chars_128(wptc_env->wpts.svc.shdl, 1,
                        &wptc_env->wpts.pru_alert_charact,
                        &wptc_pru_alert_notification,
                        param, last_found_char);

    // Retrieve PRU Static Parameter characteristic
    prf_search_chars_128(wptc_env->wpts.svc.shdl, 1,
                        &wptc_env->wpts.pru_static_charact,
                        &wptc_pru_static_param,
                        param, last_found_char);

    // Retrieve PRU Dynamic Parameter characteristic
    prf_search_chars_128(wptc_env->wpts.svc.shdl, 1,
                        &wptc_env->wpts.pru_dynamic_charact,
                        &wptc_pru_dynamic_param,
                        param, last_found_char);

    return (KE_MSG_CONSUMED);
}

static int wptc_validate_characteristics(struct svc_content_wpts *wpts)
{
    // Validating all service characteristics to see if everything is set
    // properly. For WPTS service all characteristics are MANDATORY

    if (wpts->pru_control_charact.char_hdl == ATT_INVALID_HANDLE)
    {
        //NOTE:("WPTC_TASK - wptc_validate_characteristics Error! pru_control_charact NOT FOUND \n");
        return PRF_ERR_STOP_DISC_CHAR_MISSING;
    }

    if (wpts->ptu_static_charact.char_hdl == ATT_INVALID_HANDLE)
    {
        //NOTE:("WPTC_TASK - wptc_validate_characteristics Error! pTu_static_charact NOT FOUND \n");
        return PRF_ERR_STOP_DISC_CHAR_MISSING;
    }

    if (wpts->pru_static_charact.char_hdl == ATT_INVALID_HANDLE)
    {
        //NOTE:("WPTC_TASK - wptc_validate_characteristics Error! pRu_static_charact NOT FOUND \n");
        return PRF_ERR_STOP_DISC_CHAR_MISSING;
    }

    if (wpts->pru_dynamic_charact.char_hdl == ATT_INVALID_HANDLE)
    {
        //NOTE:("WPTC_TASK - wptc_validate_characteristics Error! pru_dynamic_charact NOT FOUND \n");
        return PRF_ERR_STOP_DISC_CHAR_MISSING;
    }

    if (wpts->pru_alert_charact.char_hdl == ATT_INVALID_HANDLE)
    {
        //NOTE:("WPTC_TASK - wptc_validate_characteristics Error! pru_alert_charact NOT FOUND \n");
        return PRF_ERR_STOP_DISC_CHAR_MISSING;
    }

    return PRF_ERR_OK;
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
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    if(state == WPTC_DISCOVERING)
    {

        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
            (param->status == ATT_ERR_NO_ERROR))
        {
            if (wptc_env->discovery_mode == WPTC_DISCOVERY_MODE_PRIMARY_SERVICE)
            {

                if (wptc_env->wpts.svc.shdl == ATT_INVALID_HANDLE)
                {
                    //NOTE:("WPTC_TASK gattc_cmp_evt_handler Invalid service handle \n");
                    wptc_enable_cfm_send(wptc_env, &wptc_env->con_info,
                            PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
                // Too many services found only one such service should exist
                else if (wptc_env->nb_svc > 1)
                {

                    // stop discovery procedure.
                    wptc_enable_cfm_send(wptc_env, &wptc_env->con_info,
                            PRF_ERR_MULTIPLE_SVC);
                } else
                {
                    // Discover LLS characteristics
                    if (wptc_env->nb_svc > 0) {

                        wptc_env->discovery_mode = WPTC_DISCOVERY_MODE_ALL_CHARACTERISTICS;
                        // NOTE: using regular function to discover characteristics
                        // for service. It doesn't matter if UUID is 16 or 128 bit
                        // (UUID in this request is ignored)

                        prf_disc_char_all_send(&(wptc_env->con_info),
                                                &(wptc_env->wpts.svc));
                    } else
                    {
                        wptc_enable_cfm_send(wptc_env, &wptc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                    }
                }
            }
            else if (wptc_env->discovery_mode == WPTC_DISCOVERY_MODE_ALL_CHARACTERISTICS)
            {

                status = wptc_validate_characteristics(&(wptc_env->wpts));
                if (status == PRF_ERR_OK)
                {

                    wptc_env->discovery_mode = WPTC_DISCOVERY_MODE_DESCRIPTORS;
                    prf_disc_char_desc_send(&(wptc_env->con_info),
                                            &(wptc_env->wpts.pru_alert_charact));
                }
            }
            else if (wptc_env->discovery_mode == WPTC_DISCOVERY_MODE_DESCRIPTORS)
            {
                if (wptc_env->wpts.alert_conf_desc.desc_hdl != ATT_INVALID_HANDLE)
                {
                    status = PRF_ERR_OK;
                }
                else
                {
                    status = PRF_ERR_STOP_DISC_CHAR_MISSING;
                }

                wptc_enable_cfm_send(wptc_env, &wptc_env->con_info, status);
            }
        }
    }
    else if (state == WPTC_CONNECTED)
    {
        switch(param->req_type)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                wptc_write_char_rsp_send(wptc_env, param->status);
            }
            break;

            case GATTC_READ:
            {
                if(param->status != GATT_ERR_NO_ERROR)
                {
                    // an error occurs while reading peer device attribute
                    prf_client_att_info_rsp((prf_env_struct*) wptc_env, WPTC_RD_CHAR_RSP,
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
    struct wptc_env_tag *wptc_env = PRF_CLIENT_GET_ENV(dest_id, wptc);

    prf_client_att_info_rsp((prf_env_struct*) wptc_env, WPTC_RD_CHAR_RSP,
            GATT_ERR_NO_ERROR, param);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to proximity monitor
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
    PRF_CLIENT_DISABLE_IND_SEND(wptc_envs, dest_id, WPTC, param->conhdl);

    // message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// CONNECTED State handlers definition.

const struct ke_msg_handler wptc_connected[] =
{
    {WPTC_WR_PRU_CONTROL_REQ,                   (ke_msg_func_t)wptc_wr_pru_control_req_handler},
    {WPTC_RD_PRU_DYNAMIC_PARAMETER_REQ,         (ke_msg_func_t)wptc_rd_pru_dynamic_parameter_req_handler},
    {WPTC_WR_PTU_STATIC_PARAMETER_REQ,          (ke_msg_func_t)wptc_wr_ptu_static_req_handler},
    {WPTC_RD_PRU_STATIC_PARAMETER_REQ,          (ke_msg_func_t)wptc_rd_pru_static_parameter_req_handler},
    {WPTC_SEND_ALERT_CFG_INDNTF_REQ,            (ke_msg_func_t)wptc_send_alert_cfg_indntf_req_handler},
    {GATTC_READ_IND,                            (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,                           (ke_msg_func_t)gattc_event_ind_handler},
};


/// Specifies the Discovering state message handlers
const struct ke_msg_handler wptc_discovering[] =
{
    {GATTC_DISC_SVC_IND,          (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,         (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,    (ke_msg_func_t)gattc_disc_char_desc_ind_handler},
};


/// Default State handlers definition
const struct ke_msg_handler wptc_default_state[] =
{
    {WPTC_ENABLE_REQ,            (ke_msg_func_t)wptc_enable_req_handler},
    {WPTC_OFFLINE_ENABLE_REQ,    (ke_msg_func_t)wptc_offline_enable_req_handler},
    {GAPC_DISCONNECT_IND,        (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,              (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.

const struct ke_state_handler wptc_state_handler[WPTC_STATE_MAX] =
{
    [WPTC_IDLE]        = KE_STATE_HANDLER_NONE,
    [WPTC_CONNECTED]   = KE_STATE_HANDLER(wptc_connected),
    [WPTC_DISCOVERING] = KE_STATE_HANDLER(wptc_discovering),

};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler wptc_default_handler = KE_STATE_HANDLER(wptc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t wptc_state[WPTC_IDX_MAX] __attribute__((section("exchange_mem_case1"))); //@RETENTION MEMORY

#endif //BLE_WPT_CLIENT

/// @} WPTCTASK
