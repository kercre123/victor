/**
 ****************************************************************************************
 *
 * @file app_task.c
 *
 * @brief Handling of BLE events and responses.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app.h" 
#include "app_button_led.h"
#include "gap_task.h" 
#include "proxr_task.h" 
#include "proxr.h"
#include "llc_task.h" 
#include "smpc_task.h" 
#include "ble_msg.h" 


extern unsigned int proxm_trans_in_prog;

// application alert state structure
app_alert_state alert_state;

/**
 ****************************************************************************************
 * @brief Extracts device name from adv data if present and copy it to app_env.
 *
 * @param[in] adv_data      Pointer to advertise data.
 * @param[in] adv_data_len  Advertise data length.
 * @param[in] dev_indx      Devices index in device list.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
void app_find_device_name(unsigned char * adv_data, unsigned char adv_data_len, unsigned char dev_indx)
{
    unsigned char indx = 0;

    while (indx < adv_data_len)
    {
        if (adv_data[indx+1] == 0x09)
        {
            memcpy(app_env.devices[dev_indx].data, &adv_data[indx+2], (size_t) adv_data[indx]);
            app_env.devices[dev_indx].data_len = (unsigned char ) adv_data[indx];
        }
        indx += (unsigned char ) adv_data[indx]+1;
    }
}

/**
 ****************************************************************************************
 * @brief Handles Set Mode completion.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_set_mode_req_evt_handler(ke_msg_id_t msgid,
                                            struct gapm_cmp_evt *param,
                                            ke_task_id_t dest_id,
                                            ke_task_id_t src_id)
{
    app_env.state = APP_CONNECTABLE;

    app_adv_start(); //start advertising

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles ready indication from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

int gap_ready_evt_handler(ke_msg_id_t msgid,
                               void *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id)
{
    // We are now in Connectable State
    if (dest_id == TASK_APP)
    {
        app_rst_gap();

    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles Inquiry Request completion.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_dev_inq_req_cmp_evt_handler(ke_msg_id_t msgid,
                                    void *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id)
{
    app_env.state = APP_IDLE;

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles Inquiry result event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

int gap_dev_inq_result_handler(ke_msg_id_t msgid,
                               struct gapm_adv_report_ind *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id)
{
    if (app_env.state != APP_SCAN)
        return -1;

    return 0;
}


/**
 ****************************************************************************************
 * @brief Handles Connection request completion event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
unsigned int start_pair;
int gap_le_create_conn_req_ind_handler(ke_msg_id_t msgid,
                                                  struct gapc_connection_req_ind *param,
                                                  ke_task_id_t dest_id,
                                                  ke_task_id_t src_id)
{

  //start_pair = 1;

    if (app_env.state == APP_IDLE || app_env.state == APP_CONNECTABLE)
    {
        // Check the status

        // We are now connected
        turn_off_led();
                
        app_env.state = APP_CONNECTED;

        // Retrieve the connection info from the parameters
        app_env.proxr_device.device.conhdl = param->conhdl;
        /*
        // On Reconnection check if device is bonded and send pairing request. Otherwise it is not bonded.
        if (bdaddr_compare(&app_env.proxr_device.device.adv_addr, &param->conn_info.peer_addr))
        {
            if (app_env.proxr_device.bonded)
                start_pair = 0;
        }
        */
        memcpy(app_env.proxr_device.device.adv_addr.addr, param->peer_addr.addr, sizeof(struct bd_addr));
        app_env.proxr_device.rssi = 0xFF;
        app_env.proxr_device.txp = 0xFF;
        app_env.proxr_device.llv = 0xFF;

        // Send a request to read the LLC TX power so that we can update the PROXR service database.
        {
            struct llc_rd_tx_pw_lvl_cmd * req;
                    
            req = (struct llc_rd_tx_pw_lvl_cmd *) BleMsgAlloc(LLC_RD_TX_PW_LVL_CMD, TASK_LLC, TASK_APP,
                                                sizeof(struct llc_rd_tx_pw_lvl_cmd));
        
            req->conhdl = app_env.proxr_device.device.conhdl;
            req->type = TX_LVL_CURRENT;

            // Send the message
            BleSendMsg((void *) req);
        }

        app_connect_confirm(GAP_AUTH_REQ_NO_MITM_NO_BOND);

        //START PAIRING REMOVED FROM HERE
    }

    return 0;
}


