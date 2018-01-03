/**
 ****************************************************************************************
 *
 * @file arch_system.h
 *
 * @brief Architecture System calls
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

#ifndef _ARCH_SYSTEM_H_
#define _ARCH_SYSTEM_H_

#include <stdbool.h>   // boolean definition
#include "datasheet.h"
#include "arch.h"
#include "arch_api.h"

/*
 * DEFINES
 ****************************************************************************************
 */

extern uint32_t lp_clk_sel;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Check if RCX is selected as the low power clock.
 * @return true if RCX is selected, otherwise false
 ****************************************************************************************
 */
static __inline bool arch_clk_is_RCX20(void)
{
    return (((lp_clk_sel == LP_CLK_RCX20) && (CFG_LP_CLK == LP_CLK_FROM_OTP)) || (CFG_LP_CLK == LP_CLK_RCX20));
}

/**
 ****************************************************************************************
 * @brief Check if XTAL32 is selected as the low power clock.
 * @return true if XTAL32 is selected, otherwise false
 ****************************************************************************************
 */
static __inline bool arch_clk_is_XTAL32(void)
{
    return (((lp_clk_sel == LP_CLK_XTAL32) && (CFG_LP_CLK == LP_CLK_FROM_OTP)) || (CFG_LP_CLK == LP_CLK_XTAL32));
}

/**
 ****************************************************************************************
 * @brief Calibrate the RCX20 clock if selected from the user.
 * @return void
 ****************************************************************************************
 */
static __inline void rcx20_calibrate()
{
    if (arch_clk_is_RCX20())
    {
        calibrate_rcx20(20);
        read_rcx_freq(20);
    }
}

/**
 ****************************************************************************************
 * @brief Read RCX frequency value.
 * @return void
 ****************************************************************************************
 */
static __inline void rcx20_read_freq()
{
    if (arch_clk_is_RCX20())
        read_rcx_freq(20);
}

/**
 ****************************************************************************************
 * @brief Calibrate the RCX20 clock if selected from the user.
 * @return void
 ****************************************************************************************
 */
static __inline void xtal16__trim_init()
{
    #if USE_POWER_OPTIMIZATIONS
        SetWord16(TRIM_CTRL_REG, 0x00); // ((0x0 + 1) + (0x0 + 1)) * 250usec but settling time is controlled differently
    #else
        SetWord16(TRIM_CTRL_REG, XTAL16_TRIM_DELAY_SETTING); // ((0xA + 1) + (0x2 + 1)) * 250usec settling time
    #endif
    SetBits16(CLK_16M_REG, XTAL16_CUR_SET, 0x5);
}

void system_init(void);

/**
 ****************************************************************************************
 * @brief Tweak wakeup timer debouncing time. It corrects the debouncing time when RCX is
 * selected as the clock in sleep mode. Due to a silicon bug the wakeup timer debouncing
 * time is multiplied by ~3 when the system is clocked by RCX in sleep mode.
 * @note This function SHOULD BE called when the system is ready to enter sleep
 * (tweak = true) to load the WKUP_CTRL_REG[WKUP_DEB_VALUE] with the recalculated
 * debouncing time value or when it comes out of sleep mode (tweak = false) to reload the
 * WKUP_CTRL_REG[WKUP_DEB_VALUE] with the initial programmed debouncing time value (stored
 * in the retention memory). The above software fix does not provide full guarantee that
 * it will remedy the silicon bug under all circumstances.
 * @param[in] tweak If true the debouncing time will be corrected, otherwise the initial
 *                  programmed debouncing time, stored in the retention memory, will be
 *                  set
 * @return void
 ****************************************************************************************
 */
void arch_wkupct_tweak_deb_time(bool tweak);

/**
 ****************************************************************************************
 * @brief UART initialization function for baudrates lower than 4800 bps.
 *
 * This function is a direct replacement of the uart_init() ROM function. Any application
 * that requires a baudrate lower than 2400 bps must replace calls to uart_init() with
 * with calls to arch_uart_init_slow().
 *
 * @param[in] baudr Baudrate value
 * @param[in] mode  Mode
 * @return void
 *****************************************************************************************
 */
void arch_uart_init_slow(uint16_t baudr, uint8_t mode);

#endif

/// @}
