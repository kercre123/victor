//------------------------------------------------------------------------------
// (c) Copyright 2012, Dialog Semiconductor BV
// All Rights Reserved
//------------------------------------------------------------------------------
// Dialog SEMICONDUCTOR CONFIDENTIAL
//------------------------------------------------------------------------------
// This code includes Confidential, Proprietary Information and is a Trade 
// Secret of Dialog Semiconductor BV. All use, disclosure, and/or reproduction 
// is prohibited unless authorized in writing.
//------------------------------------------------------------------------------
// Description: Defines for peripherals bit fields

#ifndef GPIO_H_INCLUDED
#define GPIO_H_INCLUDED

#define LOWEND_KEYBOARD

#define FUNC_GPIO          0
#define FUNC_UART1_RX      1
#define FUNC_UART1_TX      2
#define FUNC_UART2_RX      3
#define FUNC_UART2_TX      4
#define FUNC_SPI_DI        5
#define FUNC_SPI_DO        6
#define FUNC_SPI_CLK       7
#define FUNC_SPI_EN        8
#define FUNC_I2C_SCL       9
#define FUNC_I2C_SDA       10

#define FUNC_UART1_IRDA_RX 11
#define FUNC_UART1_IRDA_TX 12
#define FUNC_UART2_IRDA_RX 13
#define FUNC_UART2_IRDA_TX 14

#define FUNC_ADC           15
#define FUNC_PWM0		   16
#define FUNC_PWM1		   17
#define FUNC_BLE_DIAG      18
#define FUNC_UART1_CTSN    19
#define FUNC_UART1_RTSN    20
#define FUNC_UART2_CTSN    21
#define FUNC_UART2_RTSN    22

#define FUNC_PWM2		   23
#define FUNC_PWM3		   24
#define FUNC_PWM4		   25

#define DIR_INPUT 0x000
#define DIR_PULLUP 0x100
#define DIR_PULLDOWN 0x200
#define DIR_OUTPUT 0x300

#define NO_PIN 0
#define P00_PIN 1
#define P01_PIN 2
#define P02_PIN 3
#define P03_PIN 4
#define P04_PIN 5
#define P05_PIN 6
#define P06_PIN 7
#define P07_PIN 8
#define P10_PIN 9
#define P11_PIN 10
#define P12_PIN 11
#define P13_PIN 12
#define P14_PIN 13
#define P15_PIN 14
#define P20_PIN 15
#define P21_PIN 16
#define P22_PIN 17
#define P23_PIN 18
#define P24_PIN 19
#define P25_PIN 20
#define P26_PIN 21
#define P27_PIN 22
#define P28_PIN 23
#define P29_PIN 24

#define QD_PIN_NONE   0
#define QD_PIN_P00P01 1
#define QD_PIN_P02P03 2
#define QD_PIN_P04P05 3
#define QD_PIN_P06P07 4
#define QD_PIN_P10P11 5
#define QD_PIN_P12P13 6
#define QD_PIN_P23P24 7
#define QD_PIN_P25P26 8
#define QD_PIN_P27P28 9
#define QD_PIN_P29P200 10


#define PWM_CLOCK 1
#define PWM_NORMAL  0

#define DIV_BY_TEN 0
#define NO_DIV_BY_TEN 1

#define SWTIM_CLK_SYSTEM 1
#define SWTIM_CLK_32K 0

#endif

