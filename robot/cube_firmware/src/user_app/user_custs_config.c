/**
 ****************************************************************************************
 *
 * @file user_custs_config.c
 *
 * @brief Custom1/2 Server (CUSTS1/2) profile database structure and initialization.
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

/**
 ****************************************************************************************
 * @defgroup USER_CONFIG
 * @ingroup USER
 * @brief Custom1/2 Server (CUSTS1/2) profile database structure and initialization.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "prf_types.h"
#include "attm_db_128.h"
#include "app_prf_types.h"
#include "app_customs.h"
#include "user_custs_config.h"

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static att_svc_desc128_t custs1_svc                              = DEF_CUST1_SVC_UUID_128;

static uint8_t CUST1_OTA_TARGET_UUID_128[ATT_UUID_128_LEN]       = DEF_CUST1_OTA_TARGET_UUID_128;
static uint8_t CUST1_LOADED_APP_UUID_128[ATT_UUID_128_LEN]       = DEF_CUST1_LOADED_APP_UUID_128;
static uint8_t CUST1_APP_WRITE_UUID_128[ATT_UUID_128_LEN]        = DEF_CUST1_APP_WRITE_UUID_128;
static uint8_t CUST1_APP_READ_UUID_128[ATT_UUID_128_LEN]         = DEF_CUST1_APP_READ_UUID_128;

static struct att_char128_desc custs1_ota_target_char       = {ATT_CHAR_PROP_WR,
                                                              {0, 0},
                                                              DEF_CUST1_OTA_TARGET_UUID_128};

static struct att_char128_desc custs1_loaded_app_char       = {ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF,
                                                              {0, 0},
                                                              DEF_CUST1_LOADED_APP_UUID_128};

static struct att_char128_desc custs1_app_write_char        = {ATT_CHAR_PROP_WR,
                                                              {0, 0},
                                                              DEF_CUST1_APP_WRITE_UUID_128};

static struct att_char128_desc custs1_app_read_char         = {ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF,
                                                              {0, 0},
                                                              DEF_CUST1_APP_READ_UUID_128};

static uint16_t att_decl_svc       = ATT_DECL_PRIMARY_SERVICE;
static uint16_t att_decl_char      = ATT_DECL_CHARACTERISTIC;
static uint16_t att_decl_cfg       = ATT_DESC_CLIENT_CHAR_CFG;
static uint16_t att_decl_user_desc = ATT_DESC_CHAR_USER_DESCRIPTION;

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Custom server function callback table
const struct cust_prf_func_callbacks cust_prf_funcs[] =
{
    {   TASK_CUSTS1,
        custs1_att_db,
        CUST1_IDX_NB,
        app_custs1_create_db, app_custs1_enable,
        custs1_init, NULL
    },
    {TASK_NONE, NULL, 0, NULL, NULL, NULL, NULL},   // DO NOT MOVE. Must always be last
};

/// Full CUSTOM1 Database Description - Used to add attributes into the database
struct attm_desc_128 custs1_att_db[CUST1_IDX_NB] =
{
    // CUSTOM1 Service Declaration
    [CUST1_IDX_SVC]                     = {(uint8_t*)&att_decl_svc, ATT_UUID_16_LEN, PERM(RD, ENABLE),
                                            sizeof(custs1_svc), sizeof(custs1_svc), (uint8_t*)&custs1_svc},

    // OTA Target Characteristic Declaration
    [CUST1_IDX_OTA_TARGET_CHAR]      = {(uint8_t*)&att_decl_char, ATT_UUID_16_LEN, PERM(RD, ENABLE),
                                            sizeof(custs1_ota_target_char), sizeof(custs1_ota_target_char), (uint8_t*)&custs1_ota_target_char},

    // OTA Target Characteristic Value
    [CUST1_IDX_OTA_TARGET_VAL]       = {CUST1_OTA_TARGET_UUID_128, ATT_UUID_128_LEN, PERM(WR, ENABLE),
                                            DEF_CUST1_OTA_TARGET_CHAR_LEN, 0, NULL},

    // OTA Target Characteristic User Description
    [CUST1_IDX_OTA_TARGET_USER_DESC] = {(uint8_t*)&att_decl_user_desc, ATT_UUID_16_LEN, PERM(RD, ENABLE),
                                            sizeof(CUST1_OTA_TARGET_USER_DESC) - 1, sizeof(CUST1_OTA_TARGET_USER_DESC) - 1, CUST1_OTA_TARGET_USER_DESC},

    // Loaded App Characteristic Declaration
    [CUST1_IDX_LOADED_APP_CHAR]          = {(uint8_t*)&att_decl_char, ATT_UUID_16_LEN, PERM(RD, ENABLE),
                                            sizeof(custs1_loaded_app_char), sizeof(custs1_loaded_app_char), (uint8_t*)&custs1_loaded_app_char},

    // Loaded App Characteristic Value
    [CUST1_IDX_LOADED_APP_VAL]           = {CUST1_LOADED_APP_UUID_128, ATT_UUID_128_LEN, PERM(RD, ENABLE) | PERM(NTF, ENABLE),
                                            DEF_CUST1_LOADED_APP_CHAR_LEN, 0, NULL},

    // Loaded App Client Characteristic Configuration Descriptor
    [CUST1_IDX_LOADED_APP_NTF_CFG]       = {(uint8_t*)&att_decl_cfg, ATT_UUID_16_LEN, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                            sizeof(uint16_t), 0, NULL},

    // Loaded App Characteristic User Description
    [CUST1_IDX_LOADED_APP_USER_DESC]     = {(uint8_t*)&att_decl_user_desc, ATT_UUID_16_LEN, PERM(RD, ENABLE),
                                            sizeof(CUST1_LOADED_APP_USER_DESC) - 1, sizeof(CUST1_LOADED_APP_USER_DESC) - 1, CUST1_LOADED_APP_USER_DESC},

    // App Write Characteristic Declaration
    [CUST1_IDX_APP_WRITE_CHAR]           = {(uint8_t*)&att_decl_char, ATT_UUID_16_LEN, PERM(RD, ENABLE),
                                            sizeof(custs1_app_write_char), sizeof(custs1_app_write_char), (uint8_t*)&custs1_app_write_char},

    // App Write Characteristic Value
    [CUST1_IDX_APP_WRITE_VAL]            = {CUST1_APP_WRITE_UUID_128, ATT_UUID_128_LEN, PERM(WR, ENABLE),
                                            DEF_CUST1_APP_WRITE_CHAR_LEN, 0, NULL},

    // App Write Characteristic User Description
    [CUST1_IDX_APP_WRITE_USER_DESC]      = {(uint8_t*)&att_decl_user_desc, ATT_UUID_16_LEN, PERM(RD, ENABLE),
                                            sizeof(CUST1_APP_WRITE_USER_DESC) - 1, sizeof(CUST1_APP_WRITE_USER_DESC) - 1, CUST1_APP_WRITE_USER_DESC},

    // App Read Characteristic Declaration
    [CUST1_IDX_APP_READ_CHAR]          = {(uint8_t*)&att_decl_char, ATT_UUID_16_LEN, PERM(RD, ENABLE),
                                            sizeof(custs1_app_read_char), sizeof(custs1_app_read_char), (uint8_t*)&custs1_app_read_char},

    // App Read Characteristic Value
    [CUST1_IDX_APP_READ_VAL]           = {CUST1_APP_READ_UUID_128, ATT_UUID_128_LEN, PERM(RD, ENABLE) | PERM(NTF, ENABLE),
                                            DEF_CUST1_APP_READ_CHAR_LEN, 0, NULL},

    // App Read Client Characteristic Configuration Descriptor
    [CUST1_IDX_APP_READ_NTF_CFG]       = {(uint8_t*)&att_decl_cfg, ATT_UUID_16_LEN, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                            sizeof(uint16_t), 0, NULL},

    // App Read Characteristic User Description
    [CUST1_IDX_APP_READ_USER_DESC]     = {(uint8_t*)&att_decl_user_desc, ATT_UUID_16_LEN, PERM(RD, ENABLE),
                                            sizeof(CUST1_APP_READ_USER_DESC) - 1, sizeof(CUST1_APP_READ_USER_DESC) - 1, CUST1_APP_READ_USER_DESC},

};
