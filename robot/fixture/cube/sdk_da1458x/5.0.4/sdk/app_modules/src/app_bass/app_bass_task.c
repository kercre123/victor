/**
 ****************************************************************************************
 *
 * @file app_bass_task.c
 *
 * @brief Battery server application task.
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
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_APP_PRESENT)

#if (BLE_BAS_SERVER)

#include "app_task.h"                  // Application Task API
#include "bass_task.h"
#include "app_bass.h"
#include "gpio.h"
#include "user_callback_config.h"
#include "app_entry_point.h"
#include "app.h"

/*
 * EXTERNAL VARIABLES
 ****************************************************************************************
 */

extern uint16_t bat_poll_timeout;
extern uint8_t bat_lvl_alert_used;
extern GPIO_PORT bat_led_port;
extern GPIO_PIN bat_led_pin;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles the Create DB confirmation message for the Battery Service.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bass_create_db_cfm_handler(ke_msg_id_t const msgid,
                                      struct bass_create_db_cfm const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Inform the Application Manager
    struct app_module_init_cmp_evt *cfm = KE_MSG_ALLOC(APP_MODULE_INIT_CMP_EVT,
                                                       TASK_APP,
                                                       TASK_APP,
                                                       app_module_init_cmp_evt);

    cfm->status = param->status;

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles the confirmation for the Battery Level update.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bass_batt_level_upd_cfm_handler(ke_msg_id_t const msgid,
                                           struct bass_batt_level_upd_cfm const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{

    EXECUTE_PROFILE_CALLBACK_PARAM1_PARAM2(on_batt_level_upd_cfm, gapc_get_conidx(param->conhdl), param->status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles the disable indication.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bass_disable_handler(ke_msg_id_t const msgid,
                                struct bass_disable_ind const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    app_batt_poll_stop();

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles the indication for the modification of the notification status.
 * @param[in] msgid     Id of the message received.
 * @param[in] ind       Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bass_batt_level_ntf_cfg_ind_handler(ke_msg_id_t const msgid,
                                               struct bass_batt_level_ntf_cfg_ind const *ind,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
    EXECUTE_PROFILE_CALLBACK_PARAM1_PARAM2_PARAM3(on_batt_level_ntf_cfg_ind,
                                                  gapc_get_conidx(ind->conhdl),
                                                  ind->ntf_cfg,
                                                  ind->bas_instance);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles Battery Level polling timer
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_bass_timer_handler(ke_msg_id_t const msgid,
                                  void const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    app_batt_lvl();

    app_timer_set(APP_BASS_TIMER, dest_id, bat_poll_timeout);

    return (KE_MSG_CONSUMED);
}

#ifndef CUSTOM_BATTERY_LEVEL_ALERT_LED_HANDLING
/**
 ****************************************************************************************
 * @brief Handles Battery Alert time.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_bass_alert_timer_handler(ke_msg_id_t const msgid,
                                        void const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Read LED GPIO state
    if (bat_lvl_alert_used)
    {
        if (bat_led_state)
        {
            GPIO_SetInactive(bat_led_port, bat_led_pin);
            bat_led_state = 0;
            app_timer_set(APP_BASS_ALERT_TIMER, dest_id, 20);
        }
        else
        {
            GPIO_SetActive(bat_led_port, bat_led_pin);
            bat_led_state = 1;
            app_timer_set(APP_BASS_ALERT_TIMER, dest_id, 5);
        }
    }
    return (KE_MSG_CONSUMED);
}
#endif // CUSTOM_BATTERY_LEVEL_ALERT_LED_HANDLING

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static const struct ke_msg_handler app_bass_process_handlers[]=
{
    {BASS_CREATE_DB_CFM,                    (ke_msg_func_t)bass_create_db_cfm_handler},
    {BASS_BATT_LEVEL_UPD_CFM,               (ke_msg_func_t)bass_batt_level_upd_cfm_handler},
    {BASS_BATT_LEVEL_NTF_CFG_IND,           (ke_msg_func_t)bass_batt_level_ntf_cfg_ind_handler},
    {APP_BASS_TIMER,                        (ke_msg_func_t)app_bass_timer_handler},
    {APP_BASS_ALERT_TIMER,                  (ke_msg_func_t)app_bass_alert_timer_handler},
    {BASS_DISABLE_IND,                      (ke_msg_func_t)bass_disable_handler},
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_bass_process_handler(ke_msg_id_t const msgid,
                                                     void const *param,
                                                     ke_task_id_t const dest_id,
                                                     ke_task_id_t const src_id,
                                                     enum ke_msg_status_tag *msg_ret)
{
    return app_std_process_event(msgid, param,src_id, dest_id, msg_ret, app_bass_process_handlers,
                                         sizeof(app_bass_process_handlers) / sizeof(struct ke_msg_handler));
}

#endif //(BLE_BAS_SERVER)

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
