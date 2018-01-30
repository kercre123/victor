/**
 ****************************************************************************************
 *
 * @file user_proxr.h
 *
 * @brief Proximity Project application header file.
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

#ifndef _USER_PROXR_H_
#define _USER_PROXR_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief User Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Enable push button. Register callback function for button press event.
          Must be called in periph_init();
 * @return void
 ****************************************************************************************
 */
void app_button_enable(void);

/**
 ****************************************************************************************
 * @brief Function to be called on the advertising completion event.
 * @param[in] status GAP Error code
 * @return void
 ****************************************************************************************
 */
void app_advertise_complete(const uint8_t status);

#if (BLE_SPOTA_RECEIVER)
/**
 ****************************************************************************************
 * @brief Function called when the SPOTAR status changes.
 * @param[in] spotar_event SPOTAR_START or SPOTAR_STOP
 * @return void
 ****************************************************************************************
 */
void on_spotar_status_change(const uint8_t spotar_event);
#endif // BLE_SPOTA_RECEIVER

/**
 ****************************************************************************************
 * @brief Disconnection function.
 * @param[in] param Pointer to GAPC_DISCONNECT_IND message
 * @return void
 ****************************************************************************************
*/
void user_app_on_disconnect(struct gapc_disconnect_ind const *param);

/// @} APP

#endif // _USER_PROXR_H_
