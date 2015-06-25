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
 * $LastChangedRevision: 172 $
 */

/** @file
 * @brief CPU management functions.
 * @defgroup hal_nrf24lu1p_cpu CPU management (cpu) 
 * @{
 * @ingroup hal_nrf24lu1p
 */
#ifndef CPU_H__
#define CPU_H__

#include <stdint.h>

/** Function to set the CPU in power down mode
 *  @return This function does not return until the CPU wakens
 */
void cpu_pwr_down(void);

/** Function to set the clock frequency of the chip
 *  cf is the value of the CLKCTL register but only bits 4-6 are used in this
 *  function.
 *  @param cf The clock frequency 
 */
void cpu_set_clock_frequency(uint8_t cf);

#endif // CPU_H__
/** @} */
