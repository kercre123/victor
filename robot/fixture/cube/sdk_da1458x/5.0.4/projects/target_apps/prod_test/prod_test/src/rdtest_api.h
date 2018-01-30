/**
 ****************************************************************************************
 *
 * @file rdtest_api.h
 *
 * @brief RD test API header file.
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

#ifndef _RDTEST_API_H_
#define _RDTEST_API_H_
 
#include "rdtest_lowlevel.h"
#include "rdtest_support.h" 
#include "global_io.h" 

void rdtest_initialize(uint8 version);
void rdtest_vppcontrol(uint8 state);
void rdtest_select_pulsewidth(uint8 length);
void rdtest_uart_connect(uint16 connectmap16);
void rdtest_make_loopback(uint8 port);
void rdtest_vbatcontrol(uint16 connectmap16);
void rdtest_resetpulse(uint16 delayms);
void rdtest_pulse_to_uart(uint16 connectmap16);
void rdtest_generate_pulse(void);

#endif // _RDTEST_API_H_
