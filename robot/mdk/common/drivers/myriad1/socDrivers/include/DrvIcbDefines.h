#ifndef _BRINGUP_SABRE_ICB_DEF_H_
#define _BRINGUP_SABRE_ICB_DEF_H_
/**
 * @file DrvIcbDefines.h
 *                MOVIDIA ICB Constants and Address Defines
 *                Copyright 2008, 2009 MOVIDIA Ltd.
 *                Header file with defines for IRQ sources and other constants
 * @brief Public ICB Constants
 * @author Bogdan Manciu
*/

/*===========================================================================*/
/*                           LOCAL CONSTANTS                                 */
/*===========================================================================*/
#define NUM_INT_LEVELS            (16)
#define __N_IRQS__ 47

/**@{
 * @name Interrupt types
 *        Positive And Negative
 *        Edge and Level constants
*/
#define POS_EDGE_INT  0
#define NEG_EDGE_INT  1
#define POS_LEVEL_INT 2
#define NEG_LEVEL_INT 3

/**
 * @}
 * @{
 * @name Interrupt Sources( IRQ Numbers )
 *       One or more for each Hardware Peripheral
*/
#define IRQ_WATCHDOG        0
#define IRQ_UART            1
#define IRQ_REAL_TIME_CLOCK 2
#define IRQ_TIMER        (2) // Used so we can add timer number to index
#define IRQ_TIMER_1      3
#define IRQ_TIMER_2      4
#define IRQ_TIMER_3      5
#define IRQ_TIMER_4      6
#define IRQ_TIMER_5      7
#define IRQ_TIMER_6      8
#define IRQ_TIMER_7      9
#define IRQ_TIMER_8     10
#define IRQ_IIC1        11
#define IRQ_IIC2        12
#define IRQ_SPI1        13
#define IRQ_SPI2        14
#define IRQ_SPI3        15
#define IRQ_SDIO_DEV    16
#define IRQ_SDIO_HOST_1 17
#define IRQ_SDIO_HOST_2 18
#define IRQ_SEBI        19
#define IRQ_I2S_1       20
#define IRQ_I2S_2       21
#define IRQ_I2S_3       22
///General Interrupts for each Camera
#define IRQ_CIF_1       23
#define IRQ_CIF_2       24
#define IRQ_LCD_1       25
#define IRQ_LCD_2       26
#define IRQ_USB         27
///Transport Stream interface interrupt
#define IRQ_TSI         28
///NAND flash controller Interface
#define IRQ_NFC         29
#define IRQ_GPS         30
#define IRQ_CABAC       31
/**
 * Shave vector engine general interrupt
 * One per shave 7-0
 */
#define IRQ_SVE_0       32
#define IRQ_SVE_1       33
#define IRQ_SVE_2       34
#define IRQ_SVE_3       35
#define IRQ_SVE_4       36
#define IRQ_SVE_5       37
#define IRQ_SVE_6       38
#define IRQ_SVE_7       39
#define IRQ_CMX         40
#define IRQ_GPIO_0      41
#define IRQ_GPIO_1      42
///AXI Monitor module
#define IRQ_AXM         43
#define IRQ_KEYBOARD    45
#define IRQ_TOUCHSCREEN 46
///@}

#endif
