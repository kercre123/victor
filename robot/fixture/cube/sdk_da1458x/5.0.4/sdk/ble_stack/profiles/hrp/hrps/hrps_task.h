/**
 ****************************************************************************************
 *
 * @file hrps_task.h
 *
 * @brief Header file - Heart Rate Profile Sensor Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _HRPS_TASK_H_
#define _HRPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HRPSTASK Task
 * @ingroup HRPS
 * @brief Heart Rate Profile Task.
 *
 * The HRPSTASK is responsible for handling the messages coming in and out of the
 * @ref HRPS collector block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_HR_SENSOR)
#include <stdint.h>
#include "ke_task.h"
#include "hrps.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Heart Rate task instances
#define HRPS_IDX_MAX     0x01

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the HRPS task
enum
{
    /// Disabled state
    HRPS_DISABLED,
    /// Idle state
    HRPS_IDLE,
    /// Connected state
    HRPS_CONNECTED,

    /// Number of defined states.
    HRPS_STATE_MAX,
};

/// Messages for Heart Rate Profile Sensor
enum
{
    ///Start the Heart Rate Profile Sensor - at connection
    HRPS_ENABLE_REQ = KE_FIRST_MSG(TASK_HRPS),

    /// Disable confirmation with configuration to save after profile disabled
    HRPS_DISABLE_IND,

    /// Error indication to Host
    HRPS_ERROR_IND,

    ///Send Heart Rate measurement value from APP
    HRPS_MEAS_SEND_REQ,
    ///Send Heart Rate measurement value confirm to APP so stable values can be erased
    ///if correctly sent.
    HRPS_MEAS_SEND_CFM,
    ///Inform APP of new configuration value
    HRPS_CFG_INDNTF_IND,
    ///Inform APP that Energy Expanded must be reset value
    HRPS_ENERGY_EXP_RESET_IND,

    ///Add HRS into the database
    HRPS_CREATE_DB_REQ,
    ///Inform APP about DB creation status
    HRPS_CREATE_DB_CFM,
};

/// Parameters of the @ref HRPS_ENABLE_REQ message
struct hrps_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized;
    /// b1 or b2 and b3 can go together
    uint8_t sec_lvl;
    ///Type of connection - will someday depend on button press length; can be CFG or DISCOVERY
    uint8_t con_type;

    /// Heart Rate Notification configuration
    uint16_t hr_meas_ntf_en;

    ///Body Sensor Location
    uint8_t body_sensor_loc;
};

///Parameters of the @ref HRPS_CFG_INDNTF_IND message
struct hrps_cfg_indntf_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

///Parameters of the @ref HRPS_DISABLE_IND message
struct hrps_disable_ind
{
    uint16_t conhdl;
    /// Heart Rate Notification configuration
    uint16_t hr_meas_ntf_en;
};

/////Parameters of the @ref HRPS_MEAS_SEND_REQ message
struct hrps_meas_send_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Heart Rate measurement
    struct hrs_hr_meas meas_val;
};

///Parameters of the @ref HRPS_MEAS_SEND_CFM message
struct hrps_meas_send_cfm
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

///Parameters of the @ref HRPS_ENERGY_EXP_RESET_IND message
struct hrps_energy_exp_reset_ind
{
    ///Connection handle
    uint16_t conhdl;
};

///Parameters of the @ref HRPS_CREATE_DB_REQ message
struct hrps_create_db_req
{
    ///Database configuration
    uint8_t features;
};

///Parameters of the @ref HRPS_CREATE_DB_CFM message
struct hrps_create_db_cfm
{
    ///Status
    uint8_t status;
};


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler hrps_state_handler[HRPS_STATE_MAX];
extern const struct ke_state_handler hrps_default_handler;
extern ke_state_t hrps_state[HRPS_IDX_MAX];

#endif /* #if BLE_HR_SENSOR */

/// @} HRPSTASK

#endif /* _HRPS_TASK_H_ */
