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

#include "ble_msg.h" 
#include "queue.h" 

#include "gapm_task.h"
#include "gapc_task.h"
#include "gattc_task.h"

#include "app_task.h"
#include "uart.h"

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
 * @brief Handles ble message by calling corresponding procedure.
 *
 * @param[in] blemsg    Pointer to received message.
 *
 * @return void.
 ****************************************************************************************
*/
void HandleBleMsg(ble_msg *blemsg)
{
    if (blemsg->bDstid != TASK_GTL && blemsg->bDstid != TASK_GTL)
        return;

    switch (blemsg->bType)
    {
        case GAPM_DEVICE_READY_IND:
            gap_ready_evt_handler(GAPM_DEVICE_READY_IND, (struct gap_ready_evt *)blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GAPM_CMP_EVT:
            gap_cmp_evt_handler(GAPM_CMP_EVT, (struct gapm_cmp_evt *)blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GAPM_ADV_REPORT_IND:
            gap_dev_inq_result_handler(GAPM_ADV_REPORT_IND, (struct gapm_adv_report_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GAPC_CMP_EVT:
            gapc_cmp_evt_handler(GAPC_CMP_EVT, (struct gapc_cmp_evt *)blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GAPC_CONNECTION_REQ_IND:
            gapc_connection_req_ind_handler(GAPC_CONNECTION_REQ_IND, (struct gapc_connection_req_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GAPC_DISCONNECT_IND:
            gapc_disconnect_ind_handler(GAPC_DISCONNECT_IND, (struct gapc_disconnect_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GAPC_BOND_IND:
            gap_bond_ind_handler(GAPC_BOND_IND, (struct gapc_bond_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GAPC_BOND_REQ_IND:
            gap_bond_req_ind_handler(GAPC_BOND_REQ_IND, (struct gapc_bond_req_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GATTC_DISC_SVC_IND:
            gattc_disc_svc_ind_handler(GATTC_DISC_SVC_IND, (struct gattc_disc_svc_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GATTC_DISC_CHAR_IND:
            gattc_disc_char_ind_handler(GATTC_DISC_CHAR_IND, (struct gattc_disc_char_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;
            
        case GATTC_DISC_CHAR_DESC_IND:
            gattc_disc_char_desc_ind_handler(GATTC_DISC_CHAR_IND, (struct gattc_disc_char_desc_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GATTC_EVENT_IND:
            gattc_event_ind_handler(GATTC_EVENT_IND, (struct gattc_event_ind *) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GATTC_CMP_EVT:
            gattc_cmp_evt_handler(GATTC_CMP_EVT, (struct gattc_cmp_evt*) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
            break;

        case GATTC_READ_IND:
            gattc_read_ind_handler(GATTC_READ_IND, (struct gattc_read_ind*) blemsg->bData, blemsg->bDstid, blemsg->bSrcid);
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

    WaitForSingleObject(UARTRxQueueSem, INFINITE);

    if(UARTRxQueue.First != NULL)
    {
        msg = (ble_msg*) DeQueue(&UARTRxQueue); 
        HandleBleMsg(msg);
        free(msg);
    }
    
    ReleaseMutex(UARTRxQueueSem);
}
