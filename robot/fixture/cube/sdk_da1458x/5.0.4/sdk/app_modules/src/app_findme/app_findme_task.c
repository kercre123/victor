/**
 ****************************************************************************************
 *
 * @file app_findme_task.c
 *
 * @brief Findme locator and target application task.
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

#include "rwip_config.h"               // SW configuration

#if BLE_APP_PRESENT
#include "findl_task.h"                 // Application Task API
#include "findt_task.h"                 // Application Task API
#include "app_findme_task.h"            // Application Task API
#include "app_findme.h"                 // Application Task API
#include "app_task.h"                   // Application Task API
#include "user_callback_config.h"
#include "app_entry_point.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if BLE_FINDME_TARGET
/**
 ****************************************************************************************
 * @brief Handles Alert indication from findme target.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int findt_alert_ind_handler(ke_msg_id_t const msgid,
                                   struct findt_alert_ind const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    EXECUTE_PROFILE_CALLBACK_PARAM1_PARAM2(on_findt_alert_ind, gapc_get_conidx(param->conhdl), param->alert_lvl);

    return (KE_MSG_CONSUMED);
}
#endif

#if BLE_FINDME_LOCATOR
/**
 ****************************************************************************************
 * @brief Handles enable profile confirmation for findme locator.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int findl_enable_cfm_handler(ke_msg_id_t const msgid,
                                    struct findl_enable_cfm const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    return (KE_MSG_CONSUMED);
}
#endif

#if (BLE_FINDME_TARGET || BLE_FINDME_LOCATOR)

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static const struct ke_msg_handler app_findme_process_handlers[]=
{
#if BLE_FINDME_TARGET
    {FINDT_ALERT_IND,                       (ke_msg_func_t)findt_alert_ind_handler},
#endif

#if BLE_FINDME_LOCATOR
    {FINDL_ENABLE_CFM,                      (ke_msg_func_t)findl_enable_cfm_handler},
#endif
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_findme_process_handler(ke_msg_id_t const msgid,
                                                       void const *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id,
                                                       enum ke_msg_status_tag *msg_ret)
{
    return app_std_process_event(msgid, param, src_id, dest_id, msg_ret, app_findme_process_handlers,
                                         sizeof(app_findme_process_handlers) / sizeof(struct ke_msg_handler));
}
#endif // (BLE_FINDME_TARGET || BLE_FINDME_LOCATOR)

#endif // BLE_APP_PRESENT

/// @} APPTASK
