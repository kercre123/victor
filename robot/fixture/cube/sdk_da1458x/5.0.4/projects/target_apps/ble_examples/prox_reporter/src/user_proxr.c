/**
 ****************************************************************************************
 *
 * @file user_proxr.c
 *
 * @brief Proximity reporter project source code.
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

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#include "user_periph_setup.h"
#include "wkupct_quadec.h"
#include "app_easy_msg_utils.h"
#include "gpio.h"
#include "user_callback_config.h"

#include "app_security.h"
#include "user_proxr.h"
#include "arch_api.h"
#include "app_task.h"
#include "app_proxr.h"

#if (BLE_SPOTA_RECEIVER)
#include "app_spotar.h"
#if defined(__DA14583__) && (!SPOTAR_SPI_DISABLE) 
#include "spi_flash.h"
#endif
#endif

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/

/**
 ****************************************************************************************
 * @brief Handles APP_WAKEUP_MSG sent when device exits deep sleep. Trigerred by button press.
 * @return void
 ****************************************************************************************
*/
static void app_wakeup_cb(void)
{
    // If state is not idle, ignore the message
    if (ke_state_get(TASK_APP) == APP_CONNECTABLE)
    {
        default_advertise_operation();
    }
}

/**
 ****************************************************************************************
 * @brief Button press callback function. Registered in WKUPCT driver.
 * @return void
 ****************************************************************************************
 */
static void app_button_press_cb(void)
{
#if (BLE_PROX_REPORTER)
    if (alert_state.lvl != PROXR_ALERT_NONE)
    {
        app_proxr_alert_stop();
    }
#endif

    if (GetBits16(SYS_STAT_REG, PER_IS_DOWN))
    {
        periph_init();
    }

    if (arch_ble_ext_wakeup_get())
    {
        arch_set_sleep_mode(app_default_sleep_mode);
        arch_ble_force_wakeup();
        arch_ble_ext_wakeup_off();
        app_easy_wakeup();
    }

    app_button_enable();
}

void app_button_enable(void)
{
    app_easy_wakeup_set(app_wakeup_cb);
    wkupct_register_callback(app_button_press_cb);
#if USE_PUSH_BUTTON
    if (GPIO_GetPinStatus(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN))
    {
        wkupct_enable_irq(WKUPCT_PIN_SELECT(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN), // select pin (GPIO_BUTTON_PORT, GPIO_BUTTON_PIN)
                          WKUPCT_PIN_POLARITY(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN, WKUPCT_PIN_POLARITY_LOW), // polarity low
                          1, // 1 event
                          0); // debouncing time = 0
    }
#endif // USE_PUSH_BUTTON
}

#if (BLE_SPOTA_RECEIVER)
void on_spotar_status_change(const uint8_t spotar_event)
{
#if defined(__DA14583__) && (!SPOTAR_SPI_DISABLE)
    int8_t man_dev_id = 0;

    man_dev_id = spi_flash_enable(SPI_EN_GPIO_PORT, SPI_EN_GPIO_PIN);
    if (man_dev_id == SPI_FLASH_AUTO_DETECT_NOT_DETECTED)
    {
        // The device was not identified. The default parameters are used.
        // Alternatively, an error can be asserted here.
        spi_flash_init(SPI_FLASH_DEFAULT_SIZE, SPI_FLASH_DEFAULT_PAGE);
    }

    if (spotar_event == SPOTAR_END)
    {
        // Power down SPI Flash
        spi_flash_power_down();
    }
#endif
}
#endif

void user_app_on_disconnect(struct gapc_disconnect_ind const *param)
{
    default_app_on_disconnect(NULL);
    
#if (BLE_SPOTA_RECEIVER)
    // Issue a platform reset when it is requested by the spotar procedure
    if (spota_state.reboot_requested)
    {
        // Reboot request will be served
        spota_state.reboot_requested = 0;

        // Platform reset
        platform_reset(RESET_AFTER_SPOTA_UPDATE);
    }
#endif
}

void app_advertise_complete(const uint8_t status)
{
#if BLE_PROX_REPORTER
    app_proxr_alert_stop();
#endif // BLE_PROX_REPORTER

    // Disable wakeup for BLE and timer events. Only external (GPIO) wakeup events can wakeup processor.
    if (status == GAP_ERR_CANCELED)
    {
        arch_ble_ext_wakeup_on();
    }
    app_button_enable();
}
/// @} APP
