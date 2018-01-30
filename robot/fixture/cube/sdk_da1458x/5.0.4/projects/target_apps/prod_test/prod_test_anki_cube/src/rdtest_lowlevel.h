/**
 ****************************************************************************************
 *
 * @file rdtest_lowlevel.h
 *
 * @brief RD test low level functions header file.
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

#ifndef _RDTEST_LOWLEVEL_H_
#define _RDTEST_LOWLEVEL_H_
 
#include "global_io.h"
//#include "ARMCM0.h"

#define DATAMASK     0xFF
#define DATASHIFT    0
#define ADDRESSMASK  0x0F
#define ADDRESSSHIFT 0
#define LATCHMASK    0x10
#define LATCHSHIFT   4

#define ENABLEMASK  0x20
#define ENABLESHIFT 5

#define VBAT_CTRL_ADDRESS     0 
#define UART_CONNECT_ADDRESS  2 
#define LOOPBACK_CTRL_ADDRESS 4
#define GATE_TO_UART_ADDRESS  6
#define VPP_CTRL_ADDRESS      8 
#define STARTGATE_ADDRESS     9
#define RESET_CTRL_REG        0x0A
#define PW_CTRL_ADDRESS       0x0B

void rdtest_init_ports(uint8 version);
void rdtest_ll_write_data(uint8 data);
void rdtest_ll_write_address(uint8 address);
void rdtest_ll_write_latch(uint8 latch);
void rdtest_ll_write_enable(uint8 enable);
void rdtest_ll_reset(uint8 reset);
void rdtest_updateregister(uint8 address, uint8 data);
void rdtest_enable (void);

#endif // _RDTEST_LOWLEVEL_H_
