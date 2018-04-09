/**
 ****************************************************************************************
 *
 * @file rscps_task.h
 *
 * @brief Header file - Running Speed and Cadence Profile Sensor Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _RSCPS_TASK_H_
#define _RSCPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup RSCPSTASK Task
 * @ingroup RSCPS
 * @brief Running Speed and Cadence Profile Task.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rscp_common.h"

#if (BLE_RSC_SENSOR)

#include <stdint.h>
#include "ke_task.h"
#include "rscps.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Running Speed and Cadence Profile Sensor role task instances
#define RSCPS_IDX_MAX        (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the RSCPS task
enum rscps_state
{
    /// Disabled state
    RSCPS_DISABLED,
    /// Idle state
    RSCPS_IDLE,
    /// Connected state
    RSCPS_CONNECTED,
    /// Busy state
    RSCPS_BUSY,
    /// Wait for Confirm
    RSCPS_WAIT_FOR_CFM,

    /// Number of defined states.
    RSCPS_STATE_MAX
};

/// Messages for Running Speed and Cadence Profile Sensor
enum rscps_msg_id
{
    /// Add RSCS into the database
    RSCPS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_RSCPS),
    /// Enable the RSCP Sensor task for a connection
    RSCPS_ENABLE_REQ,

    /// Inform the application that the profile has been disabled after a disconnection
    RSCPS_DISABLE_IND,

    /// Send a RSC Measurement to the peer device (Notification)
    RSCPS_NTF_RSC_MEAS_CMD,

    /// Indicate that the SC Control Point characteristic value has been written
    RSCPS_SC_CTNL_PT_REQ_IND,
    /// Application response after receiving a RSCPS_SC_CTNL_PT_REQ_IND message
    RSCPS_SC_CTNL_PT_CFM,

    /// Indicate a new NTF/IND configuration to the application
    RSCPS_NTF_IND_CFG_IND,

    /// Send a complete event status to the application
    RSCPS_CMP_EVT,
};

/// Operation Code used in the profile state machine
enum rscps_op_code
{
    /// Reserved Operation Code
    RSCPS_RESERVED_OP_CODE          = 0x00,

    /// Create Database Operation Code
    RSCPS_CREATE_DB_OP_CODE,
    /// Enable Profile Operation Code
    RSCPS_ENABLE_OP_CODE,
    /// Send RSC Measurement Operation Code
    RSCPS_SEND_RSC_MEAS_OP_CODE,

    /**
     * SC Control Point Operation
     */

    /// Set Cumulative Value Operation Code
    RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE,
    /// Start Sensor Calibration Operation Code
    RSCPS_CTNL_PT_START_CAL_OP_CODE,
    /// Update Sensor Location Operation Code
    RSCPS_CTNL_PT_UPD_LOC_OP_CODE,
    /// Request Supported Sensor Locations Operation Code
    RSCPS_CTNL_PT_SUPP_LOC_OP_CODE,
    /// Error Indication Sent Operation Code
    RSCPS_CTNL_ERR_IND_OP_CODE,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref RSCPS_CREATE_DB_REQ message
struct rscps_create_db_req
{
    /**
     * RSC Feature Value - Not supposed to be modified during the lifetime of the device
     * This value is used to decide if the SC Control Point Characteristic is part of the
     * Running Speed and Cadence service.
     */
    uint16_t rsc_feature;
    /**
     * Indicate if the Sensor Location characteristic is supported. Note that if the Multiple
     * Sensor Location feature is set has supported in the rsc_feature parameter, the
     * characteristic will be added (mandatory).
     */
    uint8_t sensor_loc_supp;
    /// Sensor location
    uint8_t sensor_loc;
};

/// Parameters of the @ref RSCPS_ENABLE_REQ message
struct rscps_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Security Level
    uint8_t sec_lvl;
    /// Connection Type
    uint8_t con_type;

    /// Stored RSC Measurement Char. Client Characteristic Configuration
    uint16_t rsc_meas_ntf_cfg;
    /// Stored SC Control Point Char. Client Characteristic Configuration
    uint16_t sc_ctnl_pt_ntf_cfg;
};

/// Parameters of the @ref RSCPS_DISABLE_IND message
struct rscps_disable_ind
{
    /// Connection handle
    uint16_t conhdl;

    /// RSC Measurement Char. Client Characteristic Configuration
    uint16_t rsc_meas_ntf_cfg;
    /// SC Control Point Char. Client Characteristic Configuration
    uint16_t sc_ctnl_pt_ntf_cfg;
};

/// Parameters of the @ref RSCPS_NTF_RSC_MEAS_CMD message
struct rscps_ntf_rsc_meas_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Flags
    uint8_t flags;
    /// Instantaneous Cadence
    uint8_t inst_cad;
    /// Instantaneous Speed
    uint16_t inst_speed;
    /// Instantaneous Stride Length
    uint16_t inst_stride_len;
    /// Total Distance
    uint32_t total_dist;
};

/// Parameters of the @ref RSCPS_SC_CTNL_PT_REQ_IND message
struct rscps_sc_ctnl_pt_req_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t op_code;
    /// Value
    union rscps_sc_ctnl_pt_req_ind_value
    {
        /// Cumulative value
        uint32_t cumul_value;
        /// Sensor Location
        uint8_t sensor_loc;
    } value;
};

/// Parameters of the @ref RSCPS_SC_CTNL_PT_CFM message
struct rscps_sc_ctnl_pt_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
    /// Value
    union rscps_sc_ctnl_pt_cfm_value
    {
        /// Sensor Location
        uint8_t sensor_loc;
        /// Supported sensor locations
        uint16_t supp_sensor_loc;
    } value;
};

/// Parameters of the @ref RSCPS_NTF_IND_CFG_IND message
struct rscps_ntf_ind_cfg_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic Code (RSC Measurement or SC Control Point)
    uint8_t char_code;
    /// Char. Client Characteristic Configuration
    uint16_t ntf_cfg;
};

/// Parameters of the @ref RSCPS_CMP_EVT message
struct rscps_cmp_evt
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

extern const struct ke_state_handler rscps_state_handler[RSCPS_STATE_MAX];
extern const struct ke_state_handler rscps_default_handler;
extern ke_state_t rscps_state[RSCPS_IDX_MAX];

#endif //(BLE_RSC_SENSOR)

/// @} RSCPSTASK

#endif //(_RSCPS_TASK_H_)
