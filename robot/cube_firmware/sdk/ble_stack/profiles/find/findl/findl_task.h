/**
 ****************************************************************************************
 *
 * @file findl_task.h
 *
 * @brief Header file - FINDLTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef FINDL_TASK_H_
#define FINDL_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup FINDLTASK Find Me Locator Task
 * @ingroup FINDL
 * @brief Find Me Locator Task
 *
 * The FINDLTASK is responsible for handling the API messages coming from the application
 * or internal tasks.
 *
 * @{
 ****************************************************************************************
 */

#if (BLE_FINDME_LOCATOR)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "ke_task.h"
#include "co_error.h"
#include "findl.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Find ME Locator task instances
#define FINDL_IDX_MAX    (BLE_CONNECTION_MAX)

/// Possible states of the FINDL task
enum
{
    /// Idle state
    FINDL_IDLE,
    /// Discovering state
    FINDL_DISCOVERING,
    /// Connected state
    FINDL_CONNECTED,

    /// Number of defined states.
    FINDL_STATE_MAX
};

///Find Me Locator API messages
enum
{
    /// Start the find me locator profile - at connection
    FINDL_ENABLE_REQ = KE_FIRST_MSG(TASK_FINDL),
    /// Enable confirm message, containing IAS attribute details if discovery conneciton type
    FINDL_ENABLE_CFM,

    /// Inform APP that the profile client role has been disabled after a disconnection
    FINDL_DISABLE_IND,

    ///Error indication
    FINDL_ERROR_IND,
    /// Alert level set request
    FINDL_SET_ALERT_REQ,
};

/// Parameters of the @ref FINDL_ENABLE_REQ message
struct findl_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    ///Connection type
    uint8_t con_type;
    ///Discovered IAS details if any
    struct ias_content ias;
};

///Parameters of the @ref FINDL_ERROR_IND message
struct findl_error_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t  status;
};

/// Parameters of the @ref FINDL_ENABLE_CFM message
struct findl_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
    /// IAs attribute content
    struct ias_content ias;
};

/// Parameters of the @ref FINDL_SET_ALERT_REQ message
struct findl_set_alert_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Alert level
    uint8_t  alert_lvl;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler findl_state_handler[FINDL_STATE_MAX];
extern const struct ke_state_handler findl_default_handler;
extern ke_state_t findl_state[FINDL_IDX_MAX];

#endif //BLE_FINDME_LOCATOR

/// @} FINDLTASK

#endif // FINDL_TASK_H_
