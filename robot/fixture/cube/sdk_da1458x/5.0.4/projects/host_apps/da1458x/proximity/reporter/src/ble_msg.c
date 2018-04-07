/**
 ****************************************************************************************
 *
 * @file ble_msg.c
 *
 * @brief Reception of ble messages sent from DA14580 embedded application over UART interface.
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
#include "ble_msg.h" 
#include "peripherals.h"
#include "queue.h"
#include "app.h"
#include "app_task.h" 
#include "smpc_task.h"

#include "gap_task.h"
#include "proxr_task.h"

#include <stdio.h>

#include "global_io.h"
#include "gpio.h"

#include "spi_hci_msg.h"

extern uint8_t rd_data[256];

/** Internal Functions**/

int HadleGapmCmpEvt(ke_msg_id_t msgid,struct gapm_cmp_evt *param, ke_task_id_t dest_id, ke_task_id_t src_id);
int HadleGapcCmpEvt(ke_msg_id_t msgid,struct gapc_cmp_evt *param, ke_task_id_t dest_id, ke_task_id_t src_id);



/*
 ****************************************************************************************
 * @brief Send message to SPI queue.
 *
 * @param[in] msg   pointer to message.
 *
 * @return void.
 ****************************************************************************************
*/
void BleSendMsg(void *msg)
{
    ble_msg *blemsg = (ble_msg *)((unsigned char *)msg - sizeof (ble_hdr));
    
    if (app_env.slave_on_sleep == SLAVE_AVAILABLE){
        spi_send_hci_msg((uint16_t)(blemsg->bLength + sizeof(ble_hdr)), (uint8_t *) blemsg);
        free(blemsg);
    }
    else{
        EnQueue(&SPITxQueue, blemsg);
        app_env.size_tx_queue++;
    }
    
}

/*
 ****************************************************************************************
 * @brief Allocate memory for ble message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @param[in] param_len Parameters length.
 *
 * @return void.
 ****************************************************************************************
*/
void *BleMsgAlloc(unsigned short id, unsigned short dest_id,
                   unsigned short src_id, unsigned short param_len)
{
    
      unsigned char *msg = (unsigned char *) malloc(sizeof(ble_msg) + param_len - sizeof (unsigned char));
      ble_msg *blemsg = (ble_msg *) msg;//(ble_msg *) malloc(sizeof(ble_msg) + param_len - sizeof (unsigned char));

    blemsg->bType    = id; // add 0x4C in MSB of bType 
    blemsg->bDstid   = dest_id;
    blemsg->bSrcid   = src_id;
    blemsg->bLength  = param_len;

    if (param_len)
        memset(blemsg->bData, 0, param_len);

    return &blemsg->bData;
}

/*
 ****************************************************************************************
 * @brief Free ble msg.
 *
 * @param[in] msg   pointer to message.
 *
 * @return void.
 ****************************************************************************************
*/
void BleFreeMsg(void *msg)
{
    ble_msg *blemsg = (ble_msg *)((unsigned char *)msg - sizeof (ble_hdr));

    free(blemsg);
}

