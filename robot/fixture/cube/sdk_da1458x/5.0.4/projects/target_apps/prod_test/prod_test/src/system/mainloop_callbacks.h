/**
 ****************************************************************************************
 *
 * @file mainloop_callbacks.h
 *
 * @brief Declaration of mainloop callbacks used by the production test application. 
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */


#ifndef _MAINLOOP_CALLBACKS_H_
#define _MAINLOOP_CALLBACKS_H_

#include "arch_api.h"

arch_main_loop_callback_ret_t app_on_ble_powered(void);

arch_main_loop_callback_ret_t app_on_full_power(void);

void app_going_to_sleep(sleep_mode_t sleep_mode);

void app_resume_from_sleep(void);


#endif // _MAINLOOP_CALLBACKS_H_
