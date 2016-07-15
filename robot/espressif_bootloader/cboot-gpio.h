#ifndef __GPIO_H__
#define __GPIO_H__

/*******************************************************************************
 * @file gpio support
 * @brief Minimal GPIO support for bootloader
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

// support for "normal" GPIOs (other than 16)
#define ETS_UNCACHED_ADDR(addr) (addr)
#define READ_PERI_REG(addr) (*((volatile uint32 *)ETS_UNCACHED_ADDR(addr)))
#define WRITE_PERI_REG(addr, val) (*((volatile uint32 *)ETS_UNCACHED_ADDR(addr))) = (uint32)(val)
#define PERIPHS_RTC_BASEADDR     0x60000700
#define REG_GPIO_BASE            0x60000300
#define GPIO_OUT_ADDRESS         (REG_GPIO_BASE + 0x00)
#define GPIO_SET_OUT_ADDRESS     (REG_GPIO_BASE + 0x04)
#define GPIO_CLEAR_OUT_ADDRESS   (REG_GPIO_BASE + 0x08)
#define GPIO_ENABLE_OUT_ADDRESS  (REG_GPIO_BASE + 0x0c)
#define GPIO_DIR_OUT_ADDRESS     (REG_GPIO_BASE + 0x10)
#define GPIO_DIR_IN_ADDRESS      (REG_GPIO_BASE + 0x14)
#define GPIO_IN_ADDRESS          (REG_GPIO_BASE + 0x18)
#define REG_IOMUX_BASE           0x60000800
#define IOMUX_PULLUP_MASK        (1<<7)
#define IOMUX_FUNC_MASK          0x0130
const uint8 IOMUX_REG_OFFS[] = {0x34, 0x18, 0x38, 0x14, 0x3c, 0x40, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x04, 0x08, 0x0c, 0x10};
const uint8 IOMUX_GPIO_FUNC[] = {0x00, 0x30, 0x00, 0x30, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};

static int get_gpio(int gpio_num) {
  // disable output buffer if set
  WRITE_PERI_REG(GPIO_DIR_IN_ADDRESS, 1<<gpio_num);

  // set GPIO function
  uint32 iomux_reg = REG_IOMUX_BASE + IOMUX_REG_OFFS[gpio_num];
  uint32 old_iomux = READ_PERI_REG(iomux_reg);
  uint32 gpio_func = IOMUX_GPIO_FUNC[gpio_num];
  uint32 new_iomux = (old_iomux & ~IOMUX_FUNC_MASK) | gpio_func; // | IOMUX_PULLUP_MASK;
  WRITE_PERI_REG(iomux_reg, new_iomux);

  // allow soft pullup to take effect if line was floating
  ets_delay_us(10);
  int result = READ_PERI_REG(GPIO_IN_ADDRESS) & (1<<gpio_num);

  return (result ? TRUE : FALSE);
}

static void set_gpio(int gpio_num, int val)
{
  uint32 iomux_reg = REG_IOMUX_BASE + IOMUX_REG_OFFS[gpio_num];
  uint32 old_iomux = READ_PERI_REG(iomux_reg);
  uint32 gpio_func = IOMUX_GPIO_FUNC[gpio_num];
  uint32 new_iomux = (old_iomux & ~IOMUX_FUNC_MASK) | gpio_func; // | IOMUX_PULLUP_MASK;
  WRITE_PERI_REG(iomux_reg, new_iomux);
  
  WRITE_PERI_REG(GPIO_DIR_OUT_ADDRESS, 1<<gpio_num); // Set pin to output
  if (val) WRITE_PERI_REG(GPIO_SET_OUT_ADDRESS, 1<<gpio_num); // write pin
  else   WRITE_PERI_REG(GPIO_CLEAR_OUT_ADDRESS, 1<<gpio_num);
}

#endif
