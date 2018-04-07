 /**
 ****************************************************************************************
 *
 * @file bcss_task.h
 *
 * @brief Header file - Body Composition Service Server Role Task.
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


#ifndef _BCSS_TASK_H_
#define _BCSS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BCSSTASK Task
 * @ingroup BCSS
 * @brief Body Composition Service Task.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BCS_SERVER)

#include "ke_task.h"
#include "bcss.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Body Composition task instances
#define BCSS_IDX_MAX    (1)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the BCSS task
enum
{
    /// Disabled state
    BCSS_DISABLED,
    /// Idle state
    BCSS_IDLE,
    /// Connected state
    BCSS_CONNECTED,

    /// Number of defined states.
    BCSS_STATE_MAX
};

/// Messages for Body Composition Service Server
enum
{
    /// Add a BCSS instance into the database
    BCSS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_BCSS),
    /// Inform APP of database creation status
    BCSS_CREATE_DB_CFM,

    /// Start the Body Composition Service Server - at connection
    BCSS_ENABLE_REQ,

    /// Disable confirmation with configuration to save after profile disabled
    BCSS_DISABLE_IND,
    /// Error indication to Host
    BCSS_ERROR_IND,

    /// Body Composition Measurement Value Indication Request
    BCSS_MEAS_VAL_IND_REQ,
    /// Body Composition Measurement Value Indication Confirmation
    BCSS_MEAS_VAL_IND_CFM,

    /// Inform APP the if Notification Configuration has been changed
    BCSS_MEAS_VAL_IND_CFG_IND,
};

/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters of the @ref BCSS_CREATE_DB_REQ message
struct bcss_create_db_req
{
    /// Type of service to instantiate (primary or secondary)
    uint8_t is_primary;
};

/// Parameters of the @ref BCSS_CREATE_DB_CFM message
struct bcss_create_db_cfm
{
    /// Status
    uint8_t status;
    /// Start Handle that can be used when including BCS in another profile
    uint16_t shdl;
    /// End Handle that can be used when including BCS in another profile
    uint16_t ehdl;
};

/// Parameters of the @ref BCSS_ENABLE_REQ message
struct bcss_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// Security Level
    uint8_t sec_lvl;
    /// Type of connection
    uint8_t con_type;

    /// Body Composition Supported Features flag
    uint32_t features;
    /// Body Composition Indication configuration
    uint16_t meas_ind_en;
};

///Parameters of the @ref BCSS_DISABLE_IND message
struct bcss_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;

    /// Body Composition Indication configuration
    uint16_t meas_ind_en;
};

///Parameters of the @ref BCSS_MEAS_VAL_IND_REQ message
struct bcss_meas_val_ind_req
{
    /// Connection handle
    uint16_t conhdl;

    /// Measurement value
    bcs_meas_t meas;
};

///Parameters of the @ref BCSS_MEAS_VAL_IND_CFM message
struct bcss_meas_val_ind_cfm
{
    /// Connection handle
    uint16_t conhdl;

    /// Status
    uint8_t status;
};

///Parameters of the @ref BCSS_MEAS_VAL_IND_CFG_IND message
struct bcss_meas_val_ind_cfg_ind
{
    /// Connection handle
    uint16_t conhdl;

    /// Notification Configuration
    uint16_t ind_cfg;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler bcss_state_handler[BCSS_STATE_MAX];
extern const struct ke_state_handler bcss_default_handler;
extern ke_state_t bcss_state[BCSS_IDX_MAX];

#endif // BLE_BCS_SERVER

/// @} BCSSTASK

#endif /* _BCSS_TASK_H_ */