/**
 ****************************************************************************************
 * @brief Handles Discconnection completion event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_discon_cmp_evt_handler(ke_msg_id_t msgid,
                               struct gapc_disconnect_ind *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id)
{
    if (param->conhdl == app_env.proxr_device.device.conhdl)
    {
        app_send_disconnect(TASK_PROXR, param->conhdl, param->reason);

        app_env.state = APP_IDLE;

        app_set_mode();
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles RSSI read request completion event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

int gap_read_rssi_cmp_evt_handler(ke_msg_id_t msgid,
                                  struct gapc_con_rssi_ind *param,
                                  ke_task_id_t dest_id,
                                  ke_task_id_t src_id)
{
    app_env.proxr_device.rssi = param->rssi;

    return 0;
}


/**
 ****************************************************************************************
 * @brief Handle Bond indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_bond_ind_handler(ke_msg_id_t msgid,
                         struct gapc_bond_ind *param,
                         ke_task_id_t dest_id,
                         ke_task_id_t src_id)
{
    switch (param->info)
    {
        case GAPC_PAIRING_SUCCEED:
            if (param->data.auth | GAP_AUTH_BOND)
                app_env.proxr_device.bonded = 1;
            break;

        case GAPC_IRK_EXCH:
            memcpy (app_env.proxr_device.irk.irk.key, param->data.irk.irk.key, KEY_LEN);
            memcpy (app_env.proxr_device.irk.addr.addr.addr, param->data.irk.addr.addr.addr, KEY_LEN);
            app_env.proxr_device.irk.addr.addr_type = param->data.irk.addr.addr_type;
            break;

        case GAPC_LTK_EXCH:
            app_env.proxr_device.ltk.ediv = param->data.ltk.ediv;
            memcpy (app_env.proxr_device.ltk.randnb.nb, param->data.ltk.randnb.nb, RAND_NB_LEN);
            app_env.proxr_device.ltk.key_size = param->data.ltk.key_size;
            memcpy (app_env.proxr_device.ltk.ltk.key, param->data.ltk.ltk.key, param->data.ltk.key_size);
            app_env.proxr_device.ltk.ediv = param->data.ltk.ediv;
            memcpy (app_env.proxr_device.ltk.randnb.nb, param->data.ltk.randnb.nb, RAND_NB_LEN);
            break;

        case GAPC_PAIRING_FAILED:
            app_env.proxr_device.bonded = 0;
            app_disconnect();
            break;
    }

    return 0;
}


/**
 ****************************************************************************************
 * @brief Handle reset GAP request.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_reset_req_evt_handler(ke_msg_id_t msgid,
                               struct gapm_cmp_evt *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id)
{
    // We are now in Connectable State
    if (dest_id == TASK_APP)
    {
        app_env.state = APP_IDLE;
        alert_state.ll_alert_lvl = 2;//PROXR_ALERT_HIGH;//Link Loss default Alert Level
        alert_state.adv_toggle = 0; //clear advertise toggle

        // Add Proximity service in the DB
        app_proxr_db_create();
    }

    return 0;
}


/**
 ****************************************************************************************
 * @brief 
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_bond_req_ind_handler(ke_msg_id_t msgid,
                             struct gapc_bond_req_ind *param,
                             ke_task_id_t dest_id,
                             ke_task_id_t src_id)
{
    app_gap_bond_cfm(param);

    return 0;
}

int gapc_encrypt_req_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_encrypt_req_ind * param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    struct gapc_encrypt_cfm* cfm = BleMsgAlloc(GAPC_ENCRYPT_CFM, src_id, dest_id, sizeof (struct gapc_encrypt_cfm));

    if(((app_env.proxr_device.bonded)
        && (memcmp(&(app_env.proxr_device.ltk.randnb), &(param->rand_nb), RAND_NB_LEN) == 0)
        && (app_env.proxr_device.ltk.ediv == param->ediv)))
    {
        cfm->found = true;
        cfm->key_size = app_env.proxr_device.ltk.key_size;
        memcpy(&(cfm->ltk), &(app_env.proxr_device.ltk.ltk), app_env.proxr_device.ltk.key_size);
        // update connection auth
        app_connect_confirm(GAP_AUTH_REQ_NO_MITM_BOND);
    }
    else
    {
        cfm->found = false;
    }

    BleSendMsg(cfm);

    return (KE_MSG_CONSUMED);
}


int gapc_encrypt_ind_handler(ke_msg_id_t const msgid,
                             struct gapc_encrypt_ind *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id)
{
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles Read Tx Power Level response from llc task.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int llc_rd_tx_pw_lvl_cmp_evt_handler(ke_msg_id_t const msgid,
                                     struct llc_rd_tx_pw_lvl_cmd_complete const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    if (param->status == CO_ERROR_NO_ERROR)
    {
        if (app_env.proxr_device.device.conhdl == param->conhdl)  
            alert_state.txp_lvl = param->tx_pow_lvl;
        else
            alert_state.txp_lvl = 0x00;
    }

    // Enable the Proximity Reporter profile
    app_proxr_enable();

    return (KE_MSG_CONSUMED);
}


int app_create_db_cfm_handler(ke_task_id_t const dest_id)
{
    // If state is not idle, ignore the message
    if (app_env.state == APP_IDLE)
    {
    app_set_mode(); //initialize gap mode 
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Update current alert level shown to the user through a LED.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static void update_visual_alert_indication(uint8_t alert_level)
{
    if (alert_level == PROXR_ALERT_NONE)
        turn_off_led();
    else if (alert_level == PROXR_ALERT_MILD)
        green_blink_slow();
    else if (alert_level == PROXR_ALERT_HIGH)
        green_blink_fast();
}

/**
 ****************************************************************************************
 * @brief Handles the PROXR_LEVEL_UPD_IND indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
void proxr_level_upd_ind_handler(ke_msg_id_t msgid,
                                 struct proxr_level_upd_ind  *param,
                                 ke_task_id_t dest_id,
                                 ke_task_id_t src_id)
{
    if (param->char_code == PROXR_IAS_CHAR)
    {
        update_visual_alert_indication(param->alert_lvl);
    }
}


/**
 ****************************************************************************************
 * @brief Handles the PROXR_LLS_ALERT_IND indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
void proxr_lls_alert_ind_handler(ke_msg_id_t msgid,
                                 struct proxr_lls_alert_ind *param,
                                 ke_task_id_t dest_id,
                                 ke_task_id_t src_id)
{
    update_visual_alert_indication(param->alert_lvl);
}
