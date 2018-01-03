/**
 ****************************************************************************************
 *
 * @file app_proxr.h
 *
 * @brief Proximity Reporter application.
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

#ifndef _APP_PROXR_H_
#define _APP_PROXR_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Proximity Reporter Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "user_profiles_config.h"

#if BLE_PROX_REPORTER
#include "gpio.h"
#include "proxr.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef struct
{
    uint32_t blink_timeout;

    uint8_t blink_toggle;

    uint8_t lvl;

    uint8_t ll_alert_lvl;

    int8_t  txp_lvl;

    GPIO_PORT port;

    GPIO_PIN pin;
} app_alert_state;

extern app_alert_state alert_state;

/*
 * DEFINES
 ****************************************************************************************
 */

enum proxr_alert_ind_char_code
{
    PROXR_ALERT_IAS = PROXR_IAS_CHAR,
    PROXR_ALERT_LLS = PROXR_LLS_CHAR
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize Proximity Reporter application.
 * @return void
 ****************************************************************************************
 */
void app_proxr_init(void);

/**
 ****************************************************************************************
 *
 * Proximity Application Functions
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Enable the proximity profile.
 * @param[in] conhdl Connection handle
 * @return void
 ****************************************************************************************
 */
void app_proxr_enable(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Start proximity Alert.
 * @param[in] lvl Alert level. Mild or High
 * @return void
 ****************************************************************************************
 */
void app_proxr_alert_start(uint8_t lvl);

/**
 ****************************************************************************************
 * @brief Stop proximity Alert.
 * @return void
 ****************************************************************************************
 */
void app_proxr_alert_stop(void);

/**
 ****************************************************************************************
 * @brief Create proximity reporter Database.
 * @return void
 ****************************************************************************************
 */
void app_proxr_create_db(void);

/**
 ****************************************************************************************
 * @brief Reinit of proximity reporter LED pins and push button. Called by periph_init().
 * param[in] port GPIO port
 * param[in] pin  GPIO pin
 * @return void
 ****************************************************************************************
 */
void app_proxr_port_reinit(GPIO_PORT port, GPIO_PIN pin);

/**
 ****************************************************************************************
 * @brief Default application handler for handling LLS or IAS alert level updates.
 * param[in] connection_idx     Connection index
 * param[in] alert_lvl          Alert level
 * param[in] char_code          Characteristic code: PROXR_LLS_CHAR or PROXR_IAS_CHAR
 * @return void
 ****************************************************************************************
 */
void default_proxr_level_upd_ind_handler(uint8_t connection_idx, uint8_t alert_lvl, uint8_t char_code);

/**
 ****************************************************************************************
 * @brief Default application handler for handling a request to start an alert on the
 *        device.
 * param[in] connection_idx     Connection index
 * param[in] alert_lvl          Alert level
 * @return void
 ****************************************************************************************
 */
void default_proxr_lls_alert_ind_handler(uint8_t connection_idx, uint8_t alert_lvl);

#endif // (BLE_PROX_REPORTER)

/// @} APP

#endif // _APP_PROXR_H_
