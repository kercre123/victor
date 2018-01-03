/**
 ****************************************************************************************
 *
 * @file user_modules_config.h
 *
 * @brief User modules configuration file.
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

#ifndef _USER_MODULES_CONFIG_H_
#define _USER_MODULES_CONFIG_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief User modules configuration.
 *
 * @{
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/***************************************************************************************/
/* Exclude or not a module in user's application code.                                 */
/*                                                                                     */
/* (0) - The module is included. The module's messages are handled by the SDK.         */
/*                                                                                     */
/* (1) - The module is excluded. The user must handle the module's messages.           */
/*                                                                                     */
/* Note:                                                                               */
/*      This setting has no effect if the respective module is a BLE Profile           */
/*      that is not used included in the user's application.                           */
/***************************************************************************************/
#define EXCLUDE_DLG_GAP             (0)
#define EXCLUDE_DLG_TIMER           (0)
#define EXCLUDE_DLG_MSG             (1)
#define EXCLUDE_DLG_SEC             (1)
#define EXCLUDE_DLG_DISS            (0)
#define EXCLUDE_DLG_PROXR           (1)
#define EXCLUDE_DLG_BASS            (1)
#define EXCLUDE_DLG_FINDL           (1)
#define EXCLUDE_DLG_FINDT           (1)
#define EXCLUDE_DLG_SPOTAR          (1)
#define EXCLUDE_DLG_CUSTS1          (0)
#define EXCLUDE_DLG_CUSTS2          (1)

/// @} APP

#endif // _USER_MODULES_CONFIG_H_
