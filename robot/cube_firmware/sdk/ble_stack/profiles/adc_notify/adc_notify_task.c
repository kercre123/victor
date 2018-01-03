/**
 ****************************************************************************************
 *
 * @file adc_notify_task.c
 *
 * @brief ADC Notify Task implementation.
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

#if (BLE_ADC_NOTIFY)

#include "gap.h"
#include "gapc.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "adc_notify.h"
#include "adc_notify_task.h"
#include "attm_cfg.h"
#include "attm_db.h"
#include "prf_utils.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ADC_NOTIFY_CREATE_DB_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int adc_notify_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct adc_notify_create_db_req const *param,
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
    adc_notify_env.con_info.prf_id = TASK_ADC_NOTIFY;

    /*---------------------------------------------------*
     * ADC Notify Service Creation
     *---------------------------------------------------*/

        //Add Service Into Database
        nb_att_16 = 4; // 2 UUID16 atts
        nb_att_32 = 0;// No UUID32 att
        nb_att_128 = 2; //1 UUID128 att
        status = attmdb_add_service(&(adc_notify_env.adc_notify_shdl), TASK_ADC_NOTIFY,
                                             nb_att_16, nb_att_32, nb_att_128, 39); //total attributte size =  16 (svc)  + 19 (desc_char) + 2 (att) + 2 (cfg)

        if (status == ATT_ERR_NO_ERROR)
        {
            //add svc attribute
            status = attmdb_add_attribute(adc_notify_env.adc_notify_shdl, ATT_UUID_128_LEN, //Data size = 16 (ATT_UUID_128_LEN)
                                               ATT_UUID_16_LEN, (uint8_t*)&att_decl_svc, PERM(RD, ENABLE),
                                               &(adc_notify_env.adc_notify_shdl));
            
            
            
            status = attmdb_att_set_value(adc_notify_env.adc_notify_shdl, ATT_UUID_128_LEN, (uint8_t *)adc_notify_svc.uuid);
            
            //add char attribute
            status = attmdb_add_attribute(adc_notify_env.adc_notify_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));
                        
            //add val attribute
            status = attmdb_add_attribute(adc_notify_env.adc_notify_shdl, sizeof(uint16_t), //Data size = 1
                                               ATT_UUID_128_LEN, (uint8_t*)&adc_notify_val.uuid, PERM(RD, ENABLE) | PERM(NTF, ENABLE),
                                               &(val_hdl));
            
            memcpy(adc_notify_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(adc_notify_char), (uint8_t *)&adc_notify_char);

            //add cfg attribute
            status = attmdb_add_attribute(adc_notify_env.adc_notify_shdl, sizeof(uint16_t), 
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_cfg, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                               &(val_hdl));

            //Disable adc_notify service
            attmdb_svc_set_permission(adc_notify_env.adc_notify_shdl, PERM(SVC, DISABLE));

            //Go to Idle State
    
            //If we are here, database has been fulfilled with success, go to idle state
            ke_state_set(TASK_ADC_NOTIFY, ADC_NOTIFY_IDLE);
        }
        
        //Send CFM to application
        struct adc_notify_create_db_cfm * cfm = KE_MSG_ALLOC(ADC_NOTIFY_CREATE_DB_CFM, src_id,
                                                    TASK_ADC_NOTIFY, adc_notify_create_db_cfm);
        cfm->status = status;
        ke_msg_send(cfm);
    
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Enable the ADC Notify role, used after connection.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int adc_notify_enable_req_handler(ke_msg_id_t const msgid,
                                    struct adc_notify_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    
    uint16_t temp = 1;
    
    // Keep source of message, to respond to it further on
    adc_notify_env.con_info.appid = src_id;
    // Store the connection handle for which this profile is enabled
    adc_notify_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (adc_notify_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&adc_notify_env, PRF_ERR_REQ_DISALLOWED,
                 ADC_NOTIFY_ERROR_IND, ADC_NOTIFY_ENABLE_REQ);
    }
    else
    {
         
        attmdb_svc_set_permission(adc_notify_env.adc_notify_shdl, param->sec_lvl);

                
        attmdb_att_set_value(adc_notify_env.adc_notify_shdl + ADC_NOTIFY_IDX_VAL,
                             sizeof(uint16_t), (uint8_t *)&param->adc_notify_val);

        adc_notify_env.feature = param->feature; 
        
        if (!adc_notify_env.feature)
        {
               temp = 0;
        }
        
        attmdb_att_set_value(adc_notify_env.adc_notify_shdl + ADC_NOTIFY_IDX_CFG,
                             sizeof(uint16_t), (uint8_t *)&temp);
        
        // Go to Connected state
        ke_state_set(TASK_ADC_NOTIFY, ADC_NOTIFY_CONNECTED);
        
        
    }

    return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief Updates characteristic's 2 value. Sends notification to peer if property is enabled.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

