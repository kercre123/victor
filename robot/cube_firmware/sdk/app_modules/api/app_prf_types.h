/**
 ****************************************************************************************
 *
 * @file app_prf_types.h
 *
 * @brief app - profiles related header file.
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

#ifndef APP_PRF_TYPES_H_
#define APP_PRF_TYPES_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#include "attm_db_128.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

 /// Format of a profile initialisation function
typedef void (*prf_func_void_t)(void);
typedef void (*prf_func_uint16_t)(uint16_t);
typedef uint8_t (*prf_func_validate_t) (uint16_t, bool, uint16_t, uint16_t, uint8_t*);

/// Structure of profile call back function table.
struct prf_func_callbacks
{
    /// Profile Task ID.
    enum KE_TASK_TYPE       task_id;
    /// Pointer to the database cteate function
    prf_func_void_t         db_create_func;
    /// Pointer to the profile enable function
    prf_func_uint16_t       enable_func;
};

/// Structure of custom profile call back function table.
struct cust_prf_func_callbacks
{
    /// Profile Task ID.
    enum KE_TASK_TYPE       task_id;
    /// pointer to the custom database table defined by user
    const struct attm_desc_128 *att_db;
    /// max number of attributes in custom database
    const uint8_t max_nb_att;
    /// Pointer to the custom database create function defined by user
    prf_func_void_t         db_create_func;
    /// Pointer to the custom profile enable function defined by user
    prf_func_uint16_t       enable_func;
    /// Pointer to the custom profile initialization function
    prf_func_void_t         init_func;
    /// Pointer to the validation function defined by user
    prf_func_validate_t     value_wr_validation_func;
};

extern const struct prf_func_callbacks prf_funcs[];

/// @} APP

#endif //APP_PRF_TYPES_H_
