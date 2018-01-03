/**
 ****************************************************************************************
 *
 * @file lanc.h
 *
 * @brief Header file - Location and Navigation Status Profile - Collector Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _LANC_H_
#define _LANC_H_

/**
 ****************************************************************************************
 * @addtogroup LANC Location and Navigation Profile Collector
 * @ingroup LAN
 * @brief Location and Navigation Profile Collector
 * @{
 ****************************************************************************************
 */


 /// LANC Location and Navigation Profile Collector role
#define BLE_LN_COLLECTOR            1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "lan_common.h"

#if (BLE_LN_COLLECTOR)

#include "ke_task.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Location and Navigation Service Characteristic Descriptors
enum lanc_lns_descs
{
    /// Location and Speed Char. - Client Characteristic Configuration
    LANC_DESC_LOC_SPEED_CL_CFG,

    /// LN Control Point Char. - Client Characteristic Configuration
    LANC_DESC_LN_CTNL_PT_CL_CFG,

    /// Navigation Char. - Client Characteristic Configuration
    LANC_DESC_NAVIGATION_CL_CFG,


    LANC_DESC_MAX,

    LANC_DESC_MASK = 0x10,
};

/// Internal codes for reading/writing a LNS characteristic with one single request
enum lanc_code
{
    /// Read LN Feature
    LANC_RD_LN_FEAT           = LANP_LANS_LN_FEAT_CHAR,
    /// Notified Location and Speed
    LANC_NTF_LOC_SPEED        = LANP_LANS_LOC_SPEED_CHAR,
    /// Read Position quality
    LANC_RD_POS_Q             = LANP_LANS_POS_Q_CHAR,
    /// Indicated LN Control Point
    LANC_IND_LN_CTNL_PT       = LANP_LANS_LN_CTNL_PT_CHAR,
    /// Notified Navigation
    LANC_NTF_NAVIGATION       = LANP_LANS_NAVIG_CHAR,

    /// Read/Write Location and Speed Client Char. Configuration Descriptor
    LANC_RD_WR_LOC_SPEED_CL_CFG = (LANC_DESC_LOC_SPEED_CL_CFG | LANC_DESC_MASK),

    /// Read LN Control Point Client Char. Configuration Descriptor
    LANC_RD_WR_LN_CTNL_PT_CFG   = (LANC_DESC_LN_CTNL_PT_CL_CFG | LANC_DESC_MASK),

    /// Read/Write Vector Client Char. Configuration Descriptor
    LANC_RD_WR_NAVIGATION_CFG   = (LANC_DESC_NAVIGATION_CL_CFG   | LANC_DESC_MASK),

};

/// Pointer to the connection clean-up function
#define LANC_CLEANUP_FNCT        (lanc_cleanup)


/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Location and Navigation Service
 */
struct lanc_lns_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - LN Feature
    ///  - Location and Speed
    ///  - Position quality
    ///  - LN Control Point
    ///  - Navigation
    struct prf_char_inf chars[LANP_LANS_CHAR_MAX];

    /// Descriptor handles:
    ///  - Location and Speed Client Cfg
    ///  - Control Point Client Cfg
    ///  - Navigation Client Cfg
    struct prf_char_desc_inf descs[LANC_DESC_MAX];
};


/// Location and Navigation Profile Collector environment variable
struct lanc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    /// Current Operation
    void *operation;

    /// Provide an indication about the last operation
    uint16_t last_req;
    /// Last characteristic code discovered
    uint8_t last_char_code;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;

    /// Location and Navigation Service Characteristics
    struct lanc_lns_content lans;
};

/// Command Message Basic Structure
struct lanc_cmd
{
    /// Connection Handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;

    /// MORE DATA
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Pool of Location and Navigation Profile Collector task environments.
extern struct lanc_env_tag **lanc_envs;

/*
 * GLOBAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the LANC module.
 * This function performs all the initializations of the LANC module.
 ****************************************************************************************
 */
void lanc_init(void);

/**
 ****************************************************************************************
 * @brief Clean-up connection dedicated environment parameters
 * This function performs cleanup of ongoing operations
 * @param[in] idx_env Current connection environment variable
 ****************************************************************************************
 */
void lanc_cleanup(prf_env_struct *idx_env);

/**
 ****************************************************************************************
 * @brief Send a LANC_CMP_EVT message when no connection exists (no environment)
 ****************************************************************************************
 */
void lanc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation);

/**
 ****************************************************************************************
 * @brief Send a LANC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void lanc_send_cmp_evt(struct lanc_env_tag *lanc_env, uint8_t operation, uint8_t status);

#endif //(BLE_LN_COLLECTOR)

/// @} LANC

#endif //(_LANC_H_)
