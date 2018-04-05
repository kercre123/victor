/**
 ****************************************************************************************
 *
 * @file rdtest_lowlevel.c
 *
 * @brief RD test low level functions source code file.
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

#include "rdtest_lowlevel.h"

void rdtest_init_ports(uint8 version)
//initialize all ports used for communication with CPLD
//the version can later be used if different communication methods are implemented, not yet used
{
	// data
	SetWord16(P3_DATA_REG, 0);
	SetWord16(P30_MODE_REG, 0x300);
	SetWord16(P31_MODE_REG, 0x300);
	SetWord16(P32_MODE_REG, 0x300);
	SetWord16(P33_MODE_REG, 0x300);
	SetWord16(P34_MODE_REG, 0x300);
	SetWord16(P35_MODE_REG, 0x300);
	SetWord16(P36_MODE_REG, 0x300);
	SetWord16(P37_MODE_REG, 0x300);

	SetWord16(P2_DATA_REG, 0); 
	// address
	SetWord16(P20_MODE_REG, 0x300);
	SetWord16(P21_MODE_REG, 0x300);
	SetWord16(P22_MODE_REG, 0x300);
	SetWord16(P23_MODE_REG, 0x300);
	// latch
	SetWord16(P24_MODE_REG, 0x300);
	// enable
	SetWord16(P25_MODE_REG, 0x300);
}

void rdtest_ll_write_data(uint8 data)
//access to databus (8bit) = P3.7, P3.6, P3.5, P3.4, P3.3, P3.2, P3.1, P3.0 
{
	SetWord16(P3_DATA_REG, ((data<<DATASHIFT)&DATAMASK));
}

void rdtest_ll_write_address(uint8 address)
//access to addressbus (4bit) = P2.3, P2.2, P2.1, P2.0
{
	uint16 now_p2;
	now_p2 = GetWord16(P2_DATA_REG);
	SetWord16(P2_DATA_REG, (now_p2&~ADDRESSMASK) | ((address<<ADDRESSSHIFT)&ADDRESSMASK));
}

void rdtest_ll_write_latch(uint8 latch)
//access to latch-pin CPLD = P2.4
{
	uint16 now_p2;
	now_p2 = GetWord16(P2_DATA_REG);
	SetWord16(P2_DATA_REG, (now_p2&~LATCHMASK) | ((latch<<LATCHSHIFT)&LATCHMASK));
}

void rdtest_ll_write_enable(uint8 enable)
//access to enable-pin CPLD = P2.5
{
	uint16 now_p2;
	now_p2 = GetWord16(P2_DATA_REG);
	SetWord16(P2_DATA_REG, (now_p2&~ENABLEMASK) | ((enable<<ENABLESHIFT)&ENABLEMASK));
}

void rdtest_ll_reset(uint8 reset)
//access to reset register/bit in CPLD
{
	rdtest_updateregister(RESET_CTRL_REG, (uint8) reset);
	rdtest_enable();
}

void rdtest_updateregister(uint8 address, uint8 data)
//update register method: provide data+address, latch the result
{
	rdtest_ll_write_latch(0);
	rdtest_ll_write_data(data);
	rdtest_ll_write_address(address);
	rdtest_ll_write_latch(1);
	rdtest_ll_write_latch(0);
}

void rdtest_enable (void)
//provide a enable to the CPLD, used for multi-address transfers
//required after registers are updated... may be delayed...
{
	rdtest_ll_write_enable(0);
	rdtest_ll_write_enable(1);
	rdtest_ll_write_enable(0);
}
