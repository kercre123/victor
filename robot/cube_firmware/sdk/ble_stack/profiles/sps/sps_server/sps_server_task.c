/**
 ****************************************************************************************
 *
 * @file sps_server_task.c
 *
 * @brief Serial Port Service Device profile task.
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

/**
 ****************************************************************************************
 * @addtogroup SPS_SERVER_TASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h"
#include "attm_util.h"
#include "prf_utils.h"
#include "app_sps_scheduler.h"
#include "sps_server_task.h"
#include "uart_sps.h"
#include "atts_util.h"

#include "app_spss.h"

#if BLE_SPS_SERVER

//Flags
//Transmission bussy flag counting the expected GATTC_NOTIFY indications
uint8_t tx_busy_flag __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

///Description
static const uint8_t sps_server_tx_desc[] = SPS_SERVER_TX_DESC;
static const uint8_t sps_server_rx_desc[] = SPS_SERVER_RX_DESC;
static const uint8_t sps_server_flow_ctrl_desc[] = SPS_SERVER_FLOW_CTRL_DESC;


/// Service
const struct att_uuid_128 sps_svc     = {SPS_SERVICE_UUID};

 
/// TX Data characteristic UUID
const struct att_uuid_128 sps_data_tx_val     = {SPS_SERVER_TX_UUID};

struct att_char128_desc sps_data_server_tx_char = {ATT_CHAR_PROP_NTF,
                                                    {0,0},
                                                    SPS_SERVER_TX_UUID}; 

// RX Data characteristic UUID
const struct att_uuid_128 sps_data_rx_val     = {SPS_SERVER_RX_UUID};

struct att_char128_desc sps_data_server_rx_char = {ATT_CHAR_PROP_WR_NO_RESP,
                                                    {0,0},
                                                    SPS_SERVER_RX_UUID}; 

//Flow characteristic UUID
const struct att_uuid_128 sps_flow_ctrl_val     = {SPS_FLOW_CTRL_UUID};

struct att_char128_desc sps_flow_ctrl_char = {ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF,
                                                    {0,0},
                                                    SPS_FLOW_CTRL_UUID}; 

/*
 * Test parameters
 ****************************************************************************************
 */

