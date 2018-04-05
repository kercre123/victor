/**
 ****************************************************************************************
 *
 * @file bmsc.h
 *
 * @brief Header file - Bond Management Service Client Declaration.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _BMSC_H_
#define _BMSC_H_

/**
 ****************************************************************************************
 * @addtogroup BMSC Bond Management Service Client
 * @ingroup BMS
 * @brief Bond Management Service Client
 * @{
 ****************************************************************************************
 */
/// Bond Management Service Client Role
#define BLE_BM_CLIENT               1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BM_CLIENT)
#include "ke_task.h"
#include "prf_types.h"

#include "bms_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Characteristics
enum
{
    /// Bond Management Control Point
    BMSC_CHAR_CP,
    /// Bond Management Feature
    BMSC_CHAR_FEATURE,

    BMSC_CHAR_MAX,
};

/// Pointer to the connection clean-up function
#define BMSC_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

///Structure containing the characteristics handles, value handles and descriptors
struct bms_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    /// - Bond Management Control Point
    /// - Bond Management Feature
    struct prf_char_inf chars[BMSC_CHAR_MAX];
};

/// Bond Management Profile Client environment variable
struct bmsc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;
    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last Char. for which a read request has been sent
    uint8_t last_char_code;

    /// counter used to check service uniqueness
    uint8_t nb_svc;

    /// BMS characteristics
    struct bms_content bms;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct bmsc_env_tag **bmsc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BMSC module.
 * This function performs all the initializations of the BMSC module.
 * @return void
 ****************************************************************************************
 */
void bmsc_init(void);

/**
 ****************************************************************************************
 * @brief Send Bond Management ATT DB discovery results to BMSC host.
 * @param[in] bmsc_env Pointer to BMSC environment variables structure
 * @param[in] con_info Pointer to profile connection information structure
 * @param[in] status Status code of error
 * @return void
 ****************************************************************************************
 */
void bmsc_enable_cfm_send(struct bmsc_env_tag *bmsc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Check if client request is possible or not
 * @param[in] bmsc_env Pointer to BMSC environment variables structure
 * @param[in] conhdl Connection handle.
 * @param[in] char_code Characteristic code.
 * @return PRF_ERR_OK if request can be performed, error code else.
 ****************************************************************************************
 */
uint8_t bmsc_validate_request(struct bmsc_env_tag *bmsc_env, uint16_t conhdl, uint8_t char_code);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param[in] bmsc_env Pointer to BMSC environment variables structure
 * @param[in] status Status code of error
 * @return void
 ****************************************************************************************
 */
void bmsc_error_ind_send(struct bmsc_env_tag *bmsc_env, uint8_t status);

#endif // (BLE_BM_CLIENT)

/// @} BMSC

#endif //_BMSC_H_
