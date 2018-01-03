/**
 ****************************************************************************************
 *
 * @file wpts_task.c
 *
 * @brief WPTS Task implementation.
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

#if (BLE_WPTS)

#include "gap.h"
#include "gapc.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "wpts.h"
#include "wpts_task.h"
#include "attm_cfg.h"
#include "attm_db.h"
#include "prf_utils.h"


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref wpts_CREATE_DB_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int wpts_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct wpts_create_db_req const *param,
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
    wpts_env.con_info.prf_id = TASK_WPTS;

    /*---------------------------------------------------*
     * Link Loss Service Creation
     *---------------------------------------------------*/

        //Add Service Into Database
        nb_att_16 = 7; // 2 UUID16 atts
        nb_att_32 = 0;// No UUID32 att
        nb_att_128 = 5; //1 UUID128 att
        status = attmdb_add_service(&(wpts_env.wpt_shdl), TASK_WPTS,
                                             nb_att_16, nb_att_32, nb_att_128, 162 + 6/*Mode Transition*/); //total attributte size 2 + 19 + 5 + 19 + 17 + 19 + 1 + 1 + 19 + 20 + 19 + 20

        if (status == ATT_ERR_NO_ERROR)
        {
            //add svc attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, ATT_UUID_16_LEN, //Data size = 16 (ATT_UUID_128_LEN)
                                               ATT_UUID_16_LEN, (uint8_t*)&att_decl_svc, PERM(RD, ENABLE),
                                               &(wpts_env.wpt_shdl));
            
                        
            status = attmdb_att_set_value(wpts_env.wpt_shdl, ATT_UUID_16_LEN, (uint8_t *)&wpt_svc);
            
////////////////////                                  
            //add char attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));
                       
            //add val attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, WPTS_PRU_CHARGING_CTRL_CHAR_LEN, //Data size = 5
                                               ATT_UUID_128_LEN, (uint8_t*)&wpt_charging_ptu_ctrl_char.attr_type, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                               &(val_hdl));
            
            memcpy(wpt_charging_ptu_ctrl_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(wpt_charging_ptu_ctrl_char), (uint8_t *)&wpt_charging_ptu_ctrl_char);

/////////////////////////////                       
            //add char attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));

            //add val attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, WPTS_PTU_STATIC_CHAR_LEN, //Data size = 17
                                               ATT_UUID_128_LEN, (uint8_t*)&wpt_ptu_static_char.attr_type, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                               &(val_hdl));
            
            memcpy(wpt_ptu_static_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(wpt_ptu_static_char), (uint8_t *)&wpt_ptu_static_char);
//////////////////////

            //add char attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));

            //add val attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, WPTS_PRU_ALERT_CHAR_LEN, //Data size = 1 + 6 Mode Transition
                                               ATT_UUID_128_LEN, (uint8_t*)&wpt_charging_pru_alert_char.attr_type,
                                               PERM(RD, ENABLE) | PERM(NTF, ENABLE) | PERM(IND, ENABLE),
                                               &(val_hdl));
            
            memcpy(wpt_charging_pru_alert_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(wpt_charging_pru_alert_char), (uint8_t *)&wpt_charging_pru_alert_char);

            //add cfg attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, 2, //Data size = 2
                                               ATT_UUID_16_LEN, (uint8_t*)&att_decl_cfg, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                               &(val_hdl));
///////////////////////////////            

            //add char attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));

            //add val attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, WPTS_PRU_STATIC_CHAR_LEN, //Data size = 20
                                               ATT_UUID_128_LEN, (uint8_t*)&wpt_charging_pru_static_char.attr_type, PERM(RD, ENABLE),
                                               &(val_hdl));
            
            memcpy(wpt_charging_pru_static_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(wpt_charging_pru_static_char), (uint8_t *)&wpt_charging_pru_static_char);

