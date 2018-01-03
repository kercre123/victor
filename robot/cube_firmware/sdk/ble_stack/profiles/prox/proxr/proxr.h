/**
 ****************************************************************************************
 *
 * @file proxr.h
 *
 * @brief Header file - PROXR.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef PROXR_H_
#define PROXR_H_

/// Proximity Profile Reporter Role
#define BLE_PROX_REPORTER       1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif



/**
 ****************************************************************************************
 * @addtogroup PROXR Proximity Reporter
 * @ingroup PROX
 * @brief Proximity Profile Reporter.
 *
 * The PROXR is responsible for providing proximity reporter functionalities to
 * upper layer module or application. The device using this profile takes the role
 * of a proximity reporter role.
 *
 * Proximity Reporter (PR): A PR (e.g. a watch, key fob, etc) is the term used by
 * this profile to describe a personal device that a user carries with them and that
 * has low power requirement (i.e. it is operating with a button cell), allowing another
 * device to monitor their connection. The device may have a simple user alert
 * functionality, for example a blinking LED or audible output.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_PROX_REPORTER)
#include "ke_task.h"
#include "prf_types.h"
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// LLS Handles offsets
enum
{
    LLS_IDX_SVC,

    LLS_IDX_ALERT_LVL_CHAR,
    LLS_IDX_ALERT_LVL_VAL,

    LLS_IDX_NB,
};

/// IAS Handles offsets
enum
{
    IAS_IDX_SVC,

    IAS_IDX_ALERT_LVL_CHAR,
    IAS_IDX_ALERT_LVL_VAL,

    IAS_IDX_NB,
};

/// TXPS Handles offsets
enum
{
    TXPS_IDX_SVC,

    TXPS_IDX_TX_POWER_LVL_CHAR,
    TXPS_IDX_TX_POWER_LVL_VAL,

    TXPS_IDX_NB,
};

///Characteristics Code for Write Indications
enum
{
    PROXR_ERR_CHAR,
    PROXR_LLS_CHAR,
    PROXR_IAS_CHAR,
};

///Alert Level Values
enum
{
    PROXR_ALERT_NONE,
    PROXR_ALERT_MILD,
    PROXR_ALERT_HIGH,
};


/// Pointer to the connection clean-up function
#define PROXR_CLEANUP_FNCT        (NULL)

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Proximity reporter environment variable
struct proxr_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    /// LLS Start Handle
    uint16_t lls_shdl;
    /// IAS Start Handle
    uint16_t ias_shdl;
    /// TXPS Start Handle
    uint16_t txps_shdl;
};

/*
 * PROXIMITY PROFILE ATTRIBUTES DECLARATION
 ****************************************************************************************
 */

/// LLS Database Description
extern const struct attm_desc proxr_lls_att_db[LLS_IDX_NB];
/// IAS Database Description
extern const struct attm_desc proxr_ias_att_db[IAS_IDX_NB];
/// TXPS Database Description
extern const struct attm_desc proxr_txps_att_db[TXPS_IDX_NB];

/*
 *  PROXIMITY PROFILE ATTRIBUTES VALUES DECLARATION
 ****************************************************************************************
 */

/// Link Loss Service
extern const att_svc_desc_t proxr_lls_svc;
/// Link Loss Service - Alert Level Characteristic
extern const struct att_char_desc proxr_lls_alert_lvl_char;

/// Immediate Alert Service
extern const att_svc_desc_t proxr_ias_svc;
/// Immediate Alert Service - Alert Level Characteristic
extern const struct att_char_desc proxr_ias_alert_lvl_char;

/// TX Power Service
extern const att_svc_desc_t proxr_txps_svc;
/// TX Power Service -TX Power Level Characteristic
extern const struct att_char_desc proxr_txps_tx_power_lvl_char;

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct proxr_env_tag proxr_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the PROXR module.
 * This function performs all the initializations of the PROXR module.
 ****************************************************************************************
 */
void proxr_init(void);

/**
 ****************************************************************************************
 * @brief Disable role.
 ****************************************************************************************
 */
void proxr_disable(uint16_t conhdl); 

#endif //BLE_PROX_REPORTER

/// @} PROXR

#endif // PROXR_H_
