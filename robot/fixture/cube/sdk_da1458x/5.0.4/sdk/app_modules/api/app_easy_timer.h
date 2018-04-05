/**
 ****************************************************************************************
 *
 * @file app_easy_timer.h
 *
 * @brief The easy timer api definitions.
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

#ifndef _APP_EASY_TIMER_H_
#define _APP_EASY_TIMER_H_

/**
 ****************************************************************************************
 * @addtogroup APP_TIMER
 * @ingroup
 *
 * @brief
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "ke_msg.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

//Timer handler type
typedef uint8_t timer_hnd;

// Timer callback function type definition
typedef void (* timer_callback)(void);

/*
 * DEFINES
 ****************************************************************************************
 */

//Value indicating an invalide timer operation
#define EASY_TIMER_INVALID_TIMER    (0x0)

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Process handler for the Easy Timer messages.
 * @param[in] msgid   Id of the message received
 * @param[in] param   Pointer to the parameters of the message
 * @param[in] dest_id ID of the receiving task instance (probably unused)
 * @param[in] src_id  ID of the sending task instance
 * @param[in] msg_ret Result of the message handler
 * @return Returns if the message is handled by the process handler
 ****************************************************************************************
 */
enum process_event_response app_timer_api_process_handler(ke_msg_id_t const msgid,
                                                          void const *param,
                                                          ke_task_id_t const dest_id,
                                                          ke_task_id_t const src_id,
                                                          enum ke_msg_status_tag *msg_ret);

/**
 ****************************************************************************************
 * @brief Create a new timer. Activate the ble if required.
 * @param[in] delay The amount of timer slots (10 ms) to wait (time resolution is 10ms)
 * @param[in] fn    The callback to be called when the timer expires
 * @return The handler of the timer for future reference. If there are not timers available
 *         EASY_TIMER_INVALID_TIMER will be returned
 ****************************************************************************************
 */
timer_hnd app_easy_timer(const uint16_t delay, timer_callback fn);

/**
 ****************************************************************************************
 * @brief Cancel an active timer.
 * @param[in] timer_id The timer handler to cancel
 * @return void
 ****************************************************************************************
 */
void app_easy_timer_cancel(const timer_hnd timer_id);

/**
 ****************************************************************************************
 * @brief Modify the delay of an existing timer.
 * @param[in] timer_id The timer handler to modify
 * @param[in] delay    The new delay value (time resolution is 10ms)
 * @return The timer handler if everything is ok
 ****************************************************************************************
 */
timer_hnd app_easy_timer_modify(const timer_hnd timer_id, const uint16_t delay);

/**
 ****************************************************************************************
 * @brief Cancel all the active timers.
 * @return void
 ****************************************************************************************
 */
void app_easy_timer_cancel_all(void);

/// @} APP_TIMER

#endif // _APP_EASY_TIMER_H_
