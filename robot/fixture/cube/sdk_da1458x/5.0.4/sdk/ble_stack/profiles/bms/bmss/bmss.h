/**
 ****************************************************************************************
 *
 * @file bmss.h
 *
 * @brief Header file - Bond Management Service Server Declaration.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _BMSS_H_
#define _BMSS_H_

/**
 ****************************************************************************************
 * @addtogroup BMSS Bond Management 'Profile' Server
 * @ingroup BMSS
 * @brief Bond Management 'Profile' Server
 * @{
 ****************************************************************************************
 */

/// Bond Management Server Role
#define BLE_BM_SERVER               1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BM_SERVER)

#include "ke_task.h"
#include "bms_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define BMS_CTRL_PT_MAX_LEN                         (512)
#define BMS_FEAT_MAX_LEN                            (4)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Bond Management Service Attribute Table Indexes
enum
{
    /// Bond Management Service
    BMS_IDX_SVC,
    /// Bond Management Control Point
    BMS_IDX_BM_CTRL_PT_CHAR,
    BMS_IDX_BM_CTRL_PT_VAL,
    BMS_IDX_BM_CTRL_PT_EXT_PROP,
    /// Bond Management Feature
    BMS_IDX_BM_FEAT_CHAR,
    BMS_IDX_BM_FEAT_VAL,

    /// Number of attributes
    BMS_IDX_NB,
};

enum
{
    BMS_CTRL_PT_CHAR,
    BMS_FEAT_CHAR,

    BMS_CHAR_MAX
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Bond Management 'Profile' Server environment variable
struct bmss_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    /// BMS Start Handles
    uint16_t shdl;

    /// Database configuration
    uint32_t features;
    uint8_t reliable_writes;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Full BMS Database Description
extern const struct attm_desc bms_att_db[BMS_IDX_NB];

/// Bond Management Service
extern const att_svc_desc_t bms_svc;

/// Bond Management Service Control Point Characteristic
extern const struct att_char_desc bms_ctrl_pt_char;

/// Bond Management Service Feature Characteristic
extern const struct att_char_desc bms_feat_char;

/// Bond Management 'Profile' Server Environment
extern struct bmss_env_tag bmss_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BMSS module.
 * This function performs all the initializations of the BMSS module.
 * @return void
 ****************************************************************************************
 */
void bmss_init(void);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to
 * requester task
 * @param[in] conhdl Connection handle
 * @return void
 ****************************************************************************************
 */
void bmss_disable(uint16_t conhdl);

#endif // BLE_BM_SERVER

/// @} BMSS

#endif // _BMSS_H_
