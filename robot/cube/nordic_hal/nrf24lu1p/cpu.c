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
 * $LastChangedRevision: 133 $
 */

/** @file
 * @brief Implementation of the cpu module for nRF24LU1+
 */

#include "nrf24lu1p.h"
#include "nordic_common.h"
#include "cpu.h"

void cpu_pwr_down(void)
{
  PWRDWN = 1;
}

void cpu_set_clock_frequency(uint8_t cf)
{
  uint8_t temp = CLKCTL;

  // Clock frequnecy is defined by bits 6-4 in CLKCTL
  temp &= ~(BIT_6 | BIT_5 | BIT_4);
  temp |= cf & (BIT_6 | BIT_5 | BIT_4);
  
  F0 = EA;
  EA = 0;
  CLKCTL = temp;
  EA = F0;
}

