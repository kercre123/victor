/**
 ****************************************************************************************
 *
 * @file device_config_task.c
 *
 * @brief Device configuration service Task implementation.
 *
 * @brief 128 UUID service. sample code
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
*/

#include "rwble_config.h"

#if (BLE_DEVICE_CONFIG)

#include "gap.h"
#include "gapc.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "device_config.h"
#include "device_config_task.h"
#include "attm_cfg.h"
#include "attm_db.h"
#include "prf_utils.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref DEVICE_CONFIG_CREATE_DB_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int device_config_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct device_config_create_db_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    //Database Creation Status
    uint8_t status;
    uint8_t nb_att_16;
    uint8_t nb_att_128;
    uint8_t nb_att_32;
    uint16_t att_decl_svc = ATT_DECL_PRIMARY_SERVICE;
    uint16_t att_decl_char = ATT_DECL_CHARACTERISTIC;
    uint16_t att_decl_cfg = ATT_DESC_CLIENT_CHAR_CFG;
    uint16_t val_hdl;
    uint16_t char_hdl; 

    //Save Profile ID
    device_config_env.con_info.prf_id = TASK_DEVICE_CONFIG;

    /*---------------------------------------------------*
     * Link Loss Service Creation
     *---------------------------------------------------*/

        //Add Service Into Database
        nb_att_16 = 6; // 1 UUID16 atts
        nb_att_32 = 0;// No UUID32 att
        nb_att_128 = 4; //2 UUID128 att
        status = attmdb_add_service(&(device_config_env.device_config_shdl), TASK_DEVICE_CONFIG,
                                             nb_att_16, nb_att_32, nb_att_128, 249); 

        if (status == ATT_ERR_NO_ERROR)
        {
            //add svc attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, ATT_UUID_128_LEN, //Data size = 16 (ATT_UUID_128_LEN)
                                               ATT_UUID_16_LEN, (uint8_t*)&att_decl_svc, PERM(RD, ENABLE),
                                               &(device_config_env.device_config_shdl));
            
            status = attmdb_att_set_value(device_config_env.device_config_shdl, ATT_UUID_128_LEN, (uint8_t *)device_config_svc.uuid);
            
//version char      
            //add char attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));
            
            //add val attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, 16 * sizeof(uint8_t), 
                                               ATT_UUID_128_LEN, (uint8_t*)&device_version_val.uuid, PERM(RD, ENABLE),
                                               &(val_hdl));
            
            memcpy(device_version_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(device_version_char), (uint8_t *)&device_version_char);
            
//configuration char      
            //add char attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));
            
            //add val attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, (MAX_PARAM_SIZE + 2) * sizeof(uint8_t), 
                                               ATT_UUID_128_LEN, (uint8_t*)&device_config_val.uuid, PERM(WR, ENABLE),
                                               &(val_hdl));
            
            memcpy(device_config_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(device_config_char), (uint8_t *)&device_config_char);
            
//read command char      
            //add char attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));
            
            //add val attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, sizeof(uint8_t), 
                                               ATT_UUID_128_LEN, (uint8_t*)&device_read_command_val.uuid, PERM(WR, ENABLE),
                                               &(val_hdl));
            
            memcpy(device_read_command_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(device_read_command_char), (uint8_t *)&device_read_command_char);
            
//read response char      
            //add char attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));
            
            //add val attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, (MAX_PARAM_SIZE + 3) * sizeof(uint8_t), 
                                               ATT_UUID_128_LEN, (uint8_t*)&device_read_response_val.uuid, PERM(IND, ENABLE),
                                               &(val_hdl));
            
            memcpy(device_read_response_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(device_read_response_char), (uint8_t *)&device_read_response_char);
            
            //add cfg attribute
            status = attmdb_add_attribute(device_config_env.device_config_shdl, sizeof(uint16_t), 
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_cfg, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                               &(val_hdl));
            //Go to Idle State
    
            //If we are here, database has been fulfilled with success, go to idle state
            ke_state_set(TASK_DEVICE_CONFIG, DEVICE_CONFIG_IDLE);
        }
        
        //Send CFM to application
        struct device_config_create_db_cfm * cfm = KE_MSG_ALLOC(DEVICE_CONFIG_CREATE_DB_CFM, src_id,
                                                    TASK_DEVICE_CONFIG, device_config_create_db_cfm);
        cfm->status = status;
        ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Enable the Sample128 role, used after connection.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

