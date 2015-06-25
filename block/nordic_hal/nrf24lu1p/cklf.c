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
 * @brief Implementation of the cklf module for nRF24LU1+
 */

#include "nrf24lu1p.h"
#include "cklf.h"

static uint16_t wd_cnt;

void cklf_regxc_write(uint8_t addr, uint16_t val)
{
  REGXH = val >> 8;
  REGXL = val & 0xff;
  REGXC = addr;
}

uint16_t cklf_regxc_read(uint8_t addr)
{
  uint16_t val;

  REGXC = addr;
  val = (uint16_t)REGXH;
  val <<= 8;
  val |= (uint16_t)REGXL;
  return val;
}

void cklf_rtc_disable(void)
{
  cklf_regxc_write(WRTCDIS, 0);
}

void cklf_rtc_init(uint8_t cnt_h, uint16_t cnt_l)
{
  cklf_regxc_write(WRTCDIS, 0);                // Disable RTC timer before updating latch
  WUF = 0;                                     // Clear any pending interrupts
  cklf_regxc_write(WGTIMER, (uint16_t)cnt_h);  // Program MS part first
  cklf_regxc_write(WRTCLAT, cnt_l);            // Write LS part and enable RTC
}

uint16_t cklf_rtc_read_lsw(void)
{
  return cklf_regxc_read(RRTC);
}

uint8_t cklf_rtc_read_msb(void)
{
  return (uint8_t)(cklf_regxc_read(RGTIMER) >> 8);
}

void cklf_rtc_wait(void)
{
  while(WUF == 0)                         // Wait until IEX6 is set
    ;
  WUF = 0;
}

void cklf_wdog_init(uint16_t cnt)
{
  wd_cnt = cnt;
  cklf_regxc_write(WWD, wd_cnt);
}

void cklf_wdog_feed(void)
{
  cklf_regxc_write(WWD, wd_cnt);
}

void cklf_gpio_wakeup(uint16_t wcon1, uint16_t wcon0)
{
  cklf_regxc_write(WWCON1, wcon1);
  cklf_regxc_write(WWCON0, wcon0);
}
