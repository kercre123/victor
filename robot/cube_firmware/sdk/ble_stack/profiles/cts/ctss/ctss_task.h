/**
 ****************************************************************************************
 *
 * @file ctss_task.h
 *
 * @brief Header file - Current Time Service.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef CTSS_TASK_H_
#define CTSS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup CTSSTASK Task
 * @ingroup CTSS
 * @brief Time Profile Server Task
 *
 * The CTSSTASK is responsible for handling the messages coming in and out of the
 * @ref CTS reporter block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_CTS_SERVER)
#include "ke_task.h"
#include "ctss.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Time Server task instances
#define CTSS_IDX_MAX     (1)

/// Possible states of the CTSS task
enum
{
    /// disabled state
    CTSS_DISABLED,
    /// idle state
    CTSS_IDLE,
    /// connected state
    CTSS_CONNECTED,

    /// Number of defined states.
    CTSS_STATE_MAX
};

/// Messages for Time Profile Server
enum
{
    /// Start the Time Profile Server Role - at connection
    CTSS_ENABLE_REQ = KE_FIRST_MSG(TASK_CTSS),

    /// Disable confirmation with configuration to save after profile disabled
    CTSS_DISABLE_IND,

    /// Error indication to Host
    CTSS_ERROR_IND,

    /// Update Current Time Request from APP
    CTSS_UPD_CURR_TIME_REQ,
    /// Update Local Time Information Request from APP
    CTSS_UPD_LOCAL_TIME_INFO_REQ,
    /// Update Reference Time Information Request from APP
    CTSS_UPD_REF_TIME_INFO_REQ,
    /// Inform APP about modification of Current Time Characteristic Client. Charact. Cfg
    CTSS_CURRENT_TIME_CCC_IND,

    /// Database Creation
    CTSS_CREATE_DB_REQ,
    /// Database Creation Confirmation
    CTSS_CREATE_DB_CFM,

    /// Inform the application about the task creation result
    CTSS_ENABLE_CFM,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref CTSS_ENABLE_REQ message
struct ctss_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized; b1 or b2 and b3 can go together
    uint8_t sec_lvl;
    /// Type of connection - will someday depend on button press length; can be CFG or NORMAL
    uint8_t con_type;

    /// Current Time notification configuration
    uint16_t current_time_ntf_en;
};

/// Parameters of the @ref CTSS_ENABLE_CFM message
struct ctss_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
};

/// Parameters of the @ref CTSS_DISABLE_IND message
struct ctss_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;

    /// Current Time notification configuration
    uint16_t ntf_en;
};

/// Parameters of the @ref CTSS_CREATE_DB_REQ message
struct ctss_create_db_req
{
    /// Database configuration
    uint8_t features;
};

/// Parameters of the @ref CTSS_CREATE_DB_CFM message
struct ctss_create_db_cfm
{
    /// Status
    uint8_t status;
};

/// Parameters of the @ref CTSS_UPD_CURR_TIME_REQ message
struct ctss_upd_curr_time_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Current Time
    struct cts_curr_time current_time;
    /**
     * Indicate if the new Current Time value can be sent if the current_time_ntf_en parameter is
     * set to 1.
     * (0 - Disable; 1- Enable)
     *
     * If the time of the Current Time Server is changed because of reference time update (adjust reason)
     * then no notifications shall be sent to the Current Time Service Client within the 15 minutes from
     * the last notification, unless one of both of the two statements below are true :
     *         - The new time information differs by more than 1min from the Current Time Server
     *         time previous to the update
     *         - The update was caused by the client (interacting with another service)
     */
    uint8_t enable_ntf_send;
};

/// Parameters of the @ref CTSS_UPD_LOCAL_TIME_INFO_REQ message
struct ctss_upd_local_time_info_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Local Time Information
    struct cts_loc_time_info loc_time_info;
};

/// Parameters of the @ref CTSS_UPD_REF_TIME_INFO_REQ message
struct ctss_upd_ref_time_info_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Reference Time Information
    struct cts_ref_time_info ref_time_info;
};

/// Parameters of the @ref CTSS_CURRENT_TIME_CCC_IND message
struct ctss_current_time_ccc_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Configuration Value
    uint16_t cfg_val;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler ctss_state_handler[CTSS_STATE_MAX];
extern const struct ke_state_handler ctss_default_handler;
extern ke_state_t ctss_state[CTSS_IDX_MAX];

#endif //BLE_CTS_SERVER

/// @} CTSSTASK
#endif // CTSS_TASK_H_
