/**
 ****************************************************************************************
 *
 * @file user_callback_config.h
 *
 * @brief Callback functions configuration file.
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

#ifndef _USER_CALLBACK_CONFIG_H_
#define _USER_CALLBACK_CONFIG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "app_api.h" 
#include "app_findme.h"
#include "app_proxr.h"
#include "app_spotar.h"

#include "user_empty_peripheral_template.h"
#include "app_prf_types.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Function to be called on the advertising completion event.
 * @param[in] uint8_t GAP Error code
 * @return void
 ****************************************************************************************
 */
void app_advertise_complete(const uint8_t);

/**
 ****************************************************************************************
 * @brief SPOTAR session start or stop event handler.
 * @param[in] spotar_event SPOTAR_START/SPOTAR_STOP
 * @return void
 ****************************************************************************************
 */
void on_spotar_status_change(const uint8_t spotar_event);

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static const struct profile_callbacks user_profile_callbacks = {
    .on_batt_level_upd_cfm      = NULL,
    .on_batt_level_ntf_cfg_ind  = NULL,

#if (BLE_FINDME_TARGET)
    .on_findt_alert_ind         = default_findt_alert_ind_hnd,
#endif

#if (BLE_PROX_REPORTER)
    .on_proxr_alert_ind         = default_proxr_alert_ind_hnd,
#endif

#if (BLE_SPOTA_RECEIVER)
    .on_spotar_status_change    = on_spotar_status_change,
#endif
};

static const struct app_callbacks user_app_callbacks = {
    .app_on_connection              = user_on_connection,
    .app_on_disconnect              = user_on_disconnect,
    .app_on_update_params_rejected  = NULL,
    .app_on_update_params_complete  = NULL,
    .app_on_set_dev_config_complete = default_app_on_set_dev_config_complete,
    .app_on_adv_nonconn_complete    = NULL,
    .app_on_adv_undirect_complete   = NULL,
    .app_on_adv_direct_complete     = NULL,
    .app_on_db_init_complete        = default_app_on_db_init_complete,
    .app_on_scanning_completed      = NULL,
    .app_on_adv_report_ind          = NULL,
#if (BLE_APP_SEC)
    .app_on_pairing_request         = default_app_on_pairing_request,
    .app_on_tk_exch_nomitm          = default_app_on_tk_exch_nomitm,
    .app_on_irk_exch                = NULL,
    .app_on_csrk_exch               = default_app_on_csrk_exch,
    .app_on_ltk_exch                = default_app_on_ltk_exch,
    .app_on_pairing_succeded        = NULL,
    .app_on_encrypt_ind             = NULL,
    .app_on_mitm_passcode_req       = NULL,
    .app_on_encrypt_req_ind         = default_app_on_encrypt_req_ind,
    .app_on_security_req_ind        = NULL,
#endif // (BLE_APP_SEC)
};

static const struct default_app_operations user_default_app_operations = {
    .default_operation_adv = default_advertise_operation,
};

static void (*const app_process_catch_rest_cb)(ke_msg_id_t const msgid, void const *param,
                                         ke_task_id_t const dest_id, ke_task_id_t const src_id) = NULL;

static const struct arch_main_loop_callbacks user_app_main_loop_callbacks = {
    .app_on_init            = default_app_on_init,
    
    // By default the watchdog timer is reloaded and resumed when the system wakes up.
    // The user has to take into account the watchdog timer handling (keep it running, 
    // freeze it, reload it, resume it, etc), when the app_on_ble_powered() is being 
    // called and may potentially affect the main loop.
    .app_on_ble_powered     = NULL,
    
    // By default the watchdog timer is reloaded and resumed when the system wakes up.
    // The user has to take into account the watchdog timer handling (keep it running, 
    // freeze it, reload it, resume it, etc), when the app_on_system_powered() is being 
    // called and may potentially affect the main loop.
    .app_on_system_powered  = NULL,
    
    .app_before_sleep       = NULL,
    .app_validate_sleep     = NULL,
    .app_going_to_sleep     = NULL,
    .app_resume_from_sleep  = NULL,
};

//place in this structure the app_<profile>_db_create and app_<profile>_enable functions
//for SIG profiles that do not have this function already implemented in the SDK
//or if you want to override the functionality. Check the prf_func array in the SDK
//for your reference of which profiles are supported.
static const struct prf_func_callbacks user_prf_funcs[] =
{
    {TASK_NONE,    NULL, NULL}   // DO NOT MOVE. Must always be last
};

#endif // _USER_CALLBACK_CONFIG_H_
