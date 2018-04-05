/**
 ****************************************************************************************
 *
 * @file paspc_task.h
 *
 * @brief Header file - Phone Alert Status Profile Client Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _PASPC_TASK_H_
#define _PASPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup PASPCTASK Phone Alert Status Profile Client Task
 * @ingroup PASPC
 * @brief Phone Alert Status Profile Client Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "pasp_common.h"

#if (BLE_PAS_CLIENT)

#include "ke_task.h"
#include "gattc_task.h"
#include "paspc.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Phone Alert Status Client task instances
#define PASPC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the PASPC task
enum paspc_states
{
    /// Idle state
    PASPC_IDLE,
    /// Connected state
    PASPC_CONNECTED,
    /// Busy state
    PASPC_BUSY,

    /// Number of defined states.
    PASPC_STATE_MAX
};

/// Message IDs
enum paspc_msg_ids
{
    /// Enable the Phone Alert Status Profile Client task - at connection
    PASPC_ENABLE_CMD = KE_FIRST_MSG(TASK_PASPC),

    /// Disable Indication
    PASPC_DISABLE_IND,

    /// Read the value of an attribute in the peer device database
    PASPC_READ_CMD,
    /// Write the value of an attribute in the peer device database
    PASPC_WRITE_CMD,

    /// Indicate that an attribute value has been received either upon notification or read response
    PASPC_VALUE_IND,
    /// Indicate the content of the pointer device Phone Alert Status service
    PASPC_PASS_CONTENT_IND,

    /// Complete Event Information
    PASPC_CMP_EVT,
};

/// Operation Codes
enum paspc_op_codes
{
    /// Reserved operation code
    PASPC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    PASPC_ENABLE_OP_CODE,
    /// Read attribute value Procedure
    PASPC_READ_OP_CODE,
    /// Write attribute value Procedure
    PASPC_WRITE_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref PASPC_ENABLE_CMD message
struct paspc_enable_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t con_type;
    /// Existing handle values PASS
    struct paspc_pass_content pass;
};

/// Parameters of the @ref PASPC_READ_CMD message
struct paspc_read_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref PASPC_WRITE_CMD message
struct paspc_write_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Write code
    uint8_t write_code;
    /// Value
    union write_value_tag
    {
        /// Ringer Control Point characteristic value
        uint8_t ringer_ctnl_pt;
        /// Alert Status characteristic client characteristic configuration value
        uint16_t alert_status_ntf_cfg;
        /// Ringer Setting characteristic client characteristic configuration value
        uint16_t ringer_setting_ntf_cfg;
    } value;
};

/// Parameters of the @ref PASPC_VALUE_IND message
struct paspc_value_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union value_tag
    {
        /// Alert Status characteristic value
        uint8_t alert_status;
        /// Ringer Setting characteristic value
        uint8_t ringer_setting;
        /// Alert Status characteristic client characteristic configuration value
        uint16_t alert_status_ntf_cfg;
        /// Ringer Setting characteristic client characteristic configuration value
        uint16_t ringer_setting_ntf_cfg;
    } value;
};

/// Parameters of the @ref PASPC_PASS_CONTENT_IND message
struct paspc_pass_content_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// Phone Alert Status Service Content
    struct paspc_pass_content pass;
};

/// Parameters of the @ref PASPC_CMP_EVT message
struct paspc_cmp_evt
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

extern const struct ke_state_handler paspc_state_handler[PASPC_STATE_MAX];
extern const struct ke_state_handler paspc_default_handler;
extern ke_state_t paspc_state[PASPC_IDX_MAX];

#endif //(BLE_PAS_CLIENT)

/// @} PASPCTASK

#endif //(_PASPC_TASK_H_)
