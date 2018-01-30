/**
 ****************************************************************************************
 *
 * @file app_button_led.h
 *
 * @brief Push Button and LED handling header file.
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

#ifndef APP_BUTTON_LED_H_
#define APP_BUTTON_LED_H_

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
void red_blink_start(void);
void green_blink_fast(void);
void green_blink_slow(void);
void turn_off_led(void);
void app_button_enable(void);
void button_isr(void);

/// @} APP

#endif // APP_BUTTON_LED_H_
