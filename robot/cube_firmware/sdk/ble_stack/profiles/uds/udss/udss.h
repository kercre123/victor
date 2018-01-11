/**
 ****************************************************************************************
 *
 * @file udss.h
 *
 * @brief Header file - User Data Service Server.
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

#ifndef UDSS_H_
#define UDSS_H_

/**
 ****************************************************************************************
 * @addtogroup UDSS User Data Service Server
 * @ingroup UDS
 * @brief User Data Service Server
 * @{
 ****************************************************************************************
 */

/// UserData Service Server Role
#define BLE_UDS_SERVER              1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_UDS_SERVER)

#include "uds_common.h"

/*
 * MACROS
 ****************************************************************************************
 */

/// Check if flag enable
#define UDS_IS(FLAG) ((udss_env.flags & (1<< (UDS_##FLAG))) == (1<< (UDS_##FLAG)))

/// Set flag enable
#define UDS_SET(FLAG) (udss_env.flags |= (1<< (UDS_##FLAG)))

/// Set flag disable
#define UDS_CLEAR(FLAG) (udss_env.flags &= ~(1<< (UDS_##FLAG)))

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximal length for Characteristic values - 18
#define UDS_VAL_MAX_LEN                         (0x12)

///User height string length
#define UDS_SYS_ID_LEN                          (0x08)
///IEEE Certif length (min 6 bytes)
#define UDS_IEEE_CERTIF_MIN_LEN                 (0x06)
///PnP ID length
#define UDS_PNP_ID_LEN                          (0x07)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Characteristics Code for Write Indications
enum
{
    UDS_USER_FIRST_NAME_CHAR,
    UDS_USER_LAST_NAME_CHAR,
    UDS_USER_EMAIL_ADDRESS_CHAR,
    UDS_USER_AGE_CHAR,
    UDS_USER_DATE_OF_BIRTH_CHAR,
    UDS_USER_GENDER_CHAR,
    UDS_USER_WEIGHT_CHAR,
    UDS_USER_HEIGHT_CHAR,
    UDS_USER_VO2_MAX_CHAR,
    UDS_USER_HEART_RATE_MAX_CHAR,
    UDS_USER_RESTING_HEART_RATE_CHAR,
    UDS_USER_MAX_RECOMMENDED_HEART_RATE_CHAR,
    UDS_USER_AEROBIC_THRESHOLD_CHAR,
    UDS_USER_ANAEROBIC_THRESHOLD_CHAR,
    UDS_USER_SPORT_TYPE_CHAR,
    UDS_USER_DATE_OF_THRESHOLD_ASSESSMENT_CHAR,
    UDS_USER_WAIST_CIRCUMFERENCE_CHAR,
    UDS_USER_HIP_CIRCUMFERENCE_CHAR,
    UDS_USER_FAT_BURN_HEART_RATE_LOW_LIMIT_CHAR,
    UDS_USER_FAT_BURN_HEART_RATE_UP_LIMIT_CHAR,
    UDS_USER_AEROBIC_HEART_RATE_LOW_LIMIT_CHAR,
    UDS_USER_AEROBIC_HEART_RATE_UP_LIMIT_CHAR,
    UDS_USER_ANEROBIC_HEART_RATE_LOW_LIMIT_CHAR,
    UDS_USER_ANEROBIC_HEART_RATE_UP_LIMIT_CHAR,
    UDS_USER_FIVE_ZONE_HEART_RATE_LIMITS_CHAR,
    UDS_USER_THREE_ZONE_HEART_RATE_LIMITS_CHAR,
    UDS_USER_TWO_ZONE_HEART_RATE_LIMIT_CHAR,
    UDS_USER_LANGUAGE_CHAR,
    UDS_USER_DB_CHANGE_INCR_CHAR,
    UDS_USER_DB_CHANGE_INCR_CFG,
    UDS_USER_INDEX_CHAR,
    UDS_USER_CTRL_POINT_CHAR,
    UDS_USER_CTRL_POINT_CFG,

    UDS_CHAR_MAX,
};

/// Service processing flags
enum
{
    /// Flag used to know if there is an on-going user control point request
    UDS_UCP_ON_GOING    = 0x01,
    UDS_UCP_PENDING_REQ = 0x02,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

///User Data Service Server Environment Variable
struct udss_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    /// Service Start HandleVAL
    uint16_t shdl;
    uint16_t ehdl;

    /// Service processing flags
    uint8_t flags;

    /// Attribute Table
    uint8_t att_tbl[UDS_CHAR_MAX];

    /// If valid user is set and db entries are valid
    bool db_valid;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct udss_env_tag udss_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the UDSS module.
 * This function performs all the initializations of the UDSS module.
 ****************************************************************************************
 */
void udss_init(void);

/**
 ****************************************************************************************
 * @brief Unpack User Control request
 * @param[in] packed_val User Control Point value packed
 * @param[in] length Packed User Control Point value length
 * @param[out] ucp_req User Control Point Request value
 * @return status of unpacking
 ****************************************************************************************
 */
uint8_t udss_unpack_ucp_req(uint8_t *packed_val, uint16_t length,
                             struct uds_ucp_req* ucp_req);


/**
 ****************************************************************************************
 * @brief Pack User Control response
 * @param[out] packed_val User Control Point value packed
 * @param[in] ucp_rsp User Control Response value
 * @return size of packed data
 ****************************************************************************************
 */
uint8_t udss_pack_ucp_rsp(uint8_t *packed_val,
                           struct uds_ucp_rsp* ucp_rsp);

/**
 ****************************************************************************************
 * @brief Send User Control Response Indication
 * @param[in] ucp_rsp user Control Response value
 * @param[in] handle User Control Point Characteristic Value attribute handle
 * @param[in] ucp_ind_src Application that requests to send UCP response indication.
 * @return PRF_ERR_IND_DISABLED if indication disabled by peer device, PRF_ERR_OK else.
 ****************************************************************************************
 */
uint8_t udss_send_ucp_rsp(struct uds_ucp_rsp * ucp_rsp, uint16_t handle,
                           ke_task_id_t ucp_ind_src);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 * @param[in] conhdl Handle of connection for which this service is disabled
 ****************************************************************************************
 */
void udss_disable(uint16_t conhdl); 

#endif //BLE_UDS_SERVER

/// @} UDSS

#endif // UDSS_H_
