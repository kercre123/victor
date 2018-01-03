/**
 ****************************************************************************************
 *
 * @file diss_task.h
 *
 * @brief Header file - DISSTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef DISS_TASK_H_
#define DISS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup DISSTASK Task
 * @ingroup DISS
 * @brief Device Information Service Server Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_DIS_SERVER)
#include <stdint.h>
#include "ke_task.h"
#include "diss.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define DISS_IDX_MAX        (1)

/// Possible states of the DISS task
enum
{
    ///Disabled state
    DISS_DISABLED,
    ///Idle state
    DISS_IDLE,
    ///Connected state
    DISS_CONNECTED,

    ///Number of defined states.
    DISS_STATE_MAX
};

/// Messages for Device Information Service Server
enum
{
    ///Add a DIS instance into the database
    DISS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_DISS),
    ///Inform APP of database creation status
    DISS_CREATE_DB_CFM,
    ///Set the value of an attribute
    DISS_SET_CHAR_VAL_REQ,

    ///Start the Device Information Service Task - at connection
    DISS_ENABLE_REQ,

    /// Inform the application that the profile service role task has been disabled after a disconnection
    DISS_DISABLE_IND,

    ///Error indication to Host
    DISS_ERROR_IND,
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref DISS_CREATE_DB_REQ message
struct diss_create_db_req
{
    ///Database configuration
    uint16_t features;
};

/// Parameters of the @ref DISS_CREATE_DB_CFM message
struct diss_create_db_cfm
{
    ///Status
    uint8_t status;
};

/// Parameters of the @ref DISS_SET_CHAR_VAL_REQ message - shall be dynamically allocated
struct diss_set_char_val_req
{
    /// Characteristic Code
    uint8_t char_code;
    /// Value length
    uint8_t val_len;
    /// Value
    uint8_t val[1];
};

/// Parameters of the @ref DISS_ENABLE_REQ message
struct diss_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized; b1 or b2 and b3 can go together
    uint8_t sec_lvl;
    ///Type of connection
    uint8_t con_type;
};

/// Parameters of the @ref DISS_DISABLE_IND message
struct diss_disable_ind
{
    ///Connection handle
    uint16_t conhdl;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler diss_state_handler[DISS_STATE_MAX];
extern const struct ke_state_handler diss_default_handler;
extern ke_state_t diss_state[DISS_IDX_MAX];

#endif //BLE_DIS_SERVER

/// @} DISSTASK
#endif // DISS_TASK_H_
