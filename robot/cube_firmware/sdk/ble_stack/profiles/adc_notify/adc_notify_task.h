/**
 ****************************************************************************************
 *
 * @file adc_notify_task.h
 *
 * @brief Header file - adc_notify_task.
 * @brief 128 UUID service. sample code
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef ADC_NOTIFY_TASK_H_
#define ADC_NOTIFY_TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
*/

#if (BLE_ADC_NOTIFY)

#include <stdint.h>
#include "ke_task.h"
#include "adc_notify.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of ADC notify task instances
#define ADC_NOTIFY_IDX_MAX                 (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the ADC_NOTIFY task
enum
{
    /// Disabled State
    ADC_NOTIFY_DISABLED,
    /// Idle state
    ADC_NOTIFY_IDLE,
    /// Connected state
    ADC_NOTIFY_CONNECTED,

    /// Number of defined states.
    ADC_NOTIFY_STATE_MAX
};

/// Messages for ADC notify
enum
{
    /// Start adc_notify. Device connection
    ADC_NOTIFY_ENABLE_REQ = KE_FIRST_MSG(TASK_ADC_NOTIFY),

    /// Disable confirm.
    ADC_NOTIFY_DISABLE_IND,

    /// Att Value change indication
    ADC_NOTIFY_CFG_IND,

    ///Create DataBase
    ADC_NOTIFY_CREATE_DB_REQ,
    ///Inform APP of database creation status
    ADC_NOTIFY_CREATE_DB_CFM,

    ///Create DataBase
    ADC_NOTIFY_UPD_CHAR_REQ,
    ///Inform APP of database creation status
    ADC_NOTIFY_UPD_CHAR_CFM,
    
    /// Error Indication
    ADC_NOTIFY_ERROR_IND,
};


/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref ADC_NOTIFY_ENABLE_REQ message
struct adc_notify_enable_req
{
    /// Connection Handle
    uint16_t conhdl;
    /// Security level
    uint8_t sec_lvl;
   
    /// characteristic 2 value
    uint16_t adc_notify_val;
    
    /// char 2 Ntf property status
    uint8_t feature;
};

/// Parameters of the @ref ADC_NOTIFY_DISABLE_IND message
struct adc_notify_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
};

/// Parameters of the @ref ADC_NOTIFY_CFG_IND message
struct adc_notify_cfg_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Value
    uint16_t val;
    
};

/// Parameters of the @ref ADC_NOTIFY_CREATE_DB_REQ message
struct adc_notify_create_db_req
{
    /// Indicate if TXPS is supported or not
    uint8_t features;
};

/// Parameters of the @ref ADC_NOTIFY_CREATE_DB_CFM message
struct adc_notify_create_db_cfm
{
    /// Status
    uint8_t status;
};

/// Parameters of the @ref ADC_NOTIFY_UPD_CHAR_REQ message
struct adc_notify_upd_char_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic Value
    uint16_t val;
};

/// Parameters of the @ref ADC_NOTIFY_UPD_CHAR_CFM message
struct adc_notify_upd_char_cfm
{
    /// Status
    uint8_t status;
};

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler adc_notify_state_handler[ADC_NOTIFY_STATE_MAX];
extern const struct ke_state_handler adc_notify_default_handler;
extern ke_state_t adc_notify_state[ADC_NOTIFY_IDX_MAX];

#endif //BLE_ADC_NOTIFY

#endif // ADC_NOTIFY_TASK_H_
