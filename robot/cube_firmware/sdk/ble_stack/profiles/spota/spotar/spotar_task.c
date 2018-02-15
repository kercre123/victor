/**
 ****************************************************************************************
 *
 * @file spotar_task.c
 *
 * @brief Software Programming Over The Air (SPOTA) Receiver Task Implementation.
 *
 * Copyright (C) 2013. Dialog Semiconductor Ltd, unpublished work. This computer 
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
 * @addtogroup SPOTARTASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_SPOTA_RECEIVER)

#include "gap.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "atts_util.h"
#include "spotar.h"
#include "spotar_task.h"
#include "attm_cfg.h"
#include "prf_utils.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SPOTAR_CREATE_DB_REQ message.
 * The handler adds SPOTA service into the database.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int spotar_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct spotar_create_db_req const *param,
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
    uint16_t att_decl_ntf_cfg = ATT_DESC_CLIENT_CHAR_CFG;
    uint16_t spotar_val_hdl;
    uint16_t spotar_char_hdl;
    uint16_t spotar_ntf_hdl;

    //Save Profile ID
    spotar_env.con_info.prf_id = TASK_SPOTAR;

    /*---------------------------------------------------*
     * SPOTAR Service Creation
     *---------------------------------------------------*/

    //Add Service Into Database
    nb_att_16 = 8; // 8 UUID16 atts
    nb_att_32 = 0;// No UUID32 att
    nb_att_128 = 6; //1 UUID128 att
    status = attmdb_add_service(&(spotar_env.spota_shdl), TASK_SPOTAR,
                                         nb_att_16, nb_att_32, nb_att_128, 153); //total attributte size = 153, 2 (svc)  + 6*19 (desc_char) + 37 (attr data)

    if (status == ATT_ERR_NO_ERROR)
    {
        //add svc attribute
        status = attmdb_add_attribute(spotar_env.spota_shdl, ATT_UUID_16_LEN, //Data size = 2 (ATT_UUID_16_LEN service uuid)
                                           ATT_UUID_16_LEN, (uint8_t*)&att_decl_svc, PERM(RD, ENABLE),
                                           &(spotar_env.spota_shdl));

        status = attmdb_att_set_value(spotar_env.spota_shdl, ATT_UUID_16_LEN, (uint8_t *)spotar_service);

        /* add char and val attribute for patch_mem
        */
        status = attmdb_add_attribute(spotar_env.spota_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                           ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                           &(spotar_char_hdl));
        
        status = attmdb_add_attribute(spotar_env.spota_shdl, sizeof(uint32_t), //Data size = 4
                                           ATT_UUID_128_LEN, (uint8_t*)&spotar_patch_mem_dev_val.uuid, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                           &(spotar_val_hdl));
        
        memcpy(spotar_patch_mem_dev_char.attr_hdl, &spotar_val_hdl, ATT_UUID_16_LEN);
        
        status = attmdb_att_set_value(spotar_char_hdl, sizeof(spotar_patch_mem_dev_char), (uint8_t *)&spotar_patch_mem_dev_char);
        
        /* add char and val attribute for gpio_map
        */
        status = attmdb_add_attribute(spotar_env.spota_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                           ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                           &(spotar_char_hdl));
        
        status = attmdb_add_attribute(spotar_env.spota_shdl, sizeof(uint32_t), //Data size = 4
                                           ATT_UUID_128_LEN, (uint8_t*)&spotar_gpio_map_val.uuid, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                           &(spotar_val_hdl));
        
        memcpy(spotar_gpio_map_char.attr_hdl, &spotar_val_hdl, ATT_UUID_16_LEN);
        
        status = attmdb_att_set_value(spotar_char_hdl, sizeof(spotar_gpio_map_char), (uint8_t *)&spotar_gpio_map_char);
        
        /* add char and val attribute for mem_info
        */
        status = attmdb_add_attribute(spotar_env.spota_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                           ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                           &(spotar_char_hdl));
        
        status = attmdb_add_attribute(spotar_env.spota_shdl, sizeof(uint32_t), //Data size = 4
                                           ATT_UUID_128_LEN, (uint8_t*)&spotar_patch_mem_info_val.uuid, PERM(RD, ENABLE),
                                           &(spotar_val_hdl));
        
        memcpy(spotar_patch_mem_info_char.attr_hdl, &spotar_val_hdl, ATT_UUID_16_LEN);
        
        status = attmdb_att_set_value(spotar_char_hdl, sizeof(spotar_patch_mem_info_char), (uint8_t *)&spotar_patch_mem_info_char);
        
        /* add char and val attribute for patch_len
        */
        status = attmdb_add_attribute(spotar_env.spota_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                           ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                           &(spotar_char_hdl));
        
        status = attmdb_add_attribute(spotar_env.spota_shdl, sizeof(uint16_t), //Data size = 2
                                           ATT_UUID_128_LEN, (uint8_t*)&spotar_patch_len_val.uuid, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                           &(spotar_val_hdl));
        
        memcpy(spotar_patch_len_char.attr_hdl, &spotar_val_hdl, ATT_UUID_16_LEN);
        
        status = attmdb_att_set_value(spotar_char_hdl, sizeof(spotar_patch_len_char), (uint8_t *)&spotar_patch_len_char);
        
        /* add char and val attribute for patch_data
        */
        status = attmdb_add_attribute(spotar_env.spota_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                           ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                           &(spotar_char_hdl));
        
        status = attmdb_add_attribute(spotar_env.spota_shdl, ((sizeof(uint8_t)) * SPOTA_PD_CHAR_SIZE), //Data size = 20
                                           ATT_UUID_128_LEN, (uint8_t*)&spotar_patch_data_val.uuid, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                           &(spotar_val_hdl));
        
        memcpy(spotar_patch_data_char.attr_hdl, &spotar_val_hdl, ATT_UUID_16_LEN);
        
        status = attmdb_att_set_value(spotar_char_hdl, sizeof(spotar_patch_data_char), (uint8_t *)&spotar_patch_data_char);
        
        /* add char and val attribute for patch_status
        */
        status = attmdb_add_attribute(spotar_env.spota_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                           ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE) | PERM(NTF, ENABLE),
                                           &(spotar_char_hdl));
        
        status = attmdb_add_attribute(spotar_env.spota_shdl, sizeof(uint8_t), //Data size = 1
                                           ATT_UUID_128_LEN, (uint8_t*)&spotar_patch_status_val.uuid, PERM(RD, ENABLE)| PERM(NTF, ENABLE),
                                           &(spotar_val_hdl));
        
        memcpy(spotar_patch_status_char.attr_hdl, &spotar_val_hdl, ATT_UUID_16_LEN);
        
        status = attmdb_att_set_value(spotar_char_hdl, sizeof(spotar_patch_status_char), (uint8_t *)&spotar_patch_status_char);
        
        status = attmdb_add_attribute(spotar_env.spota_shdl, sizeof(uint16_t), //Data size = 2 (Characteristic configuration)
                                   ATT_UUID_16_LEN, (uint8_t*) &att_decl_ntf_cfg, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                   &(spotar_ntf_hdl));
        
        
        //Disable SPOTAR service
        attmdb_svc_set_permission(spotar_env.spota_shdl, PERM(SVC, DISABLE));

        //Go to Idle State

        //If we are here, database has been fulfilled with success, go to idle state
        ke_state_set(TASK_SPOTAR, SPOTAR_IDLE);
    }

    //Send CFM to application
    struct spotar_create_db_cfm * cfm = KE_MSG_ALLOC(SPOTAR_CREATE_DB_CFM, src_id,
                                                    TASK_SPOTAR, spotar_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SPOTAR_STATUS_UPDATE_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int spotar_status_update_req_handler (ke_msg_id_t const msgid,
                                         struct spotar_status_upadet_req const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    att_size_t len;
    uint16_t* status_en;
    uint8_t* status_val;

    // Read status value from database and compare with new one. 
    // If diferent and notification is enebled, then send notification        
    attmdb_att_get_value(spotar_env.spota_shdl + SPOTA_IDX_PATCH_STATUS_VAL,
                         &len, (uint8_t**)&(status_val));
    attmdb_att_get_value(spotar_env.spota_shdl + SPOTA_IDX_PATCH_STATUS_NTF_CFG,
                         &len, (uint8_t**)&(status_en));        

    // Update the value in the attribute database
    attmdb_att_set_value(spotar_env.spota_shdl + SPOTA_IDX_PATCH_STATUS_VAL,
                     sizeof(uint8_t), (uint8_t *)&param->status);
    
    if ((*status_en) )
    {
        // Send notification
        prf_server_send_event((prf_env_struct *)&spotar_env, false, (spotar_env.spota_shdl + SPOTA_IDX_PATCH_STATUS_VAL));
    }
          
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Update Memory info characteristic value.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int spotar_mem_info_update_req_handler(ke_msg_id_t const msgid,
                                    struct spotar_patch_mem_info_upadet_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
#if 0    
    // Keep source of message, to respond to it further on
    spotar_env.con_info.appid = src_id;
    // Store the connection handle for which this profile is enabled
    spotar_env.con_info.conhdl = param->conhdl;

    // Check if the provided connection exist
    if (gap_get_rec_idx(param->conhdl) == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&spotar_env, PRF_ERR_REQ_DISALLOWED,
                                  SPOTAR_ERROR_IND, SPOTAR_ENABLE_REQ);
    }
    else
    {
#endif
        
        // Update Memory info charecteristic value

        attmdb_att_set_value(spotar_env.spota_shdl + SPOTA_IDX_PATCH_MEM_INFO_VAL,
                             sizeof(uint32_t), (uint8_t *)&param->mem_info);
      
    //}

    return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief Enable the SPOTA Receiver role, used after connection.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int spotar_enable_req_handler(ke_msg_id_t const msgid,
                                    struct spotar_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    
    uint16_t disable_val = 0x00;
    
    // Keep source of message, to respond to it further on
    spotar_env.con_info.appid = src_id;
    // Store the connection handle for which this profile is enabled
    spotar_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (spotar_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&spotar_env, PRF_ERR_REQ_DISALLOWED,
                                  SPOTAR_ERROR_IND, SPOTAR_ENABLE_REQ);
    }
    else
    {
        attmdb_att_set_value((spotar_env.spota_shdl + SPOTA_IDX_PATCH_STATUS_NTF_CFG), sizeof(uint16_t), (uint8_t*) &(disable_val));
        
        attmdb_att_set_value(spotar_env.spota_shdl + SPOTA_IDX_PATCH_STATUS_VAL, sizeof(uint8_t), (uint8_t *)&(disable_val));
        // Enable SPOTA + Set Security Level
        attmdb_svc_set_permission(spotar_env.spota_shdl, param->sec_lvl);       

        // Go to Connected state
        ke_state_set(TASK_SPOTAR, SPOTAR_ACTIVE);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_WRITE_CMD_IND message.
 * The handler will analyse what has been set and decide alert level
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
    uint8_t char_code = SPOTAR_ERR_CHAR;
    uint8_t status = PRF_APP_ERROR;

    if (KE_IDX_GET(src_id) == spotar_env.con_info.conidx)
    {
        if (param->handle == spotar_env.spota_shdl + SPOTA_IDX_PATCH_MEM_DEV_VAL)
        {
            char_code = SPOTAR_PATCH_MEM_DEV_CHAR;
            
            attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value[0]);            

            if (param->last)
            {                                                                
                // Inform APP. Allocate the Patch Mem value change indication
                struct spotar_patch_mem_dev_ind *ind = KE_MSG_ALLOC(SPOTAR_PATCH_MEM_DEV_IND,
                                                          spotar_env.con_info.appid, TASK_SPOTAR,
                                                          spotar_patch_mem_dev_ind);
                // Fill in the parameter structure
                ind->conhdl = gapc_get_conhdl(spotar_env.con_info.conidx);

                ind->mem_dev = 0;
                ind->mem_dev  = (uint32_t) ( param->value[3] << 24);
                ind->mem_dev |= (uint32_t) ( param->value[2] << 16);
                ind->mem_dev |= (uint32_t) ( param->value[1] << 8);
                ind->mem_dev |= (uint32_t) param->value[0];
                ind->char_code = char_code;

                // Send the message
                ke_msg_send(ind);
            }

            status = PRF_ERR_OK;
        }
        else if (param->handle == spotar_env.spota_shdl + SPOTA_IDX_GPIO_MAP_VAL)
        {
            char_code = SPOTAR_GPIO_MAP_CHAR;
            
            attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value[0]);            

            if (param->last)
            {                                                                
                // Inform APP. Allocate the Patch Data value change indication
                struct spotar_gpio_map_ind *ind = KE_MSG_ALLOC(SPOTAR_GPIO_MAP_IND,
                                                          spotar_env.con_info.appid, TASK_SPOTAR,
                                                          spotar_gpio_map_ind);
                // Fill in the parameter structure
                ind->conhdl = gapc_get_conhdl(spotar_env.con_info.conidx);

                ind->gpio_map = 0;
                ind->gpio_map  = (uint32_t) ( param->value[3] << 24);
                ind->gpio_map |= (uint32_t) ( param->value[2] << 16);
                ind->gpio_map |= (uint32_t) ( param->value[1] << 8);
                ind->gpio_map |= (uint32_t) param->value[0];
                ind->char_code = char_code;

                // Send the message
                ke_msg_send(ind);
            }

            status = PRF_ERR_OK;
        }
        else if (param->handle == spotar_env.spota_shdl + SPOTA_IDX_PATCH_DATA_VAL )
        {
            char_code = SPOTAR_PATCH_DATA_CHAR;
                               
            //attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value[0]);
                    
            if (param->last)
            {
                                                             
                // Inform APP. Allocate the Patch Data value change indication
                struct spotar_patch_data_ind *ind = KE_MSG_ALLOC(SPOTAR_PATCH_DATA_IND,
                                                          spotar_env.con_info.appid, TASK_SPOTAR,
                                                          spotar_patch_data_ind);
                // Fill in the parameter structure
                ind->conhdl = gapc_get_conhdl(spotar_env.con_info.conidx);

                memcpy(ind->pd, &param->value[0], param->length);
                ind->len = param->length;
                ind->char_code = char_code;

                // Send the message
                ke_msg_send(ind);
            }

            status = PRF_ERR_OK;

        }
        else if (param->handle == spotar_env.spota_shdl + SPOTA_IDX_PATCH_LEN_VAL)
        {
            char_code = SPOTAR_PATCH_LEN_CHAR;
            
            attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value[0]);            

            if (param->last)
            {                                                                
                // Inform APP. Allocate the Patch Data value change indication
                struct spotar_patch_len_ind *ind = KE_MSG_ALLOC(SPOTAR_PATCH_LEN_IND,
                                                          spotar_env.con_info.appid, TASK_SPOTAR,
                                                          spotar_patch_len_ind);
                // Fill in the parameter structure
                ind->conhdl = gapc_get_conhdl(spotar_env.con_info.conidx);

                ind->len = 0;
                ind->len  = (uint16_t) ( param->value[1] << 8);
                ind->len |= (uint16_t) ( param->value[0] );
                ind->char_code = char_code;

                // Send the message
                ke_msg_send(ind);
            }

            status = PRF_ERR_OK;
        }        
        else if (param->handle == spotar_env.spota_shdl + SPOTA_IDX_PATCH_STATUS_NTF_CFG)
        {
            char_code = SPOTA_PATCH_STATUS_NTF_CFG;            
            attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value[0]);                        
            status = PRF_ERR_OK;
        }
        else
        {
            char_code = SPOTAR_ERR_CHAR;
        }
        
        if (param->response == 1)
        {
            // Send Write Response
            atts_write_rsp_send(spotar_env.con_info.conidx, param->handle, status);
        } 
    }

    return (KE_MSG_CONSUMED);

}

