/**
 ****************************************************************************************
 *
 * @file proxr_task.h
 *
 * @brief Header file - PROXRTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef PROXR_TASK_H_
#define PROXR_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup PROXRTASK Proximity Reporter Task
 * @ingroup PROXR
 * @brief Proximity Reporter Task
 *
 * The PROXRTASK is responsible for handling the APi messages from the Application or internal
 * tasks.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_PROX_REPORTER)

#include <stdint.h>
#include "ke_task.h"
#include "proxr.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Proximity Reporter task instances
#define PROXR_IDX_MAX                 (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the PROXR task
enum
{
    /// Disabled State
    PROXR_DISABLED,
    /// Idle state
    PROXR_IDLE,
    /// Connected state
    PROXR_CONNECTED,

    /// Number of defined states.
    PROXR_STATE_MAX
};

/// PROXR_CREATE_DB_REQ - Bit values
enum
{
    PROXR_IAS_TXPS_NOT_SUP,
    PROXR_IAS_TXPS_SUP,
};

/// Messages for Proximity Reporter
enum
{
    /// Start the proximity reporter
    PROXR_ENABLE_REQ = KE_FIRST_MSG(TASK_PROXR),

    /// Disable confirm will allow sending the APP the current LLS alert level to keep
    PROXR_DISABLE_IND,

    /// LLS Alert Indication
    PROXR_LLS_ALERT_IND,

    ///Add a LLS instance and optionally a TXPS instance into the database
    PROXR_CREATE_DB_REQ,
    ///Inform APP of database creation status
    PROXR_CREATE_DB_CFM,

    /// Error Indication
    PROXR_ERROR_IND,

    /// LLS/IAS level update indication
    PROXR_LEVEL_UPD_IND,
};


/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref PROXR_ENABLE_REQ message
struct proxr_enable_req
{
    /// Connection Handle
    uint16_t conhdl;
    /// Security level
    uint8_t sec_lvl;

    /// Saved LLS alert level to set in ATT DB
    uint8_t lls_alert_lvl;
    /// TX Power level
    int8_t txp_lvl;
};

/// Parameters of the @ref PROXR_DISABLE_IND message
struct proxr_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// LLS alert level to save in APP
    uint8_t  lls_alert_lvl;
};

/// Parameters of the @ref PROXR_LLS_ALERT_IND message
struct proxr_lls_alert_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Alert level
    uint8_t alert_lvl;
};

/// Parameters of the @ref PROXR_LEVEL_UPD_IND message
struct proxr_level_upd_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Alert level
    uint8_t alert_lvl;
    /// Char Code - Indicate if IAS or LLS
    uint8_t char_code;
};

/// Parameters of the @ref PROXR_CREATE_DB_REQ message
struct proxr_create_db_req
{
    /// Indicate if TXPS is supported or not
    uint8_t features;
};

/// Parameters of the @ref PROXR_CREATE_DB_CFM message
struct proxr_create_db_cfm
{
    /// Status
    uint8_t status;
};

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler proxr_state_handler[PROXR_STATE_MAX];
extern const struct ke_state_handler proxr_default_handler;
extern ke_state_t proxr_state[PROXR_IDX_MAX];

#endif //BLE_PROX_REPORTER

/// @} PROXRTASK

#endif // PROXR_TASK_H_
