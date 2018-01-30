/**
 ****************************************************************************************
 *
 * @file arch_wdg.h
 *
 * @brief Watchdog helper functions.
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

#ifndef _ARCH_WDG_H_
#define _ARCH_WDG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "datasheet.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define     WATCHDOG_DEFAULT_PERIOD 0xC8

/// Watchdog enable/disable
#ifndef USE_WDOG
#if defined(CFG_WDOG) 
#define USE_WDOG            1
#else
#define USE_WDOG            0
#endif //CFG_WDOG
#endif

/**
 ****************************************************************************************
 * @brief     Watchdog resume.
  * @return void
 * About: Start the Watchdog
 ****************************************************************************************
 */
static __inline void  wdg_resume(void)
{
#if (USE_WDOG)    
    // Start WDOG
    SetWord16(RESET_FREEZE_REG, FRZ_WDOG);  
#endif
}

/**
****************************************************************************************
* @brief     Freeze Watchdog. Call wdg_start () to resume operation.
* @return void
* About: Freeze the Watchdog
****************************************************************************************
*/
static __inline void  wdg_freeze(void)
{
    // Freeze WDOG   
    SetWord16(SET_FREEZE_REG, FRZ_WDOG);  
}

/**
 ****************************************************************************************
 * @brief     Watchdog reload.
 * @param[in] period measured in 10.24ms units.
 * @return void
 * About: Load the default value and resume the watchdog
 ****************************************************************************************
 */
static __inline void  wdg_reload(const int period)
{
#if (USE_WDOG)    
    // WATCHDOG_DEFAULT_PERIOD * 10.24ms 
    SetWord16(WATCHDOG_REG, WATCHDOG_DEFAULT_PERIOD);  
#endif
}

/**
 ****************************************************************************************
 * @brief     Initiliaze and start the Watchdog unit.
 * @param[in] If 0 Watchdog is SW freeze capable 
 * @return void
 * About: Enable the Watchdog and configure it to 
 ****************************************************************************************
 */
 static __inline void  wdg_init (const int freeze)
{
#if (USE_WDOG)
	 wdg_reload(WATCHDOG_DEFAULT_PERIOD);
    // if freeze equals to zero WDOG can be frozen by SW!
    // it will generate an NMI when counter reaches 0 and a WDOG (SYS) Reset when it reaches -16!
    SetWord16(WATCHDOG_CTRL_REG, (freeze&0x1)); 
    wdg_resume ();
#else
    wdg_freeze();
#endif
}

#endif // _ARCH_WDG_H_
