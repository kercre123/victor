/**
 ****************************************************************************************
 *
 * @file findt_task.h
 *
 * @brief Header file - FINDTTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef FINDT_TASK_H_
#define FINDT_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup FINDTTASK Find Me Target Task
 * @ingroup FINDT
 * @brief Find Me Target Task
 *
 * The FINDTTASK is responsible for handling the APi messages from the application or
 * internal tasks.
 *
 * @{
 ****************************************************************************************
 */

#if (BLE_FINDME_TARGET)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "ke_task.h"
#include "findt.h"

/*
 * DEFINES
 ****************************************************************************************
 */
///Maximum number of instances of the Find Me Target task
#define FINDT_IDX_MAX     0x01

/// Possible states of the FINDT task
enum
{
    /// Disabled state
    FINDT_DISABLED,
    /// Idle state
    FINDT_IDLE,
    /// Connected state
    FINDT_CONNECTED,

    /// Number of defined states.
    FINDT_STATE_MAX
};

/// Messages for Find Me Target
enum
{
    /// Start the find me target profile - at connection
    FINDT_ENABLE_REQ = KE_FIRST_MSG(TASK_FINDT),

    ///Disable profile role - at disconnection
    FINDT_DISABLE_IND,

    /// Alert indication
    FINDT_ALERT_IND,

    ///Add an IAS instance into the database
    FINDT_CREATE_DB_REQ,
    ///Inform APP of database creation status
    FINDT_CREATE_DB_CFM,

    FINDT_ERROR_IND,
};

/// Parameters of the @ref FINDT_ENABLE_REQ message
struct findt_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized;
    /// b1 or b2 and b3 can go together
    uint8_t sec_lvl;
};

/// Parameters of the @ref FINDT_DISABLE_IND message
struct findt_disable_ind
{
    /// Connection handle
    uint16_t conhdl;
};

/// Parameters of the @ref FINDT_ALERT_IND message
struct findt_alert_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Alert level
    uint8_t alert_lvl;
};

///Parameters of the @ref FINDT_CREATE_DB_REQ message
struct findt_create_db_req
{
    /// Unused and unchecked parameter - Avoid to have an empty structure
    uint8_t unused;
};

///Parameters of the @ref FINDT_CREATE_DB_CFM message
struct findt_create_db_cfm
{
    ///Status
    uint8_t status;
};

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler findt_state_handler[FINDT_STATE_MAX];
extern const struct ke_state_handler findt_default_handler;
extern ke_state_t findt_state[FINDT_IDX_MAX];

#endif //BLE_FINDME_TARGET

/// @} FINDTTASK
#endif // FINDT_TASK_H_
