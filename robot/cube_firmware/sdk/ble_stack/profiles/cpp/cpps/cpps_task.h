/**
 ****************************************************************************************
 *
 * @file cpps_task.h
 *
 * @brief Header file - Cycling Power Profile Sensor Task.
 *
 * Copyright (C) RivieraWaves 2009-2014
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _CPPS_TASK_H_
#define _CPPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup CPPSTASK Task
 * @ingroup CPPS
 * @brief Cycling Power Profile Task.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cpp_common.h"

#if (BLE_CP_SENSOR)

#include <stdint.h>
#include "ke_task.h"
#include "cpps.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Cycling Power Profile Sensor role task instances
#define CPPS_IDX_MAX        (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the CPPS task
enum cpps_state
{
    /// Disabled state
    CPPS_DISABLED,
    /// Idle state
    CPPS_IDLE,
    /// Connected state
    CPPS_CONNECTED,
    /// Busy state
    CPPS_BUSY,
    /// Wait confirmation
    CPPS_WAIT_FOR_CFM,

    /// Number of defined states.
    CPPS_STATE_MAX
};

/// Messages for Cycling Power Profile Sensor
enum cpps_msg_id
{
    /// Add a CPS instance into the database
    CPPS_CREATE_DB_CMD = KE_FIRST_MSG(TASK_CPPS),

    /// Start the Cycling Power Profile Server profile
    CPPS_ENABLE_CMD,
    /// Disable Indication
    CPPS_DISABLE_IND,

    /// Send a CP Measurement to the peer device (Notification)
    CPPS_NTF_CP_MEAS_CMD,
    /// Send a CP Vector to the peer device (Notification)
    CPPS_NTF_CP_VECTOR_CMD,
    /// Send a complete event status to the application
    CPPS_CMP_EVT,

    /// Indicate that an attribute value has been written
    CPPS_NTF_IND_CFG_IND,

    /// Indicate that Control Point characteristic value has been written
    CPPS_CTNL_PT_REQ_IND,
    /// Application response after receiving a CPPS_CTNL_PT_REQ_IND message
    CPPS_CTNL_PT_CFM,
    /// Confirmation to change vector client configuration
    CPPS_VECTOR_CFG_CFM,
};

/// Operation Code used in the profile state machine
enum cpps_op_code
{
    /// Reserved Operation Code
    CPPS_RESERVED_OP_CODE          = 0x00,

    /// Create Database Operation Code
    CPPS_CREATE_DB_REQ_OP_CODE,
    /// Enable Profile Operation Code
    CPPS_ENABLE_REQ_OP_CODE,

    /// Send CP Measurement Operation Code
    CPPS_NTF_MEAS_OP_CODE,

    /// Send Vector Operation Code
    CPPS_NTF_VECTOR_OP_CODE,

    /**
     * Control Point Operation
     */
    /// Set Cumulative Value
    CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE,
    /// Update Sensor Location
    CPPS_CTNL_PT_UPD_SENSOR_LOC_OP_CODE,
    /// Request Supported Sensor Locations
    CPPS_CTNL_PT_REQ_SUPP_SENSOR_LOC_OP_CODE,
    /// Set Crank Length
    CPPS_CTNL_PT_SET_CRANK_LENGTH_OP_CODE,
    /// Request Crank Length
    CPPS_CTNL_PT_REQ_CRANK_LENGTH_OP_CODE,
    /// Set Chain Length
    CPPS_CTNL_PT_SET_CHAIN_LENGTH_OP_CODE,
    /// Request Chain Length
    CPPS_CTNL_PT_REQ_CHAIN_LENGTH_OP_CODE,
    /// Set Chain Weight
    CPPS_CTNL_PT_SET_CHAIN_WEIGHT_OP_CODE,
    /// Request Chain Weight
    CPPS_CTNL_PT_REQ_CHAIN_WEIGHT_OP_CODE,
    /// Set Span Length
    CPPS_CTNL_PT_SET_SPAN_LENGTH_OP_CODE,
    /// Request Span Length
    CPPS_CTNL_PT_REQ_SPAN_LENGTH_OP_CODE,
    /// Start Offset Compensation
    CPPS_CTNL_PT_START_OFFSET_COMP_OP_CODE,
    /// Mask CP Measurement Characteristic Content
    CPPS_CTNL_MASK_CP_MEAS_CH_CONTENT_OP_CODE,
    /// Request Sampling Rate
    CPPS_CTNL_REQ_SAMPLING_RATE_OP_CODE,
    /// Request Factory Calibration Date
    CPPS_CTNL_REQ_FACTORY_CALIBRATION_DATE_OP_CODE,

    /// Error Indication Sent Operation Code
    CPPS_CTNL_ERR_IND_OP_CODE,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref CPPS_CREATE_DB_REQ message
