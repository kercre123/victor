 /**
 ****************************************************************************************
 *
 * @file udsc_task.h
 *
 * @brief Header file - User Data Service Client Role Task.
 *
 * Copyright (C) 2015 Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */


#ifndef _UDSC_TASK_H_
#define _UDSC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup UDSCTASK Task
 * @ingroup UDSC
 * @brief User Data Service Client Task.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_UDS_CLIENT)

#include "ke_task.h"
#include "uds_common.h"
#include "udsc.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of User Data task instances
#define UDSC_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the UDSC task
enum
{
    /// Idle state
    UDSC_IDLE,
    /// Connected state
    UDSC_CONNECTED,
    /// Discovering User Data Svc and Chars
    UDSC_DISCOVERING,

    /// Number of defined states.
    UDSC_STATE_MAX,
};

/// Messages for User Data Service Client
enum
{
    /// Start the User Data Service Client - at connection
    UDSC_ENABLE_REQ = KE_FIRST_MSG(TASK_UDSC),
    /// Notify the APP that cfg connection has finished with discovery results, or that normal cnx started
    UDSC_ENABLE_CFM,

    /// Disable confirmation with configuration to save after profile disabled
    UDSC_DISABLE_IND,

    /// Perform certain database operation using User Control Point opcodes
    UDSC_UCP_OP_REQ,
    /// Result of the requested database operation
    UDSC_UCP_OP_CFM,

    /// Message to set certain Characteristic value on the remote device
    UDSC_SET_CHAR_VAL_REQ,
    /// Result of the database write
    UDSC_SET_CHAR_VAL_CFM,

    /// Read database entry
    UDSC_READ_CHAR_VAL_REQ,
    /// Result of database entry read
    UDSC_READ_CHAR_VAL_RSP,

    /// Message for configuring DB Change Incr. or User Control Point descr.
    UDSC_NTFIND_CFG_REQ,
    /// Completeness indication
    UDSC_CMP_EVT_IND,
    /// Notification that server has updated it's database
    UDSC_DB_CHANGE_NTF,

    /// Procedure Timer Timeout
    UDSC_TIMEOUT_TIMER_IND
};

/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters of the @ref UDSC_ENABLE_REQ message
struct udsc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Type of connection
    uint8_t con_type;

    /// Existing handle values
    struct uds_content uds;
};

/// Parameters of the @ref UDSC_ENABLE_CFM message
struct udsc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;

    /// Existing handle values
    struct uds_content uds;
};

/// Parameters of the @ref UDSC_UCP_OP_REQ message
struct udsc_ucp_op_req
{
    /// User control point request
    struct uds_ucp_req req;
};

/// Parameters of the @ref UDSC_UCP_OP_CFM message
struct udsc_ucp_op_cfm
{
    /// User control point response
    struct uds_ucp_rsp cfm;
};

/// Parameters of the @ref UDSC_SET_CHAR_VAL_REQ message - shall be dynamically allocated
struct udsc_set_char_val_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic Code
    uint8_t char_code;
    /// Value length
    uint8_t val_len;
    /// If its the last write and DB Change Increment Value should be changed
    bool finalize;
    /// Value
    uint8_t val[__ARRAY_EMPTY];
};

/// Parameters of the @ref UDSC_SET_CHAR_VAL_CFM message
struct udsc_set_char_val_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
};

/// Parameters of the @ref UDSC_READ_CHAR_VAL_REQ message
struct udsc_read_char_val_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic Code
    uint8_t char_code;
};

/// Parameters of the @ref UDSC_READ_CHAR_VAL_RSP message - shall be dynamically allocated
struct udsc_read_char_val_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic Code
    uint8_t char_code;
    /// Status of request
    uint8_t  status;
    /// Value length
    uint8_t val_len;
    /// Value
    uint8_t val[__ARRAY_EMPTY];
};

/// Parameters of the @ref UDSC_NTFIND_CFG_REQ message
struct udsc_ntfind_cfg_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
    /// Descriptor to configure
    uint8_t desc_code;
};

/// Parameters of the @ref UDSC_CMP_EVT_IND message
struct udsc_cmp_evt_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
};

/// Parameters of the @ref UDSC_DB_CHANGE_NTF message
struct udsc_db_change_ntf
{
    /// Value of the database change increment characteristic
    uint32_t incr_val;
    /// Connection handle
    uint16_t conhdl;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler udsc_state_handler[UDSC_STATE_MAX];
extern const struct ke_state_handler udsc_default_handler;
extern ke_state_t udsc_state[UDSC_IDX_MAX];

#endif // BLE_UDS_CLIENT

/// @} UDSCTASK

#endif /* _UDSC_TASK_H_ */
