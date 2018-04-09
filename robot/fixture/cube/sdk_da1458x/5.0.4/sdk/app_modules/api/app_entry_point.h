/**
 ****************************************************************************************
 *
 * @file app_entry_point.h
 *
 * @brief Application entry point header file.
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

#ifndef _APP_ENTRY_POINT_H_
#define _APP_ENTRY_POINT_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 *
 * @brief Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "ke_msg.h"
#include "ke_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define KE_FIND_RELATED_TASK(task) ((ke_msg_id_t)((task) >> 10))

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response
{
    PR_EVENT_HANDLED =0,
    PR_EVENT_UNHANDLED
};

/// Format of a task message handler function
typedef enum process_event_response(*process_event_func_t)(ke_msg_id_t const msgid,
                                                           void const *param,
                                                           ke_task_id_t const dest_id,
                                                           ke_task_id_t const src_id,
                                                           enum ke_msg_status_tag *msg_ret);

/// Format of a catch rest event handler function
typedef void(*catch_rest_event_func_t)(ke_msg_id_t const msgid,
                                       void const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id);

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Application entry point handler.
 * @param[in] msgid       Message Id
 * @param[in] param       Pointer to message
 * @param[in] dest_id     Destination task Id
 * @param[in] src_id      Source task Id
 * @return Message status
 ****************************************************************************************
 */
int app_entry_point_handler(ke_msg_id_t const msgid,
                            void const *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id);
                                       
/**
 ****************************************************************************************
 * @brief Application standard process event handler.
 * @param[in] msgid       Message Id
 * @param[in] param       Pointer to message
 * @param[in] src_id      Source task Id
 * @param[in] dest_id     Destination task Id
 * @param[in] msg_ret     Message status returned
 * @param[in] handlers    Pointer to message handlers
   @param[in] handler_num Handler number
 * @return process_event_response PR_EVENT_HANDLED or PR_EVENT_UNHANDLED
 ****************************************************************************************
 */
enum process_event_response app_std_process_event(ke_msg_id_t const msgid,
                                                  void const *param,
                                                  ke_task_id_t const src_id,
                                                  ke_task_id_t const dest_id,
                                                  enum ke_msg_status_tag *msg_ret,
                                                  const struct ke_msg_handler *handlers,
                                                  const int handler_num);

/// @} APP

#endif // _APP_ENTRY_POINT_H_
