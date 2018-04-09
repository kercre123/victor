/**
 ****************************************************************************************
 *
 * @file app_task.c
 *
 * @brief Handling of ble events and responses.
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


#include "app_task.h" 
#include "app.h" 
#include "queue.h" 
#include "console.h" 
//#include "gap_task.h" 
#include "gapc_task.h"
#include "gapm_task.h"
#include "proxm.h"
#include "proxm_task.h" 
#include "disc.h"
#include "disc_task.h"
#include "smpc_task.h" 
#include "ble_msg.h" 

extern unsigned int proxm_trans_in_prog;
extern uint8_t last_char[MAX_CONN_NUMBER];

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
static void app_find_device_name(unsigned char * adv_data, unsigned char adv_data_len, unsigned char dev_indx)
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
 * @brief Handles GAPM command completion events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

int gapm_cmp_evt_handler(ke_msg_id_t msgid,
                         struct gapm_cmp_evt *param,
                         ke_task_id_t dest_id,
                         ke_task_id_t src_id)
{
    if (param->status ==  CO_ERROR_NO_ERROR)
    {
        if(param->operation == GAPM_RESET)
        {
            app_set_mode(); //initialize gap mode 
        }
        else if(param->operation == GAPM_SET_DEV_CONFIG)
        {
            app_env.state = APP_SCAN;

            Sleep(100);

            app_inq();  //start scanning

            ConsoleScan();
        }
    }
    else
    {
		if(!console_main_pass_state){		
        if(param->operation == GAPM_SCAN_ACTIVE || param->operation == GAPM_SCAN_PASSIVE)
        {
            app_env.state = APP_IDLE;
        }
    }
	}
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAPC_CMP_EVT messages.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_cmp_evt_handler(ke_msg_id_t msgid,
                         struct gapc_cmp_evt *param,
                         ke_task_id_t dest_id,
                         ke_task_id_t src_id)
{
	int con_id;
	con_id=res_conidx(KE_IDX_GET(src_id));
    
    switch (param->operation)
    {
        case GAPC_ENCRYPT:
            if (param->status != CO_ERROR_NO_ERROR)
            {
                app_env.proxr_device[con_id].bonded = 0;
				app_disconnect(con_id);
            }
            break;

        default:
            break;
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles ready indication from GAPM.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_device_ready_ind_handler(ke_msg_id_t msgid,
                                  void *param,
                                  ke_task_id_t dest_id,
                                  ke_task_id_t src_id)
{
    //do nothing
    
	return 0;
}

/**
 ****************************************************************************************
 * @brief Handles GAPM_ADV_REPORT_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_adv_report_ind_handler(ke_msg_id_t msgid,
                                struct gapm_adv_report_ind *param,
                                ke_task_id_t dest_id,
                                ke_task_id_t src_id)
{
    unsigned char recorded;

        recorded = app_device_recorded(&param->report.adv_addr); 

        if (recorded <MAX_SCAN_DEVICES) //update Name
        {
            app_find_device_name(param->report.data,param->report.data_len, recorded); 
            ConsoleScan();
        }
        else
        {
            app_env.devices[app_env.num_of_devices].free = false;
            app_env.devices[app_env.num_of_devices].rssi = (char)(((474 * param->report.rssi)/1000) - 112.4) ;
            memcpy(app_env.devices[app_env.num_of_devices].adv_addr.addr, param->report.adv_addr.addr, BD_ADDR_LEN );
            app_find_device_name(param->report.data,param->report.data_len, app_env.num_of_devices); 
            ConsoleScan();
            app_env.num_of_devices++;
        }       
            
    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles the GAPC_CONNECTION_REQ_IND event.
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
int gapc_connection_req_ind_handler(ke_msg_id_t msgid,
                                    struct gapc_connection_req_ind *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id)
{
    int i;
	int con_id;
	con_id=rtrn_fst_avail();
    app_env.proxr_device[con_id].isactive =true;  
    start_pair = 1;

    if ((app_env.state == APP_IDLE) || console_main_pass_state) 
    {
        // We are now connected

        app_env.state = APP_CONNECTED;

        // Retrieve the connection index from the GAPC task instance for this connection
        app_env.proxr_device[con_id].device.conidx = KE_IDX_GET(src_id);

        // Retrieve the connection handle from the parameters
        app_env.proxr_device[con_id].device.conhdl = param->conhdl;

        // On Reconnection check if device is bonded and send pairing request. Otherwise it is not bonded.
        if (bdaddr_compare(&app_env.proxr_device[con_id].device.adv_addr, &param->peer_addr))
        {
            if (app_env.proxr_device[con_id].bonded)
                start_pair = 0;
        }
                
        memcpy(app_env.proxr_device[con_id].device.adv_addr.addr, param->peer_addr.addr, sizeof(struct bd_addr));

        for (i =0 ; i<RSSI_SAMPLES; i++)
            app_env.proxr_device[con_id].rssi[i] = -127;

        app_env.proxr_device[con_id].alert = 0;
        app_env.proxr_device[con_id].rssi_indx = 0;
        app_env.proxr_device[con_id].avg_rssi = -127;
        app_env.proxr_device[con_id].txp = -127;
        app_env.proxr_device[con_id].llv = 0xFF;
        memset(&app_env.proxr_device[con_id].dis, 0, sizeof(dis_env));
                
        ConsoleConnected(0);

        app_connect_confirm(GAP_AUTH_REQ_NO_MITM_NO_BOND,con_id);

        app_proxm_enable(con_id);
        app_disc_enable(con_id);
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles GAPC_DISCONNECT_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_disconnect_ind_handler(ke_msg_id_t msgid,
                                struct gapc_disconnect_ind *param,
                                ke_task_id_t dest_id,
                                ke_task_id_t src_id)
{
	int con_id;
	con_id=res_conidx(KE_IDX_GET(src_id));
    if (param->conhdl == app_env.proxr_device[con_id].device.conhdl)
    {

		if(!console_main_pass_state){					
        app_env.state = APP_SCAN;
        Sleep(100);
			if ((param->reason != CO_ERROR_REMOTE_USER_TERM_CON) &&
				(param->reason != CO_ERROR_CON_TERM_BY_LOCAL_HOST)){

            app_rst_gap();          
        }
		}
		app_env.proxr_device[con_id].isactive =false;
		app_env.proxr_device[con_id].bonded=0;
		app_env.proxr_device[con_id].llv = 0xFF;
		app_env.proxr_device[app_env.cur_dev].txp = -127;
		last_char[con_id]=0xFF;
		app_env.cur_dev=rtrn_prev_avail_conn();
		conn_num_flag=false;
        ConsoleScan();
		if(rtrn_con_avail()==0 && console_main_pass_state){
			console_main_pass_state=false;
			app_env.state = APP_IDLE;
			ConsoleSendScan();
		}
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles GAPC_CON_RSSI_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

int gapc_con_rssi_ind_handler(ke_msg_id_t msgid,
                              struct gapc_con_rssi_ind *param,
                              ke_task_id_t dest_id,
                              ke_task_id_t src_id)
{
    int count = 0;
    int sum = 0;
    int con_id;
    // The param->rssi field of GAPC_CON_RSSI_IND contains the raw RSSI value divided by 2.
    uint8_t raw_rssi = 2 * param->rssi;

	con_id=res_conidx(KE_IDX_GET(src_id));

    app_env.proxr_device[con_id].rssi[app_env.proxr_device[con_id].rssi_indx] = (char)(((474 * raw_rssi)/1000) - 112.4);
    app_env.proxr_device[con_id].rssi_indx ++;

    if (app_env.proxr_device[con_id].rssi_indx == RSSI_SAMPLES)
        app_env.proxr_device[con_id].rssi_indx = 0;

    while ((count <RSSI_SAMPLES) && (app_env.proxr_device[con_id].rssi[count]) != -127)
    {
        sum += app_env.proxr_device[con_id].rssi[count];
        count ++;
    }

    app_env.proxr_device[con_id].avg_rssi = (char) (sum/count);

    if (((char)app_env.proxr_device[con_id].avg_rssi - (char)app_env.proxr_device[con_id].txp) < -80)
    {
        if (!app_env.proxr_device[con_id].alert)
        {
        app_proxm_write(PROXM_SET_IMMDT_ALERT,PROXM_ALERT_MILD,con_id);
        app_env.proxr_device[con_id].alert = 1;
        }
    }
    else if (app_env.proxr_device[con_id].alert)
    {
        app_proxm_write(PROXM_SET_IMMDT_ALERT,PROXM_ALERT_NONE,con_id);
        app_env.proxr_device[con_id].alert = 0;

    }

    ConsoleConnected(1);

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
int gapc_bond_ind_handler(ke_msg_id_t msgid,
                          struct gapc_bond_ind *param,
                          ke_task_id_t dest_id,
                          ke_task_id_t src_id)
{
	int con_id;
	con_id=res_conidx(KE_IDX_GET(src_id));
    switch (param->info)
    {
        case GAPC_PAIRING_SUCCEED:
            if (param->data.auth | GAP_AUTH_BOND)
                app_env.proxr_device[con_id].bonded = 1;
            break;

        case GAPC_IRK_EXCH:
            memcpy (app_env.proxr_device[con_id].irk.irk.key, param->data.irk.irk.key, KEY_LEN);
            memcpy (app_env.proxr_device[con_id].irk.addr.addr.addr, param->data.irk.addr.addr.addr, KEY_LEN);
            app_env.proxr_device[con_id].irk.addr.addr_type = param->data.irk.addr.addr_type;
            break;

        case GAPC_LTK_EXCH:
            app_env.proxr_device[con_id].ediv = param->data.ltk.ediv;
            memcpy (app_env.proxr_device[con_id].rand_nb, param->data.ltk.randnb.nb, RAND_NB_LEN);
            app_env.proxr_device[con_id].ltk.key_size = param->data.ltk.key_size;
            memcpy (app_env.proxr_device[con_id].ltk.ltk.key, param->data.ltk.ltk.key, param->data.ltk.key_size);
            app_env.proxr_device[con_id].ltk.ediv = param->data.ltk.ediv;
            memcpy (app_env.proxr_device[con_id].ltk.randnb.nb, param->data.ltk.randnb.nb, RAND_NB_LEN);
            break;

        case GAPC_PAIRING_FAILED:
            app_env.proxr_device[con_id].bonded = 0;
			app_disconnect(con_id);
            break;
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handle GAPC_BOND_REQ_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_bond_req_ind_handler(ke_msg_id_t msgid,
                              struct gapc_bond_req_ind *param,
                              ke_task_id_t dest_id,
                              ke_task_id_t src_id)
{
	int con_id=res_conidx(KE_IDX_GET(src_id));
    switch (param->request)
    {
        case GAPC_CSRK_EXCH:
            app_gap_bond_cfm(con_id);
            break;

        default:
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
int gap_reset_req_cmp_evt_handler(ke_msg_id_t msgid,
                                  struct gap_reset_req_cmp_evt *param,
                                  ke_task_id_t dest_id,
                                  ke_task_id_t src_id)
{   
    // We are now in Connectable State
    if (dest_id == TASK_GTL)
    {
        app_env.state = APP_IDLE;           
        app_set_mode(); //initialize gap mode 

    }
    
    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles Proximity Monitor profile enable confirmation.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  proxm_enable_cfm_handler(ke_msg_id_t msgid,
                              struct proxm_enable_cfm *param,
                              ke_task_id_t dest_id,
                              ke_task_id_t src_id)
{
	int con_id;
	con_id=res_conidx(KE_IDX_GET(src_id));
    if (param->status == CO_ERROR_NO_ERROR)
    {
        //initialize proximity reporter's values to non valid
        app_env.proxr_device[con_id].llv = 0xFF;
        app_env.proxr_device[con_id].txp = 0xFF;
            

        ConsoleConnected(1);

        if (start_pair)
        {
            app_env.proxr_device[con_id].bonded = 0;
            app_security_enable(con_id);
        }
        else
            app_start_encryption(con_id);
    }

    app_proxm_read_txp(con_id);

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles Read characteristic response.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  proxm_rd_char_rsp_handler(ke_msg_id_t msgid,
                               struct proxm_rd_char_rsp *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id)
{
	int con_id;
	con_id=res_conidx(KE_IDX_GET(src_id));
    
    if (param->info.status == CO_ERROR_NO_ERROR)
    {
        if (last_char[con_id] == PROXM_RD_LL_ALERT_LVL)
            app_env.proxr_device[con_id].llv = param->info.value[0];
        else if (last_char[con_id] == PROXM_RD_TX_POWER_LVL)
            app_env.proxr_device[con_id].txp = param->info.value[0];

        last_char[con_id] = 0xFF;
        proxm_trans_in_prog = 0;

        ConsoleConnected(1);
    }

    return 0;   
}

/**
 ****************************************************************************************
 * @brief Handles write characteristic response.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  proxm_wr_char_rsp_handler(ke_msg_id_t msgid,
                               struct proxm_wr_char_rsp *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id)
{
    if (param->status == CO_ERROR_NO_ERROR)
    {
        proxm_trans_in_prog = 0;
        ConsoleConnected(1);
    }
    
    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles DIS Client profile enable confirmation.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  disc_enable_cfm_handler(ke_msg_id_t msgid,
                             struct disc_enable_cfm *param,
                             ke_task_id_t dest_id,
                             ke_task_id_t src_id)
{
    int i;
	int id;
	id=res_conhdl(param->conhdl);

    if (param->status == CO_ERROR_NO_ERROR)
    {
        
        for ( i = 0; i < DISC_CHAR_MAX; i++)
            app_env.proxr_device[id].dis.chars[i].val_hdl = param->dis.chars[i].val_hdl;

        app_disc_rd_char(DISC_MANUFACTURER_NAME_CHAR, id);
        app_disc_rd_char(DISC_MODEL_NB_STR_CHAR, id);
        app_disc_rd_char(DISC_SERIAL_NB_STR_CHAR, id);
        app_disc_rd_char(DISC_HARD_REV_STR_CHAR, id);
        app_disc_rd_char(DISC_FIRM_REV_STR_CHAR, id);
        app_disc_rd_char(DISC_SW_REV_STR_CHAR, id);
        app_disc_rd_char(DISC_SYSTEM_ID_CHAR, id);
        app_disc_rd_char(DISC_IEEE_CHAR, id);
        app_disc_rd_char(DISC_PNP_ID_CHAR, id);

    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles DIS read characteristic response.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  disc_rd_char_rsp_handler(ke_msg_id_t msgid,
                              struct disc_rd_char_rsp *param,
                              ke_task_id_t dest_id,
                              ke_task_id_t src_id)
{
    uint8_t i;
    uint8_t char_code = DISC_CHAR_MAX;
	int id;
	id=res_conhdl(param->info.conhdl);
    
    if (param->info.status == CO_ERROR_NO_ERROR)
    {
        for ( i = 0; i < DISC_CHAR_MAX; i++)
        {
            if (app_env.proxr_device[id].dis.chars[i].val_hdl == param->info.handle)
            {
                char_code = i;
                break;
            }
        }

        if (char_code != DISC_CHAR_MAX)
        {
            memcpy (app_env.proxr_device[id].dis.chars[char_code].val, param->info.value, param->info.length);
            app_env.proxr_device[id].dis.chars[char_code].val[param->info.length] = '\0';
            app_env.proxr_device[id].dis.chars[char_code].len = param->info.length;
        }
    }

    
    return 0;
}