struct cpps_create_db_req
{
    /**
     * CP Feature Value - Not supposed to be modified during the lifetime of the device
     * This bit field is set in order to decide which features are supported:
     *   Supported features (specification) ---------------- Bits 0 to 18
     */
    uint32_t cp_feature;
    /**
     * Profile characteristic configuration:
     *   Enable broadcaster mode in Measurement Characteristic --- Bit 0
     *   Enable Control Point Characteristic (*) ----------------- Bit 1
     *
     * (*) Note this characteristic is mandatory if server supports:
     *     - Wheel Revolution Data
     *     - Multiple Sensor Locations
     *     - Configurable Settings
     *     - Offset Compensation
     *     - Server allows to be requested for parameters (CPP_CTNL_PT_REQ... codes)
     */
    uint8_t prfl_config;
    /**
     * Indicate the sensor location.
     */
    uint8_t sensor_loc;
};

/// Parameters of the @ref CPPS_ENABLE_REQ message
struct cpps_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Security Level
    uint8_t sec_lvl;
    /// Connection Type
    uint8_t con_type;
    /**
     * Profile characteristic configuration:
     *   Measurement Characteristic notification config --- Bit 0
     *   Measurement Characteristic broadcast config ------ Bit 1
     *   Vector Characteristic notification config -------- Bit 2
     *   Control Point Characteristic indication config --- Bit 3
     */
    uint16_t prfl_ntf_ind_cfg;
    /// Instantaneous Power
    uint32_t instantaneous_power;
};

/// Parameters of the @ref CPPS_DISABLE_IND message
struct cpps_disable_ind
{
    /// Connection handle
    uint16_t conhdl;

    /**
     * Profile characteristic configuration:
     *   Measurement Characteristic notification config --- Bit 0
     *   Vector Characteristic notification config -------- Bit 1
     *   Control Point Characteristic indication config --- Bit 2
     */
    uint16_t prfl_ntf_ind_cfg;
    /// Instantaneous Power
    uint32_t instantaneous_power;
};

/// Parameters of the @ref CPPS_NTF_CP_MEAS_CMD message
struct  cpps_ntf_cp_meas_cmd
{
    /// Connection handle
    uint16_t conhdl;
    ///Parameters
    struct cpp_cp_meas parameters;
};

/// Parameters of the @ref CPPS_NTF_CP_VECTOR_CMD message
struct  cpps_ntf_cp_vector_cmd
{
    /// Connection handle
    uint16_t conhdl;
    ///Parameters
    struct cpp_cp_vector parameters;
};

/// Parameters of the @ref CPPS_CTNL_PT_REQ_IND message
struct cpps_ctnl_pt_req_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t op_code;
    /// Value
    union cpps_ctnl_pt_req_ind_value
    {
        /// Cumulative Value
        uint32_t cumul_val;
        /// Sensor Location
        uint8_t sensor_loc;
        /// Crank Length
        uint16_t crank_length;
        /// Chain Length
        uint16_t chain_length;
        /// Chain Weight
        uint16_t chain_weight;
        /// Span Length
        uint16_t span_length;
        /// Mask Content
        uint16_t mask_content;
    } value;
};

/// Parameters of the @ref CPPS_CTNL_PT_CFM message
struct cpps_ctnl_pt_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t op_code;
    /// Status
    uint8_t status;
    /// Value
    union cpps_sc_ctnl_pt_cfm_value
    {
        /// Supported sensor locations (up to 17)
        uint32_t supp_sensor_loc;
        /// New Cumulative Wheel revolution Value
        uint32_t cumul_wheel_rev;
        /// Crank Length
        uint16_t crank_length;
        /// Chain Length
        uint16_t chain_length;
        /// Chain Weight
        uint16_t chain_weight;
        /// Span Length
        uint16_t span_length;
        /// Offset compensation
        int16_t offset_comp;
        /// Mask Measurement content
        uint16_t mask_meas_content;
        ///Sampling rate
        uint8_t sampling_rate;
        /// New Sensor Location
        uint8_t sensor_loc;
        /// Calibration date
        struct prf_date_time factory_calibration;

    } value;
};

/// Parameters of the @ref CPPS_NTF_IND_CFG_IND message
struct cpps_ntf_ind_cfg_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic Code (CP Measurement or Control Point)
    uint8_t char_code;
    /// Char. Client Characteristic Configuration
    uint16_t ntf_cfg;
};

/// Parameters of the @ref CPPS_VECTOR_CFG_CFM message
struct cpps_vector_cfg_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
    /// Char. Client Characteristic Configuration
    uint16_t ntf_cfg;
};

/// Parameters of the @ref CPPS_CMP_EVT message
struct cpps_cmp_evt
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

extern const struct ke_state_handler cpps_state_handler[CPPS_STATE_MAX];
extern const struct ke_state_handler cpps_default_handler;
extern ke_state_t cpps_state[CPPS_IDX_MAX];

#endif //(BLE_CP_SENSOR)

/// @} CPPSTASK

#endif //(_CPPS_TASK_H_)
