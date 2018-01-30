/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief Proximity Reporter Host demo application.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
 
#include <stdlib.h>
#include "global_io.h"
#include "peripherals.h"
#include "queue.h"
#include "gpio.h"
#include "app_button_led.h"
#include "ble_msg.h"
#include "gap_task.h"
#include "app.h"
#include "spi_booter.h"
#include "proxm.h"
#include "proxm_task.h"
#include "smpc_task.h"
#include "proxr.h"
#include "proxr_task.h"
#include "atts.h"
#include "spi.h"
#include "spi_hci_msg.h"
#include "uart.h"
#include "user_periph_setup.h"


struct app_env_tag app_env;

uint8_t rd_data[256];

int poll_count;
unsigned int proxm_trans_in_prog = true;

SPI_Pad_t spi_hci_pad;

/**
 ****************************************************************************************
 * @brief Application's main function.
 *
 * @return the program's exit code.
 ****************************************************************************************
 */
int main (void)
{
    spi_hci_pad.pin = SPI_CS_PIN;
    spi_hci_pad.port = SPI_GPIO_PORT;
    
    // peripherals init
    periph_init();
    
    // App Initialization
    app_env.slave_on_sleep = SLAVE_UNAVAILABLE;
    app_env.size_tx_queue = 0;
    app_env.size_rx_queue = 0;
    
#ifdef SPI_BOOTER
    spi_send_image();
#endif
    
    spi_init(&spi_hci_pad, SPI_WORD_MODE, SPI_ROLE, SPI_POL_MODE, SPI_PHA_MODE, SPI_MINT_MODE, SPI_FREQ_MODE);
    dready_irq_enable();
    app_button_enable();
    
    while(1)
    {
        if (app_env.slave_on_sleep == SLAVE_AVAILABLE)
        {
            if (app_env.size_tx_queue > 0)
            {
                ble_msg *blemsg = (ble_msg *) DeQueue(&SPITxQueue);
                spi_send_hci_msg(blemsg->bLength + sizeof(ble_hdr), (uint8_t *) blemsg);
                free(blemsg);
                app_env.size_tx_queue--;
            }
        }
        
        if (app_env.size_rx_queue > 0)
        {
            BleReceiveMsg();
            app_env.size_rx_queue--;
        }
    }
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
    struct gapm_reset_cmd *msg = BleMsgAlloc(GAPM_RESET_CMD /*GAP_RESET_REQ*/, TASK_GAPM, TASK_APP,
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

    BleSendMsg(msg);

    return;
}

/**
 ****************************************************************************************
 * @brief Send enable request to proximity monitor profile task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxr_db_create(void)
{
    // Allocate the message
    struct proxr_create_db_req * req = BleMsgAlloc(PROXR_CREATE_DB_REQ, TASK_PROXR, TASK_APP,
                                                 sizeof(struct proxr_create_db_req));

    // Fill in the parameter structure
    req->features = PROXR_IAS_TXPS_SUP;

    // Send the message
    BleSendMsg(req);
}

void app_set_mode(void)
{
    struct gapm_set_dev_config_cmd *msg = BleMsgAlloc(GAPM_SET_DEV_CONFIG_CMD, TASK_GAPM, TASK_APP,
                                                        sizeof(struct gapm_set_dev_config_cmd));

    msg->operation = GAPM_SET_DEV_CONFIG;
    // Device Role
    msg->role = GAP_PERIPHERAL_SLV;

    // Device Appearance
    msg->appearance = 0x0000;
    
    // Device Appearance write permission requirements for peer device
    msg->appearance_write_perm = GAPM_WRITE_DISABLE;
    // Device Name write permission requirements for peer device
    msg->name_write_perm = GAPM_WRITE_DISABLE;

    // Peripheral only: *****************************************************************
    // Slave preferred Minimum of connection interval
    msg->con_intv_min = 8;         // 10ms (8*1.25ms)
    // Slave preferred Maximum of connection interval
    msg->con_intv_max = 16;        // 20ms (16*1.25ms)
    // Slave preferred Connection latency
    msg->con_latency  = 0;
    // Slave preferred Link supervision timeout
    msg->superv_to    = 100;

    // Privacy settings bit field
    msg->flags = 0;

    BleSendMsg((void *)msg);

    return;
}

void app_adv_start(void)
{
    uint8_t device_name_length;
    uint8_t device_name_avail_space;
    uint8_t device_name_temp_buf[64];

    // Allocate a message for GAP
    struct gapm_start_advertise_cmd *cmd = BleMsgAlloc(GAPM_START_ADVERTISE_CMD,
                                                        TASK_GAPM, TASK_APP,
                                                        sizeof(struct gapm_start_advertise_cmd));

    cmd->op.code     = GAPM_ADV_UNDIRECT;
    cmd->op.addr_src = GAPM_PUBLIC_ADDR;
    cmd->intv_min    = APP_ADV_INT_MIN;
    cmd->intv_max    = APP_ADV_INT_MAX;
    cmd->channel_map = APP_ADV_CHMAP;

    cmd->info.host.mode = GAP_GEN_DISCOVERABLE;

    /*-----------------------------------------------------------------------------------
     * Set the Advertising Data and the Scan Response Data
     *---------------------------------------------------------------------------------*/
    cmd->info.host.adv_data_len       = APP_ADV_DATA_MAX_SIZE;
    cmd->info.host.scan_rsp_data_len  = APP_SCAN_RESP_DATA_MAX_SIZE;

    // Advertising Data
    #if (NVDS_SUPPORT)
    if(nvds_get(NVDS_TAG_APP_BLE_ADV_DATA, &cmd->info.host.adv_data_len,
                &cmd->info.host.adv_data[0]) != NVDS_OK)
    #endif //(NVDS_SUPPORT)
    {
        cmd->info.host.adv_data_len = APP_DFLT_ADV_DATA_LEN;
        memcpy(&cmd->info.host.adv_data[0], APP_DFLT_ADV_DATA, cmd->info.host.adv_data_len);

        //Add list of UUID
        #if (BLE_APP_HT)
        cmd->info.host.adv_data_len += APP_HT_ADV_DATA_UUID_LEN;
        memcpy(&cmd->info.host.adv_data[APP_DFLT_ADV_DATA_LEN], APP_HT_ADV_DATA_UUID, APP_HT_ADV_DATA_UUID_LEN);
        #else
            #if (BLE_APP_NEB)
            cmd->info.host.adv_data_len += APP_NEB_ADV_DATA_UUID_LEN;
            memcpy(&cmd->info.host.adv_data[APP_DFLT_ADV_DATA_LEN], APP_NEB_ADV_DATA_UUID, APP_NEB_ADV_DATA_UUID_LEN);
            #endif //(BLE_APP_NEB)
        #endif //(BLE_APP_HT)
    }

    // Scan Response Data
    #if (NVDS_SUPPORT)
    if(nvds_get(NVDS_TAG_APP_BLE_SCAN_RESP_DATA, &cmd->info.host.scan_rsp_data_len,
                &cmd->info.host.scan_rsp_data[0]) != NVDS_OK)
    #endif //(NVDS_SUPPORT)
    {
        cmd->info.host.scan_rsp_data_len = APP_SCNRSP_DATA_LENGTH;
        memcpy(&cmd->info.host.scan_rsp_data[0], APP_SCNRSP_DATA, cmd->info.host.scan_rsp_data_len);
    }

    // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
    device_name_avail_space = APP_ADV_DATA_MAX_SIZE - cmd->info.host.adv_data_len - 2;

    // Check if data can be added to the Advertising data
    if (device_name_avail_space > 0)
    {
        // Get the Device Name to add in the Advertising Data (Default one or NVDS one)
        #if (NVDS_SUPPORT)
        device_name_length = NVDS_LEN_DEVICE_NAME;
        if (nvds_get(NVDS_TAG_DEVICE_NAME, &device_name_length, &device_name_temp_buf[0]) != NVDS_OK)
        #endif //(NVDS_SUPPORT)
        {
            // Get default Device Name (No name if not enough space)
            device_name_length = strlen(APP_DFLT_DEVICE_NAME);
            memcpy(&device_name_temp_buf[0], APP_DFLT_DEVICE_NAME, device_name_length);
        }

        if(device_name_length > 0)
        {
            // Check available space
            device_name_length = device_name_length < device_name_avail_space ? device_name_length : device_name_avail_space; 

            // Fill Length
            cmd->info.host.adv_data[cmd->info.host.adv_data_len]     = device_name_length + 1;
            // Fill Device Name Flag
            cmd->info.host.adv_data[cmd->info.host.adv_data_len + 1] = '\x09';
            // Copy device name
            memcpy(&cmd->info.host.adv_data[cmd->info.host.adv_data_len + 2], device_name_temp_buf, device_name_length);

            // Update Advertising Data Length
            cmd->info.host.adv_data_len += (device_name_length + 2);
        }
    }

    // Send the message
    BleSendMsg(cmd);

    // We are now connectable
    //ke_state_set(TASK_APP, APP_CONNECTABLE);

    return;
}

