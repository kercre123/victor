#ifndef __RTC_H__
#define __RTC_H__

/*******************************************************************************
 * @file RTC support
 * @brief Minimal RTC support for bootloader
 * @copyright Copyright 2015 Richard A Burton
 * @author richardaburton@gmail.com
 *
 * @license The MIT License (MIT)
 * 
 * Copyright (c) 2015 Richard A Burton (richardaburton@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define PERIPHS_RTC_BASEADDR  0x60000700
#define REG_RTC_BASE          PERIPHS_RTC_BASEADDR
#define RTC_GPIO_OUT          (REG_RTC_BASE + 0x068)
#define RTC_GPIO_ENABLE       (REG_RTC_BASE + 0x074)
#define RTC_GPIO_IN_DATA      (REG_RTC_BASE + 0x08C)
#define RTC_GPIO_CONF         (REG_RTC_BASE + 0x090)
#define PAD_XPD_DCDC_CONF     (REG_RTC_BASE + 0x0A0)
#define RTC_MEM_BASE          (REG_RTC_BASE + 0xA00)

#if 0
static uint32 get_gpio16(void) {
  // set output level to 1
  WRITE_PERI_REG(RTC_GPIO_OUT, (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(1));

  // read level
  WRITE_PERI_REG(PAD_XPD_DCDC_CONF, (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1);  // mux configuration for XPD_DCDC and rtc_gpio0 connection
  WRITE_PERI_REG(RTC_GPIO_CONF, (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);  //mux configuration for out enable
  WRITE_PERI_REG(RTC_GPIO_ENABLE, READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);  //out disable

  return (READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
}
#endif

static int system_rtc_mem(const int32 addr, uint32 *buff, const int32 length, const int writeNotRead) {
  int32 blocks;

  if (addr < 64) return FALSE;
  if (addr + length > 192) return FALSE;
  if (buff == NULL) return FALSE;

  // copy the data
  for (blocks = length - 1; blocks >= 0; blocks--)
  {
    volatile uint32 *ram = ((uint32*)buff) + blocks;
    volatile uint32 *rtc = ((uint32*)RTC_MEM_BASE) + addr + blocks;
    if (writeNotRead == TRUE)
    {
      *rtc = *ram;
    }
    else
    {
      *ram = *rtc;
    }
  }

  return TRUE;
}

#endif
