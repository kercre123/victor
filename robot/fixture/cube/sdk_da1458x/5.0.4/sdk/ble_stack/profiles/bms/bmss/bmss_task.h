/**
 ****************************************************************************************
 *
 * @file bmss_task.h
 *
 * @brief Header file - Bond Management Service Server Role Task Declaration.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _BMSS_TASK_H_
#define _BMSS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BMSSTASK Task
 * @ingroup BMSS
 * @brief Bond Management 'Profile' Task.
 *
 * The BMSS_TASK is responsible for handling the messages coming in and out of the
 * @ref BMSS block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
*/

#if (BLE_BM_SERVER)
#include "ke_task.h"
#include "bmss.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Bond Management Server task instances
#define BMSS_IDX_MAX                                (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the BMSS task
enum
{
    /// Disabled State
    BMSS_DISABLED,
    /// Idle state
    BMSS_IDLE,
    /// Connected state
    BMSS_CONNECTED,

    /// Number of defined states
    BMSS_STATE_MAX,
};

/// Messages for Bond Management Server
enum
{
    /// Add a BMSS instance into the database
    BMSS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_BMSS),
    /// Inform APP of database creation status
    BMSS_CREATE_DB_CFM,

    /// Start the Bond Management Server - at connection
    BMSS_ENABLE_REQ,

    /// Disable indication
    BMSS_DISABLE_IND,
    /// Error Indication
    BMSS_ERROR_IND,

    /// Delete Bond Request Indication
    BMSS_DEL_BOND_REQ_IND,
    /// Delete Bond Write Response
    BMSS_DEL_BOND_CFM,
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref BMSS_CREATE_DB_REQ message

struct bmss_create_db_req
{
    /// Features of BMS instance
    uint32_t features;

    /// Extended properties - reliable writes (supported or not)
    uint8_t reliable_writes;
};

/// Parameters of the @ref BMSS_CREATE_DB_CFM message
struct bmss_create_db_cfm
{
    /// Status
    uint8_t status;
};

/// Parameters of the @ref BMSS_ENABLE_REQ message
struct bmss_enable_req
{
    /// Connection Handle
    uint16_t conhdl;

    /// Security level
    uint8_t sec_lvl;
};

/// Parameters of the @ref BMSS_DISABLE_IND message
struct bmss_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
};

/// Parameters of the @ref BMSS_DEL_BOND_REQ_IND message - dynamically allocated
struct bmss_del_bond_req_ind
{
    /// Operation code and operand
    struct bms_ctrl_point_op operation;
};

/// Parameters of the @ref BMSS_DEL_BOND_CFM message
struct bmss_del_bond_cfm
{
    /// Bond deletion result status
    uint8_t status;
};

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler bmss_state_handler[BMSS_STATE_MAX];
extern const struct ke_state_handler bmss_default_handler;
extern ke_state_t bmss_state[BMSS_IDX_MAX];

#endif // BLE_BM_SERVER

/// @} BMSSTASK

#endif // _BMSS_TASK_H_
