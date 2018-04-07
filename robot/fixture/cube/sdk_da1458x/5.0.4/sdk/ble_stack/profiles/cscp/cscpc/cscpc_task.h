/**
 ****************************************************************************************
 *
 * @file cscpc_task.h
 *
 * @brief Header file - Cycling Speed and Cadence Profile Collector Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _CSCPC_TASK_H_
#define _CSCPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup CSCPCTASK Cycling Speed and Cadence Profile Collector Task
 * @ingroup CSCPC
 * @brief Cycling Speed and Cadence Profile Collector Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cscp_common.h"

#if (BLE_CSC_COLLECTOR)

#include "ke_task.h"
#include "cscpc.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Cycling Speed and Cadence Collector task instances
#define CSCPC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the CSCPC task
enum cscpc_states
{
    /// Idle state
    CSCPC_IDLE,
    /// Connected state
    CSCPC_CONNECTED,
    /// Busy state
    CSCPC_BUSY,

    /// Number of defined states.
    CSCPC_STATE_MAX
};

/// Message IDs
enum cscpc_msg_ids
{
    /// Enable the Cycling Speed and Cadence Profile Collector task - at connection
    CSCPC_ENABLE_CMD = KE_FIRST_MSG(TASK_CSCPC),

    /// Disable Indication
    CSCPC_DISABLE_IND,

    /// Read the value of an attribute in the peer device database
    CSCPC_READ_CMD,
    /// Configure sending of notification or indication
    CSCPC_CFG_NTFIND_CMD,

    /// Configure the SC Control Point value
    CSCPC_CTNL_PT_CFG_CMD,

    /// Indicate that an attribute value has been received either upon notification or read response
    CSCPC_VALUE_IND,

    /// Indicate the content of the peer device Alert Notification service
    CSCPC_CSCS_CONTENT_IND,

    /// Complete Event Information
    CSCPC_CMP_EVT,

    /// Procedure Timeout Timer
    CSCPC_TIMEOUT_TIMER_IND,
};

/// Operation Codes
enum cscpc_op_codes
{
    /// Reserved operation code
    CSCPC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    CSCPC_ENABLE_OP_CODE,
    /// Read attribute value Procedure
    CSCPC_READ_OP_CODE,

    /// Wait for the Write Response after having written a Client Char. Cfg. Descriptor.
    CSCPC_CFG_NTF_IND_OP_CODE,
    /// Wait for the Write Response after having written the SC Control Point Char.
    CSCPC_CTNL_PT_CFG_WR_OP_CODE,
    /// Wait for the Indication Response after having written the SC Control Point Char.
    CSCPC_CTNL_PT_CFG_IND_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref CSCPC_ENABLE_CMD message
struct cscpc_enable_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Connection type
    uint8_t con_type;

    /// Existing handle values CSCS
    struct cscpc_cscs_content cscs;
};

/// Parameters of the @ref CSCPC_READ_CMD message
struct cscpc_read_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref CSCPC_CFG_NTFIND_CMD message
struct cscpc_cfg_ntfind_cmd
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

/// Parameters of the @ref CSCPC_CTNL_PT_CFG_CMD message
struct cscpc_ctnl_pt_cfg_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// SC Control Point Request
    struct cscp_sc_ctnl_pt_req sc_ctnl_pt;
};

/// Parameters of the @ref CSCPC_VALUE_IND message
struct cscpc_value_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union cscpc_value_tag
    {
        /// CSC Measurement
        struct cscp_csc_meas csc_meas;
        /// CSC Feature
        uint16_t sensor_feat;
        /// Sensor Location
        uint8_t sensor_loc;
        /// SC Control Point Response
        struct cscp_sc_ctnl_pt_rsp ctnl_pt_rsp;
        /// Client Characteristic Configuration Descriptor Value
        uint16_t ntf_cfg;
    } value;
};

/// Parameters of the @ref CSCPC_CSCS_CONTENT_IND message
struct cscpc_cscs_content_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// Cycling Speed and Cadence Service Content
    struct cscpc_cscs_content cscs;
};

/// Parameters of the @ref CSCPC_CMP_EVT message
struct cscpc_cmp_evt
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

extern const struct ke_state_handler cscpc_state_handler[CSCPC_STATE_MAX];
extern const struct ke_state_handler cscpc_default_handler;
extern ke_state_t cscpc_state[CSCPC_IDX_MAX];

#endif //(BLE_CSC_COLLECTOR)

/// @} CSCPCTASK

#endif //(_CSCPC_TASK_H_)
