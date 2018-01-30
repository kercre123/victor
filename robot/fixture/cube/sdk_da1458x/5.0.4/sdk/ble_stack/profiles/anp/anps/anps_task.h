/**
 ****************************************************************************************
 *
 * @file anps_task.h
 *
 * @brief Header file - ANPSTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev: $
 *
 ****************************************************************************************
 */

#ifndef ANPS_TASK_H_
#define ANPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup ANPSTASK Task
 * @ingroup ANPS
 * @brief Alert Notification Profile Server Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_AN_SERVER)

#include "ke_task.h"
#include "anps.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximal number of Alert Notification Server task instances
#define ANPS_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the ANPS task
enum anps_states
{
    /// Disabled state
    ANPS_DISABLED,
    /// Idle state
    ANPS_IDLE,
    /// Connected state
    ANPS_CONNECTED,
    /// Busy state
    ANPS_BUSY,

    /// Number of defined states.
    ANPS_STATE_MAX
};

/// Messages for Alert Notification Profile Server
enum anps_msg_ids
{
    /// Database Creation
    ANPS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_ANPS),

    /// Start the Alert Notification Profile Server Role
    ANPS_ENABLE_REQ,
    /// Disable confirmation with configuration to save after profile disabled
    ANPS_DISABLE_IND,

    /// Update the value of a characteristic
    ANPS_NTF_ALERT_CMD,
    /// The peer device requests to be notified about new alert values
    ANPS_NTF_IMMEDIATE_REQ_IND,
    /// Indicate that the notification configuration has been modified by the peer device
    ANPS_NTF_STATUS_UPDATE_IND,

    /// Complete Event Information
    ANPS_CMP_EVT,
};

/// Operation Codes
enum anps_op_codes
{
    ANPS_RESERVED_OP_CODE              = 0x00,

    /// Database Creation Procedure
    ANPS_CREATE_DB_OP_CODE,
    /// Enable Procedure
    ANPS_ENABLE_OP_CODE,
    /// Update New Alert Char. value
    ANPS_UPD_NEW_ALERT_OP_CODE,
    /// Update Unread Alert Status Char. value
    ANPS_UPD_UNREAD_ALERT_STATUS_OP_CODE,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref ANPS_CREATE_DB_REQ message
struct anps_create_db_req
{
    /// Supported New Alert Category Characteristic Value - Shall not be 0
    struct anp_cat_id_bit_mask supp_new_alert_cat;
    /// Supported Unread Alert Category Characteristic Value - Can be 0
    struct anp_cat_id_bit_mask supp_unread_alert_cat;
};

/// Parameters of the @ref ANPS_ENABLE_REQ message
struct anps_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Security Level
    uint8_t sec_lvl;
    /// Type of connection
    uint8_t con_type;

    /// New Alert Characteristic - Saved Client Characteristic Configuration Descriptor Value
    uint16_t new_alert_ntf_cfg;
    /// Unread Alert Status Characteristic - Saved Client Characteristic Configuration Descriptor Value
    uint16_t unread_alert_status_ntf_cfg;
};

///Parameters of the @ref ANPS_DISABLE_IND message
struct anps_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// New Alert Characteristic - Current Client Characteristic Configuration Descriptor Value
    uint16_t new_alert_ntf_cfg;
    /// Unread Alert Status Characteristic - Current Client Characteristic Configuration Descriptor Value
    uint16_t unread_alert_status_ntf_cfg;
};

///Parameters of the @ref ANPS_NTF_ALERT_CMD message
struct anps_ntf_alert_cmd
{
    /// Connection Handle
    uint16_t conhdl;
    /// Operation Code (ANPS_UPD_NEW_ALERT_OP_CODE or ANPS_UPD_UNREAD_ALERT_STATUS_OP_CODE)
    uint8_t operation;
    /// New Alert Characteristic Value or Unread Alert Status Characteristic Value
    union anps_value_tag
    {
        /// New Alert
        struct anp_new_alert new_alert;
        /// Unread Alert Status
        struct anp_unread_alert unread_alert_status;
    } value;
};

///Parameters of the @ref ANPS_NTF_IMMEDIATE_REQ_IND message
struct anps_ntf_immediate_req_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// Alert type (New Alert or Unread Alert Status)
    uint8_t alert_type;
    /// Status for each category
    struct anp_cat_id_bit_mask cat_ntf_cfg;
};

///Parameters of the @ref ANPS_NTF_STATUS_UPDATE_IND message
struct anps_ntf_status_update_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// Alert type (New Alert or Unread Alert Status)
    uint8_t alert_type;
    /// Client Characteristic Configuration Descriptor Status
    uint16_t ntf_ccc_cfg;
    /// Status for each category
    struct anp_cat_id_bit_mask cat_ntf_cfg;
};

///Parameters of the @ref ANPS_CMP_EVT message
struct anps_cmp_evt
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

extern const struct ke_state_handler anps_state_handler[ANPS_STATE_MAX];
extern const struct ke_state_handler anps_default_handler;
extern ke_state_t anps_state[ANPS_IDX_MAX];

#endif //(BLE_AN_SERVER)

/// @} ANPSTASK

#endif //(ANPS_TASK_H_)