static int adc_notify_upd_char_req_handler(ke_msg_id_t const msgid,
                                    struct adc_notify_upd_char_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint8_t status = PRF_ERR_OK;

    // Check provided values
    if(param->conhdl == gapc_get_conhdl(adc_notify_env.con_info.conidx))
    {
        // Update value in DB
        attmdb_att_set_value(adc_notify_env.adc_notify_shdl + ADC_NOTIFY_IDX_VAL,
                             sizeof(uint16_t), (uint8_t *)&param->val);

        if((adc_notify_env.feature & PRF_CLI_START_NTF))
            // Send notification through GATT
            prf_server_send_event((prf_env_struct *)&adc_notify_env, false,
                    adc_notify_env.adc_notify_shdl + ADC_NOTIFY_IDX_VAL);
        
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        adc_notify_upd_char_cfm_send(status);
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
    uint8_t char_code = ADC_NOTIFY_ERR_CHAR;
    uint8_t status = PRF_APP_ERROR;

    if (KE_IDX_GET(src_id) == adc_notify_env.con_info.conidx)
    {
                
        if (param->handle == adc_notify_env.adc_notify_shdl + ADC_NOTIFY_IDX_CFG)
        {
            char_code = ADC_NOTIFY_CFG;
        }
        
        if (char_code == ADC_NOTIFY_CFG)
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
                    adc_notify_env.feature |= PRF_CLI_START_NTF;
                }
                else
                {
                    // Ntf cfg bit set to 0
                    adc_notify_env.feature &= ~PRF_CLI_START_NTF;
                }                
                
                adc_notify_send_cfg(ntf_cfg);
                
                status = PRF_ERR_OK; 
                
            }
        }
    }

    // Send Write Response
    atts_write_rsp_send(adc_notify_env.con_info.conidx, param->handle, status);
    
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to adc_notify.
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
    if (KE_IDX_GET(src_id) == adc_notify_env.con_info.conidx)
    {
        
        // In any case, inform APP about disconnection
        adc_notify_disable();
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler adc_notify_disabled[] =
{
    {ADC_NOTIFY_CREATE_DB_REQ,   (ke_msg_func_t) adc_notify_create_db_req_handler },
};

/// Idle State handler definition.
const struct ke_msg_handler adc_notify_idle[] =
{
    {ADC_NOTIFY_ENABLE_REQ,      (ke_msg_func_t) adc_notify_enable_req_handler },
};

/// Connected State handler definition.
const struct ke_msg_handler adc_notify_connected[] =
{
    {GATTC_WRITE_CMD_IND,       (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {ADC_NOTIFY_UPD_CHAR_REQ,   (ke_msg_func_t) adc_notify_upd_char_req_handler},
};

/// Default State handlers definition
const struct ke_msg_handler adc_notify_default_state[] =
{
    {GAPC_DISCONNECT_IND,    (ke_msg_func_t) gap_disconnnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler adc_notify_state_handler[ADC_NOTIFY_STATE_MAX] =
{
    [ADC_NOTIFY_DISABLED]    = KE_STATE_HANDLER(adc_notify_disabled),
    [ADC_NOTIFY_IDLE]        = KE_STATE_HANDLER(adc_notify_idle),
    [ADC_NOTIFY_CONNECTED]   = KE_STATE_HANDLER(adc_notify_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler adc_notify_default_handler = KE_STATE_HANDLER(adc_notify_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t adc_notify_state[ADC_NOTIFY_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init));

#endif //BLE_ADC_NOTIFY

