/**
 ****************************************************************************************
 *
 * @file scpps.h
 *
 * @brief Header file - Scan Parameters Profile Server.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _SCPPS_H_
#define _SCPPS_H_

/**
 ****************************************************************************************
 * @addtogroup SCPPS Scan Parameters Profile Server
 * @ingroup SCPP
 * @brief Scan Parameters Profile Server
 * @{
 ****************************************************************************************
 */

/// SCPPS Scan Parameters Profile Server Role
#define BLE_SP_SERVER           1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_SP_SERVER)

#include "scpp_common.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define SCPPS_CFG_FLAG_MANDATORY_MASK       (0x07)
#define SCPPS_CFG_FLAG_SCAN_REFRESH_MASK    (0x38)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Scan Parameters Service Attributes Indexes
enum
{
    SCPS_IDX_SVC,

    SCPS_IDX_SCAN_INTV_WD_CHAR,
    SCPS_IDX_SCAN_INTV_WD_VAL,

    SCPS_IDX_SCAN_REFRESH_CHAR,
    SCPS_IDX_SCAN_REFRESH_VAL,
    SCPS_IDX_SCAN_REFRESH_NTF_CFG,

    SCPS_IDX_NB,
};

/// Features Flag Masks
enum
{
    /// Scan Refresh Characteristic is not supported
    SCPPS_SCAN_REFRESH_CHAR_NOT_SUP,
    /// Scan Refresh Characteristic is supported
    SCPPS_SCAN_REFRESH_CHAR_SUP,
};


/// Pointer to the connection clean-up function
#define SCPPS_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Scan Parameters Profile Server environment variable
struct scpps_env_tag
{
    /// Connection info
    struct prf_con_info con_info;

    /// SCPS Start Handle
    uint16_t shdl;
    /// Database configuration
    uint8_t features;
    /// Scan Refresh NTF Configuration
    uint8_t scan_refresh_ntf_cfg;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Full SCPS Database Description
extern const struct attm_desc scpps_att_db[SCPS_IDX_NB];

///Scan Parameters Service
extern const att_svc_desc_t scps_svc;

///Scan Parameters Service Scan Interval Window Characteristic
extern const struct att_char_desc scps_scan_intv_wd_char;
///Scan Parameters Service Scan Refresh Characteristic
extern const struct att_char_desc scps_scan_refresh_char;

/// SCPPS Environment
extern struct scpps_env_tag scpps_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the SCPPS module.
 * This function performs all the initializations of the SCPPS module.
 ****************************************************************************************
 */
void scpps_init(void);

/**
 ****************************************************************************************
 * @brief Send a SCPPS_SCAN_REFRESH_SEND_CFM message to the application
 ****************************************************************************************
 */
void scpps_scan_refresh_cfm_send(uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void scpps_disable(uint16_t conhdl); 

#endif /* #if (BLE_SP_SERVER) */

/// @} SCPPS

#endif /* _SCPPS_H_ */
