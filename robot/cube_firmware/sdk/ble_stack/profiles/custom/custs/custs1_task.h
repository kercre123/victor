/**
 ****************************************************************************************
 *
 * @file custs1_task.h
 *
 * @brief Custom Service profile task header file.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef __CUSTS1_TASK_PRF_H
#define __CUSTS1_TASK_PRF_H

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#if (BLE_CUSTOM1_SERVER)

#include <stdint.h>
#include "ke_task.h"
#include "prf_types.h"
#include "compiler.h"        // compiler definition
#include "att.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
 
 #define CUSTS1_IDX_MAX        (1)
 
 
/// Possible states of the CUSTOMS task
enum
{
    /// Idle state
    CUSTS1_IDLE,
    /// Disabled state
    CUSTS1_DISABLED,
    /// Connected state
    CUSTS1_CONNECTED,
    /// Number of defined states.
    CUSTS1_STATE_MAX,
};

/// Messages for CUSTS1
enum
{
    /// Add a CUSTOMS instance into the database
    CUSTS1_CREATE_DB_REQ = KE_FIRST_MSG(TASK_CUSTS1),
    /// Inform APP of database creation status
    CUSTS1_CREATE_DB_CFM,
    
    /// Start the Custom Service Task - at connection
    CUSTS1_ENABLE_REQ,
    /// Set/update characteristic value
    CUSTS1_VAL_SET_REQ, 
    /// Set/update characteristic value and trigger a notification
    CUSTS1_VAL_NTF_REQ, 
    /// Response after receiving a CUSTS1_VAL_NTF_REQ message and a notification is triggered 
    CUSTS1_VAL_NTF_CFM,
    /// Set/update characteristic value and trigger an indication
    CUSTS1_VAL_IND_REQ, 
    ///Response after receiving a CUSTS1_VAL_IND_REQ message and an indication is triggered
    CUSTS1_VAL_IND_CFM,
    /// Indicate that the characteristic value has been written
    CUSTS1_VAL_WRITE_IND,
    /// Inform the application that the profile service role task has been disabled after a disconnection
    CUSTS1_DISABLE_IND,
    /// Profile error report
    CUSTS1_ERROR_IND,    
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref CUSTS1_CREATE_DB_REQ message
struct custs1_create_db_req
{
    ///max number of casts1 service characteristics
    uint8_t max_nb_att;                  
    uint8_t *att_tbl;
    uint8_t *cfg_flag;
    
    ///Database configuration
    uint16_t features;
};

/// Parameters of the @ref CUSTS1_CREATE_DB_CFM message
struct custs1_create_db_cfm
{
    ///Status
    uint8_t status;
};

/// Parameters of the @ref CUSTS1_ENABLE_REQ message
struct custs1_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized; b1 or b2 and b3 can go together
    uint8_t sec_lvl;
    /// Type of connection
    uint8_t con_type;
};

/// Parameters of the @ref CUSTS1_DISABLE_IND message
struct custs1_disable_ind
{
    /// Connection handle
    uint16_t conhdl;
};

/// Parameters of the @ref CUSTS1_VAL_WRITE_IND massage
struct custs1_val_write_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Handle of the attribute that has to be written
    uint16_t handle;
    /// Data length to be written
    uint16_t length;
    /// Data to be written in attribute database
    uint8_t  value[__ARRAY_EMPTY];
};

/// Parameters of the @ref CUSTS1_VAL_NTF_CFM massage
struct custs1_val_ntf_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Handle of the attribute that has been updated
    uint16_t handle;
    /// Confirmation status
    uint8_t status;
};

/// Parameters of the @ref CUSTS1_VAL_IND_CFM massage
struct custs1_val_ind_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Handle of the attribute that has been updated
    uint16_t handle;
    /// Confirmation status
    uint8_t status;
};

/// Parameters of the @ref CUSTS1_VAL_SET_REQ massage
struct custs1_val_set_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Handle of the attribute that has to be written
    uint16_t handle;
    /// Data length to be written
    uint16_t length;
    /// Data to be written in attribute database
    uint8_t  value[__ARRAY_EMPTY];
};

/// Parameters of the @ref CUSTS1_VAL_NTF_REQ massage
struct custs1_val_ntf_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Handle of the attribute that has to be written
    uint16_t handle;
    /// Data length to be written
    uint16_t length;
    /// Data to be written in attribute database
    uint8_t  value[__ARRAY_EMPTY];
};

/// Parameters of the @ref CUSTS1_VAL_IND_REQ massage
struct custs1_val_ind_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Handle of the attribute that has to be written
    uint16_t handle;
    /// Data length to be written
    uint16_t length;
    /// Data to be written in attribute database
    uint8_t  value[__ARRAY_EMPTY];
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler custs1_state_handler[CUSTS1_STATE_MAX];
extern const struct ke_state_handler custs1_default_handler;
extern ke_state_t custs1_state[CUSTS1_IDX_MAX];

#endif // BLE_CUSTOM1_SERVER
#endif // __CUSTS1_TASK_PRF_H
