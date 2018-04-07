/**
 ****************************************************************************************
 *
 * @file cscps_task.h
 *
 * @brief Header file - Cycling Speed and Cadence Profile Sensor Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _CSCPS_TASK_H_
#define _CSCPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup CSCPSTASK Task
 * @ingroup CSCPS
 * @brief Cycling Speed and Cadence Profile Task.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cscp_common.h"

#if (BLE_CSC_SENSOR)

#include <stdint.h>
#include "ke_task.h"
#include "cscps.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Cycling Speed and Cadence Profile Sensor role task instances
#define CSCPS_IDX_MAX        (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the CSCPS task
enum cscps_state
{
    /// Disabled state
    CSCPS_DISABLED,
    /// Idle state
    CSCPS_IDLE,
    /// Connected state
    CSCPS_CONNECTED,
    /// Busy state
    CSCPS_BUSY,
    /// Wait for Confirm
    CSCPS_WAIT_FOR_CFM,

    /// Number of defined states.
    CSCPS_STATE_MAX
};

/// Messages for Cycling Speed and Cadence Profile Sensor
enum cscps_msg_id
{
    /// Add CSCS into the database
    CSCPS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_CSCPS),
    /// Enable the CSCP Sensor task for a connection
    CSCPS_ENABLE_REQ,

    /// Inform the application that the profile has been disabled after a disconnection
    CSCPS_DISABLE_IND,

    /// Send a CSC Measurement to the peer device (Notification)
    CSCPS_NTF_CSC_MEAS_CMD,

    /// Indicate that the SC Control Point characteristic value has been written
    CSCPS_SC_CTNL_PT_REQ_IND,
    /// Application response after receiving a CSCPS_SC_CTNL_PT_REQ_IND message
    CSCPS_SC_CTNL_PT_CFM,

    /// Indicate a new NTF/IND configuration to the application
    CSCPS_NTF_IND_CFG_IND,

    /// Send a complete event status to the application
    CSCPS_CMP_EVT,
};

/// Operation Code used in the profile state machine
enum cscps_op_code
{
    /// Reserved Operation Code
    CSCPS_RESERVED_OP_CODE          = 0x00,

    /// Create Database Operation Code
    CSCPS_CREATE_DB_OP_CODE,
    /// Enable Profile Operation Code
    CSCPS_ENABLE_OP_CODE,
    /// Send CSC Measurement Operation Code
    CSCPS_SEND_CSC_MEAS_OP_CODE,

    /**
     * SC Control Point Operation
     */

    /// Set Cumulative Value Operation Code
    CSCPS_CTNL_PT_CUMUL_VAL_OP_CODE,
    /// Update Sensor Location Operation Code
    CSCPS_CTNL_PT_UPD_LOC_OP_CODE,
    /// Request Supported Sensor Locations Operation Code
    CSCPS_CTNL_PT_SUPP_LOC_OP_CODE,
    /// Error Indication Sent Operation Code
    CSCPS_CTNL_ERR_IND_OP_CODE,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref CSCPS_CREATE_DB_REQ message
struct cscps_create_db_req
{
    /**
     * CSC Feature Value - Not supposed to be modified during the lifetime of the device
     * This value is used to decide if the SC Control Point Characteristic is part of the
     * Cycling Speed and Cadence service.
     */
    uint16_t csc_feature;
    /**
     * Indicate if the Sensor Location characteristic is supported. Note that if the Multiple
     * Sensor Location feature is set has supported in the csc_feature parameter, the
     * characteristic will be added (mandatory).
     */
    uint8_t sensor_loc_supp;
    /// Sensor location
    uint8_t sensor_loc;
};

/// Parameters of the @ref CSCPS_ENABLE_REQ message
struct cscps_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Security Level
    uint8_t sec_lvl;
    /// Connection Type
    uint8_t con_type;

    /// Stored CSC Measurement Char. Client Characteristic Configuration
    uint16_t csc_meas_ntf_cfg;
    /// Stored SC Control Point Char. Client Characteristic Configuration
    uint16_t sc_ctnl_pt_ntf_cfg;

    /// Wheel revolution
    uint32_t wheel_rev;
};

/// Parameters of the @ref CSCPS_DISABLE_IND message
struct cscps_disable_ind
{
    /// Connection handle
    uint16_t conhdl;

    /// CSC Measurement Char. Client Characteristic Configuration
    uint16_t csc_meas_ntf_cfg;
    /// SC Control Point Char. Client Characteristic Configuration
    uint16_t sc_ctnl_pt_ntf_cfg;

    /// Total Wheel revolutions
    uint32_t wheel_revol;
};

/// Parameters of the @ref CSCPS_NTF_CSC_MEAS_CMD message
struct cscps_ntf_csc_meas_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Flags
    uint8_t flags;
    /// Cumulative Crank Revolution
    uint16_t cumul_crank_rev;
    /// Last Crank Event Time
    uint16_t last_crank_evt_time;
    /// Last Wheel Event Time
    uint16_t last_wheel_evt_time;
    /// Wheel Revolution since the last wheel event time
    int16_t wheel_rev;
};

/// Parameters of the @ref CSCPS_SC_CTNL_PT_REQ_IND message
struct cscps_sc_ctnl_pt_req_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t op_code;
    /// Value
    union cscps_sc_ctnl_pt_req_ind_value
    {
        /// Cumulative value
        uint32_t cumul_value;
        /// Sensor Location
        uint8_t sensor_loc;
    } value;
};

/// Parameters of the @ref CSCPS_SC_CTNL_PT_CFM message
struct cscps_sc_ctnl_pt_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
    /// Value
    union cscps_sc_ctnl_pt_cfm_value
    {
        /// Sensor Location
        uint8_t sensor_loc;
        /// Supported sensor locations
        uint16_t supp_sensor_loc;
        /// New Cumulative Wheel revolution Value
        uint32_t cumul_wheel_rev;
    } value;
};

/// Parameters of the @ref CSCPS_NTF_IND_CFG_IND message
struct cscps_ntf_ind_cfg_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic Code (CSC Measurement or SC Control Point)
    uint8_t char_code;
    /// Char. Client Characteristic Configuration
    uint16_t ntf_cfg;
};

/// Parameters of the @ref CSCPS_CMP_EVT message
struct cscps_cmp_evt
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Operation Status
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler cscps_state_handler[CSCPS_STATE_MAX];
extern const struct ke_state_handler cscps_default_handler;
extern ke_state_t cscps_state[CSCPS_IDX_MAX];

#endif //(BLE_CSC_SENSOR)

/// @} CSCPSTASK

#endif //(_CSCPS_TASK_H_)
