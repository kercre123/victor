/**
 ****************************************************************************************
 *
 * @file da1458x_config_advanced.h
 *
 * @brief Advanced compile configuration file.
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

#ifndef _DA1458X_CONFIG_ADVANCED_H_
#define _DA1458X_CONFIG_ADVANCED_H_

#include "da1458x_stack_config.h"

/****************************************************************************************************************/
/* If defined the OTP header data are read from SysRAM. The data are copied by bootrom code.                    */
/* Otherwise the OTP Header data are read from OTP memory.                                                      */
/* It must be undefined during development phase and for applications booting from external interfaces (UART,   */
/* SPI , I2C)                                                                                                   */
/****************************************************************************************************************/
#undef CFG_BOOT_FROM_OTP

/****************************************************************************************************************/
/* If defined NVDS structure is initialized with hardcoded values. If not defined the NDS structure is          */
/* padded with 0. The second option is used for production builds where the NVDS area in OTP or non-volatile    */
/* memory are written in manufacturing procedure                                                                */
/****************************************************************************************************************/
#define CFG_INITIALIZE_NVDS_STRUCT

/****************************************************************************************************************/
/* Low Power clock selection.                                                                                   */
/*      -LP_CLK_XTAL32      External XTAL32 oscillator                                                          */
/*      -LP_CLK_RCX20       External internal RCX20 clock                                                       */
/*      -LP_CLK_FROM_OTP    Use the selection in the corresponding field of OTP Header                          */
/****************************************************************************************************************/
#define CFG_LP_CLK              LP_CLK_RCX20

/****************************************************************************************************************/
/* If defined the application uses a hadrcoded value for XTAL16M trimming. Should be disabled for devices       */
/* where XTAL16M is calibrated and trim value is stored in OTP.                                                 */
/* Important note. The hardcoded value is the average value of the trimming values giving the optimal results   */
/* for DA14580 DK devices. May not be applicable in other designs                                               */
/****************************************************************************************************************/
#undef CFG_USE_DEFAULT_XTAL16M_TRIM_VALUE_IF_NOT_CALIBRATED

/****************************************************************************************************************/
/* Periodic wakeup period to poll GTL iface. Time in msec.                                                      */
/****************************************************************************************************************/
#define CFG_MAX_SLEEP_DURATION_PERIODIC_WAKEUP_MS                  500  // 0.5s

/****************************************************************************************************************/
/* Periodic wakeup period if GTL iface is not enabled. Time in msec.                                            */
/****************************************************************************************************************/
#define CFG_MAX_SLEEP_DURATION_EXTERNAL_WAKEUP_MS                  10000  // 10s

/****************************************************************************************************************/
/* Wakeup from external processor running host application.                                                     */
/****************************************************************************************************************/
#undef CFG_EXTERNAL_WAKEUP

/****************************************************************************************************************/
/* Wakeup external processor when a message is sent to GTL                                                      */
/****************************************************************************************************************/
#undef CFG_WAKEUP_EXT_PROCESSOR

/****************************************************************************************************************/
/* Enables True Random number Generator. A random number is generated at system initialization and used to seed */
/* the C standard library random number generator.                                                              */
/****************************************************************************************************************/
#undef CFG_TRNG

/****************************************************************************************************************/
/* To add comments                                                                                              */
/****************************************************************************************************************/
#define CFG_SKIP_SL_PATCH

/*
* Scaterfile: Memory maps
*/
#define REINIT_DESCRIPT_BUF     1       //0: keep in RetRAM, 1: re-init is required (set to 0 when Extended Sleep is used)
#define USE_MEMORY_MAP          DEEP_SLEEP_SETUP
#define DB_HEAP_SZ              1024
#define ENV_HEAP_SZ             328
#define MSG_HEAP_SZ             1312
#define NON_RET_HEAP_SZ         1024

/****************************************************************************************************************/
/* NVDS configuration                                                                                           */
/****************************************************************************************************************/
#define CFG_NVDS_TAG_BD_ADDRESS             {0x01, 0x00, 0x00, 0xCA, 0xEA, 0x80}

#define CFG_NVDS_TAG_LPCLK_DRIFT            DRIFT_BLE_DFT
#define CFG_NVDS_TAG_BLE_CA_TIMER_DUR       2000
#define CFG_NVDS_TAG_BLE_CRA_TIMER_DUR      6
#define CFG_NVDS_TAG_BLE_CA_MIN_RSSI        0x40
#define CFG_NVDS_TAG_BLE_CA_NB_PKT          100
#define CFG_NVDS_TAG_BLE_CA_NB_BAD_PKT      50

/****************************************************************************************************************/
/* To add comments                                                                                              */
/****************************************************************************************************************/
#undef CFG_RCX_MEASURE

/****************************************************************************************************************/
/* To add comments                                                                                              */
/****************************************************************************************************************/
#undef CFG_LOG_MEM_USAGE

/****************************************************************************************************************/
/* To add comments                                                                                              */
/****************************************************************************************************************/
#undef CFG_WLAN_COEX

/****************************************************************************************************************/
/* To add comments                                                                                              */
/****************************************************************************************************************/
#undef CFG_BLE_METRICS

/****************************************************************************************************************/
/* To add comments                                                                                              */
/****************************************************************************************************************/
#undef CFG_PRODUCTION_DEBUG_OUTPUT

/****************************************************************************************************************/
/* To add comments                                                                                              */
/****************************************************************************************************************/
#define CFG_PRODUCTION_TEST

/****************************************************************************************************************/
/* To add comments                                                                                              */
/****************************************************************************************************************/
#define CFG_INTEGRATED_HOST_GTL

#undef CFG_APP_AUDIO

/****************************************************************************************************************/
/* Production test configures GPIOs dynamically through host application commands.                              */
/* Therefore, the pre GPIO allocation is disabled.                                                              */
/****************************************************************************************************************/
#define GPIO_DRV_PIN_ALLOC_MON_DISABLED

#endif // _DA1458X_CONFIG_ADVANCED_H_
