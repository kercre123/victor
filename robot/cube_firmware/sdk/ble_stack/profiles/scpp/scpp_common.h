/**
 ****************************************************************************************
 *
 * @file scpp_common.h
 *
 * @brief Header File - Scan Parameters Profile common types.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _SCPP_COMMON_H_
#define _SCPP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup SCPP Scan Parameters Profile
 * @ingroup PROFILE
 * @brief Scan Parameters Profile
 *****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_SP_SERVER || BLE_SP_CLIENT)

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

enum
{
    SCPP_SERVER_REQUIRES_REFRESH    = 0x00,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Scan Interval Window Characteristic Value Structure
struct scan_intv_wd
{
    uint16_t le_scan_intv;
    uint16_t le_scan_window;
};


#endif // (BLE_SP_SERVER || BLE_SP_CLIENT)

/// @} scpp_common

#endif /* _SCPP_COMMON_H_ */
