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
#define CFG_LP_CLK              LP_CLK_XTAL32

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
#define CFG_TRNG

/****************************************************************************************************************/
/* To add comments                                                                                              */
/****************************************************************************************************************/
#define CFG_SKIP_SL_PATCH

/****************************************************************************************************************/
/* Scatterfile - Memory maps configuration                                                                      */
/* If CFG_MEM_MAP_EXT_SLEEP or no sleep mode is selected the heap memory sizes are automatically configured     */
/* If CFG_MEM_MAP_DEEP_SLEEP is selected heap memory sizes are controlled by setting the values of the following*/
/* directives. Developer can use UM-B-011: Memory map excel tool to calculate the optimal values.               */
/*  -   USE_MEMORY_MAP  Memory configuration based on sleep mode. Determines if specific data areas can be      */
/*      located in SysRAM or not.                                                                               */
/*  -   REINIT_DESCRIPT_BUF  Reinitialize BLE messages' descriptors at wakeup from Deep Sleep mode              */
/*  -   DB_HEAP_SZ  ATT database Heap Size                                                                      */
/*  -   ENV_HEAP_SZ Environment structures Heap Size                                                            */
/*  -   MSG_HEAP_SZ Message Heap Size                                                                           */
/*  -   NON_RET_HEAP_SZ Non retention Heap Size                                                                 */
/****************************************************************************************************************/
/*
 * Scatterfile: Memory maps
 */
#if defined(CFG_MEM_MAP_EXT_SLEEP) || !defined(CFG_MEM_MAP_DEEP_SLEEP)
    #define REINIT_DESCRIPT_BUF     0       //0: keep in RetRAM, 1: re-init is required (set to 0 when Extended Sleep is used)
    #define USE_MEMORY_MAP          EXT_SLEEP_SETUP
#else
    #define REINIT_DESCRIPT_BUF     0       //0: keep in RetRAM, 1: re-init is required (set to 0 when Extended Sleep is used)
    #define USE_MEMORY_MAP          DEEP_SLEEP_SETUP
    #define DB_HEAP_SZ              1024
    #define ENV_HEAP_SZ             328
    #define MSG_HEAP_SZ             1312
    #define NON_RET_HEAP_SZ         1024
#endif

/****************************************************************************************************************/
/* NVDS configuration                                                                                           */
/* - CFG_NVDS_TAG_BD_ADDRESS            Default bdaddress. If bdaddress is written in OTP header this value is  */
/*                                      ignored                                                                 */
/* - CFG_NVDS_TAG_LPCLK_DRIFT           Low power clock drift. Permitted values in ppm are:                     */
/*      + DRIFT_20PPM                                                                                           */
/*      + DRIFT_30PPM                                                                                           */
/*      + DRIFT_50PPM                                                                                           */
/*      + DRIFT_75PPM                                                                                           */
/*      + DRIFT_100PPM                                                                                          */
/*      + DRIFT_150PPM                                                                                          */
/*      + DRIFT_250PPM                                                                                          */
/*      + DRIFT_500PPM                  Default value (500 ppm)                                                 */
/* - CFG_NVDS_TAG_BLE_CA_TIMER_DUR      Channel Assessment Timer duration (Multiple of 10ms)                    */
/* - CFG_NVDS_TAG_BLE_CRA_TIMER_DUR     Channel Reassessment Timer duration (Multiple of CA timer duration)     */
/* - CFG_NVDS_TAG_BLE_CA_MIN_RSSI       Minimal RSSI Threshold                                                  */
/* - CFG_NVDS_TAG_BLE_CA_NB_PKT         Number of packets to receive for statistics                             */
/* - CFG_NVDS_TAG_BLE_CA_NB_BAD_PKT     Number  of bad packets needed to remove a channel                       */
/****************************************************************************************************************/
#define CFG_NVDS_TAG_BD_ADDRESS             {0x03, 0x00, 0x00, 0xCA, 0xEA, 0x80}

#define CFG_NVDS_TAG_LPCLK_DRIFT            DRIFT_500PPM
#define CFG_NVDS_TAG_BLE_CA_TIMER_DUR       2000
#define CFG_NVDS_TAG_BLE_CRA_TIMER_DUR      6
#define CFG_NVDS_TAG_BLE_CA_MIN_RSSI        0x40
#define CFG_NVDS_TAG_BLE_CA_NB_PKT          100
#define CFG_NVDS_TAG_BLE_CA_NB_BAD_PKT      50

/****************************************************************************************************************/
/* Enables the logging of heap memories usage. The feature can be used in development/debug mode                */
/* Application must be executed in Keil debugger environment. Developer must stop execution and type            */
/* disp_memlog in debugger's command window. Heap memory statistics will be displayed on window                 */
/****************************************************************************************************************/
#undef CFG_LOG_MEM_USAGE

/****************************************************************************************************************/
/* Enables WLAN coexistence signaling interface.                                                                */
/****************************************************************************************************************/
#undef CFG_WLAN_COEX

/****************************************************************************************************************/
/* Enables the BLE statistics measurement feature.                                                              */
/****************************************************************************************************************/
#undef CFG_BLE_METRICS

/****************************************************************************************************************/
/* Output the Hardfault arguments to serial/UART interface.                                                     */
/****************************************************************************************************************/
#undef CFG_PRODUCTION_DEBUG_OUTPUT

/****************************************************************************************************************/
/* Used in DA14583. Enables reading the bdaddress from internal SPI flash. If non DA14583 applications the      */
/* feature is disabled.                                                                                         */
/****************************************************************************************************************/
#undef CFG_READ_BDADDR_FROM_DA14583_FLASH

#endif // _DA1458X_CONFIG_ADVANCED_H_
