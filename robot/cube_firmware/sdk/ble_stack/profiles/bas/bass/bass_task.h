/**
 ****************************************************************************************
 *
 * @file bass_task.h
 *
 * @brief Header file - Battery Service Server Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _BASS_TASK_H_
#define _BASS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BAPSTASK Task
 * @ingroup BAPS
 * @brief Battery 'Profile' Task.
 *
 * The BAPS_TASK is responsible for handling the messages coming in and out of the
 * @ref BAPS block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BAS_SERVER)

#include "ke_task.h"
#include "bass.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Battery Server task instances
#define BASS_IDX_MAX     0x01

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the BASS task
enum
{
    /// Disabled state
    BASS_DISABLED,
    /// Idle state
    BASS_IDLE,
    /// Connected state
    BASS_CONNECTED,

    /// Number of defined states.
    BASS_STATE_MAX
};

/// Messages for Battery Server
enum
{
    /// Add a BAS instance into the database
    BASS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_BASS),
    /// Inform APP of database creation status
    BASS_CREATE_DB_CFM,

    /// Start the Battery Server - at connection
    BASS_ENABLE_REQ,

    /// Disable confirmation with configuration to save after profile disabled
    BASS_DISABLE_IND,
    /// Error indication to Host
    BASS_ERROR_IND,

    /// Battery Level Value Update Request
    BASS_BATT_LEVEL_UPD_REQ,
    /// Inform APP if Battery Level value has been notified or not
    BASS_BATT_LEVEL_UPD_CFM,
    /// Inform APP that Battery Level Notification Configuration has been changed
    BASS_BATT_LEVEL_NTF_CFG_IND,
};

/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters of the @ref BASS_CREATE_DB_REQ message
struct bass_create_db_req
{
    /// Number of BAS to add
    uint8_t bas_nb;
    /// Features of each BAS instance
    uint8_t features[BASS_NB_BAS_INSTANCES_MAX];
};

/// Parameters of the @ref BASS_CREATE_DB_CFM message
struct bass_create_db_cfm
{
    /// Status
    uint8_t status;
};

/// Parameters of the @ref BASS_ENABLE_REQ message
struct bass_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// Security Level
    uint8_t sec_lvl;
    /// Type of connection
    uint8_t con_type;

    /// Battery Level Notification Configurations
    uint16_t batt_level_ntf_cfg[BASS_NB_BAS_INSTANCES_MAX];
    /// Last Battery Level
    uint8_t old_batt_lvl[BASS_NB_BAS_INSTANCES_MAX];
    /// Current Battery Level
    uint8_t current_batt_lvl[BASS_NB_BAS_INSTANCES_MAX];

    /// Battery Level Characteristic Presentation Format - Should not change during connection
    struct prf_char_pres_fmt batt_level_pres_format[BASS_NB_BAS_INSTANCES_MAX];
};

///Parameters of the @ref BASS_DISABLE_IND message
struct bass_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;

    /// Battery Level Notification configuration
    uint16_t batt_level_ntf_cfg[BASS_NB_BAS_INSTANCES_MAX];
    /// Battery Level
    uint8_t batt_lvl[BASS_NB_BAS_INSTANCES_MAX];
};

///Parameters of the @ref BASS_BATT_LEVEL_UPD_REQ message
struct bass_batt_level_upd_req
{
    /// Connection handle
    uint16_t conhdl;
    /// BAS instance
    uint8_t bas_instance;
    /// Battery Level
    uint8_t batt_level;
};

///Parameters of the @ref BAPS_BATT_LEVEL_UPD_CFM message
struct bass_batt_level_upd_cfm
{
    ///Connection handle
    uint16_t conhdl;
    ///status
    uint8_t status;
};

///Parameters of the @ref BASS_BATT_LEVEL_NTF_CFG_IND message
struct bass_batt_level_ntf_cfg_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Notification Configuration
    uint16_t ntf_cfg;
    /// BAS instance
    uint8_t bas_instance;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler bass_state_handler[BASS_STATE_MAX];
extern const struct ke_state_handler bass_default_handler;
extern ke_state_t bass_state[BASS_IDX_MAX];

#endif // BLE_BAS_SERVER

/// @} BASSTASK

#endif /* _BASS_TASK_H_ */
