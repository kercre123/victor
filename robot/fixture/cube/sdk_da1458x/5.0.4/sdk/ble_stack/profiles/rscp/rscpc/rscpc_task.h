/**
 ****************************************************************************************
 *
 * @file rscpc_task.h
 *
 * @brief Header file - Running Speed and Cadence Profile Collector Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _RSCPC_TASK_H_
#define _RSCPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup RSCPCTASK Running Speed and Cadence Profile Collector Task
 * @ingroup RSCPC
 * @brief Running Speed and Cadence Profile Collector Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rscp_common.h"

#if (BLE_RSC_COLLECTOR)

#include "ke_task.h"
#include "rscpc.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Running Speed and Cadence Collector task instances
#define RSCPC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the RSCPC task
enum rscpc_states
{
    /// Idle state
    RSCPC_IDLE,
    /// Connected state
    RSCPC_CONNECTED,
    /// Busy state
    RSCPC_BUSY,

    /// Number of defined states.
    RSCPC_STATE_MAX
};

/// Message IDs
enum rscpc_msg_ids
{
    /// Enable the Running Speed and Cadence Profile Collector task - at connection
    RSCPC_ENABLE_CMD = KE_FIRST_MSG(TASK_RSCPC),

    /// Disable Indication
    RSCPC_DISABLE_IND,

    /// Read the value of an attribute in the peer device database
    RSCPC_READ_CMD,
    /// Configure sending of notification or indication
    RSCPC_CFG_NTFIND_CMD,

    /// Configure the SC Control Point value
    RSCPC_CTNL_PT_CFG_CMD,

    /// Indicate that an attribute value has been received either upon notification or read response
    RSCPC_VALUE_IND,

    /// Indicate the content of the peer device Alert Notification service
    RSCPC_RSCS_CONTENT_IND,

    /// Complete Event Information
    RSCPC_CMP_EVT,

    /// Procedure Timeout Timer
    RSCPC_TIMEOUT_TIMER_IND,
};

/// Operation Codes
enum rscpc_op_codes
{
    /// Reserved operation code
    RSCPC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    RSCPC_ENABLE_OP_CODE,
    /// Read attribute value Procedure
    RSCPC_READ_OP_CODE,

    /// Wait for the Write Response after having written a Client Char. Cfg. Descriptor.
    RSCPC_CFG_NTF_IND_OP_CODE,
    /// Wait for the Write Response after having written the SC Control Point Char.
    RSCPC_CTNL_PT_CFG_WR_OP_CODE,
    /// Wait for the Indication Response after having written the SC Control Point Char.
    RSCPC_CTNL_PT_CFG_IND_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref RSCPC_ENABLE_CMD message
struct rscpc_enable_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Connection type
    uint8_t con_type;

    /// Existing handle values RSCS
    struct rscpc_rscs_content rscs;
};

/// Parameters of the @ref RSCPC_READ_CMD message
struct rscpc_read_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref RSCPC_CFG_NTFIND_CMD message
struct rscpc_cfg_ntfind_cmd
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

/// Parameters of the @ref RSCPC_CTNL_PT_CFG_CMD message
struct rscpc_ctnl_pt_cfg_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// SC Control Point Request
    struct rscp_sc_ctnl_pt_req sc_ctnl_pt;
};

/// Parameters of the @ref RSCPC_VALUE_IND message
struct rscpc_value_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union rscpc_value_tag
    {
        /// RSC Measurement
        struct rscp_rsc_meas rsc_meas;
        /// RSC Feature
        uint16_t sensor_feat;
        /// Sensor Location
        uint8_t sensor_loc;
        /// SC Control Point Response
        struct rscp_sc_ctnl_pt_rsp ctnl_pt_rsp;
        /// Client Characteristic Configuration Descriptor Value
        uint16_t ntf_cfg;
    } value;
};

/// Parameters of the @ref RSCPC_RSCS_CONTENT_IND message
struct rscpc_rscs_content_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// Running Speed and Cadence Service Content
    struct rscpc_rscs_content rscs;
};

/// Parameters of the @ref RSCPC_CMP_EVT message
struct rscpc_cmp_evt
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

extern const struct ke_state_handler rscpc_state_handler[RSCPC_STATE_MAX];
extern const struct ke_state_handler rscpc_default_handler;
extern ke_state_t rscpc_state[RSCPC_IDX_MAX];

#endif //(BLE_RSC_COLLECTOR)

/// @} RSCPCTASK

#endif //(_RSCPC_TASK_H_)
