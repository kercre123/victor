/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief Proximity Monitor Host demo application.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <windows.h>
#include <conio.h>
#include <stdint.h>          
#include "console.h"
#include "queue.h"          // queue functions & definitions
#include "ble_msg.h"
#include "uart.h"
#include "gap.h"
//#include "gap_task.h"
#include "gapc_task.h"
#include "gapm_task.h"
#include "app.h"
#include "app_task.h"
#include "proxm.h"
#include "proxm_task.h"
#include "disc.h"
#include "disc_task.h"
#include "smpc_task.h"

struct app_env_tag app_env;
unsigned short Comport = 0xffff; 
int poll_count;
unsigned int proxm_trans_in_prog = true;
uint8_t last_char[MAX_CONN_NUMBER];


/*
 ****************************************************************************************
 * @brief Exit application.
 *
 * @return void.
 ****************************************************************************************
*/
void app_exit(void)
{
    // Stop the threads
    StopConsoleTask = TRUE;
    StopRxTask = TRUE;

    Sleep(100);
}

/**
 ****************************************************************************************
 * @brief Set Bondage mode.
 *
 * @return void.
 ****************************************************************************************
 */
void app_set_mode(void)
{
    struct gapm_set_dev_config_cmd *msg = BleMsgAlloc(GAPM_SET_DEV_CONFIG_CMD, TASK_GAPM, TASK_GTL,
                                                      sizeof(struct gapm_set_dev_config_cmd));

    msg->operation = GAPM_SET_DEV_CONFIG;
    msg->role = GAP_CENTRAL_MST; // Device Role
    memset( msg->irk.key, 0, sizeof(struct gap_sec_key));
    msg->appearance = 0;
    msg->appearance_write_perm = GAPM_WRITE_DISABLE;
    msg->name_write_perm = GAPM_WRITE_DISABLE;

    BleSendMsg((void *)msg);

    return;
}

/**
 ****************************************************************************************
 * @brief Send Reset request to GAP task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_rst_gap(void)
{
    struct gapm_reset_cmd *msg = BleMsgAlloc(GAPM_RESET_CMD, TASK_GAPM, TASK_GTL,
                                             sizeof(struct gapm_reset_cmd));

    int i;

    msg->operation = GAPM_RESET;

    app_env.state = APP_IDLE;
    
    //init scan devices list
    app_env.num_of_devices = 0;

    for (i=0; i < MAX_SCAN_DEVICES; i++)
    {
        app_env.devices[i].free = true;
        app_env.devices[i].adv_addr.addr[0] = '\0';
        app_env.devices[i].data[0] = '\0';
        app_env.devices[i].data_len = 0;
        app_env.devices[i].rssi = 0;
    }
    
    BleSendMsg((void *)msg);

    return;
}

/**
 ****************************************************************************************
 * @brief Send Inquiry (devices discovery) request to GAP task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_inq(void)
{
    struct gapm_start_scan_cmd *msg = BleMsgAlloc(GAPM_START_SCAN_CMD, TASK_GAPM, TASK_GTL,
                                                  sizeof(struct gapm_start_scan_cmd));

    int i;
    //init scan devices list
    app_env.num_of_devices = 0;

    for (i=0; i < MAX_SCAN_DEVICES; i++)
    {
        
        app_env.devices[i].free = true;
        app_env.devices[i].adv_addr.addr[0] = '\0';
        app_env.devices[i].data[0] = '\0';
        app_env.devices[i].data_len = 0;
        app_env.devices[i].rssi = 0;
    }
    msg->mode = GAP_GEN_DISCOVERY;
    msg->op.code = GAPM_SCAN_ACTIVE;
    msg->op.addr_src = GAPM_PUBLIC_ADDR;
    msg->filter_duplic = SCAN_FILT_DUPLIC_EN;
    msg->interval = 10;
    msg->window = 5;

    BleSendMsg((void *)msg);

    return;
}

/**
 ****************************************************************************************
 * @brief Send Connect request to GAP task.
 *
 * @param[in] indx  Peer device's index in discovered devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_connect(unsigned char indx)
{
    struct gapm_start_connection_cmd *msg;
	/*
    if (app_env.devices[indx].free == true)
    {
        return;
    }*/
    msg = (struct gapm_start_connection_cmd *) BleMsgAlloc(GAPM_START_CONNECTION_CMD , TASK_GAPM, TASK_GTL,
                                                           sizeof(struct gapm_start_connection_cmd ));

    msg->nb_peers = 1;
    memcpy((void *) &msg->peers[0].addr, (void *)&app_env.devices[indx].adv_addr.addr, BD_ADDR_LEN);
    msg->con_intv_min = 100;
    msg->con_intv_max = 100;
    msg->ce_len_min = 0x0;
    msg->ce_len_max = 0x5;
    msg->con_latency = 0;
    msg->op.addr_src = GAPM_PUBLIC_ADDR;
    msg->peers[0].addr_type = GAPM_PUBLIC_ADDR;
    msg->superv_to = 0x1F4;// 500 -> 5000 ms ;
    msg->scan_interval = 0x180;
    msg->scan_window = 0x160;
    msg->op.code = GAPM_CONNECTION_DIRECT;
    
    BleSendMsg((void *) msg);
}

