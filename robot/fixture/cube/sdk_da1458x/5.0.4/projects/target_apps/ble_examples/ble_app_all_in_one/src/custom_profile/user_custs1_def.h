/**
 ****************************************************************************************
 *
 * @file user_custs1_def.h
 *
 * @brief Custom1 Server (CUSTS1) profile database declarations.
 *
 * Copyright (C) 2016. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _USER_CUSTS1_DEF_H_
#define _USER_CUSTS1_DEF_H_

/**
 ****************************************************************************************
 * @defgroup USER_CONFIG
 * @ingroup USER
 * @brief Custom1 Server (CUSTS1) profile database declarations.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "attm_db_128.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define DEF_CUST1_SVC_UUID_128            {0x2F, 0x2A, 0x93, 0xA6, 0xBD, 0xD8, 0x41, 0x52, 0xAC, 0x0B, 0x10, 0x99, 0x2E, 0xC6, 0xFE, 0xED}

#define DEF_CUST1_CTRL_POINT_UUID_128     {0x20, 0xEE, 0x8D, 0x0C, 0xE1, 0xF0, 0x4A, 0x0C, 0xB3, 0x25, 0xDC, 0x53, 0x6A, 0x68, 0x86, 0x2D}
#define DEF_CUST1_LED_STATE_UUID_128      {0x4F, 0x43, 0x31, 0x3C, 0x93, 0x92, 0x42, 0xE6, 0xA8, 0x76, 0xFA, 0x3B, 0xEF, 0xB4, 0x87, 0x5A}
#define DEF_CUST1_ADC_VAL_1_UUID_128      {0x17, 0xB9, 0x67, 0x98, 0x4C, 0x66, 0x4C, 0x01, 0x96, 0x33, 0x31, 0xB1, 0x91, 0x59, 0x00, 0x15}
#define DEF_CUST1_ADC_VAL_2_UUID_128      {0x23, 0x68, 0xEC, 0x52, 0x1E, 0x62, 0x44, 0x74, 0x9A, 0x1B, 0xD1, 0x8B, 0xAB, 0x75, 0xB6, 0x6E}
#define DEF_CUST1_BUTTON_STATE_UUID_128   {0x9E, 0xE7, 0xBA, 0x08, 0xB9, 0xA9, 0x48, 0xAB, 0xA1, 0xAC, 0x03, 0x1C, 0x2E, 0x0D, 0x29, 0x6C}
#define DEF_CUST1_INDICATEABLE_UUID_128   {0x28, 0xD5, 0xE1, 0xC1, 0xE1, 0xC5, 0x47, 0x29, 0xB5, 0x57, 0x65, 0xC3, 0xBA, 0x47, 0x15, 0x9E}
#define DEF_CUST1_LONG_VALUE_UUID_128     {0x8C, 0x09, 0xE0, 0xD1, 0x81, 0x54, 0x42, 0x40, 0x8E, 0x4F, 0xD2, 0xB3, 0x77, 0xE3, 0x2A, 0x77}

#define DEF_CUST1_CTRL_POINT_CHAR_LEN     1
#define DEF_CUST1_LED_STATE_CHAR_LEN      1
#define DEF_CUST1_ADC_VAL_1_CHAR_LEN      2
#define DEF_CUST1_ADC_VAL_2_CHAR_LEN      2
#define DEF_CUST1_BUTTON_STATE_CHAR_LEN   1
#define DEF_CUST1_INDICATEABLE_CHAR_LEN   20
#define DEF_CUST1_LONG_VALUE_CHAR_LEN     50

#define CUST1_CONTROL_POINT_USER_DESC     "Control Point"
#define CUST1_LED_STATE_USER_DESC         "LED State"
#define CUST1_ADC_VAL_1_USER_DESC         "ADC Value 1"
#define CUST1_ADC_VAL_2_USER_DESC         "ADC Value 2"
#define CUST1_BUTTON_STATE_USER_DESC      "Button State"
#define CUST1_INDICATEABLE_USER_DESC      "Indicateable"
#define CUST1_LONG_VALUE_CHAR_USER_DESC   "Long Value"

/// Custom1 Service Data Base Characteristic enum
enum
{
    CUST1_IDX_SVC = 0,

    CUST1_IDX_CONTROL_POINT_CHAR,
    CUST1_IDX_CONTROL_POINT_VAL,
    CUST1_IDX_CONTROL_POINT_USER_DESC,

    CUST1_IDX_LED_STATE_CHAR,
    CUST1_IDX_LED_STATE_VAL,
    CUST1_IDX_LED_STATE_USER_DESC,

    CUST1_IDX_ADC_VAL_1_CHAR,
    CUST1_IDX_ADC_VAL_1_VAL,
    CUST1_IDX_ADC_VAL_1_NTF_CFG,
    CUST1_IDX_ADC_VAL_1_USER_DESC,

    CUST1_IDX_ADC_VAL_2_CHAR,
    CUST1_IDX_ADC_VAL_2_VAL,
    CUST1_IDX_ADC_VAL_2_USER_DESC,

    CUST1_IDX_BUTTON_STATE_CHAR,
    CUST1_IDX_BUTTON_STATE_VAL,
    CUST1_IDX_BUTTON_STATE_NTF_CFG,
    CUST1_IDX_BUTTON_STATE_USER_DESC,

    CUST1_IDX_INDICATEABLE_CHAR,
    CUST1_IDX_INDICATEABLE_VAL,
    CUST1_IDX_INDICATEABLE_IND_CFG,
    CUST1_IDX_INDICATEABLE_USER_DESC,

    CUST1_IDX_LONG_VALUE_CHAR,
    CUST1_IDX_LONG_VALUE_VAL,
    CUST1_IDX_LONG_VALUE_NTF_CFG,
    CUST1_IDX_LONG_VALUE_USER_DESC,

    CUST1_IDX_NB
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct attm_desc_128 custs1_att_db[CUST1_IDX_NB];

/// @} USER_CONFIG

#endif // _USER_CUSTS1_DEF_H_
