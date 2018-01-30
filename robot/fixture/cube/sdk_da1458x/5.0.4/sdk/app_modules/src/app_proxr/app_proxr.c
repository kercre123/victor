/**
 ****************************************************************************************
 *
 * @file app_proxr.c
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

#include "rwble_config.h"

#if (BLE_PROX_REPORTER)
#include "app_api.h"                // application task definitions
#include "proxr_task.h"              // proximity functions
#include "gpio.h"
#include "wkupct_quadec.h"
#include "app_proxr.h"
#include "app_prf_perm_types.h"
#include "user_periph_setup.h"

//application allert state structrure
app_alert_state alert_state __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_proxr_init()
{
    alert_state.port = GPIO_ALERT_LED_PORT;
    alert_state.pin = GPIO_ALERT_LED_PIN;
    alert_state.ll_alert_lvl = PROXR_ALERT_HIGH; // Link Loss default Alert Level
    alert_state.txp_lvl = 0x00;                  // TX power level indicator
}

void app_proxr_enable(uint16_t conhdl)
{
    // Allocate the message
    struct proxr_enable_req * req = KE_MSG_ALLOC(PROXR_ENABLE_REQ, TASK_PROXR, TASK_APP,
                                                 proxr_enable_req);

    // init application alert state
    app_proxr_alert_stop();

    // Fill in the parameter structure
    req->conhdl = conhdl;
    req->sec_lvl = get_user_prf_srv_perm(TASK_PROXR);
    req->lls_alert_lvl = (uint8_t)alert_state.ll_alert_lvl;
    req->txp_lvl = alert_state.txp_lvl;

    // Send the message
    ke_msg_send(req);
}

void app_proxr_alert_start(uint8_t lvl)
{
    alert_state.lvl = lvl;

    if (alert_state.lvl == PROXR_ALERT_MILD)
    {
        alert_state.blink_timeout = 150;
    }
    else
    {
        alert_state.blink_timeout = 50;
    }

    alert_state.blink_toggle = 1;
    GPIO_SetActive( alert_state.port, alert_state.pin);

    ke_timer_set(APP_PROXR_TIMER, TASK_APP, alert_state.blink_timeout);
}

void app_proxr_alert_stop(void)
{
    alert_state.lvl = PROXR_ALERT_NONE; //level;

    alert_state.blink_timeout = 0;
    alert_state.blink_toggle = 0;

    GPIO_SetInactive( alert_state.port, alert_state.pin);

    ke_timer_clear(APP_PROXR_TIMER, TASK_APP);
}

void app_proxr_create_db(void)
{
    // Add PROXR in the database
    struct proxr_create_db_req *req = KE_MSG_ALLOC(PROXR_CREATE_DB_REQ,
                                                  TASK_PROXR, TASK_APP,
                                                  proxr_create_db_req);

    req->features = PROXR_IAS_TXPS_SUP;

    ke_msg_send(req);
}

void app_proxr_port_reinit(GPIO_PORT port, GPIO_PIN pin)
{
    alert_state.port = port;
    alert_state.pin = pin;

    if (alert_state.blink_toggle == 1)
    {
        GPIO_SetActive(alert_state.port, alert_state.pin);
    }
    else
    {
        GPIO_SetInactive(alert_state.port, alert_state.pin);
    }
}

/**
 ****************************************************************************************
 * @brief Update proximity application alert.
 * @param[in] lvl     Alert level: PROXR_ALERT_NONE, PROXR_ALERT_MILD or PROXR_ALERT_HIGH
 * @return void
 ****************************************************************************************
 */
static void alert_update(uint8_t alert_lvl)
{
    if (alert_lvl)
    {
        app_proxr_alert_start(alert_lvl);
    }
    else
    {
        app_proxr_alert_stop();
    }
}

void default_proxr_level_upd_ind_handler(uint8_t connection_idx, uint8_t alert_lvl, uint8_t char_code)
{
    if (char_code == PROXR_IAS_CHAR)
    {
        alert_update(alert_lvl);
    }
}

void default_proxr_lls_alert_ind_handler(uint8_t connection_idx, uint8_t alert_lvl)
{
    alert_update(alert_lvl);
}

#endif // BLE_PROX_REPORTER

/// @} APP
