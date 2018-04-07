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
 
#include "gapc_task.h"
#include "gapm_task.h"
#include "ble_msg.h" 
#include "queue.h" 
#include "app.h" 
#include "smpc_task.h"
#include "app_task.h"
#include "proxr_task.h"
#include "diss_task.h"
#include "uart.h"
#include "app_spotar.h"

/** Internal Functions**/

int HadleGapmCmpEvt(ke_msg_id_t msgid,struct gapm_cmp_evt *param, ke_task_id_t dest_id, ke_task_id_t src_id);
int HadleGapcCmpEvt(ke_msg_id_t msgid,struct gapc_cmp_evt *param, ke_task_id_t dest_id, ke_task_id_t src_id);

/*
 ****************************************************************************************
 * @brief Send message to UART iface.
 *
 * @param[in] msg   pointer to message.
 *
 * @return void.
 ****************************************************************************************
*/
void BleSendMsg(void *msg)
{
    ble_msg *blemsg = (ble_msg *)((unsigned char *)msg - sizeof (ble_hdr));

    UARTSend(blemsg->bLength + sizeof(ble_hdr), (unsigned char *) blemsg);

    free(blemsg);
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
    ble_msg *blemsg = (ble_msg *) malloc(sizeof(ble_msg) + param_len - sizeof (unsigned char));

    blemsg->bType    = id;
    blemsg->bDstid   = dest_id;
    blemsg->bSrcid   = src_id;
    blemsg->bLength  = param_len;

    if (param_len)
        memset(blemsg->bData, 0, param_len);

    return blemsg->bData;
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
    if (blemsg->bDstid != TASK_GTL)
        return;

    switch (blemsg->bType)
    {
        //
        // GAPM events
        //

        // Command Complete event
        case GAPM_CMP_EVT:  
            HadleGapmCmpEvt(blemsg->bType, (struct gapm_cmp_evt *)blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break; 

        // Event triggered to inform that lower layers are ready
        case GAPM_DEVICE_READY_IND: 
            gapm_device_ready_ind_handler(blemsg->bType, (struct gap_ready_evt *)blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        // Local device name indication event
        case GAPM_DEV_NAME_IND:
            break;

        // Local device appearance indication event
        case GAPM_APPEARANCE_IND:
            break;

        // Local device version indication event
        case GAPM_DEV_VERSION_IND:
            break;

        // Local device BD Address indication event
        case GAPM_DEV_BDADDR_IND:
            break;

        // White List Size indication event
        case GAPM_WHITE_LIST_SIZE_IND:
            break;

        // Advertising or scanning report information event
        case GAPM_ADV_REPORT_IND:
            gapm_adv_report_ind_handler(blemsg->bType, (struct gapm_adv_report_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        // Name of peer device indication
        case GAPM_PEER_NAME_IND:
            break;

        // Privacy flag value has been updated
        case GAPM_UPDATED_PRIVACY_IND:
            break;

        // Reconnection address has been updated
        case GAPM_UPDATED_RECON_ADDR_IND:
            break;

        // Indicate that resolvable random address has been solved
        case GAPM_ADDR_SOLVED_IND:
            break;

        //  AES-128 block result indication
        case GAPM_USE_ENC_BLOCK_IND:
            break;

        // Random Number Indication
        case GAPM_GEN_RAND_NB_IND:
            break;

        // Indication containing information about memory usage.
        case GAPM_DBG_MEM_INFO_IND:
            break;

        // Advertising channel Tx power level
        case GAPM_DEV_ADV_TX_POWER_IND:
            break;

        // Limited discoverable timeout indication
        case GAPM_LIM_DISC_TO_IND:
            break;

        // Scan timeout indication
        case GAPM_SCAN_TO_IND:
            break;

        // Address renewal timeout indication
        case GAPM_ADDR_RENEW_TO_IND:
            break;

        //
        // GAPC events
        //

        // Command Complete event
        case GAPC_CMP_EVT:  
            HadleGapcCmpEvt(blemsg->bType, (struct gapc_cmp_evt *)blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break; 

        // Indicate that a connection has been established
        case GAPC_CONNECTION_REQ_IND:
            gapc_connection_req_ind_handler(blemsg->bType, (struct gapc_connection_req_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        // Indicate that a link has been disconnected
        case GAPC_DISCONNECT_IND:
            gapc_disconnect_ind_handler(blemsg->bType,  (struct gapc_disconnect_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        // Name of peer device indication
        case GAPC_PEER_NAME_IND:
            break;

        // Indication of peer version info
        case GAPC_PEER_VERSION_IND:
            break;

        // Indication of peer features info
        case GAPC_PEER_FEATURES_IND:
            break;

        // Indication of ongoing connection RSSI
        case GAPC_CON_RSSI_IND:
            gapc_con_rssi_ind_handler(blemsg->bType, (struct gapc_con_rssi_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        // Indication of peer privacy info
        case GAPC_PRIVACY_IND:
            break;

        // Indication of peer reconnection address info
        case GAPC_RECON_ADDR_IND:
            break;

        // Request of updating connection parameters indication
        case GAPC_PARAM_UPDATE_REQ_IND:
            break;

        // Connection parameters updated indication
        case GAPC_PARAM_UPDATED_IND:
            break;

        // Bonding requested by peer device indication message.
        case GAPC_BOND_REQ_IND:
            gapc_bond_req_ind_handler(blemsg->bType, (struct gapc_bond_req_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        // Bonding information indication message
        case GAPC_BOND_IND:
             gapc_bond_ind_handler(blemsg->bType,  (struct gapc_bond_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        // Encryption requested by peer device indication message.
        case GAPC_ENCRYPT_REQ_IND:
            gapc_encrypt_req_ind_handler(blemsg->bType, (struct gapc_encrypt_req_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        // Encryption information indication message
        case GAPC_ENCRYPT_IND:
            gapc_encrypt_ind_handler(blemsg->bType, (struct gapc_encrypt_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        // Security requested by peer device indication message
        case GAPC_SECURITY_IND:
            break;

        // Indicate the current sign counters to the application
        case GAPC_SIGN_COUNTER_IND:
            break;

        // Indication of ongoing connection Channel Map
        case GAPC_CON_CHANNEL_MAP_IND:
            break;

        // Parameter update procedure timeout indication
        case GAPC_PARAM_UPDATE_TO_IND:
            break;

        //
        // PROXR events
        //
        case PROXR_CREATE_DB_CFM:
            proxr_create_db_cfm_handler(blemsg->bDstid);
            break;
        case PROXR_LEVEL_UPD_IND:
            proxr_level_upd_ind_handler(blemsg->bDstid, (struct proxr_level_upd_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case PROXR_LLS_ALERT_IND:
            proxr_lls_alert_ind_handler(blemsg->bDstid, (struct proxr_lls_alert_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;


        case PROXR_DISABLE_IND:
            break;

        //
        // DISS events
        //
        case DISS_CREATE_DB_CFM:
            diss_create_db_cfm_handler(blemsg->bType, (struct diss_create_db_cfm *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
        case DISS_DISABLE_IND:
            break;
        case DISS_ERROR_IND:
            printf("Rcved DISS_ERROR_IND Msg\n");
            break;


		//
		// SPOTAR events
		//
		case SPOTAR_CREATE_DB_CFM:
			spotar_create_db_cfm_handler(blemsg->bDstid, (struct spotar_create_db_cfm *) blemsg->bData);
			break;
		case SPOTAR_DISABLE_IND:
			app_close_image_file(spota_state.upgrade_file);
			break;
		case SPOTAR_PATCH_MEM_DEV_IND:
			spotar_patch_mem_dev_ind_handler(blemsg->bDstid, (struct spotar_patch_mem_dev_ind *) blemsg->bData);
			break;
		case SPOTAR_GPIO_MAP_IND:
			break;
		case SPOTAR_PATCH_LEN_IND:
			spotar_patch_len_ind_handler(blemsg->bDstid, (struct spotar_patch_len_ind *) blemsg->bData);
			break;
		case SPOTAR_PATCH_DATA_IND:
			spotar_patch_data_ind_handler(blemsg->bDstid, (struct spotar_patch_data_ind *) blemsg->bData);
			break;
        default:
            printf("Rcved UNKNOWN Msg 0x%x\n", blemsg->bType);
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
    WaitForSingleObject(UARTRxQueueSem, INFINITE);
    if(UARTRxQueue.First != NULL)
    {
        msg = (ble_msg*) DeQueue(&UARTRxQueue); 
        HandleBleMsg(msg);
        free(msg);
    }
    
    ReleaseMutex(UARTRxQueueSem);
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
    if (param->status == CO_ERROR_NO_ERROR)
    {
        switch(param->operation)
        {
            case GAPM_NO_OP:// No operation.
                break;
            case GAPM_RESET:// Reset BLE subsystem: LL and HL.
                gapm_reset_completion_handler (msgid, (struct gapm_cmp_evt *)param, dest_id, src_id);
                break;
            case GAPM_CANCEL:// Cancel currently executed operation.
                break;
            case GAPM_SET_DEV_CONFIG:// Set device configuration
                return gapm_set_dev_config_completion_handler(msgid, (struct gapm_cmp_evt *)param, dest_id, src_id);
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
            case GAPM_DBG_GET_MEM_INFO:// Get memory usage
                break;
            case GAPM_PLF_RESET:// Perform a platform reset
                break;
            case GAPM_GET_DEV_ADV_TX_POWER:// Get device advertising power level
                break;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAPC_CMP_EVT message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int HadleGapcCmpEvt(ke_msg_id_t msgid,
                    struct gapc_cmp_evt *param,
                    ke_task_id_t dest_id,
                    ke_task_id_t src_id)
{
    switch(param->operation)
    {
        case GAPC_NO_OP: // No operation
            break;
        case GAPC_DISCONNECT: // Disconnect link
            break;
        case GAPC_GET_PEER_NAME: // Retrieve name of peer device
            break; 
        case GAPC_GET_PEER_VERSION: // Retrieve peer device version info.
            break; 
        case GAPC_GET_PEER_FEATURES: // Retrieve peer device features.
            break; 
        case GAPC_GET_CON_RSSI: // Retrieve connection RSSI.
            break; 
        case GAPC_GET_PRIVACY: // Retrieve Privacy Info.
            break; 
        case GAPC_GET_RECON_ADDR: // Retrieve Reconnection Address Value.
            break; 
        case GAPC_SET_PRIVACY: // Set Privacy flag.
            break; 
        case GAPC_SET_RECON_ADDR: // Set Reconnection Address Value.
            break; 
        case GAPC_UPDATE_PARAMS: // Perform update of connection parameters.
            break; 
        case GAPC_BOND: // Start bonding procedure.
            break; 
        case GAPC_ENCRYPT: // Start encryption procedure.
            break; 
        case GAPC_SECURITY_REQ: // Start security request procedure
            break; 
        case GAPC_GET_CON_CHANNEL_MAP: // Retrieve Connection Channel MAP.
            break;
    }
    return (KE_MSG_CONSUMED);
}
