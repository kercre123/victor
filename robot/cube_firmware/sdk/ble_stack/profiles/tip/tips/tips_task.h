/**
 ****************************************************************************************
 *
 * @file tips_task.h
 *
 * @brief Header file - TIPSTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef TIPS_TASK_H_
#define TIPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup TIPSTASK Task
 * @ingroup TIPS
 * @brief Time Profile Server Task
 *
 * The TIPSTASK is responsible for handling the messages coming in and out of the
 * @ref TIPS reporter block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_TIP_SERVER)
#include "ke_task.h"
#include "tips.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Time Server task instances
#define TIPS_IDX_MAX     (BLE_CONNECTION_MAX)

/// Possible states of the TIPS task
enum
{
    /// disabled state
    TIPS_DISABLED,
    /// idle state
    TIPS_IDLE,
    /// connected state
    TIPS_CONNECTED,

    /// Number of defined states.
    TIPS_STATE_MAX
};

/// Messages for Time Profile Server
enum
{
    ///Start the Time Profile Server Role - at connection
    TIPS_ENABLE_REQ = KE_FIRST_MSG(TASK_TIPS),

    ///Disable confirmation with configuration to save after profile disabled
    TIPS_DISABLE_IND,

    ///Error indication to Host
    TIPS_ERROR_IND,

    ///Update Current Time Request from APP
    TIPS_UPD_CURR_TIME_REQ,
    ///Update Local Time Information Request from APP
    TIPS_UPD_LOCAL_TIME_INFO_REQ,
    ///Update Reference Time Information Request from APP
    TIPS_UPD_REF_TIME_INFO_REQ,
    ///Update Time With DST Request from APP
    TIPS_UPD_TIME_DST_REQ,
    ///Inform APP about modification of Current Time Characteristic Client. Charact. Cfg
    TIPS_CURRENT_TIME_CCC_IND,
    ///Update Time Update State Request from APP
    TIPS_UPD_TIME_UPD_STATE_REQ,
    ///Inform APP about modification of Time Update Control Point Characteristic Value
    TIPS_TIME_UPD_CTNL_PT_IND,

    ///Database Creation
    TIPS_CREATE_DB_REQ,
    ///Database Creation Confirmation
    TIPS_CREATE_DB_CFM,

    ///Inform the application about the task creation result
    TIPS_ENABLE_CFM,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref TIPS_ENABLE_REQ message
struct tips_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized; b1 or b2 and b3 can go together
    uint8_t sec_lvl;
    ///Type of connection - will someday depend on button press length; can be CFG or NORMAL
    uint8_t con_type;

    ///Current Time notification configuration
    uint16_t current_time_ntf_en;
};

/// Parameters of the @ref TIPS_ENABLE_CFM message
struct tips_enable_cfm
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

///Parameters of the @ref TIPS_DISABLE_IND message
struct tips_disable_ind
{
    ///COnnection Handle
    uint16_t conhdl;

    ///Current Time notification configuration
    uint16_t current_time_ntf_en;
};

///Parameters of the @ref TIPS_UPD_CURR_TIME_REQ message
struct tips_upd_curr_time_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Current Time
    struct tip_curr_time current_time;
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

///Parameters of the @ref TIPS_UPD_LOCAL_TIME_INFO_REQ message
struct tips_upd_local_time_info_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Local Time Information
    struct tip_loc_time_info loc_time_info;
};

///Parameters of the @ref TIPS_UPD_REF_TIME_INFO_REQ message
struct tips_upd_ref_time_info_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Reference Time Information
    struct tip_ref_time_info ref_time_info;
};

///Parameters of the @ref TIPS_UPD_TIME_DST_REQ message
struct tips_upd_time_dst_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Time With DST
    struct tip_time_with_dst time_with_dst;
};

///Parameters of the @ref TIPS_CURRENT_TIME_CCC_IND message
struct tips_current_time_ccc_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Configuration Value
    uint16_t cfg_val;
};

///Parameters of the @ref TIPS_UPD_TIME_UPD_STATE_REQ message
struct tips_upd_time_upd_state_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Time Update State
    struct tip_time_upd_state time_upd_state;
};

///Parameters of the @ref TIPS_TIME_UPD_CTNL_PT_IND message
struct tips_time_upd_ctnl_pt_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Time Update Control Point Value
    tip_time_upd_contr_pt value;
};

///Parameters of the @ref TIPS_CREATE_DB_REQ message
struct tips_create_db_req
{
    ///Database configuration
    uint8_t features;
};

///Parameters of the @ref TIPS_CREATE_DB_CFM message
struct tips_create_db_cfm
{
    ///Status
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler tips_state_handler[TIPS_STATE_MAX];
extern const struct ke_state_handler tips_default_handler;
extern ke_state_t tips_state[TIPS_IDX_MAX];

#endif //BLE_TIP_SERVER

/// @} TIPSTASK
#endif // TIPS_TASK_H_
