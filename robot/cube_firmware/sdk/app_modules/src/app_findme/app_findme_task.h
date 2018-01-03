/**
 ****************************************************************************************
 *
 * @file app_findme_task.h
 *
 * @brief Findme Target task header.
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

#ifndef APP_FINDME_TASK_H_
#define APP_FINDME_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Findme Application task handlers.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if BLE_FINDME_TARGET
#include "findt_task.h"
#endif

#if BLE_FINDME_LOCATOR
#include "findl_task.h"
#endif

#if (BLE_FINDME_TARGET || BLE_FINDME_LOCATOR)
#include "ke_msg.h"
#endif

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if (BLE_FINDME_TARGET || BLE_FINDME_LOCATOR)
/**
 ****************************************************************************************
 * @brief Process handler for the Application FINDME messages.
 * @param[in] msgid   Id of the message received
 * @param[in] param   Pointer to the parameters of the message
 * @param[in] dest_id ID of the receiving task instance (probably unused)
 * @param[in] src_id  ID of the sending task instance
 * @param[in] msg_ret Result of the message handler
 * @return Returns if the message is handled by the process handler
 ****************************************************************************************
 */
enum process_event_response app_findme_process_handler(ke_msg_id_t const msgid,
                                                       void const *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id,
                                                       enum ke_msg_status_tag *msg_ret);
#endif

/// @} APP

#endif // APP_FINDME_TASK_H_
