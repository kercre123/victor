    /**
 ****************************************************************************************
 *
 * @file otpc.h
 *
 * @brief eeprom driver over i2c interface header file.
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

#define OTPC_MODE_STBY    0x0
#define OTPC_MODE_MREAD   0x1
#define OTPC_MODE_MPROG   0x2
#define OTPC_MODE_AREAD   0x3
#define OTPC_MODE_APROG   0x4
#define OTPC_MODE_TBLANK  0x5
#define OTPC_MODE_TDEC    0x6
#define OTPC_MODE_TWR     0x7

#define OTP_BASE_ADDRESS 0x40000

void otpc_clock_enable(void);
void otpc_clock_disable(void);
 
int otpc_write_fifo(unsigned int mem_addr, unsigned int cel_addr, unsigned int num_of_words);
 

