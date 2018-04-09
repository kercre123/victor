/**
 ****************************************************************************************
 *
 * @file scppc.h
 *
 * @brief Header file - Scan Parameters Profile -Client Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _SCPPC_H_
#define _SCPPC_H_


/**
 ****************************************************************************************
 * @addtogroup SCPPC Scan Parameters Profile Client
 * @ingroup SCPP
 * @brief Scan Parameters Profile Client
 * @{
 ****************************************************************************************
 */

/// SCPPC Scan Parameters Profile Client role
#define BLE_SP_CLIENT               1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_SP_CLIENT)

#include "scpp_common.h"
#include "ke_task.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Characteristics
enum
{
    /// Scan Interval Window
    SCPPC_CHAR_SCAN_INTV_WD,
    /// Scan Refresh
    SCPPC_CHAR_SCAN_REFRESH,

    SCPPC_CHAR_MAX,
};


/// Characteristic descriptors
enum
{
    /// Scan Refresh Client Config
    SCPPC_DESC_SCAN_REFRESH_CFG,

    SCPPC_DESC_MAX,
};


/// Pointer to the connection clean-up function
#define SCPPC_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

///Structure containing the characteristics handles, value handles and descriptors
struct scps_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Scan Interval Window
    ///     - Scan Refresh
    struct prf_char_inf chars[SCPPC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Scan Refresh Client Cfg
    struct prf_char_desc_inf descs[SCPPC_DESC_MAX];
};

/// Scan Parameters Profile Client environment variable
struct scppc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;
    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;

    // Last Scan Interval Window value written on server
    struct scan_intv_wd scan_intv_wd;

    ///SCPS characteristics
    struct scps_content scps;

    /// Last char. code requested to read.
    uint8_t last_char_code;

    /// counter used to check service uniqueness
    uint8_t nb_svc;
};


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
extern struct scppc_env_tag **scppc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the SCPPC module.
 * This function performs all the initializations of the SCPPC module.
 ****************************************************************************************
 */
void scppc_init(void);

/**
 ****************************************************************************************
 * @brief Send SCP ATT DB discovery results to SCPPC host.
 ****************************************************************************************
 */
void scppc_enable_cfm_send(struct scppc_env_tag *scppc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void scppc_error_ind_send(struct scppc_env_tag *scppc_env, uint8_t status);

#endif /* (BLE_SP_CLIENT) */

/// @} SCPPC

#endif /* _SCPPC_H_ */
