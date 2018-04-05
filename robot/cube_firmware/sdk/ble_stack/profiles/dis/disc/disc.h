/**
 ****************************************************************************************
 *
 * @file disc.h
 *
 * @brief Header file - Device Information Service - Client Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef DISC_H_
#define DISC_H_

/**
 ****************************************************************************************
 * @addtogroup DIS Device Information Service
 * @ingroup PROFILE
 * @brief Device Information Service
 *****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup DISC Device Information Service Client
 * @ingroup DIS
 * @brief Device Information Service Client
 * @{
 ****************************************************************************************
 */
 
/// Device Information Service Client Role
#define BLE_DIS_CLIENT          1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF      1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_DIS_CLIENT)
#include <stdint.h>
#include <stdbool.h>
#include "ke_task.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

enum
{
    DISC_MANUFACTURER_NAME_CHAR,
    DISC_MODEL_NB_STR_CHAR,
    DISC_SERIAL_NB_STR_CHAR,
    DISC_HARD_REV_STR_CHAR,
    DISC_FIRM_REV_STR_CHAR,
    DISC_SW_REV_STR_CHAR,
    DISC_SYSTEM_ID_CHAR,
    DISC_IEEE_CHAR,
    DISC_PNP_ID_CHAR,

    DISC_CHAR_MAX,
};

/// Pointer to the connection clean-up function
#define DISC_CLEANUP_FNCT        (NULL)
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Device Information Service
 */
struct disc_dis_content
{
    /// service info
    struct prf_svc svc;

    /// Characteristic info:
    struct prf_char_inf chars[DISC_CHAR_MAX];
};

/// Device Information Service Client environment variable
struct disc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last char. code requested to read.
    uint8_t last_char_code;

    /// counter used to check service uniqueness
    uint8_t nb_svc;

    ///Current Time Service Characteristics
    struct disc_dis_content dis;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct disc_env_tag **disc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the DISC module.
 * This function performs all the initializations of the DISC module.
 ****************************************************************************************
 */
void disc_init(void);


/**
 ****************************************************************************************
 * @brief Send Thermometer ATT DB discovery results to HTPC host.
 ****************************************************************************************
 */
void disc_enable_cfm_send(struct disc_env_tag *disc_env, struct prf_con_info *con_info, uint8_t status);

#endif //BLE_DIS_CLIENT

/// @} DISC

#endif // DISC_H_
