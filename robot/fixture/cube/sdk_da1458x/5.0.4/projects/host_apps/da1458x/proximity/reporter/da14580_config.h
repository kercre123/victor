/**
 ****************************************************************************************
 *
 * @file da14580_config.h
 *
 * @brief Compile configuration file.
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

#ifndef DA14580_CONFIG_H_
#define DA14580_CONFIG_H_

/// Enable host app as external SPI master boot device (0: disabled, 1: enabled)
#define SPI_BOOTER
// Choose boot image to be loaded to slave device (if SPI_BOOTER is defined)
#define PROX_REPORTER_EXT_581

#define CFG_BLE 1
#define CFG_ALLROLES 1
#define CFG_PRF_PXPR 1
#define BLE_PROX_REPORTER 1
#define CFG_CON 6
#define CFG_SECURITY_ON 1


#endif // DA14580_CONFIG_H_