/*
 ****************************************************************************************
 * @brief Handles ble by calling corresponding procedure.
 *
 * @param[in] blemsg    Pointer to received message.
 *
 * @return void.
 ****************************************************************************************
*/
void HandleBleMsg(ble_msg *blemsg)
{

    if (blemsg->bDstid != TASK_APP && blemsg->bDstid != TASK_GTL)
        return;
    
    switch (blemsg->bType)
    {
        case GAPM_CMP_EVT:  
            HadleGapmCmpEvt(blemsg->bType, (struct gapm_cmp_evt *)blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break; 
        case GAPC_CMP_EVT:  
            HadleGapcCmpEvt(blemsg->bType, (struct gapc_cmp_evt *)blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break; 
        case GAPM_DEVICE_READY_IND: 
            gap_ready_evt_handler(blemsg->bType, (struct gap_ready_evt *)blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case GAPM_GET_DEV_INFO_CMD:
            gap_dev_inq_req_cmp_evt_handler(blemsg->bType, (struct gap_dev_inq_req_cmp_evt *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case GAPM_ADV_REPORT_IND:
            gap_dev_inq_result_handler(blemsg->bType, (struct gapm_adv_report_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case GAPC_CONNECTION_REQ_IND:
            gap_le_create_conn_req_ind_handler(blemsg->bType, (struct gapc_connection_req_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case GAPC_DISCONNECT_IND:
            gap_discon_cmp_evt_handler(blemsg->bType,  (struct gapc_disconnect_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case GAPC_CON_RSSI_IND:
            gap_read_rssi_cmp_evt_handler(blemsg->bType, (struct gapc_con_rssi_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case GAPC_BOND_IND:
            gap_bond_ind_handler(blemsg->bType,  (struct gapc_bond_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case GAPC_BOND_REQ_IND:
            gap_bond_req_ind_handler(blemsg->bType, (struct gapc_bond_req_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case GAPC_ENCRYPT_REQ_IND:
            gapc_encrypt_req_ind_handler(blemsg->bType, (struct gapc_encrypt_req_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case GAPC_ENCRYPT_IND:
            gapc_encrypt_ind_handler(blemsg->bType, (struct gapc_encrypt_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
            case PROXR_CREATE_DB_CFM:
            app_create_db_cfm_handler(blemsg->bDstid);
            break;
        case PROXR_LEVEL_UPD_IND:
            proxr_level_upd_ind_handler(blemsg->bDstid, (struct proxr_level_upd_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case PROXR_LLS_ALERT_IND:
            proxr_lls_alert_ind_handler(blemsg->bDstid, (struct proxr_lls_alert_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case PROXR_DISABLE_IND:
            break;
        case LLC_RD_TX_PW_LVL_CMP_EVT:
            llc_rd_tx_pw_lvl_cmp_evt_handler(LLC_RD_TX_PW_LVL_CMP_EVT,
                                      (struct llc_rd_tx_pw_lvl_cmd_complete *) blemsg->bData, 
                                      blemsg->bDstid, blemsg->bSrcid);
            break;
        default:
            break;
    }

}

/*
 ****************************************************************************************
 * @brief Receives ble message from UART iface.
 *
 * @return void.
 ****************************************************************************************
*/
void BleReceiveMsg(void)
{
    ble_msg *msg;
    if(SPIRxQueue.First != NULL)
    {
        msg = (ble_msg*) DeQueue(&SPIRxQueue);
        HandleBleMsg((ble_msg*)msg);
        free(msg);
    }
    
}

/**
 ****************************************************************************************
 * @brief Handles GAPM_CMP_EVT message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int HadleGapmCmpEvt(ke_msg_id_t msgid,
                    struct gapm_cmp_evt *param,
                    ke_task_id_t dest_id,
                    ke_task_id_t src_id)
{
    if (param->status ==  CO_ERROR_NO_ERROR)
    {
    switch(param->operation)
    {
        case GAPM_NO_OP:// No operation.
            break;
        case GAPM_RESET:// Reset BLE subsystem: LL and HL.
            gapm_reset_req_evt_handler (msgid, (struct gapm_cmp_evt *)param, dest_id, src_id);//IZP to check
            break;
        case GAPM_CANCEL:// Cancel currently executed operation.
            break;
        case GAPM_SET_DEV_CONFIG:// Set device configuration
            return gapm_set_mode_req_evt_handler(msgid, (struct gapm_cmp_evt *)param, dest_id, src_id);
        case GAPM_SET_DEV_NAME: // Set device name
            break;
        case GAPM_SET_CHANNEL_MAP:// Set device channel map
            break;
        case  GAPM_GET_DEV_NAME:// Get Local device name
            break;
        case GAPM_GET_DEV_VERSION:// Get Local device version
            break;
        case GAPM_GET_DEV_BDADDR:// Get Local device BD Address
            break;
        case GAPM_GET_WLIST_SIZE:// Get White List Size.
            break;
        case GAPM_ADD_DEV_IN_WLIST:// Add devices in white list.
            break;
        case GAPM_RMV_DEV_FRM_WLIST:// Remove devices form white list.
            break;
        case GAPM_CLEAR_WLIST:// Clear all devices from white list.
            break;
        case GAPM_ADV_NON_CONN:// Start non connectable advertising
        case GAPM_ADV_UNDIRECT:// Start undirected connectable advertising
        case GAPM_ADV_DIRECT:// Start directed connectable advertising
            break;
        case GAPM_SCAN_ACTIVE:// Start active scan operation
        case GAPM_SCAN_PASSIVE:   // Start passive scan operation
            break;
        case GAPM_CONNECTION_DIRECT:// Direct connection operation
            break;
        case GAPM_CONNECTION_AUTO:// Automatic connection operation
            break;
        case GAPM_CONNECTION_SELECTIVE:// Selective connection operation
            break;
        case GAPM_CONNECTION_NAME_REQUEST:// Name Request operation (requires to start a direct connection)
            break;
        case GAPM_RESOLV_ADDR:// Resolve device address
            break;
        case GAPM_GEN_RAND_ADDR:// Generate a random address
            break;
        case GAPM_USE_ENC_BLOCK:// Use the controller's AES-128 block
            break;
        case GAPM_GEN_RAND_NB:// Generate a 8-byte random number
            break;
        case  GAPM_DBG_GET_MEM_INFO:// Get memory usage
            break;
        case  GAPM_PLF_RESET:// Perform a platform reset
            break;
        case  GAPM_GET_DEV_ADV_TX_POWER:// Get device advertising power level
            break;
        }
}
  return (KE_MSG_CONSUMED);
}

int HadleGapcCmpEvt(ke_msg_id_t msgid,
                      struct gapc_cmp_evt *param,
                      ke_task_id_t dest_id,
                      ke_task_id_t src_id)
{

    switch(param->operation)
    {
        case GAPC_NO_OP: // No operation
            break;
        case GAPC_DISCONNECT:    // Disconnect link
            break;
        case GAPC_GET_PEER_NAME: // Retrieve name of peer device
            break; 
        case GAPC_GET_PEER_VERSION:// Retrieve peer device version info.
            break; 
        case GAPC_GET_PEER_FEATURES:// Retrieve peer device features.
            break; 
        case GAPC_GET_CON_RSSI:// Retrieve connection RSSI.
            break; 
        case GAPC_GET_PRIVACY:// Retrieve Privacy Info.
            break; 
        case GAPC_GET_RECON_ADDR: // Retrieve Reconnection Address Value.
            break; 
        case GAPC_SET_PRIVACY:// Set Privacy flag.
            break; 
        case GAPC_SET_RECON_ADDR:// Set Reconnection Address Value.
            break; 
        case GAPC_UPDATE_PARAMS:// Perform update of connection parameters.
            break; 
        case GAPC_BOND:// Start bonding procedure.
            break; 
        case GAPC_ENCRYPT:// Start encryption procedure.
            break; 
        case GAPC_SECURITY_REQ:// Start security request procedure
            break; 
    }
    return -1;
}
