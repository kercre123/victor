/**
 ****************************************************************************************
 *
 * @file custs2_task.h
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

#ifndef __CUSTS2_TASK_PRF_H
#define __CUSTS2_TASK_PRF_H

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#if (BLE_CUSTOM2_SERVER)

#include <stdint.h>
#include "ke_task.h"
#include "prf_types.h"
#include "compiler.h"        // compiler definition
#include "att.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
 
 #define CUSTS2_IDX_MAX        (1)
 
 
/// Possible states of the CUSTOMS task
enum
{
    /// Idle state
    CUSTS2_IDLE,
    /// Disabled state
    CUSTS2_DISABLED,
    /// Connected state
    CUSTS2_CONNECTED,
    /// Number of defined states.
    CUSTS2_STATE_MAX,
};

/// Messages for CUSTS2
enum
{
    /// Add a CUSTOMS instance into the database
    CUSTS2_CREATE_DB_REQ = KE_FIRST_MSG(TASK_CUSTS2),
    /// Inform APP of database creation status
    CUSTS2_CREATE_DB_CFM,
    
    /// Start the Custom Service Task - at connection
    CUSTS2_ENABLE_REQ,
    /// Set/update characteristic value
    CUSTS2_VAL_SET_REQ, 
    /// Set/update characteristic value and trigger a notification
    CUSTS2_VAL_NTF_REQ, 
    /// Response after receiving a CUSTS2_VAL_NTF_REQ message and a notification is triggered 
    CUSTS2_VAL_NTF_CFM,
    /// Set/update characteristic value and trigger an indication
    CUSTS2_VAL_IND_REQ, 
    ///Response after receiving a CUSTS2_VAL_IND_REQ message and an indication is triggered
    CUSTS2_VAL_IND_CFM,
    /// Indicate that the characteristic value has been written
    CUSTS2_VAL_WRITE_IND,
    /// Inform the application that the profile service role task has been disabled after a disconnection
    CUSTS2_DISABLE_IND,
    /// Profile error report
    CUSTS2_ERROR_IND,    
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref CUSTS2_CREATE_DB_REQ message
struct custs2_create_db_req
{
    ///max number of casts1 service characteristics
    uint8_t max_nb_att;                  
    uint8_t *att_tbl;
    uint8_t *cfg_flag;
    
    ///Database configuration
    uint16_t features;
};

/// Parameters of the @ref CUSTS2_CREATE_DB_CFM message
struct custs2_create_db_cfm
{
    ///Status
    uint8_t status;
};

/// Parameters of the @ref CUSTS2_ENABLE_REQ message
struct custs2_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized; b1 or b2 and b3 can go together
    uint8_t sec_lvl;
    /// Type of connection
    uint8_t con_type;
};

/// Parameters of the @ref CUSTS2_DISABLE_IND message
struct custs2_disable_ind
{
    /// Connection handle
    uint16_t conhdl;
};

/// Parameters of the @ref CUSTS2_VAL_WRITE_IND massage
struct custs2_val_write_ind
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

/// Parameters of the @ref CUSTS2_VAL_NTF_CFM massage
struct custs2_val_ntf_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Handle of the attribute that has been updated
    uint16_t handle;
    /// Confirmation status
    uint8_t status;
};

/// Parameters of the @ref CUSTS2_VAL_IND_CFM massage
struct custs2_val_ind_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Handle of the attribute that has been updated
    uint16_t handle;
    /// Confirmation status
    uint8_t status;
};

/// Parameters of the @ref CUSTS2_VAL_SET_REQ massage
struct custs2_val_set_req
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

/// Parameters of the @ref CUSTS2_VAL_NTF_REQ massage
struct custs2_val_ntf_req
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

/// Parameters of the @ref CUSTS2_VAL_IND_REQ massage
struct custs2_val_ind_req
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
extern const struct ke_state_handler custs2_state_handler[CUSTS2_STATE_MAX];
extern const struct ke_state_handler custs2_default_handler;
extern ke_state_t custs2_state[CUSTS2_IDX_MAX];

#endif // BLE_CUSTOM2_SERVER
#endif // __CUSTS2_TASK_PRF_H
