/**
 ****************************************************************************************
 *
 * @file findl.h
 *
 * @brief Header file - FINDL.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef FINDL_H_
#define FINDL_H_

/**
 ****************************************************************************************
 * @addtogroup FIND Find Me Profile
 * @ingroup PROFILE
 * @brief Find Me Profile
 *
 * The FIND module is the responsible block for implementing the Find Me profile
 * functionalities in the BLE Host.
 *
 * The Find Me Profile defines the functionality required in a device that allows the user
 * to find a peer device by setting its alarm level.
 *****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup FINDL Find Me Locator
 * @ingroup FIND
 * @brief Find Me Profile Locator
 *
 * The FINDL is responsible for providing Find Me profile Locator functionalities to
 * upper layer module or application. The device using this profile takes the role
 * of find me locator.
 *
 * Find Me Locator (LOC): A LOC (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can set an alarm level value
 * in the peer Find Me target Device (TG), causing the TG to start a sound or flashing light
 * or other type of signal allowing it to be located.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Discover Immediate Alert Service range(from APP) and Send result (to APP)
 *  - Discover Alert Level Characteristic in the IAS range(from APP) and Send Result (to APP)
 *  - Set alert level in Target (from APP)
 *  - Error indications (to APP)
 *
 *  The application should remember the discovered IAS range and Alert Level Char. handles
 *  for the next connection to a known peer (that may also advertise it supports Find Me Profile.)
 *  This allows the setting of the alert level to be faster.
 *
 *  The enable/disable of the profile and devices disconnection is handled in the application,
 *  depending on its User Input.
 *
 * @{
 ****************************************************************************************
 */
 
 /// FINDL Find Me Locator role
#define BLE_FINDME_LOCATOR          1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif


#if (BLE_FINDME_LOCATOR)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "ke_task.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Alert levels
enum
{
    /// No alert
    FINDL_ALERT_NONE    = 0x00,
    /// Mild alert
    FINDL_ALERT_MILD,
    /// High alert
    FINDL_ALERT_HIGH
};

/// Pointer to the connection clean-up function
#define FINDL_CLEANUP_FNCT        (NULL)

/// Immediate Alert service details container
struct ias_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    /// - Alert Level
    struct prf_char_inf alert_lvl_char;
};

/// Find Me Locator environment variable
struct findl_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;
    /// Last UUID requested - for GATT complete event differentiation
    uint16_t last_uuid_req;

    /// Found IAS details
    struct ias_content ias;

    /// Last char. code requested to read.
    uint8_t last_char_code;

    /// counter used to check service uniqueness
    uint8_t nb_svc;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct findl_env_tag **findl_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the FINDL module.
 * This function performs all the initializations of the FINDL module.
 ****************************************************************************************
 */
void findl_init(void);

/**
 ****************************************************************************************
 * @brief Send Enable Confirm message to the application.
 * @param status  Status to send.
 ****************************************************************************************
 */
void findl_enable_cfm_send(struct findl_env_tag *findl_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send Error Indication message to the application.
 * @param status  Status to send.
 ****************************************************************************************
 */
void findl_error_ind_send(struct findl_env_tag *findl_env, uint8_t status);

#endif //BLE_FINDME_LOCATOR

/// @} FINDL
#endif // FINDL_H_
