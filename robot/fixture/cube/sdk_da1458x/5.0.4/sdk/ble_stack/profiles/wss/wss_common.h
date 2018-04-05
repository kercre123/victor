/**
 ****************************************************************************************
 *
 * @file wss_common.h
 *
 * @brief Header file - Weight Scale Service common types.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef WSS_COMMON_H_
#define WSS_COMMON_H_
/**
 ************************************************************************************
 * @addtogroup WSSS Weight Scale Service Collector
 * @ingroup WSS
 * @brief Weight Scale Service Collector
 * @{
 ************************************************************************************
 */

/*
 * INCLUDE FILES
 ************************************************************************************
 */

#if (BLE_WSS_SERVER || BLE_WSS_COLLECTOR)

#include "prf_types.h"


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Weight Measurement Flags field bit values
enum
{
    /// Imperial Measurement Units (weight in lb and height in inches)
    WSS_MEAS_FLAG_UNIT_IMPERIAL      = 0x01,
    /// Time Stamp present
    WSS_MEAS_FLAG_TIME_STAMP         = 0x02,
    /// User ID present
    WSS_MEAS_FLAG_USERID_PRESENT     = 0x04,
    /// BMI & Height present
    WSS_MEAS_FLAG_BMI_HT_PRESENT     = 0x08,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Weight measurement structure - shall be dynamically allocated
struct wss_wt_meas
{
    /// Flags
    uint8_t flags;
    /// Weight
    float weight;
    /// Time stamp
    struct prf_date_time datetime;
    /// User ID
    uint8_t userid;
    /// BMI
    float bmi;
    /// Height
    float height;
};

#endif
/// @} blp_common

#endif /* WSS_COMMON_H_ */
