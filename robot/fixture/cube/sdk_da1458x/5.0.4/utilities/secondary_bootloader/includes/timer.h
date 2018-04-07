/**
 ****************************************************************************************
 *
 * @file timer.h
 *
 * @brief timer handling functions header file.
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
 

#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED
#include <stdint.h>
void start_timer(WORD times_x_fourms);
void stop_timer(void);

   
#endif