/**
 ****************************************************************************************
 * @brief Send Read rssi request to GAP task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_read_rssi(void)
{
	int i;
	for (i=0;i<MAX_CONN_NUMBER;i++){
		if (app_env.proxr_device[i].isactive == true){
			struct gapc_get_info_cmd * req = BleMsgAlloc(GAPC_GET_INFO_CMD, KE_BUILD_ID(TASK_GAPC, app_env.proxr_device[i].device.conidx), TASK_GTL,
                                                 sizeof(struct gapc_get_info_cmd));
    req->operation = GAPC_GET_CON_RSSI;
    BleSendMsg((void *) req);
}
	}
}

/**
 ****************************************************************************************
 * @brief Send disconnect request to GAP task.
 *
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_disconnect(int con_id)
{

	struct gapc_disconnect_cmd *req = BleMsgAlloc(GAPC_DISCONNECT_CMD, KE_BUILD_ID(TASK_GAPC, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                                 sizeof(struct gapc_disconnect_cmd));

    req->operation = GAPC_DISCONNECT;
    req->reason = CO_ERROR_REMOTE_USER_TERM_CON;


    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Send pairing request.
 *
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void
 ****************************************************************************************
 */
void app_security_enable(int con_id)
{
    // Allocate the message
    struct gapc_bond_cmd * req = BleMsgAlloc(GAPC_BOND_CMD, KE_BUILD_ID(TASK_GAPC, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                             sizeof(struct gapc_bond_cmd));
    

    req->operation = GAPC_BOND;

    req->pairing.sec_req = GAP_NO_SEC;  //GAP_SEC1_NOAUTH_PAIR_ENC;

    // OOB information
    req->pairing.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;

    // IO capabilities
    req->pairing.iocap          = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;

    // Authentication requirements
    req->pairing.auth           = GAP_AUTH_REQ_NO_MITM_BOND; //SMP_AUTH_REQ_NO_MITM_NO_BOND;

    // Encryption key size
    req->pairing.key_size       = 16;

    //Initiator key distribution
    req->pairing.ikey_dist      = 0x04; //SMP_KDIST_ENCKEY | SMP_KDIST_IDKEY | SMP_KDIST_SIGNKEY;

    //Responder key distribution
    req->pairing.rkey_dist      = 0x03; //SMP_KDIST_ENCKEY | SMP_KDIST_IDKEY | SMP_KDIST_SIGNKEY;

    // Send the message
    BleSendMsg(req);
}

/**
 ****************************************************************************************
 * @brief Send connection confirmation.
 *
 *  @param[in] auth  Authentication requirements.
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_connect_confirm(uint8_t auth,int con_id)
{
    // confirm connection
    struct gapc_connection_cfm *cfm = BleMsgAlloc(GAPC_CONNECTION_CFM, KE_BUILD_ID(TASK_GAPC, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                                  sizeof (struct gapc_connection_cfm));

    cfm->auth = auth;
    cfm->authorize = GAP_AUTHZ_NOT_SET;

    // Send the message
    BleSendMsg(cfm);
}

/**
 ****************************************************************************************
 * @brief generate key.
 *
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_gen_csrk(int con_id)
{
    // Counter
    uint8_t i;

    // Randomly generate the CSRK
    for (i = 0; i < KEY_LEN; i++)
    {
        app_env.proxr_device[con_id].csrk.key[i] = (((KEY_LEN) < (16 - i)) ? 0 : rand()%256);
    }

}

/**
 ****************************************************************************************
 * @brief Confirm bonding.
 *
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_gap_bond_cfm(int con_id)
{
    struct gapc_bond_cfm * req = BleMsgAlloc(GAPC_BOND_CFM, KE_BUILD_ID(TASK_GAPC, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                             sizeof(struct gapc_bond_cfm));

    req->request = GAPC_CSRK_EXCH;

    req->accept = 0; //accept

    req->data.pairing_feat.auth = GAP_AUTH_REQ_NO_MITM_BOND;

    req->data.pairing_feat.sec_req = GAP_SEC2_NOAUTH_DATA_SGN;

    app_gen_csrk(con_id);

    memcpy(req->data.csrk.key, app_env.proxr_device[con_id].csrk.key, KEY_LEN); 

    // Send the message
    BleSendMsg(req);

}

/**
 ****************************************************************************************
 * @brief Start Encryption with pre-agreed key.
 *
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_start_encryption(int con_id)
{
    // Allocate the message
    struct gapc_encrypt_cmd * req = BleMsgAlloc(GAPC_ENCRYPT_CMD, KE_BUILD_ID(TASK_GAPC, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                                sizeof(struct gapc_encrypt_cmd));


    req->operation = GAPC_ENCRYPT;
    memcpy(&req->ltk.ltk, &app_env.proxr_device[con_id].ltk, sizeof(struct gapc_ltk));
    
    // Send the message
    BleSendMsg(req);

}

/**
 ****************************************************************************************
 * @brief Send enable request to proximity monitor profile task.
 *
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxm_enable(int con_id)
{
        
    // Allocate the message
    struct proxm_enable_req * req = BleMsgAlloc(PROXM_ENABLE_REQ, KE_BUILD_ID(TASK_PROXM, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                                sizeof(struct proxm_enable_req));

    // Fill in the parameter structure
    req->conhdl = app_env.proxr_device[con_id].device.conhdl;
    req->con_type = PRF_CON_DISCOVERY;
        
    // Send the message
    BleSendMsg((void *) req);

}

/**
 ****************************************************************************************
 * @brief Send read request for Link Loss Alert Level characteristic to proximity monitor profile task.
 *
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxm_read_llv(int con_id)
{   
    struct proxm_rd_alert_lvl_req * req = BleMsgAlloc(PROXM_RD_ALERT_LVL_REQ,  KE_BUILD_ID(TASK_PROXM, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                                      sizeof(struct proxm_rd_alert_lvl_req));
    
    last_char[con_id] = PROXM_RD_LL_ALERT_LVL;

    req->conhdl = app_env.proxr_device[con_id].device.conhdl;
    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Send read request for Tx Power Level characteristic to proximity monitor profile task.
 *
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxm_read_txp(int con_id)
{
    struct proxm_rd_txpw_lvl_req * req = BleMsgAlloc(PROXM_RD_TXPW_LVL_REQ, KE_BUILD_ID(TASK_PROXM, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                                     sizeof(struct proxm_rd_txpw_lvl_req));
    
    last_char[con_id] = PROXM_RD_TX_POWER_LVL;
    
    req->conhdl = app_env.proxr_device[con_id].device.conhdl;
    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Send write request to proximity monitor profile task.
 *
 *  @param[in] chr Characteristic to be written.
 *  @param[in] val Characteristic's Value.
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxm_write(unsigned int chr, unsigned char val, int con_id)
{   
    struct proxm_wr_alert_lvl_req * req = BleMsgAlloc(PROXM_WR_ALERT_LVL_REQ,  KE_BUILD_ID(TASK_PROXM, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                                      sizeof(struct proxm_wr_alert_lvl_req));
    
    req->conhdl = app_env.proxr_device[con_id].device.conhdl;
    req->svc_code = chr;
    req->lvl = val;
    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Send enable request to DIS client profile task.
 *
 *  @param[in] con_id	Index in connected devices list.

 * @return void.
 ****************************************************************************************
 */
void app_disc_enable(unsigned int con_id)
{       
    // Allocate the message
    struct proxm_enable_req * req = BleMsgAlloc(DISC_ENABLE_REQ, TASK_DISC, TASK_GTL,
                                                sizeof(struct proxm_enable_req));

    // Fill in the parameter structure
    req->conhdl = app_env.proxr_device[con_id].device.conhdl;
    req->con_type = PRF_CON_DISCOVERY;
        
    // Send the message
    BleSendMsg((void *) req);

}

/**
 ****************************************************************************************
 * @brief Send read request to DIS Client profile task.
 *
 *  @param[in] char_code Characteristic to be retrieved.
 *  @param[in] con_id	Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_disc_rd_char(uint8_t char_code, int con_id)
{   
	struct disc_rd_char_req * req = BleMsgAlloc(DISC_RD_CHAR_REQ, KE_BUILD_ID(TASK_DISC, app_env.proxr_device[con_id].device.conidx), TASK_GTL,
                                                sizeof(struct disc_rd_char_req));
    req->conhdl = app_env.proxr_device[con_id].device.conhdl;
    req->char_code = char_code; 

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief bd addresses compare.
 *
 *  @param[in] bd_address1  Pointer to bd_address 1.
 *  @param[in] bd_address2  Pointer to bd_address 2.
 *
 * @return true if addresses are equal / false if not.
 ****************************************************************************************
 */
bool bdaddr_compare(struct bd_addr *bd_address1,
                    struct bd_addr *bd_address2)
{
    unsigned char idx;

    for (idx=0; idx < BD_ADDR_LEN; idx++)
    {
        /// checks if the addresses are similar
        if (bd_address1->addr[idx] != bd_address2->addr[idx])
        {
           return (false);
        }
    }
    return(true);
}

/**
 ****************************************************************************************
 * @brief Check if device is in application's discovered devices list.
 *
 *  @param[in] padv_addr  Pointer to devices bd_addr.
 *
 * @return Index in list. if return value equals MAX_SCAN_DEVICES device is not listed.
 ****************************************************************************************
 */
unsigned char app_device_recorded(struct bd_addr *padv_addr)
{
    int i;

    for (i=0; i < MAX_SCAN_DEVICES; i++)
    {
        if (app_env.devices[i].free == false)
            if (bdaddr_compare(&app_env.devices[i].adv_addr, padv_addr))
                break;
    }

    return i;
            
}

/**
 ****************************************************************************************
 * @brief Cancel the ongoing air operation.
 *
 * @return void
 ****************************************************************************************
 */
void app_cancel(void)
{
    struct gapm_cancel_cmd *msg;
    
    msg = (struct gapm_cancel_cmd *) BleMsgAlloc(GAPM_CANCEL_CMD, TASK_GAPM, TASK_GTL,
                                                 sizeof(struct gapm_cancel_cmd));

    msg->operation = GAPM_CANCEL;

    BleSendMsg((void *) msg);
}

/**
 ****************************************************************************************
 * @brief Handles commands sent from console interface.
 *
 * @return void.
 ****************************************************************************************
 */
static void ConsoleEvent(void)
{
    console_msg *msg;
    WaitForSingleObject(ConsoleQueueSem, INFINITE);

    if(ConsoleQueue.First != NULL)
    {
        msg = (console_msg*) DeQueue(&ConsoleQueue); 
        switch(msg->type)
        {
            case CONSOLE_DEV_DISC_CMD:
				if (console_main_pass_state){
					app_cancel_gap();
					Sleep(100);
					app_inq();
				}
                 if (app_env.state == APP_IDLE)
                 {
                    app_env.state = APP_SCAN;
                    ConsoleScan();
                    app_inq();
                 }
                 else if (app_env.state == APP_SCAN)
                 {
                     app_rst_gap();
                 }
                 break;
           
            case CONSOLE_CONNECT_CMD:
                if ((app_env.state == APP_IDLE) || console_main_pass_state)
                {
					app_connect(msg->idx);
                }
                else if (app_env.state == APP_SCAN)
                {
                    app_cancel();
                    ConsoleSendConnnect(msg->idx);
                }
                 break;

            case CONSOLE_RD_LLV_CMD:
                 if (app_env.state == APP_CONNECTED && !proxm_trans_in_prog)
                 {
                    proxm_trans_in_prog = 3;
                    app_proxm_read_llv(msg->idx);
                 }
                 break;
            case CONSOLE_RD_TXP_CMD:
                 if (app_env.state == APP_CONNECTED && !proxm_trans_in_prog)
                 {
                    proxm_trans_in_prog = 3;
					app_proxm_read_txp(msg->idx);
                 }
                 break;
            case CONSOLE_WR_LLV_CMD:
                if (app_env.state == APP_CONNECTED && !proxm_trans_in_prog)
                 {
                    proxm_trans_in_prog = 3;
                    app_proxm_write(PROXM_SET_LK_LOSS_ALERT, msg->val,msg->idx);
                }
                break;
            case CONSOLE_WR_IAS_CMD:
                if (app_env.state == APP_CONNECTED && !proxm_trans_in_prog)
                {
                    proxm_trans_in_prog = 3;
                    app_proxm_write(PROXM_SET_IMMDT_ALERT, msg->val,msg->idx);
                }
                break;
            case CONSOLE_DISCONNECT_CMD:
                 if (app_env.state == APP_CONNECTED)
                 {
					 app_disconnect(msg->val);
                 }
                break;
            case CONSOLE_EXIT_CMD:               
				app_rst_gap();
				Sleep(100);
                app_exit();
                break;

            default:
                 break;
        }

        free(msg);
    }
    
    ReleaseMutex(ConsoleQueueSem);

}

/**
 ****************************************************************************************
 * @brief Read COM port number from console
 *
 * @return the COM port number.
 ****************************************************************************************
 */
unsigned int ConsoleReadPort(void)
{
    char buffer[256];
    char *buffer2;

    buffer2 = fgets(buffer, 256, stdin);

    if (buffer2 && buffer2[0] != '\n')
    {
        return atoi(buffer2);
    }
    else
        return 0xffffffff;

}

/**
 ****************************************************************************************
* @brief Find first available slot in device list.
*
* @return device list index
****************************************************************************************
*/
unsigned int rtrn_fst_avail(void){
	int i;
	for (i=0;i<MAX_CONN_NUMBER;i++){
		if (app_env.proxr_device[i].isactive == false){
		return i;
		}
	}
	return MAX_CONN_NUMBER;
}

/**
****************************************************************************************
* @brief Find previously available connection device and return it.
*
* @return valid connection index
****************************************************************************************
**/
unsigned int rtrn_prev_avail_conn(void){
	unsigned int icnt=0;
	for (icnt=app_env.cur_dev;icnt>0;icnt--){
		if (app_env.proxr_device[icnt].isactive == true){
		return icnt;
		}
	}
	for (icnt=MAX_CONN_NUMBER-1;icnt>app_env.cur_dev;icnt--){
		if (app_env.proxr_device[icnt].isactive == true){
		return icnt;
		}
	}
	return 0;
}


/**
****************************************************************************************
* @brief Find the number of active connections
*
* @return the number of active connections
****************************************************************************************
**/
unsigned int rtrn_con_avail(void){
	int i,con_avail=0;
	for (i=0;i<MAX_CONN_NUMBER;i++){
		if (app_env.proxr_device[i].isactive == true){
		con_avail++;
		}
	}
	return con_avail;
}

/**
 ****************************************************************************************
 * @brief Cancel last GAPM command
 *
 * @return void 
 ****************************************************************************************
 */
void app_cancel_gap(void)
{
    struct gapm_cancel_cmd *msg = BleMsgAlloc(GAPM_CANCEL_CMD, TASK_GAPM, TASK_GTL,
                                             sizeof(struct gapm_cancel_cmd));


    msg->operation = GAPM_CANCEL;

    BleSendMsg((void *) msg);
}

/**
 ****************************************************************************************
 * @brief Resolve connection index (cidx).
 * 
 *  @param[in] cidx Connection index
 *
 * @return the index in connected devices list.
 ****************************************************************************************
 */
unsigned short res_conidx(unsigned short cidx)
{
	int i; 
    for (i=0; i < MAX_SCAN_DEVICES; i++)
    {
		if (app_env.proxr_device[i].device.conidx==cidx){
		return i;
		}
    } 
	return MAX_SCAN_DEVICES;
}

/**
 **************************************************************************************** 
 * @brief Returns the index that corresponds to the given connection handle
 *
 *  @param[in] conhdl Connection handler
 *
 * @return the index in connected devices list.
 ****************************************************************************************
 */
unsigned short res_conhdl(uint16_t conhdl)
{
	int i; 
    for (i=0; i < MAX_SCAN_DEVICES; i++)
    {
		if (app_env.proxr_device[i].device.conhdl==conhdl){
		return i;
		}
    } 
	return MAX_SCAN_DEVICES;  //exclude value
}

/**
 ****************************************************************************************
 * @brief Application's main function.
 * 
 *  @param[in] argc  The number of command line arguments.
 *  @param[in] argv  The array that contains the command line arguments.
 *
 * @return the program's exit code.
 ****************************************************************************************
 */
int main(int argc, char** argv)
{
	int cnt;

	app_env.cur_dev=0;
	for (cnt=0;cnt<MAX_CONN_NUMBER;cnt++){
	app_env.proxr_device[cnt].isactive = false;					//Sets all slots as inactive
	last_char[cnt]=0xFF;										//Sets default value for the TxP and LLA indicator of each conn
	}

    // initializing com port with command line args
    ConsoleTitle();

    if((argc == 2) && ((argv[1][1] >= '0') && (argv[1][1] <= '9')) && ((argv[1][0] >= '0') && (argv[1][0] <= '9')))
    {
        printf ("DBG: %c  %c", argv[1][0], argv[1][1]);
        Comport = ((argv[1][1]) - '0');
        Comport += (((argv[1][0]) - '0') * 10);
    }
    else if ((argc == 2) && ((argv[1][0] >= '0') && (argv[1][0] <= '9')))
    {
        Comport = ((argv[1][0]) - '0');
    }
    else
    {
        printf("No command line arguments.\n");
        // keep prompting user until he enters a value between 1-65535 or a blank line
        while(1) {
            int com_port_number;

            printf("Enter COM port number (values: 1-65535, blank to exit): ");
            com_port_number= ConsoleReadPort();

            if (com_port_number > 0 && com_port_number <= 0xFFFF) 
            {
                Comport = com_port_number;
                break;
            }

            if (com_port_number == 0xFFFFFFFF)
                exit(0);
        }
    }
  
    if (!InitUART(Comport, 115200))
        InitTasks();
    else
        exit (EXIT_FAILURE);

    printf("Waiting for DA1458x Device\n");

    app_rst_gap();

    while((StopConsoleTask == false) && (StopRxTask == false))
    {
        BleReceiveMsg();

        ConsoleEvent();

        if (app_env.state == APP_CONNECTED)
        {
            poll_count++;
          
            if (poll_count == 200)	//was initially 1 sec (poll_count == 10)
            {
                poll_count = 0;
                app_read_rssi();
            }
        }

        Sleep(10);

        if (proxm_trans_in_prog > 0)
            proxm_trans_in_prog--;

    }

    exit (EXIT_SUCCESS);
}