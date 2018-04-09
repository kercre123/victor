/**
 ****************************************************************************************
 *
 * @file device_config.c
 *
 * @brief Device configuration service. Sample code
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

#include "device_config.h"
#include "device_config_task.h"
#include "attm_db.h"
#include "gapc.h"

/*
 *  DEVICE_CONFIG PROFILE ATTRIBUTES VALUES DEFINTION
 ****************************************************************************************
 */

/// device_config Service 
const struct att_uuid_128 device_config_svc     = {{0x23, 0x80, 0xc5, 0x20, 0x68, 0xd2, 0x11, 0xe4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}};

/// Version characteristic
const struct att_uuid_128 device_version_val     = {{0x24, 0x80, 0xc5, 0x20, 0x68, 0xd2, 0x11, 0xe4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}};

struct att_char128_desc device_version_char = { ATT_CHAR_PROP_RD,
                                                                    {0,0},
                                                                    {0x24, 0x80, 0xc5, 0x20, 0x68, 0xd2, 0x11, 0xe4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}; 

/// Configuration characteristic
const struct att_uuid_128 device_config_val     = {{0x25, 0x80, 0xc5, 0x20, 0x68, 0xd2, 0x11, 0xe4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}};

struct att_char128_desc device_config_char = { ATT_CHAR_PROP_WR,
                                                                    {0,0},
                                                                    {0x25, 0x80, 0xc5, 0x20, 0x68, 0xd2, 0x11, 0xe4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}; 

/// Read command characteristic
const struct att_uuid_128 device_read_command_val     = {{0x26, 0x80, 0xc5, 0x20, 0x68, 0xd2, 0x11, 0xe4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}};

struct att_char128_desc device_read_command_char = { ATT_CHAR_PROP_WR,
                                                                    {0,0},
                                                                    {0x26, 0x80, 0xc5, 0x20, 0x68, 0xd2, 0x11, 0xe4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}; 

/// Read response characteristic
const struct att_uuid_128 device_read_response_val     = {{0x27, 0x80, 0xc5, 0x20, 0x68, 0xd2, 0x11, 0xe4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}};

struct att_char128_desc device_read_response_char = { ATT_CHAR_PROP_IND,
                                                                    {0,0},
                                                                    {0x27, 0x80, 0xc5, 0x20, 0x68, 0xd2, 0x11, 0xe4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}; 

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct device_config_env_tag device_config_env __attribute__((section("retention_mem_area0"),zero_init));

static const struct ke_task_desc TASK_DESC_DEVICE_CONFIG = {device_config_state_handler, &device_config_default_handler, device_config_state, DEVICE_CONFIG_STATE_MAX, DEVICE_CONFIG_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Device configuration task creation
 * @return void
 ****************************************************************************************
 */
void device_config_init(void)
{
    // Reset Environment
    memset(&device_config_env, 0, sizeof(device_config_env));
    
    // Create DEVICE_CONFIG task
    ke_task_create(TASK_DEVICE_CONFIG, &TASK_DESC_DEVICE_CONFIG);

    ke_state_set(TASK_DEVICE_CONFIG, DEVICE_CONFIG_DISABLED);
}

/**
 ****************************************************************************************
 * @brief Send indication for write configuration charcteristic 
 * @return void
 ****************************************************************************************
 */
void device_config_send_val(uint8_t *val, uint16_t length, uint8_t param_id)
{
    // Allocate the alert value change indication
   struct device_config_val_ind *ind = KE_MSG_ALLOC(DEVICE_CONFIG_VAL_IND,
                                              device_config_env.con_info.appid, TASK_DEVICE_CONFIG,
                                              device_config_val_ind);
   // Fill in the parameter structure
   ind->conhdl = gapc_get_conhdl(device_config_env.con_info.conidx);
   ind->param_id = param_id;
   ind->size = length;
   memcpy(ind->val, val, length);
   
   // Send the message
   ke_msg_send(ind);
}

/**
 ****************************************************************************************
 * @brief Send device configuration service diasbled indication
 * @return void
 ****************************************************************************************
 */
void device_config_disable(void)
{
    
    // Disable service in database
    attmdb_svc_set_permission(device_config_env.device_config_shdl, PERM_RIGHT_DISABLE);

    struct device_config_disable_ind *ind = KE_MSG_ALLOC(DEVICE_CONFIG_DISABLE_IND,
                                                 device_config_env.con_info.appid, TASK_DEVICE_CONFIG,
                                                 device_config_disable_ind);

    // Fill in the parameter structure
    ind->conhdl   = gapc_get_conhdl(device_config_env.con_info.conidx);

    // Send the message
    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(TASK_DEVICE_CONFIG, DEVICE_CONFIG_IDLE);
}

/**
 ****************************************************************************************
 * @brief Send read command indication to application
 * @return void
 ****************************************************************************************
 */
void device_config_read_val_request(uint8_t param_id)
{
    
    // Allocate message
   struct device_config_read_request_ind *ind = KE_MSG_ALLOC(DEVICE_READ_REQUEST_IND,
                                              device_config_env.con_info.appid, TASK_DEVICE_CONFIG,
                                              device_config_read_request_ind);
    
    // Fill in the parameter structure
    ind->conhdl   = gapc_get_conhdl(device_config_env.con_info.conidx);

    ind->param_id = param_id;
    
    // Send the message
    ke_msg_send(ind);
}

#endif //BLE_DEVICE_CONFIG