/*
 * Profile functions
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SPS_SERVER_CREATE_DB_REQ message.
 * The handler adds SPS_SERVER Service into the database using the database
 * configuration value given in param.
 * @param[in] msgid Id of the message received 
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

static int sps_server_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct sps_server_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    
    uint8_t status;
    uint8_t nb_att_16;
    uint8_t nb_att_128;
    uint8_t nb_att_32;
    uint16_t att_decl_svc = ATT_DECL_PRIMARY_SERVICE;
    uint16_t att_decl_char = ATT_DECL_CHARACTERISTIC;
    uint16_t att_decl_cfg = ATT_DESC_CLIENT_CHAR_CFG;
    uint16_t att_decl_desc = ATT_DESC_CHAR_USER_DESCRIPTION;
    uint16_t val_hdl;
    uint16_t char_hdl; 
    uint16_t desc_hdl; 
   	
    //Save Application ID
    sps_server_env.appid = src_id;

    //Add Service Into Database
    nb_att_16 = 10; //  UUID16 atts
    nb_att_32 = 0;// No UUID32 att
    nb_att_128 = 3; // UUID128 att
                                             
    do 
    {
        status = attmdb_add_service(&(sps_server_env.shdl), TASK_SPS_SERVER,
                                             nb_att_16, nb_att_32, nb_att_128,440); // 16 + (19 + 160 + 2 + 14) + (19 + 160 + 2 + 14) + (19 + 1 + 2 +12) = 440
        
        if (status != ATT_ERR_NO_ERROR)
            break;
        
        //add svc attribute
        status = attmdb_add_attribute(sps_server_env.shdl, ATT_UUID_128_LEN, //Data size = 16 (ATT_UUID_128_LEN)
                                           ATT_UUID_16_LEN, (uint8_t*)&att_decl_svc, PERM(RD, ENABLE),
                                           &(sps_server_env.shdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;
        
        status = attmdb_att_set_value(sps_server_env.shdl, ATT_UUID_128_LEN, (uint8_t *)sps_svc.uuid);
        
        if (status != ATT_ERR_NO_ERROR)
            break;
        
	//RX Data characteristic     
        //add char attribute
        status = attmdb_add_attribute(sps_server_env.shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                           ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                           &(char_hdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;

        
        //add val attribute
        status = attmdb_add_attribute(sps_server_env.shdl, 160, //Data size = 160
                                           ATT_UUID_128_LEN, (uint8_t*)&sps_data_tx_val.uuid,  PERM(NTF, ENABLE),
                                           &(val_hdl));

        if (status != ATT_ERR_NO_ERROR)
            break;
        
        memcpy(sps_data_server_tx_char.attr_hdl, &val_hdl, sizeof(uint16_t));
        
        status = attmdb_att_set_value(char_hdl, sizeof(sps_data_server_tx_char), (uint8_t *)&sps_data_server_tx_char);
        
        if (status != ATT_ERR_NO_ERROR)
            break;
        
        //add cfg attribute
        status = attmdb_add_attribute(sps_server_env.shdl, sizeof(uint16_t), 
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_cfg,  PERM(WR, ENABLE),
                                               &(val_hdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;

        status = attmdb_add_attribute(sps_server_env.shdl, SPS_SERVER_TX_DESC_LEN, 
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_desc, PERM(RD, ENABLE),
                                               &(desc_hdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;
        
        status = attmdb_att_set_value(desc_hdl, SPS_SERVER_TX_DESC_LEN, (uint8_t*) sps_server_tx_desc);
        
        if (status != ATT_ERR_NO_ERROR)
            break;

	//TX Data characteristics
        //add char attribute
        status = attmdb_add_attribute(sps_server_env.shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                           ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                           &(char_hdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;

        
        //add val attribute
        status = attmdb_add_attribute(sps_server_env.shdl, 160, //Data size = 160
                                           ATT_UUID_128_LEN, (uint8_t*)&sps_data_rx_val.uuid, PERM(WR, ENABLE),
                                           &(val_hdl));

        if (status != ATT_ERR_NO_ERROR)
            break;
        
        memcpy(sps_data_server_rx_char.attr_hdl, &val_hdl, sizeof(uint16_t));
        
        status = attmdb_att_set_value(char_hdl, sizeof(sps_data_server_rx_char), (uint8_t *)&sps_data_server_rx_char);
        
        if (status != ATT_ERR_NO_ERROR)
            break;
        
        //add cfg attribute
        status = attmdb_add_attribute(sps_server_env.shdl, sizeof(uint16_t), // 2
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_cfg, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                               &(val_hdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;

        status = attmdb_add_attribute(sps_server_env.shdl, SPS_SERVER_RX_DESC_LEN, // 4
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_desc, PERM(RD, ENABLE),
                                               &(desc_hdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;
        
        status = attmdb_att_set_value(desc_hdl, SPS_SERVER_RX_DESC_LEN, (uint8_t*) sps_server_rx_desc);
        
        if (status != ATT_ERR_NO_ERROR)
            break;

//Flow Ctrl characteristic     
        //add char attribute
        status = attmdb_add_attribute(sps_server_env.shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                           ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                           &(char_hdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;

        
        //add val attribute
        status = attmdb_add_attribute(sps_server_env.shdl, sizeof(uint8_t), //Data size = 1
                                           ATT_UUID_128_LEN, (uint8_t*)&sps_flow_ctrl_val.uuid, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE),
                                           &(val_hdl));

        if (status != ATT_ERR_NO_ERROR)
            break;

        
        memcpy(sps_flow_ctrl_char.attr_hdl, &val_hdl, sizeof(uint16_t));
        
        status = attmdb_att_set_value(char_hdl, sizeof(sps_flow_ctrl_char), (uint8_t *)&sps_flow_ctrl_char);
        
        if (status != ATT_ERR_NO_ERROR)
            break;
        
        //add cfg attribute
        status = attmdb_add_attribute(sps_server_env.shdl, sizeof(uint16_t), 
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_cfg, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                               &(val_hdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;

        status = attmdb_add_attribute(sps_server_env.shdl, SPS_SERVER_FLOW_CTRL_DESC_LEN, 
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_desc, PERM(RD, ENABLE),
                                               &(desc_hdl));
        
        if (status != ATT_ERR_NO_ERROR)
            break;
        
        status = attmdb_att_set_value(desc_hdl, SPS_SERVER_FLOW_CTRL_DESC_LEN, (uint8_t*) sps_server_flow_ctrl_desc);
        
        break;
        
    }while(0);
    
    //Disable GLS
    attmdb_svc_set_permission(sps_server_env.shdl, PERM(SVC, DISABLE));

    //Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle test
        ke_state_set(TASK_SPS_SERVER, SPS_SERVER_IDLE);
    }

    //Send response to application
    struct sps_server_create_db_cfm * cfm = KE_MSG_ALLOC(SPS_SERVER_CREATE_DB_CFM, sps_server_env.appid,
                                                    TASK_SPS_SERVER, sps_server_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SPS_SERVER_ENABLE_REQ message.
 * The handler enables the Serial Port Service Device profile.
 * @param[in] msgid Id of the message received .
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance 
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int sps_server_enable_req_handler(ke_msg_id_t const msgid,
                                    struct sps_server_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    //Save Application ID
    sps_server_env.con_info.prf_id = dest_id;
    sps_server_env.con_info.appid = src_id;
    sps_server_env.appid = src_id;
    
    // Save the connection handle associated to the profile
    sps_server_env.con_info.conidx = gapc_get_conidx(param->conhdl);
    
    // Save the connection handle associated to the profile
    sps_server_env.conhdl = param->conhdl;
   
    //Enable Service
    attmdb_svc_set_permission(sps_server_env.shdl, PERM(SVC, ENABLE));
	
    //Reset flags
    tx_busy_flag = 0;
    
    // Go to active state
    ke_state_set(TASK_SPS_SERVER, SPS_SERVER_ACTIVE);
       
    //Send response to application
    struct sps_server_enable_cfm * cfm = KE_MSG_ALLOC(SPS_SERVER_ENABLE_CFM, sps_server_env.appid,
                                                    TASK_SPS_SERVER, sps_server_enable_cfm);
    
    cfm->data_hdl = sps_server_env.shdl + SPS_SERVER_TX_DATA_VAL;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SPS_SERVER_ENABLE_REQ message.
 * The handler enables the Serial Port Service Device profile.
 * @param[in] msgid Id of the message received .
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance 
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                    struct gapc_disconnect_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    //Disable SPS in database
    attmdb_svc_set_permission(sps_server_env.shdl, PERM(SVC, DISABLE));

    //Go to idle state
    ke_state_set(TASK_SPS_SERVER, SPS_SERVER_IDLE);
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref gattc_cmp_evt message.
 * The handler enables the Serial Port Service Device profile.
 * @param[in] msgid Id of the message received .
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance 
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gattc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t *data_ptr;
    unsigned read_amount;
    uint8_t res;
    
    if(param->req_type == GATTC_NOTIFY)
    {
        tx_busy_flag--;
        if ((tx_busy_flag == 0) && (ble_flags.txAllowed == TRUE))
		{
            if ((read_amount = app_item_count(&uarttoble_buffer)) > TX_WAIT_LEVEL)
            {
                if (read_amount > TX_SIZE)
                {
                    read_amount = TX_SIZE;
                }
                read_amount = (read_amount<TX_START_FRAME_DATA_SIZE    ? read_amount : (((read_amount-TX_START_FRAME_DATA_SIZE) / TX_CONTINUE_FRAME_DATA_SIZE)*TX_CONTINUE_FRAME_DATA_SIZE + TX_START_FRAME_DATA_SIZE));
                read_amount = app_get_item_address(&uarttoble_buffer, &data_ptr, read_amount);
                res = attmdb_att_set_value(sps_server_env.shdl + SPS_SERVER_TX_DATA_VAL, sizeof(uint8_t) * read_amount, data_ptr);
                if (res == ATT_ERR_NO_ERROR)
                {
                    app_release_items(&uarttoble_buffer, read_amount);
                    prf_server_send_event((prf_env_struct *)&sps_server_env, false, sps_server_env.shdl + SPS_SERVER_TX_DATA_VAL);
                    tx_busy_flag++;
                    app_check_uart_xon();
                }
            }
        }
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Send first part of data in buffer @ref sps_server_send_notify_flow_control_state_req message.
 * @param[in] msgid Id of the message received 
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int sps_server_init_ble_tx_req_handler(ke_msg_id_t const msgid,
                                struct sps_server_init_ble_tx_req const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t *data_ptr;
    unsigned read_amount;
    uint8_t res;

    if (ke_state_get(TASK_SPS_SERVER)==SPS_SERVER_ACTIVE)
    {
        if ((tx_busy_flag == 0) && (ble_flags.txAllowed == TRUE))
        {
            if ((read_amount = app_item_count(&uarttoble_buffer)) > 0)
            {
                if (read_amount > TX_SIZE)
                {
                    read_amount = TX_SIZE;
                }
                read_amount = (read_amount<TX_START_FRAME_DATA_SIZE ? read_amount : (((read_amount-TX_START_FRAME_DATA_SIZE) / TX_CONTINUE_FRAME_DATA_SIZE)*TX_CONTINUE_FRAME_DATA_SIZE + TX_START_FRAME_DATA_SIZE));
                read_amount = app_get_item_address(&uarttoble_buffer, &data_ptr, read_amount);
                res = attmdb_att_set_value(sps_server_env.shdl + SPS_SERVER_TX_DATA_VAL, sizeof(uint8_t) * read_amount, data_ptr);
                if (res == ATT_ERR_NO_ERROR)
                {
                    app_release_items(&uarttoble_buffer, read_amount);
                    prf_server_send_event((prf_env_struct *)&sps_server_env, false, sps_server_env.shdl + SPS_SERVER_TX_DATA_VAL);
                    tx_busy_flag++;
                    app_check_uart_xon();
                }
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Send flow control state @ref sps_server_send_notify_flow_control_state_req message.
 * @param[in] msgid Id of the message received 
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int sps_server_send_notify_flow_control_state_req_handler(ke_msg_id_t const msgid,
                                   struct sps_server_send_notify_flow_control_state_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
 
{
    // Set value in data base
    attmdb_att_set_value(sps_server_env.shdl + SPS_FLOW_CTRL_VAL, sizeof(param->flow_control_state), (uint8_t*) &(param->flow_control_state ));

    // Send notification to the client
    prf_server_send_event((prf_env_struct *)&(sps_server_env), false, sps_server_env.shdl + SPS_FLOW_CTRL_VAL);
    
    //Increase busy flag
    tx_busy_flag++;
    
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_WRITE_CMD_IND message.
 * Receive and proces incoming data 
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_cmd_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{    
    if (param->handle == sps_server_env.shdl + SPS_SERVER_TX_DATA_CFG) 
    {
        //send write response if required
        if (param->response==0x01)
        {
            atts_write_rsp_send(sps_server_env.con_info.conidx, param->handle, PRF_ERR_OK);
        }
    }
    else if (param->handle == (sps_server_env.shdl + SPS_SERVER_RX_DATA_VAL))
    {
        // send data to the application
        app_ble_push((uint8_t*)&(param->value[0]), param->length);
    }
    else if (param->handle == (sps_server_env.shdl + SPS_SERVER_RX_DATA_CFG))
    {
        //send write response if required
        if (param->response==0x01)
        {
            atts_write_rsp_send(sps_server_env.con_info.conidx, param->handle, PRF_ERR_OK);
        }
    }
    else if (param->handle == sps_server_env.shdl + SPS_FLOW_CTRL_VAL)
    {
        //update ble flow control flag
        app_update_transmit_allowed(&ble_flags, param->value[0]);
    }
    else if (param->handle == sps_server_env.shdl + SPS_FLOW_CTRL_CFG)
    {
        //provide ble flow control state when flow control att is configured to 0x0100
        if (param->value[0]==0x01 && param->value[1]==0x00)
        {
            struct sps_server_request_flow_control_ind * msg = KE_MSG_ALLOC(SPS_SERVER_REQ_FLOW_CONTROL_IND, sps_server_env.appid,
                                                    TASK_SPS_SERVER, sps_server_request_flow_control_ind);
    
            ke_msg_send(msg);
        }
        //send write response if required
        if (param->response==0x01)
        {
            atts_write_rsp_send(sps_server_env.con_info.conidx, param->handle, PRF_ERR_OK);				
        }
    }
    return (KE_MSG_CONSUMED);
}


/*
 * TASK DESCRIPTOR DEFINITIONS
 ****************************************************************************************
 */