void app_connect_confirm(uint8_t auth)
{
    // confirm connection
    struct gapc_connection_cfm *cfm = BleMsgAlloc(GAPC_CONNECTION_CFM, 
                                                  KE_BUILD_ID(TASK_GAPC, app_env.proxr_device.device.conhdl),
                                                  TASK_APP,
                                                  sizeof(struct gapc_connection_cfm));

    cfm->auth = auth;
    cfm->authorize = GAP_AUTHZ_NOT_SET;

    // Send the message
    BleSendMsg(cfm);
}

/**
 ****************************************************************************************
 * @brief Send disconnect request to GAP task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_disconnect(void)
{
    struct gapc_disconnect_cmd * req = BleMsgAlloc(GAPC_DISCONNECT_CMD,
                                                    KE_BUILD_ID(TASK_GAPC, app_env.proxr_device.device.conhdl),
                                                    TASK_APP,
                                                    sizeof(struct gapc_disconnect_cmd ));

    req->operation = GAPC_DISCONNECT;
    req->reason = CO_ERROR_REMOTE_USER_TERM_CON;

    BleSendMsg(req);
}

/**
 ****************************************************************************************
 * @brief Send enable request to proximity reporter profile task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxr_enable(void)
{
    // Allocate the message
    struct proxr_enable_req * req = BleMsgAlloc(PROXR_ENABLE_REQ, TASK_PROXR, TASK_APP,
                                                 sizeof(struct proxr_enable_req));

    // Fill in the parameter structure
    req->conhdl = app_env.proxr_device.device.conhdl;
    // Fill in the parameter structure
    req->sec_lvl = 1; //PERM(SVC, ENABLE);
    req->lls_alert_lvl = (uint8_t) alert_state.ll_alert_lvl;  
    req->txp_lvl = alert_state.txp_lvl; 

    // Send the message
    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief generate ltk key.
 *
 * @return void.
 ****************************************************************************************
 */
