/**
 ****************************************************************************************
 *
 * @file pasps_task.h
 *
 * @brief Header file - PASPSTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

#ifndef PASPS_TASK_H_
#define PASPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup PASPSTASK Task
 * @ingroup PASPS
 * @brief Phone Alert Status Profile Server Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "pasp_common.h"

#if (BLE_PAS_SERVER)

#include <stdint.h>
#include "ke_task.h"
#include "pasps.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the PASPS task
enum pasps_states
{
    /// Disabled state
    PASPS_DISABLED,
    /// Idle state
    PASPS_IDLE,
    /// Connected state
    PASPS_CONNECTED,
    /// Busy state
    PASPS_BUSY,

    /// Number of defined states.
    PASPS_STATE_MAX
};

/// Messages for Phone Alert Status Profile Server
enum pasps_msg_ids
{
    /// Add a PAS instance into the database
    PASPS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_PASPS),

    /// Start the Phone Alert Status Profile Server profile
    PASPS_ENABLE_REQ,
    /// Disable Indication
    PASPS_DISABLE_IND,

    /// Update the value of a characteristic
    PASPS_UPDATE_CHAR_VAL_CMD,
    /// Indicate that an attribute value has been written
    PASPS_WRITTEN_CHAR_VAL_IND,

    /// Complete Event Information
    PASPS_CMP_EVT,
};

/// Operation Codes
enum pasps_op_codes
{
    PASPS_RESERVED_OP_CODE              = 0x00,

    /// Database Creation Procedure
    PASPS_CREATE_DB_OP_CODE,
    /// Enable Procedure
    PASPS_ENABLE_OP_CODE,
    /// Update Alert Status Char. value
    PASPS_UPD_ALERT_STATUS_OP_CODE,
    /// Update Ringer Setting Char. value
    PASPS_UPD_RINGER_SETTING_OP_CODE,
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref PASPS_CREATE_DB_REQ message
struct pasps_create_db_req
{
    /// Alert Status Char. Value
    uint8_t alert_status;
    /// Ringer Settings Char. Value
    uint8_t ringer_setting;
};

/// Parameters of the @ref PASPS_ENABLE_REQ message
struct pasps_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Security Level
    uint8_t sec_lvl;
    /// Type of connection - can be CFG or NORMAL
    uint8_t con_type;

    /// Alert Status Characteristic Notification Configuration
    uint16_t alert_status_ntf_cfg;
    /// Ringer Setting Characteristic Notification Configuration
    uint16_t ringer_setting_ntf_cfg;
};

///Parameters of the @ref PASPS_DISABLE_IND message
struct pasps_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;

    /// Alert Status Characteristic Notification Configuration
    uint16_t alert_status_ntf_cfg;
    /// Ringer Setting Characteristic Notification Configuration
    uint16_t ringer_setting_ntf_cfg;
};

///Parameters of the @ref PASPS_UPDATE_CHAR_VAL_CMD message
struct pasps_update_char_val_cmd
{
    /// Connection Handle
    uint16_t conhdl;

    /// Operation Code (PASPS_UPD_ALERT_STATUS_OP_CODE or PASPS_UPD_RINGER_SETTING_OP_CODE)
    uint8_t operation;
    /// Alert Status Characteristic Value or Ringer Setting Characteristic Value
    uint8_t value;
};

///Parameters of the @ref PASPS_WRITTEN_CHAR_VAL_IND message
struct pasps_written_char_val_ind
{
    /// Connection Handle
    uint16_t conhdl;

    /// Attribute Code
    uint8_t att_code;
    /// Written value
    union written_value_tag
    {
        /// Ringer Control Point Char. Value
        uint8_t ringer_ctnl_pt;
        /// Alert Status Notification Configuration
        uint16_t alert_status_ntf_cfg;
        /// Ringer Setting Notification Configuration
        uint16_t ringer_setting_ntf_cfg;
    } value;
};

///Parameters of the @ref PASPS_CMP_EVT message
struct pasps_cmp_evt
{
    /// Connection Handle
    uint16_t conhdl;

    /// Operation
    uint8_t operation;
    /// Status
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler pasps_state_handler[PASPS_STATE_MAX];
extern const struct ke_state_handler pasps_default_handler;
extern ke_state_t pasps_state[PASPS_IDX_MAX];

#endif //(BLE_PAS_SERVER)

/// @} PASPSTASK

#endif // PASPS_TASK_H_