/**
 ****************************************************************************************
 * @brief Disconnection indication to SPOTA receiver.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_disconnect_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{

    // Check Connection Handle
    if (KE_IDX_GET(src_id) == spotar_env.con_info.conidx)
    {
        // Abnormal reason - APP must alert to the level specified in the Alert Level Char.
        if ((param->reason != CO_ERROR_REMOTE_USER_TERM_CON) &&
            (param->reason != CO_ERROR_CON_TERM_BY_LOCAL_HOST))
        {
            //Get Alert Level value stored in DB
            //attsdb_att_get_value(proxr_env.lls_shdl + LLS_IDX_ALERT_LVL_VAL,
            //                     &length, &alert_lvl);

            //Send it to APP
            //proxr_alert_start(*alert_lvl, PROXR_LLS_CHAR);
        }

        // In any case, inform APP about disconnection
        spotar_disable(param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler spotar_disabled[] =
{
    {SPOTAR_CREATE_DB_REQ,   (ke_msg_func_t) spotar_create_db_req_handler },
};

/// Idle State handler definition.
const struct ke_msg_handler spotar_idle[] =
{
    {SPOTAR_ENABLE_REQ,      (ke_msg_func_t) spotar_enable_req_handler },
};

/// Connected State handler definition.
const struct ke_msg_handler spotar_active[] =
{
    {GATTC_WRITE_CMD_IND,    (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {SPOTAR_PATCH_MEM_INFO_UPDATE_REQ,    (ke_msg_func_t) spotar_mem_info_update_req_handler},
    {SPOTAR_STATUS_UPDATE_REQ,    (ke_msg_func_t) spotar_status_update_req_handler},

};

/// Default State handlers definition
const struct ke_msg_handler spotar_default_state[] =
{
    {GAPC_DISCONNECT_IND,    (ke_msg_func_t) gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler spotar_state_handler[SPOTAR_STATE_MAX] =
{
    [SPOTAR_DISABLED]    = KE_STATE_HANDLER(spotar_disabled),
    [SPOTAR_IDLE]        = KE_STATE_HANDLER(spotar_idle),
    [SPOTAR_ACTIVE]   = KE_STATE_HANDLER(spotar_active),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler spotar_default_handler = KE_STATE_HANDLER(spotar_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t spotar_state[SPOTAR_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_SPOTA_RECEIVER

/// @} SPOTARTASK
