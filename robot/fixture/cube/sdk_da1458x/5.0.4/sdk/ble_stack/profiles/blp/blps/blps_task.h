/**
 ****************************************************************************************
 *
 * @file blps_task.h
 *
 * @brief Header file - Blood Pressure Profile Sensor Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _BLPS_TASK_H_
#define _BLPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BLPSTASK Task
 * @ingroup BLPS
 * @brief Blood Pressure Profile Task.
 *
 * The BLPSTASK is responsible for handling the messages coming in and out of the
 * @ref BLPS collector block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_BP_SENSOR)
#include <stdint.h>
#include "ke_task.h"
#include "blps.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Blood Pressure task instances
#define BLPS_IDX_MAX     0x01

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the BLPS task
enum
{
    /// Disabled state
    BLPS_DISABLED,
    /// Idle state
    BLPS_IDLE,
    /// Connected state
    BLPS_CONNECTED,

    /// Number of defined states.
    BLPS_STATE_MAX
};

enum
{
    /// measurement sent by profile
    BLPS_BP_MEAS_SEND,
    /// peer device confirm reception
    BLPS_CENTRAL_IND_CFM,
};

/// Messages for Blood Pressure Profile Sensor
enum
{
    ///Start the Blood Pressure Profile Sensor - at connection
    BLPS_ENABLE_REQ = KE_FIRST_MSG(TASK_BLPS),

    /// Disable confirmation with configuration to save after profile disabled
    BLPS_DISABLE_IND,

    /// Error indication to Host
    BLPS_ERROR_IND,

    ///Send blood pressure measurement value from APP
    BLPS_MEAS_SEND_REQ,
    ///Send blood pressure measurement value confirm to APP so stable values can be erased
    ///if correctly sent.
    BLPS_MEAS_SEND_CFM,
    ///Inform APP of new configuration value
    BLPS_CFG_INDNTF_IND,

    ///Add BPS into the database
    BLPS_CREATE_DB_REQ,
    ///Inform APP about DB creation status
    BLPS_CREATE_DB_CFM,
};

/// Parameters of the @ref BLPS_ENABLE_REQ message
struct blps_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized;
    /// b1 or b2 and b3 can go together
    /// it matches the permission bytes order of settings
    uint8_t sec_lvl;
    ///Type of connection - will someday depend on button press length; can be CFG or DISCOVERY
    uint8_t con_type;

    /// Blood Pressure indication configuration
    uint16_t bp_meas_ind_en;
    /// Intermediate Cuff Pressure Notification configuration
    uint16_t interm_cp_ntf_en;

    /// Blood Pressure Feature
    uint16_t bp_feature;
};

///Parameters of the @ref BLPS_DISABLE_IND message
struct blps_disable_ind
{
    uint16_t conhdl;
    /// Blood Pressure indication configuration
    uint16_t bp_meas_ind_en;
    /// Intermediate Cuff Pressure Notification configuration
    uint16_t interm_cp_ntf_en;
};

///Parameters of the @ref BLPS_CFG_INDNTF_IND message
struct blps_cfg_indntf_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
    ///Own code for differentiating between Blood Pressure Measurement, and Intermediate
    /// Cuff Pressure Measurement characteristics
    uint8_t char_code;
};

/////Parameters of the @ref BLPS_MEAS_SEND_REQ message
struct blps_meas_send_req
{
    ///Connection handle
    uint16_t conhdl;
    /// Flag indicating if it is a intermediary cuff pressure measurement (0) or
    /// stable blood pressure measurement (1).
    uint8_t flag_interm_cp;
    ///Blood Pressure measurement
    struct bps_bp_meas meas_val;
};

///Parameters of the @ref BLPS_MEAS_SEND_CFM message
struct blps_meas_send_cfm
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

///Parameters of the @ref BLPS_CREATE_DB_REQ message
struct blps_create_db_req
{
    ///Database Configuration
    uint8_t features;
};

///Parameters of the @ref BLPS_CREATE_DB_CFM message
struct blps_create_db_cfm
{
    ///Status
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler blps_state_handler[BLPS_STATE_MAX];
extern const struct ke_state_handler blps_default_handler;
extern ke_state_t blps_state[BLPS_IDX_MAX];

#endif /* #if BLE_BP_SENSOR */

/// @} BLPSTASK

#endif /* _BLPS_TASK_H_ */