/// Disabled State handler definition.
const struct ke_msg_handler sps_server_disabled[] =
{
    {SPS_SERVER_CREATE_DB_REQ,              (ke_msg_func_t) sps_server_create_db_req_handler}
};

/// IDLE State handlers definition
const struct ke_msg_handler sps_server_idle[] =
{
    {SPS_SERVER_ENABLE_REQ,                 (ke_msg_func_t)sps_server_enable_req_handler}
};

/// ACTIVE State handlers definition
const struct ke_msg_handler sps_server_active[] =
{
    {GAPC_DISCONNECT_IND,                   (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                         (ke_msg_func_t)gattc_cmp_evt_handler},
    {GATTC_WRITE_CMD_IND,                   (ke_msg_func_t)gattc_write_cmd_ind_handler},
    {SPS_SERVER_INIT_BLE_TX_REQ,            (ke_msg_func_t)sps_server_init_ble_tx_req_handler},
    {SPS_SERVER_SEND_FLOW_CONTROL_REQ,		(ke_msg_func_t)sps_server_send_notify_flow_control_state_req_handler},
}; 

/// Specifies the message handler structure for every input state
const struct ke_state_handler sps_server_state_handler[SPS_SERVER_STATE_MAX] =
{
    /// DISABLE State message handlers.
    [SPS_SERVER_DISABLED]  = KE_STATE_HANDLER(sps_server_disabled),
    /// IDLE State message handlers.
    [SPS_SERVER_IDLE]      = KE_STATE_HANDLER(sps_server_idle),
    /// ACTIVE State message handlers.
    [SPS_SERVER_ACTIVE]    = KE_STATE_HANDLER(sps_server_active),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler sps_server_default_handler = KE_STATE_HANDLER_NONE;

/// Defines the placeholder for the states of all the task instances.
ke_state_t sps_server_state[SPS_SERVER_IDX_MAX];

#endif /*#if BLE_SPS_SERVER*/

/// @} SPS_SERVER_TASK
