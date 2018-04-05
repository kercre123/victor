/**
 ****************************************************************************************
 *
 * @file paspc.h
 *
 * @brief Header file - Phone Alert Status Profile - Client Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _PASPC_H_
#define _PASPC_H_

/**
 ****************************************************************************************
 * @addtogroup PASPC Phone Alert Status Profile Client
 * @ingroup PASP
 * @brief Phone Alert Status Profile Client
 * @{
 ****************************************************************************************
 */

/// PASPC Phone Alert Status Profile Client role
#define BLE_PAS_CLIENT              1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "pasp_common.h"

#if (BLE_PAS_CLIENT)

#include "ke_task.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Phone Alert Status Service Characteristics
enum
{
    /// Alert Status
    PASPC_CHAR_ALERT_STATUS,
    /// Ringer Setting
    PASPC_CHAR_RINGER_SETTING,
    /// Ringer Control Point
    PASPC_CHAR_RINGER_CTNL_PT,

    PASPC_CHAR_MAX,
};

/// Phone Alert Status Service Characteristic Descriptors
enum
{
    /// Alert Status Char. - Client Characteristic Configuration
    PASPC_DESC_ALERT_STATUS_CL_CFG,
    /// Ringer Setting Char. - Client Characteristic Configuration
    PASPC_DESC_RINGER_SETTING_CL_CFG,

    PASPC_DESC_MAX,

    PASPC_DESC_MASK = 0x10,
};

/// Internal codes for reading/writing a PAS characteristic with one single request
enum
{
    /// Read PAS Alert Status
    PASPC_RD_ALERT_STATUS              = PASPC_CHAR_ALERT_STATUS,
    /// Read PAS Ringer Setting
    PASPC_RD_RINGER_SETTING            = PASPC_CHAR_RINGER_SETTING,
    /// Write PAS Ringer Control Point
    PASPC_WR_RINGER_CTNL_PT            = PASPC_CHAR_RINGER_CTNL_PT,

    /// Read/Write PAS Alert Status Client Characteristic Configuration Descriptor
    PASPC_RD_WR_ALERT_STATUS_CFG       = (PASPC_DESC_ALERT_STATUS_CL_CFG | PASPC_DESC_MASK),
    /// Read PAS Ringer Setting Client Characteristic Configuration Descriptor
    PASPC_RD_WR_RINGER_SETTING_CFG     = (PASPC_DESC_RINGER_SETTING_CL_CFG | PASPC_DESC_MASK),
};


/// Pointer to the connection clean-up function
#define PASPC_CLEANUP_FNCT        (NULL)

/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Phone Alert Status Service
 */
struct paspc_pass_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Alert Status
    ///  - Ringer Setting
    ///  - Ringer Control Point
    struct prf_char_inf chars[PASPC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Alert Status Client Cfg
    ///  - Ringer Setting Client Cfg
    struct prf_char_desc_inf descs[PASPC_DESC_MAX];
};


/// Phone Alert Status Profile Client environment variable
struct paspc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last characteristic code used in a read or a write request
    uint16_t last_char_code;
    /// Current operation code
    uint8_t operation;

    /// Counter used to check service uniqueness
    uint8_t nb_svc;

    /// Phone Alert Status Service Characteristics
    struct paspc_pass_content pass;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Pool of Phone Alert Status Profile Client task environments.
extern struct paspc_env_tag **paspc_envs;

/*
 * GLOBAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the PASPC module.
 * This function performs all the initializations of the PASPC module.
 ****************************************************************************************
 */
void paspc_init(void);

/**
 ****************************************************************************************
 * @brief Send a PASPC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void paspc_send_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status);

#endif //(BLE_PAS_CLIENT)

/// @} PASPC

#endif //(_PASPC_H_)
