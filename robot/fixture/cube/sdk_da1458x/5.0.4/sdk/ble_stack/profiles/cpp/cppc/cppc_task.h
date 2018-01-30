/**
 ****************************************************************************************
 *
 * @file cppc_task.h
 *
 * @brief Header file - Cycling Power Profile Collector Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _CPPC_TASK_H_
#define _CPPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup CPPCTASK Cycling Power Profile Collector Task
 * @ingroup CPPC
 * @brief Cycling Power Profile Collector Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cpp_common.h"

#if (BLE_CP_COLLECTOR)

#include "ke_task.h"
#include "cppc.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Cycling Power Collector task instances
#define CPPC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the CPPC task
enum cppc_states
{
    /// Idle state
    CPPC_IDLE,
    /// Connected state
    CPPC_CONNECTED,
    /// Busy state
    CPPC_BUSY,

    /// Number of defined states.
    CPPC_STATE_MAX
};

/// Message IDs
enum cppc_msg_ids
{
    /// Enable the Cycling Power Profile Collector task - at connection
    CPPC_ENABLE_CMD = KE_FIRST_MSG(TASK_CPPC),

    /// Disable Indication
    CPPC_DISABLE_IND,

    /// Read the value of an attribute in the peer device database
    CPPC_READ_CMD,
    /// Configure sending of notification or indication
    CPPC_CFG_NTFIND_CMD,

    /// Configure the SC Control Point value
    CPPC_CTNL_PT_CFG_CMD,

    /// Indicate that an attribute value has been received either upon notification or read response
    CPPC_VALUE_IND,

    /// Indicate that an control point response has been triggered by peer device
    CPPC_CTNL_PT_RSP_IND,

    /// Indicate the content of the peer device Alert Notification service
    CPPC_CPS_CONTENT_IND,

    /// Complete Event Information
    CPPC_CMP_EVT,

    /// Procedure Timeout Timer
    CPPC_TIMEOUT_TIMER_IND,
};

/// Operation Codes
enum cppc_op_code
{
    /// Reserved operation code
    CPPC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    CPPC_ENABLE_OP_CODE,
    /// Read attribute value Procedure
    CPPC_READ_OP_CODE,

    /// Wait for the Write Response after having written a Client Char. Cfg. Descriptor.
    CPPC_CFG_NTF_IND_OP_CODE,

    /// Wait for the Write Response after having written the Control Point Char.
    CPPC_CTNL_PT_CFG_WR_OP_CODE,
    /// Wait for the Indication Response after having written the Control Point Char.
    CPPC_CTNL_PT_CFG_IND_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref CPPC_ENABLE_CMD message
struct cppc_enable_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Connection type
    uint8_t con_type;

    /// Existing handle values CPS
    struct cppc_cps_content cps;
};

/// Parameters of the @ref CPPC_READ_CMD message
struct cppc_read_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref CPPC_CFG_NTFIND_CMD message
struct cppc_cfg_ntfind_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Descriptor code
    uint8_t desc_code;
    /// Ntf/Ind Configuration
    uint16_t ntfind_cfg;
};

/// Parameters of the @ref CPPC_CTNL_PT_CFG_CMD message
struct cppc_ctnl_pt_cfg_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// SC Control Point Request
    struct cpp_ctnl_pt_req ctnl_pt;
};

/// Parameters of the @ref CPPC_VALUE_IND message
struct cppc_value_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union cppc_value_tag
    {
        /// CP Measurement
        struct cpp_cp_meas cp_meas;
        /// CP Feature
        uint32_t sensor_feat;
        /// Sensor Location
        uint8_t sensor_loc;
        /// Sensor Vector
        struct cpp_cp_vector cp_vector;
        /// Client Characteristic Configuration Descriptor Value
        uint16_t ntf_cfg;
    } value;
};


/// Parameters of the @ref CPPC_CTNL_PT_RSP_IND message
struct cppc_ctnl_pt_rsp_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// SC Control Point Response
    struct cpp_ctnl_pt_rsp rsp;
};

/// Parameters of the @ref CPPC_CPS_CONTENT_IND message
struct cppc_cps_content_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// Cycling Power Service Content
    struct cppc_cps_content cps;
};

/// Parameters of the @ref CPPC_CMP_EVT message
struct cppc_cmp_evt
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation code
    uint8_t operation;
    /// Status
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler cppc_state_handler[CPPC_STATE_MAX];
extern const struct ke_state_handler cppc_default_handler;
extern ke_state_t cppc_state[CPPC_IDX_MAX];

#endif //(BLE_CP_COLLECTOR)

/// @} CPPCTASK

#endif //(_CPPC_TASK_H_)