static int device_config_enable_req_handler(ke_msg_id_t const msgid,
                                    struct device_config_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    
    // Keep source of message, to respond to it further on
    device_config_env.con_info.appid = src_id;
    // Store the connection handle for which this profile is enabled
    device_config_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (device_config_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&device_config_env, PRF_ERR_REQ_DISALLOWED,
                 DEVICE_CONFIG_ERROR_IND, DEVICE_CONFIG_ENABLE_REQ);
    }
    else
    {
        
        uint16_t temp = 2;// indication capability always enabled. 
        
        // Beacon Configuration
        attmdb_svc_set_permission(device_config_env.device_config_shdl, param->sec_lvl);

        attmdb_att_set_value(device_config_env.device_config_shdl + DEVICE_READ_RESPONSE_IDX_CFG,
                             sizeof(uint16_t), (uint8_t *)&temp); // indication capability always enabled at connection

        device_config_env.feature |= PRF_CLI_START_IND;

        attmdb_att_set_value(device_config_env.device_config_shdl + DEVICE_VERSION_IDX_VAL,
                             param->version_length * sizeof(uint8_t), (uint8_t *)param->version); 
     
    }        
        // Go to Connected state
     ke_state_set(TASK_DEVICE_CONFIG, DEVICE_CONFIG_CONNECTED);

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_WRITE_CMD_IND message.
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
    
    uint8_t status = PRF_ERR_OK;
    uint16_t ntf_cfg;

    if (KE_IDX_GET(src_id) == device_config_env.con_info.conidx)
    {
        
        switch (param->handle - device_config_env.device_config_shdl)
        {
            
            case  DEVICE_CONFIG_IDX_VAL:
              
                if (param->length != param->value[1] + 2)
                {
                    status  = PRF_APP_ERROR;
                    break;
                }
                    
                device_config_send_val((uint8_t *)&param->value[2], param->value[1], param->value[0]);
                break;
            
            case DEVICE_READ_REQUEST_IDX_VAL:
                if (param->length != 1)
                    status = PRF_ERR_UNEXPECTED_LEN; 
                
                device_config_read_val_request(param->value[0]);
                
                break;
                
            case DEVICE_READ_RESPONSE_IDX_CFG:
                        
                ntf_cfg = co_read16p(&param->value[0]);
            
                if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND))
                {
                    //Save value in DB
                    attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&param->value[0]);
                    
                    // Conserve information in environment
                    if (ntf_cfg == PRF_CLI_START_IND)
                    {
                        // Ntf cfg bit set to 1
                        device_config_env.feature |= PRF_CLI_START_IND;
                    }
                    else
                    {
                        // Ntf cfg bit set to 0
                        device_config_env.feature &= ~PRF_CLI_START_IND;
                    }
                }
                
                break;
            default:
                status = PRF_APP_ERROR;
        }
       
    }
    else
    {
        status  = PRF_APP_ERROR;
    }
    
    // Send Write Response
    atts_write_rsp_send(device_config_env.con_info.conidx, param->handle, status);
    
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles read rspons message snet by application task.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

static int device_config_read_response_cmd_handler(ke_msg_id_t const msgid,
                                      struct device_config_read_response_cmd const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{

    uint8_t size;

    // Check provided values
    if(param->conhdl == gapc_get_conhdl(device_config_env.con_info.conidx))
    {
        uint8_t temp_buffer[MAX_PARAM_SIZE + 3];
        
        temp_buffer[0] = param->param_id;
        temp_buffer[1] = param->size;
        temp_buffer[2] = param->status;
        
        if ((param->status == 0) && (param->size < MAX_PARAM_SIZE))
        {
            size = param->size + 3;
            memcpy (&temp_buffer[3], param->val, param->size);
        }
        else
            size = 3; //set size to 0.
        
        
        // Update value in DB
        attmdb_att_set_value(device_config_env.device_config_shdl + DEVICE_READ_RESPONSE_IDX_VAL,
                             size, (uint8_t *)&temp_buffer);

        if((device_config_env.feature & PRF_CLI_START_IND))
            // Send indication through GATT
            prf_server_send_event((prf_env_struct *)&device_config_env, true,
                        device_config_env.device_config_shdl + DEVICE_READ_RESPONSE_IDX_VAL);
    
    }
   
    return (KE_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Disconnection indication to device_config.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gap_disconnnect_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_disconnect_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{

    // Check Connection Handle
    if (KE_IDX_GET(src_id) == device_config_env.con_info.conidx)
    {
        
        // In any case, inform APP about disconnection
        device_config_disable();
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler device_config_disabled[] =
{
    {DEVICE_CONFIG_CREATE_DB_REQ,   (ke_msg_func_t) device_config_create_db_req_handler },
};

/// Idle State handler definition.
const struct ke_msg_handler device_config_idle[] =
{
    {DEVICE_CONFIG_ENABLE_REQ,      (ke_msg_func_t) device_config_enable_req_handler },
};

/// Connected State handler definition.
const struct ke_msg_handler device_config_connected[] =
{
    {GATTC_WRITE_CMD_IND,       (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {DEVICE_READ_RESPONSE_CMD,   (ke_msg_func_t) device_config_read_response_cmd_handler},
};

/// Default State handlers definition
const struct ke_msg_handler device_config_default_state[] =
{
    {GAPC_DISCONNECT_IND,    (ke_msg_func_t) gap_disconnnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler device_config_state_handler[DEVICE_CONFIG_STATE_MAX] =
{
    [DEVICE_CONFIG_DISABLED]    = KE_STATE_HANDLER(device_config_disabled),
    [DEVICE_CONFIG_IDLE]        = KE_STATE_HANDLER(device_config_idle),
    [DEVICE_CONFIG_CONNECTED]   = KE_STATE_HANDLER(device_config_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler device_config_default_handler = KE_STATE_HANDLER(device_config_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t device_config_state[DEVICE_CONFIG_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init));

#endif //BLE_DEVICE_CONFIG

