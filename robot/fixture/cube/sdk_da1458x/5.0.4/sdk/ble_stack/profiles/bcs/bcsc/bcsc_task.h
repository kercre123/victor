 /**
 ****************************************************************************************
 *
 * @file bcsc_task.h
 *
 * @brief Header file - Body Composition Service Client Role Task.
 *
 * Copyright (C) 2015 Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */


#ifndef _BCSC_TASK_H_
#define _BCSC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BCSCTASK Task
 * @ingroup BCSC
 * @brief Body Composition Service Client Task.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BCS_CLIENT)

#include "ke_task.h"
#include "bcsc.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Body Composition task instances
#define BCSC_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the BCSC task
enum
{
    /// Idle state
    BCSC_IDLE,
    /// Connected state
    BCSC_CONNECTED,
    /// Discovering Body Composition Svc and Chars
    BCSC_DISCOVERING,

    /// Number of defined states.
    BCSC_STATE_MAX,
};

/// Messages for Body Composition Service Client
enum
{
    /// Start the Body Composition Service Client - at connection
    BCSC_ENABLE_REQ = KE_FIRST_MSG(TASK_BCSC),
    /// Notify the APP that cfg connection has finished with discovery results, or that normal cnx started
    BCSC_ENABLE_CFM,

    /// Disable confirmation with configuration to save after profile disabled
    BCSC_DISABLE_IND,
    /// Error indication to the APP
    BCSC_ERROR_IND,

    /// Message to read a BCS Feature Char. value
    BCSC_READ_FEATURES_REQ,
    /// Message for BCS Feature Char. value read response
    BCSC_READ_FEATURES_CFM,

    /// Message for configuring the BCS Measurement Char. indication
    BCSC_REGISTER_REQ,
    /// Message for indication configuration response
    BCSC_REGISTER_CFM,

    /// Body Composition Measurement Value Indication send to the APP
    BCSC_MEAS_VAL_IND,
};

/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters of the @ref BCSC_ENABLE_REQ message
struct bcsc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Type of connection
    uint8_t con_type;

    /// Existing handle values
    struct bcs_content bcs;
};

/// Parameters of the @ref BCSC_ENABLE_CFM message
struct bcsc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;

    /// Existing handle values
    struct bcs_content bcs;
};

/// Parameters of the @ref BCSC_ERROR_IND message
struct bcsc_error_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
};

/// Parameters of the @ref BCSC_READ_FEATURES_REQ message
struct bcsc_read_features_req
{
    /// Connection handle
    uint16_t conhdl;
};

/// Parameters of the @ref BCSC_READ_FEATURES_CFM message
struct bcsc_read_features_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Peer features
    uint32_t features;
    /// Status
    uint8_t status;
};

/// Parameters of the @ref BCSC_REGISTER_REQ message
struct bcsc_register_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

/// Parameters of the @ref BCSC_REGISTER_CFM message
struct bcsc_register_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t  status;
};

/// Parameters of the @ref BCSC_MEAS_VAL_IND message
struct bcsc_meas_val_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Body Composition measurement value
    bcs_meas_t meas;
    /// Flags indicating valid fields of the measurement value
    uint16_t flags;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler bcsc_state_handler[BCSC_STATE_MAX];
extern const struct ke_state_handler bcsc_default_handler;
extern ke_state_t bcsc_state[BCSC_IDX_MAX];

#endif // BLE_BCS_CLIENT

/// @} BCSCTASK

#endif /* _BCSC_TASK_H_ */
