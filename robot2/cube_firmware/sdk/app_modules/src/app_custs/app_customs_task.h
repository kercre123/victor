/**
 ****************************************************************************************
 *
 * @file app_customs_task.h
 *
 * @brief Custom Service task header.
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

#ifndef _APP_CUSTOMS_TASK_H_
#define _APP_CUSTOMS_TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if BLE_CUSTOM_SERVER

#include "ke_msg.h"
#include "custs1_task.h"
#include "custs2_task.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if BLE_CUSTOM1_SERVER
/**
 ****************************************************************************************
 * @brief Process handler for the Application Custom 1 Service messages.
 * @param[in] msgid   Id of the message received
 * @param[in] param   Pointer to the parameters of the message
 * @param[in] dest_id ID of the receiving task instance (probably unused)
 * @param[in] src_id  ID of the sending task instance
 * @param[in] msg_ret Result of the message handler
 * @return Returns if the message is handled by the process handler
 ****************************************************************************************
 */
enum process_event_response app_custs1_process_handler(ke_msg_id_t const msgid,
                                                       void const *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id,
                                                       enum ke_msg_status_tag *msg_ret);
#endif // BLE_CUSTOM1_SERVER

#if BLE_CUSTOM2_SERVER
/**
 ****************************************************************************************
 * @brief Process handler for the Application Custom 2 Service messages.
 * @param[in] msgid   Id of the message received
 * @param[in] param   Pointer to the parameters of the message
 * @param[in] dest_id ID of the receiving task instance (probably unused)
 * @param[in] src_id  ID of the sending task instance
 * @param[in] msg_ret Result of the message handler
 * @return Returns if the message is handled by the process handler
 ****************************************************************************************
 */
enum process_event_response app_custs2_process_handler(ke_msg_id_t const msgid,
                                                       void const *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id,
                                                       enum ke_msg_status_tag *msg_ret);
#endif // BLE_CUSTOM2_SERVER

#endif // BLE_CUSTOM_SERVER

#endif // _APP_CUSTOMS_TASK_H_
