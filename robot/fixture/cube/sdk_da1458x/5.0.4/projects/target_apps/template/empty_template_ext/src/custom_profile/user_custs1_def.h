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

/// Custom1 Service Data Base Characteristic enum
enum
{
    CUST1_IDX_SVC = 0,

    CUST1_IDX_NB
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct attm_desc_128 custs1_att_db[CUST1_IDX_NB];

/// @} USER_CONFIG

#endif // _USER_CUSTS1_DEF_H_
