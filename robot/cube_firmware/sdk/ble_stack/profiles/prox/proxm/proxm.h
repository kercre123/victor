/**
 ****************************************************************************************
 *
 * @file proxm.h
 *
 * @brief Header file - PROXM.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef PROXM_H_
#define PROXM_H_

/**
 ****************************************************************************************
 * @addtogroup PROX Proximity Profile
 * @ingroup PROFILE
 * @brief Proximity Profile
 *
 * The PROX module is the responsible block for implementing the proximity profile
 * functionalities in the BLE Host.
 *
 * The Proximity Profile defines the functionality required in a device that can
 * alert the user when the user's personal device moves further away or closer together
 * to another communicating device.
 *****************************************************************************************
 */

/// Proximity Profile Monitor Role
#define BLE_PROX_MONITOR       1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF      1
#endif

/**
 ****************************************************************************************
 * @addtogroup PROXM Proximity Monitor
 * @ingroup PROX
 * @brief Proximity Profile Monitor
 *
 * The PROXM is responsible for providing proximity profile monitor functionalities to
 * upper layer module or application. The device using this profile takes the role
 * of a proximity monitor role.
 *
 * Proximity Monitor (PM): A PM (e.g. PC, phone, electronic door entry system, etc)
 * is the term used by this profile to describe a device that monitors the distance
 * between itself and the connected PR device. The profile on the PM device constantly
 * monitors the path loss between itself and the communicating Proximity Reporter
 * device. The profile provides indications to an application which can cause an alert
 * to the user.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_PROX_MONITOR)

#include "ke_task.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

///Link Loss or Immediate Alert code for setting alert through one message
enum
{
    ///Code for LLS Alert Level Char.
    PROXM_SET_LK_LOSS_ALERT = 0x00,
    ///Code for IAS Alert Level Char.
    PROXM_SET_IMMDT_ALERT,
};

/// Read Characteristic Code
enum
{
    /// Read Link Loss Service Alert Level Characteristic Value
    PROXM_RD_LL_ALERT_LVL,
    /// Read TX Power Service TX Power Level Characteristic Value
    PROXM_RD_TX_POWER_LVL,
};

///Alert Level Values
enum
{
    PROXM_ALERT_NONE,
    PROXM_ALERT_MILD,
    PROXM_ALERT_HIGH,
};

/// Pointer to the connection clean-up function
#define PROXM_CLEANUP_FNCT        (NULL)
/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Service information
struct svc_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    /// - Alert Level for IAS and LLS
    /// - TX Power Level for TXPS
    struct prf_char_inf charact;
};

/// Proximity monitor environment variable
struct proxm_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last service for which something was discovered
    uint16_t last_svc_req;

    /// Last char. code requested to read.
    uint8_t last_char_code;
    /// counter used to check service uniqueness
    uint8_t nb_svc;

    /// Link Loss Service information recovered from discovery
    struct svc_content lls;
    /// Immediate Alert Service information recovered from discovery
    struct svc_content ias;
    /// Tx Power Service information recovered from discovery
    struct svc_content txps;
};


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct proxm_env_tag **proxm_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the PROXM module.
 * This function performs all the initializations of the PROXM module.
 ****************************************************************************************
 */
void proxm_init(void);

/**
 ****************************************************************************************
 * @brief Send enable confirmation to application.
 * @param status Status of the enable: either OK or error encountered while discovery.
 ****************************************************************************************
 */
void proxm_enable_cfm_send(struct proxm_env_tag *proxm_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a PROXM_WR_CHAR_RSP message to the application
 *
 * @param status PROXM Error code.
 ****************************************************************************************
 */
void proxm_write_char_rsp_send(struct proxm_env_tag *proxm_env, uint8_t status);

#endif //BLE_PROX_MONITOR

/// @} PROXM

#endif // PROXM_H_
