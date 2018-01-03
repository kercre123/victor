/**
 ****************************************************************************************
 *
 * @file accel_task.h
 *
 * @brief Accelerometer profile task declaration
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ACCEL_TASK_H_
#define ACCEL_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup ACCELTASK Accelerator Task
 * @ingroup ACCEL
 * @brief Accelerometer task implementation
 *
 * @{
 ****************************************************************************************
 */



/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "ke_task.h"
#include "accel.h"
#if (BLE_ACCEL)

/*
 * INSTANCES
 ****************************************************************************************
 */
/// Maximum number of instances of the ACCEL task
#define ACCEL_IDX_MAX 1


/*
 * STATES
 ****************************************************************************************
 */
/// Possible states of the LLM task
enum
{
    /// DISABLED state
    ACCEL_DISABLED,
    /// IDLE state
    ACCEL_IDLE,
    /// ACTIVE state
    ACCEL_ACTIVE,
    /// Number of states.
    ACCEL_STATE_MAX
};


/*
 * MESSAGES
 ****************************************************************************************
 */
/// Message API of the ACCEL task
enum
{
    /// Enables the accelerometer profile. The profile has to be enabled only
    /// once a connection has been established by the application
    ACCEL_ENABLE_REQ = KE_FIRST_MSG(TASK_ACCEL),
    /// Enables the accelerometer profile. The profile has to be enabled only
    /// once a connection has been established by the application
    ACCEL_DISABLE_REQ,
    /// Indicates accelerometer value to the profile
    ACCEL_VALUE_REQ,
    /// Indicates to the application that the accelerometer has to be started
    ACCEL_START_IND,
    /// Indicates to the application that the accelerometer has to be stopped
    ACCEL_STOP_IND,
    /// Indicates to the application that a line has to be written on the display
    ACCEL_WRITE_LINE_IND,

    /// Create Accel database request
    ACCEL_CREATE_DB_REQ,
    /// Create Accel database response
    ACCEL_CREATE_DB_CFM
};

/// @ref ACCEL_ENABLE_REQ parameters structure description.
struct accel_enable_req
{
    /// Application Task Id
    ke_task_id_t appid;
    /// Connection handle
    uint16_t conhdl;
};

/// @ref ACCEL_VALUE_REQ parameters structure description.
struct accel_value_req
{
    /// Acceleration values for the three axis
    uint8_t accel[ACCEL_MAX];
};

/// @ref ACCEL_START_IND parameters structure description.
struct accel_start_ind
{
    /// Axis to be enabled
    uint16_t accel_en[ACCEL_MAX];
    /// Accelerometer range
    uint8_t range;
};

/// @ref ACCEL_WRITE_LINE_IND parameters structure description.
struct accel_write_line_ind
{
    /// Line to be written
    uint8_t line;
    /// Axis to be enabled
    char text[ACCEL_LINE_LEN_MAX];
};


///Parameters of the @ref ACCEL_CREATE_DB_REQ message
struct accel_create_db_req
{
    ///Accel Service start handle (0 = automatic handle allocation)
    uint16_t start_hdl;
};

///Parameters of the @ref ACCEL_CREATE_DB_CFM message
struct accel_create_db_cfm
{
    ///Status
    uint8_t status;
};


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler accel_state_handler[ACCEL_STATE_MAX];
extern const struct ke_state_handler accel_default_handler;
extern ke_state_t accel_state[ACCEL_IDX_MAX];


#endif /* BLE_ACCEL*/

/// @} ACCELTASK

#endif // ACCEL_TASK_H_
