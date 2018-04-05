/**
 ****************************************************************************************
 *
 * @file system_settings.h
 *
 * @brief DA14580 RF preffered settings.
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
#ifndef _SYSTEM_SETTINGS_H_
#define _SYSTEM_SETTINGS_H_


/*
 * Radio preferred settings
 ****************************************************************************************
 */


#define PREF_BLE_RADIOPWRUPDN_REG           0x754054C
#define PREF_RF_LF_CTRL_REG                 0x4C
#define PREF_RF_CP_CTRL_REG                 0x7F7F
#define PREF_RF_REF_OSC_REG                 0x29AC
#define PREF_RF_ENABLE_CONFIG1_REG          0x909
#define PREF_RF_ENABLE_CONFIG2_REG          0x922
#define PREF_RF_ENABLE_CONFIG6_REG          0x22
#define PREF_RF_ENABLE_CONFIG9_REG          0x204
#define PREF_RF_ENABLE_CONFIG10_REG         0x422
#define PREF_RF_ENABLE_CONFIG13_REG         0xD030
#define PREF_RF_ENABLE_CONFIG14_REG         0x433
#define PREF_RF_ENABLE_CONFIG19_REG         0x11EE
#define PREF_RF_CNTRL_TIMER_3_REG           0x410
#define PREF_RF_CNTRL_TIMER_4_REG           0x22E
#define PREF_RF_CNTRL_TIMER_8_REG           0x23E
#define PREF_RF_CNTRL_TIMER_9_REG           0x22E
#define PREF_RF_CNTRL_TIMER_10_REG          0x22E    
#define PREF_RF_CNTRL_TIMER_11_REG          0x230
#define PREF_RF_CNTRL_TIMER_12_REG          0x239 
#define PREF_RF_CNTRL_TIMER_13_REG          0x145
#define PREF_RF_CNTRL_TIMER_14_REG          0x2044
#define PREF_BIAS_CTRL1_REG                 0x6888
#define PREF_RF_DEM_CTRL_REG                0x59
#define PREF_RF_AGC_CTRL1_REG               0x950D
#define PREF_RF_AGC_CTRL2_REG               0x43
#define PREF_RF_AGC_LUT_23_REG              0x4202
#define PREF_RF_AFC_CTRL_REG                0xD5
#define PREF_RF_DC_OFFSET_CTRL2_REG         0x1D2
#define PREF_RF_DC_OFFSET_CTRL3_REG         0xDCE4
#define PREF_RF_DC_OFFSET_CTRL4_REG         0x9210
#define PREF_RF_PA_CTRL_REG                 0x7853
#define PREF_RF_SYNTH_CTRL2_REG             0x108B
#define PREF_RF_VCOCAL_CTRL_REG             0x63
#define PREF_RF_MGAIN_CTRL_REG              0x9503
#define PREF_RF_VCO_CALCAP_BIT14_REG        0xD59D
#define PREF_RF_LF_RES_CTRL_REG             0x7F7F
#define PREF_RF_MGAIN_CTRL2_REG             0x8	

#endif // _SYSTEM_SETTINGS_H_

