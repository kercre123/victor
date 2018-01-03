/**
 ****************************************************************************************
 *
 * @file basc.h
 *
 * @brief Header file - Battery Service - Client Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _BASC_H_
#define _BASC_H_

/**
 ****************************************************************************************
 * @addtogroup BASC Battery Service Client
 * @ingroup BAS
 * @brief Battery Service Client
 * @{
 ****************************************************************************************
 */

/// Battery Service Client Role
#define BLE_BAS_CLIENT             1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BAS_CLIENT)
#include "ke_task.h"
#include "prf_types.h"



/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Battery Service instances we can handle
#define BASC_NB_BAS_INSTANCES_MAX         (2)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Battery Service Characteristics
enum
{
    /// Battery Level
    BAS_CHAR_BATT_LEVEL,

    BAS_CHAR_MAX,
};

/// Battery Service Descriptors
enum
{
    /// Battery Level Characteristic Presentation Format
    BAS_DESC_BATT_LEVEL_PRES_FORMAT,
    /// Battery Level Client Characteristic Configuration
    BAS_DESC_BATT_LEVEL_CFG,

    BAS_DESC_MAX,

    /// Mask used for application criteria
    BAS_DESC_MASK = 0x10,
};

/// Internal codes for reading a BAS characteristic with one single request
enum
{
    ///Read BAS Battery Level
    BASC_RD_BAS_BATT_LEVEL               = BAS_CHAR_BATT_LEVEL,

    ///Read BAS Battery Level Characteristic Presentation Format
    BASC_RD_BAS_BATT_LEVEL_PRES_FORMAT   = (BAS_DESC_MASK | BAS_DESC_BATT_LEVEL_PRES_FORMAT),
    ///Read BAS Battery Level Client Cfg. Desc
    BASC_RD_BAS_BATT_LEVEL_CLI_CFG       = (BAS_DESC_MASK | BAS_DESC_BATT_LEVEL_CFG),
};

/// Indications type for Battery Level value
enum
{
    // The basc_batt_level_ind msg is sent to App as a read response
    BASC_BATT_LEVEL_RD_RSP,
    // The basc_batt_level_ind msg is sent to App as a notification
    BASC_BATT_LEVEL_NTF,
};

/// Pointer to the connection clean-up function
#define BASC_CLEANUP_FNCT        (NULL)
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

///Structure containing the characteristics handles, value handles and descriptors
struct bas_content
{
    /// service info
    struct prf_svc svc;

    /// Characteristic Info:
    /// - Battery Level
    struct prf_char_inf chars[BAS_CHAR_MAX];

    /// Descriptor handles:
    /// - Battery Level Client Characteristic Configuration
    /// - Battery Level Characteristic Presentation Format
    struct prf_char_desc_inf descs[BAS_DESC_MAX];
};

/// Battery 'Profile' Client environment variable
struct basc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    ///Last requested UUID
    uint16_t last_uuid_req;
    ///Last instance of service for which something was requested
    uint8_t last_svc_inst_req;
    /// Last char. code requested to read.
    uint8_t last_char_code;

    ///Number of BAS instances found
    uint8_t bas_nb;

    ///BAS characteristics
    struct bas_content bas[BASC_NB_BAS_INSTANCES_MAX];
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct basc_env_tag **basc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BAPC module.
 * This function performs all the initializations of the BAPC module.
 ****************************************************************************************
 */
void basc_init(void);

/**
 ****************************************************************************************
 * @brief Send BAP ATT DB discovery results to BAPC host.
 ****************************************************************************************
 */
void basc_enable_cfm_send(struct basc_env_tag *basc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void basc_error_ind_send(struct basc_env_tag *basc_env, uint8_t status);

#endif /* (BLE_BAS_CLIENT) */

/// @} BASC

#endif /* _BASC_H_ */
