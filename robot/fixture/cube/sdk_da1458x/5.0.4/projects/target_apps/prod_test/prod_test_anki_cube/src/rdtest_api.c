/**
 ****************************************************************************************
 *
 * @file rdtest_api.c
 *
 * @brief RD test API source code file.
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

#include "rdtest_API.h"

void rdtest_initialize(uint8 version)
//initialize ports and put CPLD in known (safe) state
//version can later be used to control the lowlevel communication method, not yet implemented
{
  rdtest_init_ports(version);
  rdtest_vppcontrol(0);
  rdtest_uart_connect(0);
  rdtest_make_loopback(0);
  rdtest_vbatcontrol(0);
  rdtest_pulse_to_uart(0);
	//rdtest_select_pulsewidth(0);
}

void rdtest_vppcontrol(uint8 state)
//controls the Vpp OTP programming voltage
{
  rdtest_updateregister(VPP_CTRL_ADDRESS, state&0x01);
	rdtest_enable();
}

void rdtest_select_pulsewidth(uint8 length)
//controls the pulsewidth selection
{
  rdtest_updateregister(PW_CTRL_ADDRESS, length);
	rdtest_enable();
}

void rdtest_uart_connect(uint16 connectmap16)
//connect or disconnects an FTDI-uart to DUT_connector
{
  rdtest_updateregister(UART_CONNECT_ADDRESS+0, (uint8) ((connectmap16>>0)&0xFF));
  rdtest_updateregister(UART_CONNECT_ADDRESS+1, (uint8) ((connectmap16>>8)&0xFF));
  rdtest_enable();
}

void rdtest_make_loopback(uint8 port)
//CPLD internal loopback, only port only!
{
  uint16 map = 0x0000;
  if (port!=0) {
    map = (0x01 << (port-1));
  }
  rdtest_updateregister(LOOPBACK_CTRL_ADDRESS+0, (uint8) ((map>>0)&0xFF));
  rdtest_updateregister(LOOPBACK_CTRL_ADDRESS+1, (uint8) ((map>>8)&0xFF));
  rdtest_enable();
}

void rdtest_vbatcontrol(uint16 connectmap16)
//connect Vbat to DUT_connector
{
  rdtest_updateregister(VBAT_CTRL_ADDRESS+0, (uint8) ((connectmap16>>0)&0xFF));
  rdtest_updateregister(VBAT_CTRL_ADDRESS+1, (uint8) ((connectmap16>>8)&0xFF));
  rdtest_enable();
}

void rdtest_resetpulse(uint16 delayms)
//generate a resetpulse to all devices
{
    rdtest_ll_reset(0);
    rdtest_ll_reset(1);
    delay_systick_us(1000*delayms);
    rdtest_ll_reset(0);
}

void rdtest_pulse_to_uart(uint16 connectmap16)
//route xtal-measurement pulse to uart pin
{	
	rdtest_updateregister(GATE_TO_UART_ADDRESS+0, (uint8) ((connectmap16>>0)&0xFF));
  rdtest_updateregister(GATE_TO_UART_ADDRESS+1, (uint8) ((connectmap16>>8)&0xFF));
	rdtest_enable();
}

void rdtest_generate_pulse(void)
//generates 1 xtal-measurement pulse
{
	rdtest_updateregister(STARTGATE_ADDRESS, 0);
	rdtest_enable();
  rdtest_updateregister(STARTGATE_ADDRESS, 1);
	rdtest_enable();
	rdtest_updateregister(STARTGATE_ADDRESS, 0);
	rdtest_enable();
}


