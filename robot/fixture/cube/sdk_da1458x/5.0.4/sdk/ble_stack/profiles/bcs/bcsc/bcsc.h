/**
 ****************************************************************************************
 *
 * @file bcsc.h
 *
 * @brief Header file - Body Composition Service Client.
 *
 * Copyright (C) 2015 Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _BCSC_H_
#define _BCSC_H_

/**
 ****************************************************************************************
 * @addtogroup BCSC Body Composition Service Client
 * @ingroup BCS
 * @brief Body Composition Service Client
 * @{
 ****************************************************************************************
 */

/// Body Composition Client Role
#define BLE_BCS_CLIENT              1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BCS_CLIENT)
#include "prf_types.h"
#include "bcs_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Characteristics
enum
{
    /// Body Composition Feature
    BCSC_CHAR_BC_FEATURE,
    /// Body Composition Measurement
    BCSC_CHAR_BC_MEAS,

    BCSC_CHAR_MAX,
};

/// Characteristic descriptors
enum
{
    /// Body Composition Measurement client cfg
    BCSC_DESC_MEAS_CLI_CFG,
    BCSC_DESC_MAX,
    BCSC_DESC_MASK = 0x10,
};

/// Pointer to the connection clean-up function
#define BCSC_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure containing the characteristics handles, value handles and descriptors
struct bcs_content
{
    /// Service info
    struct prf_svc svc;
    /// If service is a secondary service
    bool is_secondary;

    /// Characteristic info:
    ///  - Body Composition Feature
    ///  - Body Composition Measurement
    struct prf_char_inf chars[BCSC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Body Composition Measurement client cfg
    struct prf_char_desc_inf descs[BCSC_DESC_MAX];
};

///Body Composition Service Client Environment Variable
struct bcsc_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;
    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last Char. for which a read request has been sent
    uint8_t last_char_code;

    /// Counter used to check service uniqueness
    uint8_t nb_svc;

    /// Characteristics
    struct bcs_content bcs;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Body Composition Client Environment Variable
extern struct bcsc_env_tag **bcsc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BCSC module.
 * This function performs all the initializations of the BCSC module.
 ****************************************************************************************
 */
void bcsc_init(void);

/**
 ****************************************************************************************
 * @brief Send Body Composition ATT DB discovery results to BCSC host.
 * @param[in] bcsc_env Body Composition Service client environment variable.
 * @param[in] con_info Profile connection info.
 * @param[in] status Status code of error.
 ****************************************************************************************
 */
void bcsc_enable_cfm_send(struct bcsc_env_tag *bcsc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack Body Composition measurement data into a comprehensive structure.
 * @param[out] pmeas_val    Pointer to BC measurement structure destination.
 * @param[out] pmeas_flags  Pointer to BC measurement flags destination.
 * @param[in] packed_bp     Pointer of the packed data of Body Composition Measurement
 *                          information.
 ****************************************************************************************
 */
void bcsc_unpack_meas_value(bcs_meas_t *pmeas_val, uint16_t *pmeas_flags, uint8_t *packed_bp);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param[in] bcsc_env Body Composition Service client environment variable.
 * @param[in] status Status code of error.
 ****************************************************************************************
 */
void bcsc_error_ind_send(struct bcsc_env_tag *bcsc_env, uint8_t status);

#endif /* #if (BLE_BCS_CLIENT) */

/// @} BCSC

#endif /* _BCSC_H_ */
