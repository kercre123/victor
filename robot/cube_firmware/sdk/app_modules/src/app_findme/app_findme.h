/**
 ****************************************************************************************
 *
 * @file app_findme.h
 *
 * @brief Findme Target/Locator application.
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

#ifndef _APP_FINDME_H_
#define _APP_FINDME_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Findme Target/Locator Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "user_profiles_config.h"

#if BLE_FINDME_TARGET
#include "findt_task.h"
#endif

#if BLE_FINDME_LOCATOR
#include "findl_task.h"
#endif

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if BLE_FINDME_TARGET
/**
 ****************************************************************************************
 * @brief Enable Findme Target profile.
 * @param[in] conhdl            The connection handle
 * @return void
 ****************************************************************************************
 */
void app_findt_enable(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Default Findme Target alert indication handler.
 * @param[in] connection_idx    The connection id number
 * @param[in] alert_lvl         The alert level
 * @return void
 ****************************************************************************************
 */
void default_findt_alert_ind_hnd(uint8_t connection_idx, uint8_t alert_lvl);
#endif // BLE_FINDME_TARGET

#if BLE_FINDME_LOCATOR
/**
 ****************************************************************************************
 * @brief Initialize Findme Locator profile.
 * @return void
 ****************************************************************************************
 */
void app_findl_init(void);

/**
 ****************************************************************************************
 * @brief Enable Findme Locator profile.
 * @param[in] conhdl            The connection handle
 * @return void
 ****************************************************************************************
 */
void app_findl_enable(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Send Immediate alert.
 * @return void
 ****************************************************************************************
 */
void app_findl_set_alert(void);
#endif // BLE_FINDME_LOCATOR

/// @} APP

#endif // _APP_FINDME_H_
