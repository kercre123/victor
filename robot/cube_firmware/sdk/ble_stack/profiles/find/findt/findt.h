/**
 ****************************************************************************************
 *
 * @file findt.h
 *
 * @brief Header file - FINDT.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef FINDT_H_
#define FINDT_H_

/**
 ****************************************************************************************
 * @addtogroup FINDT Find Me Target
 * @ingroup FIND
 * @brief Find Me Profile Target.
 *
 * In the Find Me BLE Profile, the device that is in Target role will act as GATT server.
 *
 * The Target will react to alert levels written by the Locator in the Alert Level
 * Characteristic of the Immediate Alert Service(IAS) present in the ATT DB of the device.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Indicate that the alert level has been written by the Locator. (to APP)
 *
 * There shall be only one IAS instance on the device. There shall be only one
 * Alert Level Characteristic in the IAS. The characteristic properties shall be
 * Write No Response only.
 *
 *  The enable/disable of the profile and devices disconnection is handled in the application,
 *  depending on its User Input.
 * @{
 ****************************************************************************************
 */


/// Find Me Profile Target Role
#define BLE_FINDME_TARGET       1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif


#if (BLE_FINDME_TARGET)
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include <stdbool.h>
#include "gap.h"
#include "prf_types.h"

///Attributes State Machine
enum
{
    FINDT_IAS_IDX_SVC,

    FINDT_IAS_IDX_ALERT_LVL_CHAR,
    FINDT_IAS_IDX_ALERT_LVL_VAL,

    FINDT_IAS_IDX_NB,
};

/// Alert levels
enum
{
    /// No alert
    FINDT_ALERT_NONE    = 0x00,
    /// Mild alert
    FINDT_ALERT_MILD,
    /// High alert
    FINDT_ALERT_HIGH
};

/// Pointer to the connection clean-up function
#define FINDT_CLEANUP_FNCT        (NULL)

/*
 * DEFINES
 ****************************************************************************************
 */
/// Find Me Target environment variable
struct findt_env_tag
{
    ///Connection Information
    struct prf_con_info con_info;

    ///IAS Start Handle
    uint16_t shdl;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct attm_desc findt_att_db[FINDT_IAS_IDX_NB];

extern const att_svc_desc_t findt_ias_svc;

extern const struct att_char_desc findt_alert_lvl_char;

extern struct findt_env_tag findt_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialization of the FINDT module.
 * This function performs all the initializations of the FINDT module.
 ****************************************************************************************
 */
void findt_init(void);

/**
 ****************************************************************************************
 * @brief Disable role.
 ****************************************************************************************
 */
void findt_disable(uint16_t conhdl);


#endif //BLE_FINDME_TARGET

/// @} FINDT

#endif // FINDT_H_
