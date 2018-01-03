/**
 ****************************************************************************************
 *
 * @file sample128_task.c
 *
 * @brief Sample128 Task implementation.
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

#if (BLE_SAMPLE128)

#include "gap.h"
#include "gapc.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "sample128.h"
#include "sample128_task.h"
#include "attm_cfg.h"
#include "attm_db.h"
#include "prf_utils.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SAMPLE128_CREATE_DB_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int sample128_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct sample128_create_db_req const *param,
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
    sample128_env.con_info.prf_id = TASK_SAMPLE128;

    /*---------------------------------------------------*
     * Sample128 Service Creation
     *---------------------------------------------------*/

        //Add Service Into Database

        nb_att_16  = 4;   // 4 UUID16 Attribute declaration types
        nb_att_32  = 0;    // 0 UUID32 Attribute declaration types
        nb_att_128 = 2;   // 2 UUID128 Attribute declaration types

        status = attmdb_add_service(  &(sample128_env.sample128_shdl),
                                      TASK_SAMPLE128,
                                      nb_att_16,
                                      nb_att_32,
                                      nb_att_128,
                                      58 // See calcualtion below
                                   );

        //   Total Data portion of GATT database = 58 data bytes:
        //    16  Primary service declaration
        //   + 19  Declaration of characteristic 1
        //   +  1  Value declaration of characteristic 1
        //   + 19  Declaration of characteristic 2
        //   +  1  Value declaration of characteristic 2
        //   +  2  Client configuration declaration of characteristic 2
        //   = 58  Data bytes total

        if (status == ATT_ERR_NO_ERROR)
        {
            // Add the primary service attribute /////////////////////////////////////////////////////////////////
            status = attmdb_add_attribute(  sample128_env.sample128_shdl, // Attribute Handle
                                            ATT_UUID_128_LEN, // Data size = 16 (ATT_UUID_128_LEN)
                                            ATT_UUID_16_LEN,  // Size of declaration type  ID
                                            (uint8_t*)&att_decl_svc, // 0x2800 for a primary service declaration
                                            PERM(RD, ENABLE), // Permissions
                                            &(sample128_env.sample128_shdl) // Attribute Handle
                                         );
            
            
            // Add the value of the primary service attribute (The custom UUID)
            status = attmdb_att_set_value(  sample128_env.sample128_shdl, // Attribute handle
                                            ATT_UUID_128_LEN, // The value is the 128 bit UUID of the service
                                            (uint8_t *)sample128_svc.uuid  // UUID of the service
                                         );
            
            
            // Characterisitic 1: ////////////////////////////////////////////////////////////////////////////////
            
            // Add characteristic declaration attribute to database
            status = attmdb_add_attribute(  sample128_env.sample128_shdl,
                                            ATT_UUID_128_LEN + 3, // Data size = 19 (ATT_UUID_128_LEN + 3)
                                            ATT_UUID_16_LEN,  // Size of declaration type ID
                                            (uint8_t*) &att_decl_char, // 0x2803 for a characteristic declaration
                                            PERM(RD, ENABLE), // Permissions
                                            &(char_hdl) // Handle to the characteristic declaration
                                         );
            
            
            // Add characteristic value declaration attribute to database
            status = attmdb_add_attribute(  sample128_env.sample128_shdl,
                                            sizeof(uint8_t), // Data size = 1 Byte
                                            ATT_UUID_128_LEN,// Size of custom declaration type = 128bit
                                            (uint8_t*)&sample128_1_val.uuid, // UUID of the characteristic value
                                            PERM(RD, ENABLE) | PERM(WR, ENABLE),// Permissions
                                            &(val_hdl) // handle to the value attribute
                                         );
            
            // Store the value handle for characteristic 1
            memcpy(sample128_1_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            // Set initial value of characteristic 1
            status = attmdb_att_set_value(char_hdl, sizeof(sample128_1_char), (uint8_t *)&sample128_1_char);
            

            //Characteristic 2: //////////////////////////////////////////////////////////////////////////////////
            
            // Add characteristic declaration attribute to database
            status = attmdb_add_attribute(  sample128_env.sample128_shdl,
                                            ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                            ATT_UUID_16_LEN,//Size of declaration type ID
                                            (uint8_t*) &att_decl_char, // 0x2803 for a characteristic declaration
                                            PERM(RD, ENABLE),// Permissions
                                            &(char_hdl) // Handle to the characteristic declaration
                                         );

            // Add characteristic value declaration attribute to database
            status = attmdb_add_attribute(  sample128_env.sample128_shdl,
                                            sizeof(uint8_t), //Data size = 1 Byte
                                            ATT_UUID_128_LEN,// Size of custom declaration type ID = 128bit
                                            (uint8_t*)&sample128_2_val.uuid, // UUID of the characteristic value
                                            PERM(RD, ENABLE) | PERM(NTF, ENABLE),// Permissions
                                            &(val_hdl) // Handle to the value attribute
                                         );

            // Store the value handle for characteristic 2
            memcpy(sample128_2_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            // Set initial value of characteristic 2
            status = attmdb_att_set_value(char_hdl, sizeof(sample128_2_char), (uint8_t *)&sample128_2_char);

            // Add client configuration declaration attribute to database ( Facilitates Notify )
            status = attmdb_add_attribute(  sample128_env.sample128_shdl,
                                            sizeof(uint16_t), // Data size 2bytes (16bit)
                                            ATT_UUID_16_LEN, // Size of client configuration type ID
                                            (uint8_t*) &att_decl_cfg, // 0x2902 UUID of client configuration declaration type
                                            PERM(RD, ENABLE) | PERM(WR, ENABLE), // Permissions
                                            &(val_hdl) // Handle to value attribute
                                         );

            //////////////////////////////////////////////////////////////////////////////////////////////////////

            //Disable sample128 service
            attmdb_svc_set_permission(sample128_env.sample128_shdl, PERM(SVC, DISABLE));

    
            //If we are here, database has been fulfilled with success, go to idle state
            ke_state_set(TASK_SAMPLE128, SAMPLE128_IDLE);
        }
        
        //Send CFM to application
        struct sample128_create_db_cfm * cfm = KE_MSG_ALLOC(SAMPLE128_CREATE_DB_CFM, src_id,
                                                    TASK_SAMPLE128, sample128_create_db_cfm);
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
static int sample128_enable_req_handler(ke_msg_id_t const msgid,
                                    struct sample128_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    
    uint16_t temp = 1;
    
    // Keep source of message, to respond to it further on
    sample128_env.con_info.appid = src_id;
    // Store the connection handle for which this profile is enabled
    sample128_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (sample128_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&sample128_env, PRF_ERR_REQ_DISALLOWED,
                 SAMPLE128_ERROR_IND, SAMPLE128_ENABLE_REQ);
    }
    else
    {
        // Sample128 service permissions
        attmdb_svc_set_permission(sample128_env.sample128_shdl, param->sec_lvl);

        // Set characteristic 1 to specified value
        attmdb_att_set_value(sample128_env.sample128_shdl + SAMPLE128_1_IDX_VAL,
                             sizeof(uint8_t), (uint8_t *)&param->sample128_1_val);
        
        // Set characteristic 2 to specified value
        attmdb_att_set_value(sample128_env.sample128_shdl + SAMPLE128_2_IDX_VAL,
                             sizeof(uint8_t), (uint8_t *)&param->sample128_2_val);

        sample128_env.feature = param->feature; 
        
        if (!sample128_env.feature)
        {
               temp = 0;
        }
        
        attmdb_att_set_value(sample128_env.sample128_shdl + SAMPLE128_2_IDX_CFG,
                             sizeof(uint16_t), (uint8_t *)&temp);
        
        // Go to Connected state
        ke_state_set(TASK_SAMPLE128, SAMPLE128_CONNECTED);
    }

    return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief Updates value of characteristic 2. Sends notification to peer if property is enabled.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

static int sample128_upd_char2_req_handler(ke_msg_id_t const msgid,
                                    struct sample128_upd_char2_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint8_t status = PRF_ERR_OK;

    // Check provided values
    if(param->conhdl == gapc_get_conhdl(sample128_env.con_info.conidx))
    {
        // Update value in database
        attmdb_att_set_value(sample128_env.sample128_shdl + SAMPLE128_2_IDX_VAL,
                             sizeof(uint8_t), (uint8_t *)&param->val);

        if((sample128_env.feature & PRF_CLI_START_NTF))
            // Send notification through GATT
            prf_server_send_event((prf_env_struct *)&sample128_env, false,
                    sample128_env.sample128_shdl + SAMPLE128_2_IDX_VAL);
        
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        sample128_upd_char2_cfm_send(status);
    }

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
    uint8_t char_code = SAMPLE128_ERR_CHAR;
    uint8_t status = PRF_APP_ERROR;

    if (KE_IDX_GET(src_id) == sample128_env.con_info.conidx)
    {
        if (param->handle == sample128_env.sample128_shdl + SAMPLE128_1_IDX_VAL)
        {
            char_code = SAMPLE128_1_CHAR;
        }
        
        if (param->handle == sample128_env.sample128_shdl + SAMPLE128_2_IDX_CFG)
        {
            char_code = SAMPLE128_2_CFG;
        }
        
        if (char_code == SAMPLE128_1_CHAR)
        {
            
            //Save value in DB
            attmdb_att_set_value(param->handle, sizeof(uint8_t), (uint8_t *)&param->value[0]);
            
            if(param->last)
            {
                sample128_send_val(param->value[0]);
            }

            status = PRF_ERR_OK;
               
        }
        else if (char_code == SAMPLE128_2_CFG)
        {
            
            // Written value
            uint16_t ntf_cfg;

            // Extract value before check
            ntf_cfg = co_read16p(&param->value[0]);
        
            // Only update configuration if value for stop or notification enable
            if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
            {
                //Save value in DB
                attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&param->value[0]);
                
                // Conserve information in environment
                if (ntf_cfg == PRF_CLI_START_NTF)
                {
                    // Ntf cfg bit set to 1
                    sample128_env.feature |= PRF_CLI_START_NTF;
                }
                else
                {
                    // Ntf cfg bit set to 0
                    sample128_env.feature &= ~PRF_CLI_START_NTF;
                }                
                
                status = PRF_ERR_OK; 
                
            }
        }
    }

    // Send Write Response
    atts_write_rsp_send(sample128_env.con_info.conidx, param->handle, status);
    
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to sample128.
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
    if (KE_IDX_GET(src_id) == sample128_env.con_info.conidx)
    {
        
        // In any case, inform APP about disconnection
        sample128_disable();
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler sample128_disabled[] =
{
    {SAMPLE128_CREATE_DB_REQ,   (ke_msg_func_t) sample128_create_db_req_handler },
};

/// Idle State handler definition.
const struct ke_msg_handler sample128_idle[] =
{
    {SAMPLE128_ENABLE_REQ,      (ke_msg_func_t) sample128_enable_req_handler },
};

/// Connected State handler definition.
const struct ke_msg_handler sample128_connected[] =
{
    {GATTC_WRITE_CMD_IND,       (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {SAMPLE128_UPD_CHAR2_REQ,   (ke_msg_func_t) sample128_upd_char2_req_handler},
};

/// Default State handlers definition
const struct ke_msg_handler sample128_default_state[] =
{
    {GAPC_DISCONNECT_IND,    (ke_msg_func_t) gap_disconnnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler sample128_state_handler[SAMPLE128_STATE_MAX] =
{
    [SAMPLE128_DISABLED]    = KE_STATE_HANDLER(sample128_disabled),
    [SAMPLE128_IDLE]        = KE_STATE_HANDLER(sample128_idle),
    [SAMPLE128_CONNECTED]   = KE_STATE_HANDLER(sample128_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler sample128_default_handler = KE_STATE_HANDLER(sample128_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t sample128_state[SAMPLE128_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init));

#endif //BLE_SAMPLE128

