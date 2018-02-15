/**
 ****************************************************************************************
 *
 * @file app_bass.c
 *
 * @brief Battery server application.
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

#include "rwip_config.h"

#if BLE_BAS_SERVER
#include "app_task.h"                // application task definitions
#include "bass_task.h"
#include "app_bass.h"
#include "gpio.h"
#include "battery.h"
#include "user_periph_setup.h"
#include "app_prf_perm_types.h"
#include "app.h"

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

uint16_t bat_poll_timeout     __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
uint8_t cur_batt_level        __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
uint8_t batt_alert_en         __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
uint8_t bat_led_state         __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
uint8_t bat_lvl_alert_used    __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
GPIO_PORT bat_led_port        __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
GPIO_PIN bat_led_pin          __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
static uint16_t active_conhdl __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_bass_create_db(void)
{
    // Add BASS in the database
    struct bass_create_db_req * req = KE_MSG_ALLOC(BASS_CREATE_DB_REQ, TASK_BASS,
                                                   TASK_APP, bass_create_db_req);

    req->bas_nb = 1;
    req->features[0] = BAS_BATT_LVL_NTF_SUP;
#ifndef USE_ONE_BAS_INSTANCE
    req->features[1] = BAS_BATT_LVL_NTF_NOT_SUP;
#endif //USE_ONE_BAS_INSTANCE

    // Send the message
    ke_msg_send(req);
}

void app_batt_init(void)
{
    // called from app.c::app_init() when chip boots
#ifdef USED_BATTERY_TYPE
    cur_batt_level = battery_get_lvl(USED_BATTERY_TYPE);
#else
    cur_batt_level = battery_get_lvl(BATT_CR2032);
#endif
}

void app_batt_config(uint8_t batt_lvl, uint8_t lvl_alert_used, GPIO_PORT led_port, GPIO_PIN led_pin)
{
    bat_lvl_alert_used  = lvl_alert_used;
    bat_led_port        = led_port;
    bat_led_pin         = led_pin;
    cur_batt_level      = batt_lvl;
}

void app_bass_enable(uint16_t conhdl)
{
    // Allocate the message
    struct bass_enable_req * req = KE_MSG_ALLOC(BASS_ENABLE_REQ, TASK_BASS, TASK_APP,
                                                bass_enable_req);
    // Fill in the parameter structure
    active_conhdl       = conhdl;
    req->conhdl         = conhdl;
    req->sec_lvl        = get_user_prf_srv_perm(TASK_BASS);
    req->con_type       = PRF_CON_NORMAL;   // PRF_CON_DISCOVERY;
    req->batt_level_ntf_cfg[0]  = 0;        // Notification OFF by default.
    req->old_batt_lvl[0]        = cur_batt_level;
    req->current_batt_lvl[0]    = cur_batt_level;
#ifndef USE_ONE_BAS_INSTANCE
    req->batt_level_ntf_cfg[1]  = 0;
    req->old_batt_lvl[1]        = 0;
    req->current_batt_lvl[1]    = 0;
#endif //USE_ONE_BAS_INSTANCE

    struct prf_char_pres_fmt *batt_level_pres_format = req->batt_level_pres_format;

    batt_level_pres_format[0].unit = 0x27AD;
    batt_level_pres_format[0].description = 0;
    batt_level_pres_format[0].format = 4;
    batt_level_pres_format[0].exponent = 0;
    batt_level_pres_format[0].name_space = 1;

    // Send the message
    ke_msg_send(req);

    app_batt_config(0, USE_BAT_LEVEL_ALERT, GPIO_BAT_LED_PORT, GPIO_BAT_LED_PIN);
    app_batt_poll_start(APP_BASS_POLL_INTERVAL);
}

void app_batt_lvl(void)
{
    uint8_t batt_lvl;

#if defined(USED_BATTERY_TYPE)
    batt_lvl = battery_get_lvl(USED_BATTERY_TYPE);
#else
    batt_lvl = battery_get_lvl(BATT_CR2032);
#endif

    if (batt_lvl != cur_batt_level)
    {
        app_batt_set_level(batt_lvl);
    }

    //update old_batt_lvl for the next use
    cur_batt_level = batt_lvl;

#if defined(BATTERY_ALERT_AT_PERCENTAGE_LEFT)
    if(bat_lvl_alert_used)
    {
      if (batt_lvl < BATTERY_ALERT_AT_PERCENTAGE_LEFT)
      {
          app_batt_alert_start(); // Start battery level alert
      }

      if (batt_lvl > BATTERY_ALERT_AT_PERCENTAGE_LEFT + 5)
      {
          app_batt_alert_stop();  // Stop battery level alert
      }
    }
#else
    //if battery level is below 5% then start a battery alert to notify user
    if(bat_lvl_alert_used && batt_lvl < 5)
    {
        app_batt_alert_start();
    }
#endif //defined(BATTERY_ALERT_AT_PERCENTAGE_LEFT)
}

void app_batt_set_level(uint8_t batt_lvl)
{
    // Allocate the message
    struct bass_batt_level_upd_req * req = KE_MSG_ALLOC(BASS_BATT_LEVEL_UPD_REQ, TASK_BASS, TASK_APP,
                                                        bass_batt_level_upd_req);

    // Fill in the parameter structure
    req->conhdl = active_conhdl;
    req->bas_instance = 0;
    req->batt_level = batt_lvl;

    // Send the message
    ke_msg_send(req);
}

void app_batt_poll_start(uint16_t poll_timeout)
{
    bat_poll_timeout = poll_timeout;

    app_timer_set(APP_BASS_TIMER, TASK_APP, 10);    //first poll in 100 ms
}

void app_batt_poll_stop(void)
{
    ke_timer_clear(APP_BASS_TIMER, TASK_APP);
}

void app_batt_alert_start(void)
{
    batt_alert_en = 1;

#ifndef CUSTOM_BATTERY_LEVEL_ALERT_LED_HANDLING
    if (bat_lvl_alert_used)
    {
        GPIO_SetActive( bat_led_port, bat_led_pin);
    }
    bat_led_state = 1;
    app_timer_set(APP_BASS_ALERT_TIMER, TASK_APP, 200);
#else
    if (bat_lvl_alert_used)
    {
        leds_set_battery_low_warning();
    }
#endif //CUSTOM_BATTERY_LEVEL_ALERT_LED_HANDLING
}

void app_batt_alert_stop(void)
{
    batt_alert_en = 0;

#ifndef CUSTOM_BATTERY_LEVEL_ALERT_LED_HANDLING
    if (bat_lvl_alert_used)
    {
        GPIO_SetInactive(bat_led_port, bat_led_pin);
    }
    ke_timer_clear(APP_BASS_ALERT_TIMER, TASK_APP);
    bat_led_state = 0;
#else
    if (bat_lvl_alert_used)
    {
        leds_clear_battery_low_warning();
    }
#endif //CUSTOM_BATTERY_LEVEL_ALERT_LED_HANDLING
}

void app_batt_port_reinit(void)
{
#ifndef CUSTOM_BATTERY_LEVEL_ALERT_LED_HANDLING
    if (bat_lvl_alert_used)
    {
        if(bat_led_state == 1)
        {
            GPIO_SetActive(bat_led_port, bat_led_pin);
        }
        else
        {
            GPIO_SetInactive(bat_led_port, bat_led_pin);
        }
    }
#endif //CUSTOM_BATTERY_LEVEL_ALERT_LED_HANDLING
}

#endif //BLE_BAS_SERVER

/// @} APP