/////////////////////////////////

            //add char attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, ATT_UUID_128_LEN + 3, //Data size = 19 (ATT_UUID_128_LEN + 3)
                                               ATT_UUID_16_LEN, (uint8_t*) &att_decl_char, PERM(RD, ENABLE),
                                               &(char_hdl));

            //add val attribute
            status = attmdb_add_attribute(wpts_env.wpt_shdl, WPTS_PRU_DYNAMIC_CHAR_LEN, //Data size = 20
                                               ATT_UUID_128_LEN, (uint8_t*)&wpt_charging_pru_dynamic_char.attr_type, PERM(RD, ENABLE),
                                               &(val_hdl));
            
            memcpy(wpt_charging_pru_dynamic_char.attr_hdl, &val_hdl, sizeof(uint16_t));
            
            status = attmdb_att_set_value(char_hdl, sizeof(wpt_charging_pru_dynamic_char), (uint8_t *)&wpt_charging_pru_dynamic_char);

/////////////////////////////////
            //Disable wpt service
            attmdb_svc_set_permission(wpts_env.wpt_shdl, PERM(SVC, DISABLE));

            //Go to Idle State
    
            //If we are here, database has been fulfilled with success, go to idle state
            ke_state_set(TASK_WPTS, WPTS_IDLE);
        }
        
        //Send CFM to application
        struct wpts_create_db_cfm * cfm = KE_MSG_ALLOC(WPTS_CREATE_DB_CFM, src_id,
                                                    TASK_WPTS, wpts_create_db_cfm);
        cfm->status = status;
        ke_msg_send(cfm);
    

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Enable the wpts role, used after connection.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
uint8_t pru_static_data[WPTS_PRU_STATIC_CHAR_LEN] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
uint8_t pru_dynamic_data[WPTS_PRU_DYNAMIC_CHAR_LEN] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

