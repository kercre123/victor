/**
 ****************************************************************************************
 *
 * @file device_config.h
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

#ifndef DEVICE_CONFIG_H_
#define DEVICE_CONFIG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_DEVICE_CONFIG)

#include "ke_task.h"
#include "atts.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Handles offsets
enum
{
    DEVICE_CONFIG_IDX_SVC,

    DEVICE_VERSION_IDX_CHAR,
    DEVICE_VERSION_IDX_VAL,
    
    DEVICE_CONFIG_IDX_CHAR,
    DEVICE_CONFIG_IDX_VAL,
    
    DEVICE_READ_REQUEST_IDX_CHAR,
    DEVICE_READ_REQUEST_IDX_VAL,

    DEVICE_READ_RESPONSE_IDX_CHAR,
    DEVICE_READ_RESPONSE_IDX_VAL,
    DEVICE_READ_RESPONSE_IDX_CFG,
    
    DEVICE_CONFIG_IDX_NB,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// device_config environment variable
struct device_config_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    /// Sample128  svc Start Handle
    uint16_t device_config_shdl;
    
    //Notification property status
    uint8_t feature;

};

/*
 *  DEVICE_CONFIG PROFILE ATTRIBUTES VALUES DECLARATION
 ****************************************************************************************
 */

/// device_config Service
extern const struct att_uuid_128 device_config_svc;

/// Version characteristic
extern const struct att_uuid_128 device_version_val;
/// Version value
extern struct att_char128_desc device_version_char; 
/// Configuration - Characteristic
extern struct att_char128_desc device_config_char;
/// Configuration - Value
extern const struct att_uuid_128 device_config_val;
/// Read command characteristic
extern const struct att_uuid_128 device_read_command_val;
/// Read command value
extern struct att_char128_desc device_read_command_char; 
/// Read response characteristic
extern const struct att_uuid_128 device_read_response_val;
/// Read response value
extern struct att_char128_desc device_read_response_char; 

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct device_config_env_tag device_config_env;
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************** **********************************
 * @brief Initialization of the DEVICE_CONFIG module.
 * This function performs all the initializations of the DEVICE_CONFIG module.
 ****************************************************************************************
 */
void device_config_init(void);

/**
 ****************************************************************************************
 * @brief Send value change to application.
 * @param val Value.
 ****************************************************************************************
 */
 
void device_config_send_val(uint8_t *val, uint16_t length, uint8_t param_id);

/**
 ****************************************************************************************
 * @brief Disable role.
 ****************************************************************************************
 */
void device_config_disable(void);

/**
 ****************************************************************************************
 * @brief Sends indication to application task thet a read request is received
 ****************************************************************************************
 */

void device_config_read_val_request(uint8_t param_id);
#endif //BLE_DEVICE_CONFIG

#endif // DEVICE_CONFIG_H_
