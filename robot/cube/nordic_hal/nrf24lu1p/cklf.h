/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic 
 * Semiconductor ASA.Terms and conditions of usage are described in detail 
 * in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT. 
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRENTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *              
 * $LastChangedRevision: 171 $
 */

/** @file
 * @brief Interface functions for the low frequency clock module.
 * @defgroup hal_nrf24lu1p_cklf Low frequency clock (cklf)
 * @{
 * @ingroup hal_nrf24lu1p
 * in the nRF24LU1
 */
#ifndef CKLF_H__
#define CKLF_H__

#include <stdint.h>

/** Function to write to the low frequency clock interface (CKLF) in nRF24LU1.
 * @param addr the address of the register to write
 * @param val the value to write to the register
 */
void cklf_regxc_write(uint8_t addr, uint16_t val);

/** Function to read the low frequency clock interface (CKLF) in nRF24LU1.
 * @param addr the address of the register to read
 * @return the value read
 */
uint16_t cklf_regxc_read(uint8_t addr);

/** Function to disable the RTC.
 * This function should be called before reading values from the RTC counter.
 */
void cklf_rtc_disable(void);

/** Function to initialize the RTC.
 *  @param cnt_h The upper 8 bits of the 24 bit value to load into the RTC latch
 *  @param cnt_l The lower 16 bits of the 24 bit value to load into the RTC latch
 */
void cklf_rtc_init(uint8_t cnt_h, uint16_t cnt_l);

/** Function to read the lower 16 bits of the RTC counter.
 *  To ensure consistency between the return value of this function and
 *  cklf_rtc_read_msb the RTC should be disabled before calling these functions.
 *  @return The lower 16 bits of the RTC counter.
 */
uint16_t cklf_rtc_read_lsw(void);

/** Function to read the upper 8 bits of the RTC counter.
 *  To ensure consistency between the return value of this function and
 *  cklf_rtc_read_lsw the RTC should be disabled before calling these functions.
 *  @return The upper 8 bits of the RTC counter.
 */
uint8_t cklf_rtc_read_msb(void);

/** Function to wait for the RTC counter to reach 0.
 * This function enters an infinite loop polling the RTC interrupt flag. When
 * the flag is set the flag is cleared and the function returns.
 *
 */
void cklf_rtc_wait(void);

/** Function to initialize and enable the watchdog.
 * @param cnt The value to load into the watchdog counter.
 */
void cklf_wdog_init(uint16_t cnt);

/** Function to reload the watchdog.
 * This function reloads the watchdog counter with the parameter given in the
 * hal_wdog_init function.
 */
void cklf_wdog_feed(void);

/** Function to program the GPIO wakeup functionality
 *  @param wcon0 Value of WCON0 register (P00 - P03) 
 *  @param wcon1 Value of WCON1 register (P04 - P07)
 */
void cklf_gpio_wakeup(uint16_t wcon1, uint16_t wcon0);

#endif // CKLF_H__
/** @} */
