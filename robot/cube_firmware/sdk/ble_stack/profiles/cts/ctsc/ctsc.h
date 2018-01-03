/**
 ****************************************************************************************
 *
 * @file ctsc.h
 *
 * @brief Header file - Current Time Service Client.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef CTSC_H_
#define CTSC_H_

/**
 ****************************************************************************************
 * @addtogroup CTSC Current Time Service Client
 * @ingroup CTS
 * @brief Current Time Service Client
 * @{
 ****************************************************************************************
 */

/// Current Time Service Client Role
#define BLE_CTS_CLIENT              1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_CTS_CLIENT)
#include "ke_task.h"
#include "cts_common.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Current Time Service Characteristics
enum
{
    // Current Time Service
    CTSC_CHAR_CTS_CURRENT_TIME,
    // Local Time Information
    CTSC_CHAR_CTS_LOC_TIME_INFO,
    // Reference Time Information
    CTSC_CHAR_CTS_REF_TIME_INFO,

    CTSC_CHAR_MAX
};

/// Current Time Service Characteristic Descriptor
enum
{
    // Current Time Client Config
    CTSC_DESC_CTS_CT_CLI_CFG,

    CTSC_DESC_MAX,

    CTSC_DESC_MASK = 0x10
};

/// Internal codes for reading CTS characteristics
enum
{
    // Read Current Time
    CTSC_RD_CURRENT_TIME         = CTSC_CHAR_CTS_CURRENT_TIME,
    // Read Local Time Info
    CTSC_RD_LOC_TIME_INFO        = CTSC_CHAR_CTS_LOC_TIME_INFO,
    // Read Reference Time Info
    CTSC_RD_REF_TIME_INFO        = CTSC_CHAR_CTS_REF_TIME_INFO,
    // Read Current Time Client Cfg Descriptor
    CTSC_RD_CT_CLI_CFG           = (CTSC_DESC_MASK | CTSC_DESC_CTS_CT_CLI_CFG)
};

/// Pointer to the connection clean-up function
#define CTSC_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptor for
 * the Current Time Service
 */
struct ctsc_cts_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    struct prf_char_inf chars[CTSC_CHAR_MAX];

    /// Descriptor handles:
    struct prf_char_desc_inf desc[CTSC_DESC_MAX];
};

/// Current Time Service Client environment variable
struct ctsc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last char. code requested to read.
    uint8_t last_char_code;

    /// Counter used to check service uniqueness
    uint8_t nb_svc;

    struct ctsc_cts_content cts;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the CTSC module.
 * This function performs all the initializations of the CTSC module.
 ****************************************************************************************
 */
void ctsc_init(void);

/**
 ****************************************************************************************
 * @brief Send Current Time ATT DB discovery results to CTSC host.
 ****************************************************************************************
 */
void ctsc_enable_cfm_send(struct ctsc_env_tag *ctsc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send error indication from service to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void ctsc_error_ind_send(struct ctsc_env_tag *ctsc_env, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack the received current time value
 ****************************************************************************************
 */
void ctsc_unpack_curr_time_value(struct cts_curr_time* p_ct_val, uint8_t* packed_ct);


extern struct ctsc_env_tag **ctsc_envs;

#endif // BLE_CTS_CLIENT
/// @} CTSC
#endif // CTSC_H_