void app_sec_gen_ltk(uint8_t key_size)
{
    // Counter
    uint8_t i;
    app_env.proxr_device.ltk.key_size = key_size;

    // Randomly generate the LTK and the Random Number
    for (i = 0; i < RAND_NB_LEN; i++)
    {
        app_env.proxr_device.ltk.randnb.nb[i] = rand()%256;

    }

    // Randomly generate the end of the LTK
    for (i = 0; i < KEY_LEN; i++)
    {
        app_env.proxr_device.ltk.ltk.key[i] = (((key_size) < (16 - i)) ? 0 : rand()%256);
    }

    // Randomly generate the EDIV
    app_env.proxr_device.ltk.ediv = rand()%65536;
}


uint32_t app_gen_tk()
{
    // Generate a PIN Code (Between 100000 and 999999)
    return (100000 + (rand()%900000));
}

/**
 ****************************************************************************************
 * @brief Confirm bonding.
 *
 * @return void.
 ****************************************************************************************
 */
void app_gap_bond_cfm(struct gapc_bond_req_ind *param)
{
    struct gapc_bond_cfm * cfm = BleMsgAlloc(GAPC_BOND_CFM, KE_BUILD_ID(TASK_GAPC, app_env.proxr_device.device.conhdl), TASK_APP,
                                             sizeof(struct gapc_bond_cfm));
    switch(param->request)
    {
        // Bond Pairing request
        case GAPC_PAIRING_REQ:
        {
            

            cfm->request = GAPC_PAIRING_RSP;
            cfm->accept = true;

            // OOB information
            cfm->data.pairing_feat.oob            = GAP_OOB_AUTH_DATA_NOT_PRESENT;
            // Encryption key size
            cfm->data.pairing_feat.key_size       = KEY_LEN;
            // IO capabilities
            cfm->data.pairing_feat.iocap          = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
            // Authentication requirements
            cfm->data.pairing_feat.auth           = GAP_AUTH_REQ_NO_MITM_BOND;
            //Initiator key distribution
            cfm->data.pairing_feat.ikey_dist      = GAP_KDIST_SIGNKEY;
            //Responder key distribution
            cfm->data.pairing_feat.rkey_dist      = GAP_KDIST_ENCKEY;
            //Security requirements
            cfm->data.pairing_feat.sec_req        = GAP_NO_SEC;
        }
        break;

        // Used to retrieve pairing Temporary Key
        case GAPC_TK_EXCH:
        {
            if(param->data.tk_type == GAP_TK_DISPLAY)
            {
                uint32_t pin_code = app_gen_tk();
                cfm->request = GAPC_TK_EXCH;
                cfm->accept = true;

                memset(cfm->data.tk.key, 0, KEY_LEN);

                cfm->data.tk.key[12] = (uint8_t)((pin_code & 0xFF000000) >> 24);
                cfm->data.tk.key[13] = (uint8_t)((pin_code & 0x00FF0000) >> 16);
                cfm->data.tk.key[14] = (uint8_t)((pin_code & 0x0000FF00) >>  8);
                cfm->data.tk.key[15] = (uint8_t)((pin_code & 0x000000FF) >>  0);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        break;
        
        // Used for Long Term Key exchange
        case GAPC_LTK_EXCH:
        {
            // generate ltk
            app_sec_gen_ltk((uint8_t)param->data.key_size);

            cfm->request = GAPC_LTK_EXCH;

            cfm->accept = true;

            cfm->data.ltk.key_size = app_env.proxr_device.ltk.key_size;
            cfm->data.ltk.ediv = app_env.proxr_device.ltk.ediv;

            memcpy(&(cfm->data.ltk.randnb), &(app_env.proxr_device.ltk.randnb) , RAND_NB_LEN);
            memcpy(&(cfm->data.ltk.ltk), &(app_env.proxr_device.ltk.ltk) , KEY_LEN);
        }
        break;
        
        default:
        {
            BleFreeMsg(cfm);//free
            return;
        }
    }

    // Send the message
    BleSendMsg(cfm);
}

void app_send_disconnect(uint16_t dst, uint16_t conhdl, uint8_t reason)
{
    struct gapc_disconnect_ind * disconnect_ind = BleMsgAlloc(GAPC_DISCONNECT_IND,
            dst, TASK_APP, sizeof(struct gapc_disconnect_ind));

    // fill parameters
    disconnect_ind->conhdl   = conhdl;
    disconnect_ind->reason   = reason;

    // send indication
    BleSendMsg(disconnect_ind);
}
