/**
 ****************************************************************************************
 *
 * @file user_custs_config.h
 *
 * @brief Custom1/2 Server (CUSTS1/2) profile database initialization.
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

#ifndef _USER_CUSTS_CONFIG_H_
#define _USER_CUSTS_CONFIG_H_

/**
 ****************************************************************************************
 * @defgroup USER_CONFIG
 * @ingroup USER
 * @brief Custom1/2 Server (CUSTS1/2) profile database initialization.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_prf_types.h"
#include "attm_db_128.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define DEF_CUST1_SVC_UUID_128            {0x30, 0xF8, 0x22, 0x1F, 0x8E, 0x30, 0x4C, 0xFB, 0x8B, 0x59, 0x19, 0xD2, 0x0F, 0xC7, 0xF6, 0xC6}

#define DEF_CUST1_OTA_TARGET_UUID_128     {0xA4, 0x57, 0x15, 0x68, 0x9D, 0x5F, 0x44, 0x18, 0xB5, 0x92, 0x40, 0x51, 0x9C, 0xBA, 0x90, 0x95}
#define DEF_CUST1_LOADED_APP_UUID_128     {0x9E, 0xB7, 0xB7, 0x2E, 0x0E, 0xD5, 0x48, 0x91, 0xA6, 0x16, 0x85, 0x8D, 0x75, 0xA1, 0x0A, 0x45}
#define DEF_CUST1_APP_WRITE_UUID_128      {0x4A, 0xD9, 0x02, 0x4E, 0x8C, 0x59, 0x48, 0x79, 0x8D, 0xA5, 0x59, 0x67, 0x90, 0x52, 0xA7, 0x0E}
#define DEF_CUST1_APP_READ_UUID_128       {0xAB, 0x4C, 0x82, 0x77, 0x94, 0x2A, 0x47, 0x36, 0x81, 0x7B, 0xB1, 0x5F, 0xAF, 0x14, 0xEF, 0x43}

#define DEF_CUST1_OTA_TARGET_CHAR_LEN     20
#define DEF_CUST1_LOADED_APP_CHAR_LEN     20
#define DEF_CUST1_APP_WRITE_CHAR_LEN      20
#define DEF_CUST1_APP_READ_CHAR_LEN       20

#define CUST1_OTA_TARGET_USER_DESC        "OTA Write Point"
#define CUST1_LOADED_APP_USER_DESC        "Loaded Program Version"
#define CUST1_APP_WRITE_USER_DESC         "Application Write"
#define CUST1_APP_READ_USER_DESC          "Application Read"

/// Custom1 Service Data Base Characteristic enum
enum
{
    CUST1_IDX_SVC = 0,

    CUST1_IDX_OTA_TARGET_CHAR,
    CUST1_IDX_OTA_TARGET_VAL,
    CUST1_IDX_OTA_TARGET_USER_DESC,

    CUST1_IDX_LOADED_APP_CHAR,
    CUST1_IDX_LOADED_APP_VAL,
    CUST1_IDX_LOADED_APP_NTF_CFG,
    CUST1_IDX_LOADED_APP_USER_DESC,

    CUST1_IDX_APP_WRITE_CHAR,
    CUST1_IDX_APP_WRITE_VAL,
    CUST1_IDX_APP_WRITE_USER_DESC,

    CUST1_IDX_APP_READ_CHAR,
    CUST1_IDX_APP_READ_VAL,
    CUST1_IDX_APP_READ_NTF_CFG,
    CUST1_IDX_APP_READ_USER_DESC,

    CUST1_IDX_NB
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct attm_desc_128 custs1_att_db[CUST1_IDX_NB];
extern const struct cust_prf_func_callbacks cust_prf_funcs[];

#endif // _USER_CUSTS_CONFIG_H_