static int wpts_enable_req_handler(ke_msg_id_t const msgid,
                                    struct wpts_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{    
    uint8_t zero[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    // Keep source of message, to respond to it further on
    wpts_env.con_info.appid = src_id;
    // Store the connection handle for which this profile is enabled
    wpts_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (wpts_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&wpts_env, PRF_ERR_REQ_DISALLOWED,
                                  WPTS_ERROR_IND, WPTS_ENABLE_REQ);
    }
    else
    {
        // wpt service
        attmdb_svc_set_permission(wpts_env.wpt_shdl, param->sec_lvl);

        //Set initial Values
        attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_CTRL_IDX_VAL,
                             WPTS_PRU_CHARGING_CTRL_CHAR_LEN, (uint8_t *)zero);

        attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PTU_STATIC_IDX_VAL,
                             WPTS_PTU_STATIC_CHAR_LEN, (uint8_t *)zero);
        
        attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_ALERT_IDX_VAL,
                             WPTS_PRU_ALERT_CHAR_LEN, (uint8_t *)zero);
        
        attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_ALERT_IDX_CFG,
                            WPTS_PRU_ALERT_CONFIR_CHAR_LEN, (uint8_t *)zero);

        // Update value in DB
        attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_STATIC_IDX_VAL,
                             WPTS_PRU_STATIC_CHAR_LEN, pru_static_data);
        
        attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_DYNAMIC_IDX_VAL,
                            WPTS_PRU_DYNAMIC_CHAR_LEN, pru_dynamic_data);
        
        // Go to Connected state
        ke_state_set(TASK_WPTS, WPTS_CONNECTED);
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
    uint16_t value = 0x0000;
    uint8_t status = PRF_ERR_OK;
    
    struct pru_control_value* pru_ctrl = 0;
    struct ptu_static_value*  ptu_static = 0;

    if (KE_IDX_GET(src_id) == wpts_env.con_info.conidx)
    {
        if (param->handle == wpts_env.wpt_shdl + WPTS_CHARGING_PRU_CTRL_IDX_VAL)
        {
            // PRU CONTROL VALUE used for eg. enabling charging
            
            if (param->length == WPTS_PRU_CHARGING_CTRL_CHAR_LEN)
            {
                pru_ctrl = (struct pru_control_value*)&param->value;
                
                // Data validation
                if ((pru_ctrl->time_set > WPTS_CTRL_TIME_80_MS) ||
                    // bits 0..3 RFU
                    ((pru_ctrl->enables & 0x0F) != 0))
                {
                    status = PRF_ERR_INVALID_PARAM;
                }
                // Check for permissions correctness
                else if ((pru_ctrl->permission != WPTS_CTRL_PERM_WITHOUT_REASON) &&
                         (pru_ctrl->permission != WPTS_CTRL_PERM_TIME_AVAIL_POW) &&
                         (pru_ctrl->permission != WPTS_CTRL_PERM_DENIED_LATCHING) &&
                         (pru_ctrl->permission != WPTS_CTRL_PERM_DENIED_LIMIT_POW) &&
                         (pru_ctrl->permission != WPTS_CTRL_PERM_DENIED_LIMIT_DEV_NUM) &&
                         (pru_ctrl->permission != WPTS_CTRL_PERM_DENIED_CLASS_SUPPORT))
                {
                    status = PRF_ERR_INVALID_PARAM;
                }
                    
                if (PRF_ERR_OK == status)
                {
                    // Set value in the database
                    attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value);
                    
                    if (param->last)
                    {
                        // Inform APP of configuration change
                        struct wpts_ctrl_change_ind* ind = KE_MSG_ALLOC(WPTS_CTRL_CHANGE_IND,
                                                                        wpts_env.con_info.appid, TASK_WPTS,
                                                                        wpts_ctrl_change_ind);
                        
                        ind->conhdl = gapc_get_conhdl(wpts_env.con_info.conidx);
                        ind->enables    = pru_ctrl->enables;
                        ind->permission = pru_ctrl->permission;
                        ind->time_set   = pru_ctrl->time_set;
                        ind->rfu_1      = pru_ctrl->rfu;

                        ke_msg_send(ind);
                    }
                }
            }
            else
            {
                status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }
            // Inform App layer about wrong PTU data
            if (status != PRF_ERR_OK)
            {
                wpts_error_ind_send(wpts_env.con_info.appid,
                                    WPTS_ERROR_WR_PTU_CONTROL_CHANGE_CMD,
                                    status);
            }
        }
        else if (param->handle == wpts_env.wpt_shdl + WPTS_CHARGING_PTU_STATIC_IDX_VAL)
        {
            // PTU STATIC VALUE
            
            if (param->length == WPTS_PTU_STATIC_CHAR_LEN)
            {
                ptu_static = (struct ptu_static_value*)&param->value;
                
                // Data validation
                if ((ptu_static->max_source_impedance > WPTC_STATIC_MAX_SRC_IMPEDANCE_375)  ||
                   (ptu_static->max_load_resistance > WPTC_STATIC_MAX_LOAD_RESISTANCE_55)   ||
                   (ptu_static->num_devices_supported >= WPTC_MAX_SUPPORTED_DEV)            ||
                   (ptu_static->ptu_class > WPTC_CLASS_5))
                {
                    status = PRF_ERR_INVALID_PARAM;
                }
                
                if (PRF_ERR_OK == status)
                {
                    // Set value in the database
                    attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value);
                    
                    if (param->last)
                    {
                        // Inform APP of configuration change
                        struct wpts_ptu_static_val_ind* ind = KE_MSG_ALLOC(WPTS_PTU_STATIC_VAL_IND,
                                                                           wpts_env.con_info.appid, TASK_WPTS,
                                                                           wpts_ptu_static_val_ind);

                        ind->conhdl = gapc_get_conhdl(wpts_env.con_info.conidx);

                        ind->valid          = ptu_static->opt_fields_validity;
                        ind->power          = ptu_static->ptu_power;
                        ind->max_impedance  = ptu_static->max_source_impedance;
                        ind->max_resistance = ptu_static->max_load_resistance;
                        ind->rfu_1          = ptu_static->rfu_1;
                        ind->class          = ptu_static->ptu_class;
                        ind->hw_ver         = ptu_static->hardware_rev;
                        ind->sw_ver         = ptu_static->firmware_rev;
                        ind->prtcl_ver      = ptu_static->protocol_rev;
                        ind->max_dev        = ptu_static->num_devices_supported;
                        ind->rfu_2          = ptu_static->rfu_2;
                        ind->rfu_3          = ptu_static->rfu_3;
                        ind->rfu_4          = ptu_static->rfu_4;
                        ke_msg_send(ind);
                    }
                }
            }
            else
            {
                status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }
            // Inform App layer about wrong PTU data
            if (status != PRF_ERR_OK)
            {
                wpts_error_ind_send(wpts_env.con_info.appid,
                                    WPTS_ERROR_WR_PTU_STATIC_VAL_CMD,
                                    status);
            }
        }
        else if (param->handle == wpts_env.wpt_shdl + WPTS_CHARGING_PRU_ALERT_IDX_CFG)
        {
            // PRU_ALERT NOTIFICATION configuration
            
            if (param->length == WPTS_PRU_ALERT_CONFIR_CHAR_LEN) // 2 byte
            {
                // Extract value before check
                memcpy(&value, &(param->value), sizeof(uint16_t));
                
                if (value > (PRF_CLI_START_NTF | PRF_CLI_START_IND))
                {
                    status = PRF_ERR_INVALID_PARAM;
                }
                
                if (PRF_ERR_OK == status)
                {
                    if ((value & PRF_CLI_START_NTF) == PRF_CLI_START_NTF)
                    {
                        wpts_env.features |= WPTS_FEATURE_ALERT_NTF_ACTIVE;
                    }
                    else
                    {
                        wpts_env.features &= ~WPTS_FEATURE_ALERT_NTF_ACTIVE;
                    }
                    
                    if ((value & PRF_CLI_START_IND) == PRF_CLI_START_IND)
                    {
                        wpts_env.features |= WPTS_FEATURE_ALERT_IND_ACTIVE;
                    }
                    else
                    {
                        wpts_env.features &= ~WPTS_FEATURE_ALERT_IND_ACTIVE;
                    }
                    
                    // Set value in the database
                    attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value);
                    
                    if (param->last)
                    {
                        // Inform APP of configuration change
                        struct wpts_alert_cfg_indntf_ind* ind = KE_MSG_ALLOC(WPTS_ALERT_CFG_INDNTF_IND,
                                                                             wpts_env.con_info.appid, TASK_WPTS,
                                                                             wpts_alert_cfg_indntf_ind);

                        ind->conhdl = gapc_get_conhdl(wpts_env.con_info.conidx);
                        memcpy(&ind->cfg_val, &value, sizeof(uint16_t));

                        ke_msg_send(ind);
                    }
                }
            }
            else
            {
                status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }
        }
        else
        {
            status = PRF_APP_ERROR;
        }
        
        // Send Write Response only if client requests for RSP (ignored when 'Write Without Response' is used)
        if (param->response == 1)
        {
            // Send Write Response
            atts_write_rsp_send(wpts_env.con_info.conidx, param->handle, status);
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * The handler will analyse what has been set and decide alert level
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_read_cmd_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_cmd_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    att_size_t length;
    uint8_t pru_dynamic[WPTS_PRU_DYNAMIC_CHAR_LEN] = {0};
    uint8_t* pru_ptr = pru_dynamic;
    
    if (KE_IDX_GET(src_id) == wpts_env.con_info.conidx)
    {
        if (wpts_env.wpt_shdl + WPTS_CHARGING_PRU_DYNAMIC_IDX_VAL == param->handle)
        {
            attmdb_att_get_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_DYNAMIC_IDX_VAL,
                                 &length, &pru_ptr);
            
            if ((pru_ptr[WPTS_PRU_DYNAMIC_CHAR_LEN - 4] & WPTS_ALERT_ADJ_POWER_COMPLETE_RSP) == WPTS_ALERT_ADJ_POWER_COMPLETE_RSP)
            {
                pru_ptr[WPTS_PRU_DYNAMIC_CHAR_LEN - 4] &= ~WPTS_ALERT_ADJ_POWER_COMPLETE_RSP;
                
                attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_DYNAMIC_IDX_VAL,
                                  WPTS_PRU_DYNAMIC_CHAR_LEN, pru_ptr);
                
                memcpy(pru_dynamic_data, pru_ptr, WPTS_PRU_DYNAMIC_CHAR_LEN);
            }
            // Inform APP about PRU Dynamic Characteristic read action
            void* ind = ke_msg_alloc(WPTS_PRU_DYNAMIC_VAL_READ_DB_IND,
                                     wpts_env.con_info.appid, TASK_WPTS, 0);
            
            ke_msg_send(ind);
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY message meaning that Measurement
 * notification has been correctly sent to peer device (but not confirmed by peer device).
 * *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{

    if (param->req_type == GATTC_NOTIFY || param->req_type == GATTC_INDICATE)
    {
        wpts_alert_send_cfm_send(wpts_env.con_info.appid, param->status);
    }
    return (KE_MSG_CONSUMED);
}

static int wpts_alert_send_req_handler(ke_msg_id_t const msgid,
                                      struct wpts_alert_send_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    // Alert bit fields value
    uint8_t value[sizeof(uint8_t) + WPTS_BD_ADDR_LEN] = {0};
    // check if feature was activated - ntf as default
    uint8_t is_ntfind_active = WPTS_FEATURE_ALERT_NTF_ACTIVE;

    if (param->conhdl == gapc_get_conhdl(wpts_env.con_info.conidx))
    {
        value[0] = param->alert_val;
        // set Device Address
        memcpy(&value[1], param->dev_address, WPTS_BD_ADDR_LEN);
        if (param->is_ind)
        {
            is_ntfind_active = WPTS_FEATURE_ALERT_IND_ACTIVE;
        }
        // Update value in DB
        attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_ALERT_IDX_VAL,
                             sizeof(uint8_t) + WPTS_BD_ADDR_LEN, value);
        
        if (WPTS_IS_SUPPORTED(is_ntfind_active))
        {
            // Send notification through GATT
            prf_server_send_event((prf_env_struct *)&wpts_env, param->is_ind,
                    wpts_env.wpt_shdl + WPTS_CHARGING_PRU_ALERT_IDX_VAL);
        }
        else
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }
    
    if (status != PRF_ERR_OK)
    {
        // Value has not been sent
        wpts_alert_send_cfm_send(src_id, status);
    }
    
    return (KE_MSG_CONSUMED);
}

