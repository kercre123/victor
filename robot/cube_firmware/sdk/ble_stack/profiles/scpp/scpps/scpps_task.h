/**
 ****************************************************************************************
 *
 * @file scpps_task.h
 *
 * @brief Header file - Scan Parameters Profile Server Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _SCPPS_TASK_H_
#define _SCPPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup SCPPSTASK Task
 * @ingroup SCPPS
 * @brief Scan Parameters Profile Task.
 *
 * The SCPPS_TASK is responsible for handling the messages coming in and out of the
 * @ref SCPPS block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_SP_SERVER)

#include <stdint.h>
#include "ke_task.h"
#include "scpps.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Scan Parameters Device task instances
#define SCPPS_IDX_MAX     0x01

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the SCPPS task
enum
{
    /// Disabled state
    SCPPS_DISABLED,
    /// Idle state
    SCPPS_IDLE,
    /// Connected state
    SCPPS_CONNECTED,

    /// Number of defined states.
    SCPPS_STATE_MAX
};

/// Messages for Scan Parameters Profile Server
enum
{
    /// Add a SCPS instance into the database
    SCPPS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_SCPPS),
    /// Inform APP of database creation status
    SCPPS_CREATE_DB_CFM,

    /// Start the Scan Parameters Profile Server - at connection
    SCPPS_ENABLE_REQ,
    /// Disable confirmation with configuration to save after profile disabled
    SCPPS_DISABLE_IND,
    /// Error indication to Host
    SCPPS_ERROR_IND,

    /// Indicate to APP that the Scan Interval Window value has been written by Client
    SCPPS_SCAN_INTV_WD_IND,
    /// Indicate to APP that the Scan Refresh Notification Configuration has been changed
    SCPPS_SCAN_REFRESH_NTF_CFG_IND,

    /// Notify client about new Scan Refresh Value
    SCPPS_SCAN_REFRESH_SEND_REQ,
    /// Inform APP if Scan Refresh Value has been notified
    SCPPS_SCAN_REFRESH_SEND_CFM,
};

/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters of the @ref SCPPS_CREATE_DB_REQ message
struct scpps_create_db_req
{
    /// Optional features to add
    uint8_t features;
};

/// Parameters of the @ref SCPPS_CREATE_DB_CFM message
struct scpps_create_db_cfm
{
    /// Status about DB creation
    uint8_t status;
};

/// Parameters of the @ref SCPPS_ENABLE_REQ message
struct scpps_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Security level
    uint8_t sec_lvl;
    /// Type of connection - can be CFG or DISCOVERY
    uint8_t con_type;

    /// Scan Refresh Notification Configurations
    uint16_t scan_refresh_ntf_en;
};

///Parameters of the @ref SCPPS_DISABLE_IND message
struct scpps_disable_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Scan Refresh Notification configuration
    uint16_t scan_refresh_ntf_en;
};

///Parameters of the @ref SCPPS_SCAN_INTV_WD_IND message
struct scpps_scan_intv_wd_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Scan Interval Window
    struct scan_intv_wd scan_intv_wd;
};

///Parameters of the @ref SCPPS_SCAN_REFRESH_NTF_CFG_IND message
struct scpps_scan_refresh_ntf_cfg_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Scan Refresh Notification Configuration
    uint16_t scan_refresh_ntf_en;
};

///Parameters of the @ref SCPPS_SCAN_REFRESH_SEND_REQ message
struct scpps_scan_refresh_send_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Scan Refresh Notification Configuration
    uint8_t scan_refresh;
};

///Parameters of the @ref SCPPS_SCAN_REFRESH_SEND_CFM message
struct scpps_scan_refresh_send_cfm
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler scpps_state_handler[SCPPS_STATE_MAX];
extern const struct ke_state_handler scpps_default_handler;
extern ke_state_t scpps_state[SCPPS_IDX_MAX];

#endif // BLE_SP_SERVER

/// @} SCPPSTASK

#endif /* _SCPPS_TASK_H_ */
