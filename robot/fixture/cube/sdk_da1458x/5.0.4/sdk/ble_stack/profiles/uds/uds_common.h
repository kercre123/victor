/**
 ****************************************************************************************
 *
 * @file uds_common.h
 *
 * @brief Header File - User Data service common types.
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


#ifndef _UDS_COMMON_H_
#define _UDS_COMMON_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_UDS_CLIENT || BLE_UDS_SERVER)

#include "prf_types.h"
#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

/// User control point packet max length
#define UDS_USER_CTRL_PT_MAX_LEN     (20)

/// User consent code minimum and maximum values
#define UDS_CONSENT_CODE_MIN_VAL     (0x0000)
#define UDS_CONSENT_CODE_MAX_VAL     (0x270F)

/// No consent procedure was performed
#define UDS_ERROR_DATA_ACCESS_NOT_PERMITTED (0x80)
/// Improperly configured
#define UDS_ERROR_IMPROPERLY_CONFIGURED (0xFD)
/// Procedure in progress
#define UDS_ERROR_PROC_IN_PROGRESS      (0xFE)

/// User Data Service Error Code
enum uds_error_code
{
    /// UCP Procedure already in progress
    UDS_ERR_ACCESS_NOT_PERMITTED = (0x80),

};

/// User control point op codes
enum uds_ucp_op_code
{
    /// No opcode specified - reserved value
    UDS_NO_OP             = 0x00,

    /// Register new user
    UDS_REQ_REG_NEW_USER  = 0x01,

    /// Consent
    UDS_REQ_CONSENT       = 0x02,
    
    /// Delete current user
    UDS_REQ_DEL_USER_DATA = 0x03,
    
    /// Response code
    UDS_REQ_RSP_CODE      = 0x20,
};

/// User control point response value
enum uds_ucp_rsp_val
{
    /// Success
    UDS_RSP_SUCCESS           = 0x01,

    /// Operation code not supported
    UDS_RSP_OP_CODE_NOT_SUP   = 0x02,
    
    /// Invalid parameter
    UDS_RSP_INVALID_PARAMETER = 0x03,
    
    /// Operation failed
    UDS_RSP_OPERATION_FAILED  = 0x04,
    
    /// User not authorized
    UDS_RSP_USER_NOT_AUTH     = 0x05,
};

/// UDS database fields
enum uds_char_flags {
    UDS_CHAR_FIRST_NAME                         = 0x00000001,
    UDS_CHAR_LAST_NAME                          = 0x00000002,
    UDS_CHAR_EMAIL_ADDRESS                      = 0x00000004,
    UDS_CHAR_AGE                                = 0x00000008,
    UDS_CHAR_DATE_OF_BIRTH                      = 0x00000010,
    UDS_CHAR_GENDER                             = 0x00000020,
    UDS_CHAR_WEIGHT                             = 0x00000040,
    UDS_CHAR_HEIGHT                             = 0x00000080,
    UDS_CHAR_VO2_MAX                            = 0x00000100,
    UDS_CHAR_HEART_RATE_MAX                     = 0x00000200,
    UDS_CHAR_RESTING_HEART_RATE                 = 0x00000400,
    UDS_CHAR_MAX_RECOMMENDED_HEART_RATE         = 0x00000800,
    UDS_CHAR_AEROBIC_THRESHOLD                  = 0x00001000,
    UDS_CHAR_ANAEROBIC_THRESHOLD                = 0x00002000,
    UDS_CHAR_SPORT_TYPE                         = 0x00004000,
    UDS_CHAR_DATE_OF_THRESHOLD_ASSESSMENT       = 0x00008000,
    UDS_CHAR_WAIST_CIRCUMFERENCE                = 0x00010000,
    UDS_CHAR_HIP_CIRCUMFERENCE                  = 0x00020000,
    UDS_CHAR_FAT_BURN_HEART_RATE_LOW_LIMIT      = 0x00040000,
    UDS_CHAR_FAT_BURN_HEART_RATE_UP_LIMIT       = 0x00080000,
    UDS_CHAR_AEROBIC_HEART_RATE_LOW_LIMIT       = 0x00100000,
    UDS_CHAR_AEROBIC_HEART_RATE_UP_LIMIT        = 0x00200000,
    UDS_CHAR_ANEROBIC_HEART_RATE_LOW_LIMIT      = 0x00400000,
    UDS_CHAR_ANEROBIC_HEART_RATE_UP_LIMIT       = 0x00800000,
    UDS_CHAR_FIVE_ZONE_HEART_RATE_LIMITS        = 0x01000000,
    UDS_CHAR_THREE_ZONE_HEART_RATE_LIMITS       = 0x02000000,
    UDS_CHAR_TWO_ZONE_HEART_RATE_LIMIT          = 0x04000000,
    UDS_CHAR_LANGUAGE                           = 0x08000000,
};

/// Gender characteristic values
enum uds_gender
{
    UDS_GENDER_MALE                             = 0x00,
    UDS_GENDER_FEMALE                           = 0x01,
    UDS_GENDER_UNSPECIFIED                      = 0x02,
};

/// Sport Type for Aerobic and Anaerobic Thresholds characteristic values
enum uds_sport_type
{
    UDS_SPORT_UNSPECIFIED                       = 0x00,
    UDS_SPORT_RUNNING                           = 0x01,
    UDS_SPORT_CYCLING                           = 0x02,
    UDS_SPORT_ROWING                            = 0x03,
    UDS_SPORT_CROSS_TRAINING                    = 0x04,
    UDS_SPORT_CLIMBING                          = 0x05,
    UDS_SPORT_SKIING                            = 0x06,
    UDS_SPORT_SKATING                           = 0x07,
    UDS_SPORT_ARM_EXERCISING                    = 0x08,
    UDS_SPORT_LOWER_BODY_EXERCISING             = 0x09,
    UDS_SPORT_UPPER_BODY_EXERCISING             = 0x0A,
    UDS_SPORT_WHOLE_BODY_EXERCISING             = 0x0B,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Date of Birth and Threshold Assessment Date
struct uds_date
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
};

/// Five Zone Heart Rate Limits
struct uds_hr_limit_5zone
{
    uint8_t verylight_to_light;
    uint8_t light_to_moderate;
    uint8_t moderate_to_hard;
    uint8_t hard_to_max;
};

/// Three Zone Heart Rate Limits
struct uds_hr_limit_3zone
{
    uint8_t light_to_moderate;
    uint8_t moderate_to_hard;
};

/// Two Zone Heart Rate Limits
struct uds_hr_limit_2zone
{
    uint8_t fat_to_fitness;
};

/// User control point request
struct uds_ucp_req
{
    /// Operation code
    uint8_t op_code;

    /// Parameter
    union
    {
        /// Register new user parameter
        struct
        {
            /// Consent code
            uint16_t consent_code;
        } reg_new_user;
        
        /// Consent parameter
        struct
        {
            /// User index
            uint8_t user_idx;
            /// Consent code
            uint16_t consent_code;
        } consent;
    } parameter;
};

/// User control point response
struct uds_ucp_rsp
{
    /// Operation code
    uint8_t op_code;
    
    /// Request operation code
    uint8_t req_op_code;
    
    /// Response value
    uint8_t rsp_val;

    /// Response parameter
    union
    {
        /// Structure for register new user response parameter
        struct
        {
            /// User index
            uint8_t user_idx;
        } reg_new_user;
    } parameter;
};



#endif /* #if (BLE_UDS_CLIENT || BLE_UDS_SERVER) */

/// @} uds_common

#endif /* _UDS_COMMON_H_ */