static int wpts_static_val_req_handler(ke_msg_id_t const msgid,
                                       struct wpts_pru_static_val_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    uint8_t *pru_param = (uint8_t *)param;
    
    // Save data - if it's updated before profile is enabled
    memcpy(pru_static_data, pru_param, WPTS_PRU_STATIC_CHAR_LEN);
    // Update value in DB
    status = attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_STATIC_IDX_VAL,
                                  WPTS_PRU_STATIC_CHAR_LEN, pru_static_data);
    
    // Inform App about request status
    wpts_static_val_cfm_send(src_id, status);
    
    return (KE_MSG_CONSUMED);
}

static int wpts_dynamic_val_req_handler(ke_msg_id_t const msgid,
                                        struct wpts_pru_dynamic_val_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    // PRU Dynamic param casted to u8 (issue related to 8bit processor)
    uint8_t *pru_param = (uint8_t *)param;
    
    pru_param += sizeof(param->conhdl);
    // Update value in DB
    memcpy(pru_dynamic_data, pru_param, WPTS_PRU_DYNAMIC_CHAR_LEN);
    status = attmdb_att_set_value(wpts_env.wpt_shdl + WPTS_CHARGING_PRU_DYNAMIC_IDX_VAL,
                                  WPTS_PRU_DYNAMIC_CHAR_LEN, pru_dynamic_data);
   
    // Inform App about request status
    wpts_dynamic_val_cfm_send(src_id, status);
    
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to wpts.
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
    if (KE_IDX_GET(src_id) == wpts_env.con_info.conidx)
    {
        // reset Power Sharing Mode bit to prevent wrong re-connection procedure
        pru_dynamic_data[WPTS_PRU_DYNAMIC_CHAR_LEN - 4] &= ~WPTS_ALERT_ADJ_POWER_COMPLETE_RSP;
        // In any case, inform APP about disconnection
        wpts_disable();
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler wpts_disabled[] =
{
    {WPTS_CREATE_DB_REQ,                (ke_msg_func_t)wpts_create_db_req_handler},
};

/// Idle State handler definition.
const struct ke_msg_handler wpts_idle[] =
{
    {WPTS_PRU_STATIC_VAL_UPD_DB_REQ,    (ke_msg_func_t)wpts_static_val_req_handler},
    {WPTS_PRU_DYNAMIC_VAL_UPD_DB_REQ,   (ke_msg_func_t)wpts_dynamic_val_req_handler},
    {WPTS_ENABLE_REQ,                   (ke_msg_func_t)wpts_enable_req_handler},
};

/// Connected State handler definition.
const struct ke_msg_handler wpts_connected[] =
{
    {GATTC_WRITE_CMD_IND,               (ke_msg_func_t)gattc_write_cmd_ind_handler},
    {GATTC_READ_CMD_IND,                (ke_msg_func_t)gattc_read_cmd_ind_handler},
    {GATTC_CMP_EVT,                     (ke_msg_func_t)gattc_cmp_evt_handler},
    {WPTS_ALERT_SEND_REQ,               (ke_msg_func_t)wpts_alert_send_req_handler},
    {WPTS_PRU_DYNAMIC_VAL_UPD_DB_REQ,   (ke_msg_func_t)wpts_dynamic_val_req_handler},
};

/// Default State handlers definition
const struct ke_msg_handler wpts_default_state[] =
{
    {GAPC_DISCONNECT_IND,               (ke_msg_func_t)gap_disconnnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler wpts_state_handler[WPTS_STATE_MAX] =
{
    [WPTS_DISABLED]    = KE_STATE_HANDLER(wpts_disabled),
    [WPTS_IDLE]        = KE_STATE_HANDLER(wpts_idle),
    [WPTS_CONNECTED]   = KE_STATE_HANDLER(wpts_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler wpts_default_handler = KE_STATE_HANDLER(wpts_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t wpts_state[WPTS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// WPTC Static value tables

// look at WPTC_GET_MAX_SRC_IMPEDANCE macro note (wpt_common.h)
const uint16_t ptu_max_src_imp[WPTC_STATIC_MAX_SRC_IMPEDANCE_END >> WPTC_STATIC_IMP_RESIS_SHIFT] = {
                                                                      50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 
                                                                      150, 175, 200, 225, 250, 275, 300, 350, 375,
                                                                      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// look at WPTC_GET_MAX_LOAD_RESISTANCE macro note (wpt_common.h)
const uint8 ptu_max_load_resistance[WPTC_STATIC_MAX_LOAD_RESISTANCE_END >> WPTC_STATIC_IMP_RESIS_SHIFT] = {
                                                                            5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55,
                                                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                                            0, 0, 0, 0, 0, 0, 0 };

#endif //BLE_WPTS

