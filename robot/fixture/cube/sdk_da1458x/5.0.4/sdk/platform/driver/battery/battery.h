/**
 ****************************************************************************************
 *
 * @file battery.h
 *
 * @brief Battery driver header file.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
 
#ifndef _BATTERY_H_
#define _BATTERY_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

// Battery types definitions
#define BATT_CR2032                         (1)            //CR2032 coin cell battery
#define BATT_CR1225                         (2)            //CR1225 coin cell battery
#define BATT_AAA                            (3)            //AAA alkaline battery (1 cell in boost, 2 cells in buck mode)

#define BATTERY_MEASUREMENT_BOOST_AT_1V5    (0x340)
#define BATTERY_MEASUREMENT_BOOST_AT_1V0    (0x230)
#define BATTERY_MEASUREMENT_BOOST_AT_0V9    (0x1F0)
#define BATTERY_MEASUREMENT_BOOST_AT_0V8    (0x1B0)

#define BATTERY_MEASUREMENT_BUCK_AT_3V0     (0x6B0)
#define BATTERY_MEASUREMENT_BUCK_AT_2V8     (0x640)
#define BATTERY_MEASUREMENT_BUCK_AT_2V6     (0x5D0)
#define BATTERY_MEASUREMENT_BUCK_AT_2V4     (0x550)

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Returns battery level percentage for the specific battery type.
 * @param[in] batt_type     Battery type. Supported types defined in battery.h
 * @return Battery level. 0 - 100%
 ****************************************************************************************
 */
uint8_t battery_get_lvl(uint8_t batt_type);

#endif // _BATTERY_H_
