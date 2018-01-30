/**
 ****************************************************************************************
 *
 * @file app_proxr_task.c
 *
 * @brief Proximity Reporter application Task Implementation.
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

#include "rwble_config.h"              // SW configuration

#if (BLE_APP_PRESENT)

#if (BLE_PROX_REPORTER)

#include "app_proxr.h"
#include "app_proxr_task.h"
#include "app_task.h"                  // Application Task API
#include "gpio.h"
#include "arch_api.h"
#include "app_entry_point.h"
#include "user_callback_config.h"
#include "app.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles proximity reporter's profile database creation confirmation.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxr_create_db_cfm_handler(ke_msg_id_t const msgid,
                                       struct proxr_create_db_cfm const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // If not in GATT DB initialization state, ignore the message
    if (ke_state_get(dest_id) == APP_DB_INIT)
    {
        // Inform the Application Manager
        struct app_module_init_cmp_evt *cfm = KE_MSG_ALLOC(APP_MODULE_INIT_CMP_EVT,
                                                           TASK_APP, TASK_APP,
                                                           app_module_init_cmp_evt);

        cfm->status = param->status;

        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disable indication from the Proximity Reporter profile.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxr_disable_ind_handler(ke_msg_id_t const msgid,
                                     struct proxr_disable_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    alert_state.ll_alert_lvl = param->lls_alert_lvl;

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles LLS Alert indication from proximity reporter profile
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxr_lls_alert_ind_handler(ke_msg_id_t const msgid,
                                       struct proxr_lls_alert_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    EXECUTE_PROFILE_CALLBACK_PARAM1_PARAM2(on_proxr_lls_alert_ind,
                                           gapc_get_conidx(param->conhdl),
                                           param->alert_lvl);
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles LLS or IAS Alert level value update by peer.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxr_level_upd_ind_handler(ke_msg_id_t const msgid,
                                       struct proxr_level_upd_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    EXECUTE_PROFILE_CALLBACK_PARAM1_PARAM2_PARAM3(on_proxr_level_upd_ind,
                                                  gapc_get_conidx(param->conhdl),
                                                  param->alert_lvl,
                                                  param->char_code);
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles proximity alert timer.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_proxr_timer_handler(ke_msg_id_t const msgid,
                                   void const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    if (alert_state.blink_toggle)
    {
        GPIO_SetInactive(alert_state.port, alert_state.pin);
        alert_state.blink_toggle = 0;
    }
    else
    {
        GPIO_SetActive(alert_state.port, alert_state.pin);
        alert_state.blink_toggle = 1;
    }

    ke_timer_set(APP_PROXR_TIMER, dest_id, alert_state.blink_timeout);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static const struct ke_msg_handler app_proxr_process_handlers[] =
{
    {APP_PROXR_TIMER,                         (ke_msg_func_t)app_proxr_timer_handler},
    {PROXR_CREATE_DB_CFM,                   (ke_msg_func_t)proxr_create_db_cfm_handler},
    {PROXR_DISABLE_IND,                     (ke_msg_func_t)proxr_disable_ind_handler},
    {PROXR_LLS_ALERT_IND,                   (ke_msg_func_t)proxr_lls_alert_ind_handler},
    {PROXR_LEVEL_UPD_IND,                   (ke_msg_func_t)proxr_level_upd_ind_handler},
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_proxr_process_handler(ke_msg_id_t const msgid,
                                                      void const *param,
                                                      ke_task_id_t const dest_id,
                                                      ke_task_id_t const src_id,
                                                      enum ke_msg_status_tag *msg_ret)
{
    return app_std_process_event(msgid, param, src_id, dest_id, msg_ret,
                                 app_proxr_process_handlers,
                                 sizeof(app_proxr_process_handlers) / sizeof(struct ke_msg_handler));
}

#endif //BLE_PROX_REPORTER

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
