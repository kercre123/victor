 /**
 ****************************************************************************************
 *
 * @file uart_booter.h
 *
 * @brief  user booter protocol settings
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
 
#ifndef _UART_BOOTER_H
#define _UART_BOOTER_H
#include "global_io.h" 

#define max_code_length          0xA800  // MAX 42K bytes
#define SYSRAM_COPY_BASE_ADDRESS 2000
#define SYSRAM_BASE_ADDRESS      0
#define SOH   0x01              
#define STX   0x02              
#define ACK   0x06              
#define NAK   0x15           

#endif
