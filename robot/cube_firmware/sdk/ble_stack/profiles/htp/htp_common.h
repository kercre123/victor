/**
 ****************************************************************************************
 *
 * @file htp_common.h
 *
 * @brief Header File - Health Thermometer Profile common types.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _HTP_COMMON_H_
#define _HTP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup HTP Health Thermometer Profile
 * @ingroup PROFILE
 * @brief Health Thermometer Profile
 *****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_HT_COLLECTOR || BLE_HT_THERMOM)

#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

///Temperature Measurement Flags field bit values
enum
{
    HTPT_FLAG_CELSIUS             = 0x00,
    HTPT_FLAG_FAHRENHEIT,
    HTPT_FLAG_TIME,
    HTPT_FLAG_TYPE                 = 0x04,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

///Temperature Measurement Value Structure
struct htp_temp_meas
{
    ///Temp
    uint32_t temp;
    ///Time stamp
    struct prf_date_time time_stamp;
    ///Flag
    uint8_t  flags;
    ///Type
    uint8_t  type;
};


#endif /* #if (BLE_HT_COLLECTOR || BLE_HT_THERMOMETER) */

/// @} htp_common

#endif /* _HTP_COMMON_H_ */
