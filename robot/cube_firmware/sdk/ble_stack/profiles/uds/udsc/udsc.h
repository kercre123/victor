/**
 ****************************************************************************************
 *
 * @file udsc.h
 *
 * @brief Header file - User Data Service Client.
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

#ifndef _UDSC_H_
#define _UDSC_H_

/**
 ****************************************************************************************
 * @addtogroup UDSC User Data Service Client
 * @ingroup UDS
 * @brief User Data Service Client
 * @{
 ****************************************************************************************
 */

/// User Data Service Client Role
#define BLE_UDS_CLIENT              1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_UDS_CLIENT)
#include "prf_types.h"
#include "uds_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Characteristics
enum
{
    UDSC_FIRST_NAME_CHAR,
    UDSC_LAST_NAME_CHAR,
    UDSC_EMAIL_ADDRESS_CHAR,
    UDSC_AGE_CHAR,
    UDSC_DATE_OF_BIRTH_CHAR,
    UDSC_GENDER_CHAR,
    UDSC_WEIGHT_CHAR,
    UDSC_HEIGHT_CHAR,
    UDSC_VO2_MAX_CHAR,
    UDSC_HEART_RATE_MAX_CHAR,
    UDSC_RESTING_HEART_RATE_CHAR,
    UDSC_MAX_RECOMMENDED_HEART_RATE_CHAR,
    UDSC_AEROBIC_THRESHOLD_CHAR,
    UDSC_ANAEROBIC_THRESHOLD_CHAR,
    UDSC_SPORT_TYPE_CHAR,
    UDSC_DATE_OF_THRESHOLD_ASSESSMENT_CHAR,
    UDSC_WAIST_CIRCUMFERENCE_CHAR,
    UDSC_HIP_CIRCUMFERENCE_CHAR,
    UDSC_FAT_BURN_HEART_RATE_LOW_LIMIT_CHAR,
    UDSC_FAT_BURN_HEART_RATE_UP_LIMIT_CHAR,
    UDSC_AEROBIC_HEART_RATE_LOW_LIMIT_CHAR,
    UDSC_AEROBIC_HEART_RATE_UP_LIMIT_CHAR,
    UDSC_ANEROBIC_HEART_RATE_LOW_LIMIT_CHAR,
    UDSC_ANEROBIC_HEART_RATE_UP_LIMIT_CHAR,
    UDSC_FIVE_ZONE_HEART_RATE_LIMITS_CHAR,
    UDSC_THREE_ZONE_HEART_RATE_LIMITS_CHAR,
    UDSC_TWO_ZONE_HEART_RATE_LIMIT_CHAR,
    UDSC_LANGUAGE_CHAR,
    UDSC_DB_CHANGE_INCR_CHAR,
    UDSC_INDEX_CHAR,
    UDSC_CTRL_POINT_CHAR,

    UDSC_CHAR_MAX,
};

/// Characteristic descriptors
enum
{
    UDSC_USER_DBC_INC_CFG,
    UDSC_DESC_UCP_CFG,
    UDSC_DESC_MAX,
};

/// Pointer to the connection clean-up function
#define UDSC_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure containing the characteristics handles, value handles and descriptors
struct uds_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - User Data Feature
    ///  - User Data Measurement
    struct prf_char_inf chars[UDSC_CHAR_MAX];

    /// Descriptor handles:
    ///  - User Data Measurement client cfg
    struct prf_char_desc_inf descs[UDSC_DESC_MAX];
};

/// User Data Service Client Environment Variable
struct udsc_env_tag
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
    struct uds_content uds;

    /// Ongoing UCP operation code
    uint8_t op_code;
    /// User data entry is written
    bool db_update;

    /// Value of recently notified or read DB change increment characteristic
    uint32_t db_change_incr;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// User Data Client Environment Variable
extern struct udsc_env_tag **udsc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the UDSC module.
 * This function performs all the initializations of the UDSC module.
 ****************************************************************************************
 */
void udsc_init(void);

/**
 ****************************************************************************************
 * @brief Send User Data ATT DB discovery results to UDSC host.
 * @param[in] udsc_env User Data Service client environment variable.
 * @param[in] prf_con_info Profile connection info.
 * @param[in] status Status code of error.
 ****************************************************************************************
 */
void udsc_enable_cfm_send(struct udsc_env_tag *udsc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send indication from profile to Host, with proprietary status codes.
 * @param[in] udsc_env User Data Service client environment variable.
 * @param[in] status Status code of error.
 ****************************************************************************************
 */
void udsc_cmp_evt_ind_send(struct udsc_env_tag *udsc_env, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send characteristic value indication from profile to Host.
 * @param[in] udsc_env User Data Service client environment variable.
 * @param[in] status Status code of error.
 * @param[in] char_code Code of the read characteristic.
 * @param[in] length Length of the Characteristic Value.
 * @param[in] value Characteristic value.
 ****************************************************************************************
 */
void udsc_read_char_val_rsp_send(struct udsc_env_tag *udsc_env, uint8_t status,
                                            uint8_t char_code, uint16_t length,
                                            const uint8_t *value);

/**
 ****************************************************************************************
 * @brief Send characteristic value write result from profile to Host.
 * @param[in] udsc_env User Data Service client environment variable.
 * @param[in] status Status code of error.
 ****************************************************************************************
 */
void udsc_set_char_val_cfm_send(struct udsc_env_tag *udsc_env, uint8_t status);

#endif /* #if (BLE_UDS_CLIENT) */

/// @} UDSC

#endif /* _UDSC_H_ */
