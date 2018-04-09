/**
 ****************************************************************************************
 *
 * @file sample128_task.h
 *
 * @brief Header file - sample128_task.
 * @brief 128 UUID service. sample code
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef SAMPLE128_TASK_H_
#define SAMPLE128_TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
*/

#if (BLE_SAMPLE128)

#include <stdint.h>
#include "ke_task.h"
#include "sample128.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Sample128 task instances
#define SAMPLE128_IDX_MAX                 (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the SAMPLE128 task
enum
{
    /// Disabled State
    SAMPLE128_DISABLED,
    /// Idle state
    SAMPLE128_IDLE,
    /// Connected state
    SAMPLE128_CONNECTED,

    /// Number of defined states.
    SAMPLE128_STATE_MAX
};

/// Messages for Sample128
enum
{
    /// Start sample128. Device connection
    SAMPLE128_ENABLE_REQ = KE_FIRST_MSG(TASK_SAMPLE128),

    /// Disable confirm.
    SAMPLE128_DISABLE_IND,

    /// Att Value change indication
    SAMPLE128_VAL_IND,

    ///Create DataBase
    SAMPLE128_CREATE_DB_REQ,
    ///Inform APP of database creation status
    SAMPLE128_CREATE_DB_CFM,

    ///Create DataBase
    SAMPLE128_UPD_CHAR2_REQ,
    ///Inform APP of database creation status
    SAMPLE128_UPD_CHAR2_CFM,
    
    /// Error Indication
    SAMPLE128_ERROR_IND,
};


/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref SAMPLE128_ENABLE_REQ message
struct sample128_enable_req
{
    /// Connection Handle
    uint16_t conhdl;
    /// Security level
    uint8_t sec_lvl;

    /// characteristic 1 value
    uint8_t sample128_1_val;
    
    /// characteristic 2 value
    uint8_t sample128_2_val;
    
    /// char 2 Ntf property status
    uint8_t feature;
};

/// Parameters of the @ref SAMPLE128_DISABLE_IND message
struct sample128_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
};

/// Parameters of the @ref SAMPLE128_VAL_IND message
struct sample128_val_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Value
    uint8_t val;
    
};

/// Parameters of the @ref SAMPLE128_CREATE_DB_REQ message
struct sample128_create_db_req
{
    /// Indicate if TXPS is supported or not
    uint8_t features;
};

/// Parameters of the @ref SAMPLE128_CREATE_DB_CFM message
struct sample128_create_db_cfm
{
    /// Status
    uint8_t status;
};

/// Parameters of the @ref SAMPLE128_UPD_CHAR2_REQ message
struct sample128_upd_char2_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic Value
    uint8_t val;
};

/// Parameters of the @ref SAMPLE128_UPD_CHAR2_CFM message
struct sample128_upd_char2_cfm
{
    /// Status
    uint8_t status;
};

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler sample128_state_handler[SAMPLE128_STATE_MAX];
extern const struct ke_state_handler sample128_default_handler;
extern ke_state_t sample128_state[SAMPLE128_IDX_MAX];

#endif //BLE_SAMPLE128

#endif // SAMPLE128_TASK_H_
