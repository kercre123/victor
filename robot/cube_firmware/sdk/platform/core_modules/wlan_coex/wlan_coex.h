/**
 ****************************************************************************************
 *
 * @file wlan_coex.h
 *
 * @brief WLAN coexistence API. 
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
#ifndef _WLAN_COEX_H_
#define _WLAN_COEX_H_

/// Coexistence Definitions
//active scan
#define BLEMPRIO_SCAN       0x01
//advertise
#define BLEMPRIO_ADV        0x02
//connection request
#define BLEMPRIO_CONREQ     0x04
//control packet
#define BLEMPRIO_LLCP       0x10
//data packet
#define BLEMPRIO_DATA       0x20
//missed events
#define BLEMPRIO_MISSED     0x40

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void wlan_coex_init(void); 
void wlan_coex_check_signals(void);
void wlan_coex_enable(void);

#if DEVELOPMENT_DEBUG
void wlan_coex_reservations(void);
#endif

#endif  //_WLAN_COEX_H_
