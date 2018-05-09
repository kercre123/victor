/**
 ****************************************************************************************
 *
 * @file user_profiles_config.h
 *
 * @brief Configuration file for the profiles used in the application.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _USER_PROFILES_CONFIG_H_
#define _USER_PROFILES_CONFIG_H_

/**
 ****************************************************************************************
 * @defgroup APP_CONFIG
 * @ingroup APP
 * @brief  Application configuration file
 *
 * This file contains the configuaration of the profiles used by the application.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

/// Add below the profiles that the application wishes to use by including the <profile_name>.h file.

#include "diss.h"
#include "custs1.h"

/*
 * PROFILE CONFIGUARTION
 ****************************************************************************************
 */

/// Add profile specific configurations

/* DISS profile configurations
 * -----------------------------------------------------------------------------------------
 */

#define APP_DIS_MANUFACTURER_NAME       ("Anki")
#define APP_DIS_MANUFACTURER_NAME_LEN   ((sizeof(APP_DIS_MANUFACTURER_NAME)-1))
#define APP_DIS_MODEL_NB_STR            ("DVT4")
#define APP_DIS_MODEL_NB_STR_LEN        ((sizeof(APP_DIS_MODEL_NB_STR)-1))
#define APP_DIS_SW_REV                  DA14580_REFDES_SW_VERSION
#define APP_DIS_FIRM_REV                DA14580_SW_VERSION
#define APP_DIS_FEATURES                (DIS_MANUFACTURER_NAME_CHAR_SUP | DIS_MODEL_NB_STR_CHAR_SUP | DIS_SW_REV_STR_CHAR_SUP | DIS_FIRM_REV_STR_CHAR_SUP)

#endif // _USER_PROFILES_CONFIG_H_
