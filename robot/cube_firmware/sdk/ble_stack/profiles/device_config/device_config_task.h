/**
 ****************************************************************************************
 *
 * @file device_config_task.h
 *
 * @brief Header file - device_config_task.
 * @brief 128 UUID service. 
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef DEVICE_CONFIG_TASK_H_
#define DEVICE_CONFIG_TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
*/

#if (BLE_DEVICE_CONFIG)

#include <stdint.h>
#include "ke_task.h"
#include "device_config.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Sample128 task instances
#define DEVICE_CONFIG_IDX_MAX                 (1)

#define MAX_PARAM_SIZE         64

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the DEVICE_CONFIG task
enum
{
    /// Disabled State
    DEVICE_CONFIG_DISABLED,
    /// Idle state
    DEVICE_CONFIG_IDLE,
    /// Connected state
    DEVICE_CONFIG_CONNECTED,
    /// Number of defined states.
    DEVICE_CONFIG_STATE_MAX
};

/// Messages for Device configuration
enum
{
    /// Start device_config. Device connection
    DEVICE_CONFIG_ENABLE_REQ = KE_FIRST_MSG(TASK_DEVICE_CONFIG),

    /// Disable confirm.
    DEVICE_CONFIG_DISABLE_IND,

    /// Att Value change indication
    DEVICE_CONFIG_VAL_IND,
    
    /// Read value request indication
    DEVICE_READ_REQUEST_IND,
    
    /// Read value response indication
    DEVICE_READ_RESPONSE_CMD,

    ///Create DataBase
    DEVICE_CONFIG_CREATE_DB_REQ,
    ///Inform APP of database creation status
    DEVICE_CONFIG_CREATE_DB_CFM,
    
    /// Error Indication
    DEVICE_CONFIG_ERROR_IND,
};


/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref DEVICE_CONFIG_ENABLE_REQ message
struct device_config_enable_req
{
    /// Connection Handle
    uint16_t conhdl;
    /// Security level
    uint8_t sec_lvl;
    
    /// Configuartion setup version
    uint8_t version[16];
    
    uint8_t version_length; 
    
    
};

/// Parameters of the @ref DEVICE_CONFIG_DISABLE_IND message
struct device_config_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
};

/// Parameters of the @ref DEVICE_CONFIG_VAL_IND message
struct device_config_val_ind
{
    /// Connection handle
    uint16_t conhdl;
    
    /// Value
    uint8_t val[64];
    
    /// Connection handle
    uint8_t size;
    
    // Characteristic id 
    uint8_t param_id;
    
};

/// Parameters of the @ref DEVICE_READ_REQUEST_IND message
struct device_config_read_request_ind
{
    /// Connection handle
    uint16_t conhdl;
    
    // Characteristic id 
    uint8_t param_id;
    
};

/// Parameters of the @ref DEVICE_READ_RESPONSE_CMD message
struct device_config_read_response_cmd
{
    /// Connection handle
    uint16_t conhdl;
    
    /// Value
    uint8_t val[64];
    
    /// Connection handle
    uint8_t size;
    
    // Characteristic id 
    uint8_t param_id;
    
    // status 
    uint8_t status;
    
};

/// Parameters of the @ref DEVICE_CONFIG_CREATE_DB_REQ message
struct device_config_create_db_req
{
    uint8_t dummy;
};

/// Parameters of the @ref DEVICE_CONFIG_CREATE_DB_CFM message
struct device_config_create_db_cfm
{
    /// Status
    uint8_t status;
};

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler device_config_state_handler[DEVICE_CONFIG_STATE_MAX];
extern const struct ke_state_handler device_config_default_handler;
extern ke_state_t device_config_state[DEVICE_CONFIG_IDX_MAX];

#endif //BLE_DEVICE_CONFIG

#endif // DEVICE_CONFIG_TASK_H_
