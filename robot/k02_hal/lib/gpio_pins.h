/*
 * Copyright (c) 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __FSL_GPIO_PINS_H__
#define __FSL_GPIO_PINS_H__

#include "fsl_gpio_driver.h"

/*! @file */
/*!*/
/*! This file contains gpio pin definitions used by gpio peripheral driver.*/
/*! The enums in _gpio_pins map to the real gpio pin numbers defined in*/
/*! gpioPinLookupTable. And this might be different in different board.*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief gpio pin names.*/
/*!*/ 
/*! This should be defined according to board setting.*/
enum _gpio_pins
{
    kGpioLED1       = GPIO_MAKE_PIN(HW_GPIOA, 2),  /* FRDM-K22F120M LED1 (Green LED) */
    kGpioLED2       = GPIO_MAKE_PIN(HW_GPIOA, 1),  /* FRDM-K22F120M LED2 (Red LED) */
    kGpioLED3       = GPIO_MAKE_PIN(HW_GPIOD, 5),  /* FRDM-K64F120M LED3 (Blue LED)*/
    kGpioSW1        = GPIO_MAKE_PIN(HW_GPIOC, 1),  /* FRDM-K22F120M SW2 */
    kGpioSW2        = GPIO_MAKE_PIN(HW_GPIOB, 17), /* FRDM-K22F120M SW3 */
    kGpioAccelINT1  = GPIO_MAKE_PIN(HW_GPIOD, 0),  /* FRDM-K22F120M MMA8451Q/FXOS87000CB INT1 */
    kGpioAccelINT2  = GPIO_MAKE_PIN(HW_GPIOD, 1),  /* FRDM-K22F120M MMA8451Q/FXOS87000CB INT2 */
    kGpioUsbFLGA    = GPIO_MAKE_PIN(HW_GPIOC, 0),  /* FRDM-K22F120M USB FLGA */
    kGpioUartDemoTX = GPIO_MAKE_PIN(HW_GPIOE, 0),  /* FRDM-K22F120M UART 1 TX pin (OpenSDA port) */
    kGpioUartDemoRX = GPIO_MAKE_PIN(HW_GPIOE, 1),  /* FRDM-K22F120M UART 1 RX pin (OpenSDA port) */
    kGpioI2Caddr1   = GPIO_MAKE_PIN(HW_GPIOC, 3),  /* FRDM-K22F120M I2C address pin */
    kGpioI2Caddr2   = GPIO_MAKE_PIN(HW_GPIOC, 6),  /* FRDM-K22F120M I2C address pin */
    kGpioSpi0Cs0    = GPIO_MAKE_PIN(HW_GPIOC, 4),  /* FRDM-K22F120M SPI0 CS0 pin */
    kGpioSpi0Cs1    = GPIO_MAKE_PIN(HW_GPIOC, 3),  /* FRDM-K22F120M SPI0 CS1 pin */
    kGpioRFCE       = GPIO_MAKE_PIN(HW_GPIOC, 11), /* FRDM-K22F120M RF CE pin */
    kGpioRFIRQ      = GPIO_MAKE_PIN(HW_GPIOD, 0)   /* FRDM-K22F120M RF IRQ pin */
};
extern gpio_input_pin_user_config_t switchPinsIrq[];
extern gpio_input_pin_user_config_t switchPins[];
extern gpio_input_pin_user_config_t accelIntPins[];
extern gpio_input_pin_user_config_t i2cAddressPins[];
extern gpio_output_pin_user_config_t gpioUartDemoTxPin[];
extern gpio_input_pin_user_config_t gpioUartDemoRxPin[];
extern gpio_output_pin_user_config_t ledPins[];
extern gpio_output_pin_user_config_t spiCsPin[];
extern gpio_output_pin_user_config_t rfCEPin[];
extern gpio_input_pin_user_config_t rfIRQPin[];

#endif /* __FSL_GPIO_PINS_H__ */
