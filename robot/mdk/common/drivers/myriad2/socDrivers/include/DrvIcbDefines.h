#ifndef _BRINGUP_SABRE_ICB_DEF_H_
#define _BRINGUP_SABRE_ICB_DEF_H_
/**
 * @file DrvIcbDef.h
 *                MOVIDIA ICB Constants and Address Defines
 *                Copyright 2008, 2009 MOVIDIA Ltd.
 *                Header file with defines for IRQ sources and other constants
 * @brief Public ICB Constants
 * @author Bogdan Manciu
*/

#include <registersMyriad2.h>

/*===========================================================================*/
/*                           LOCAL CONSTANTS                                 */
/*===========================================================================*/
#define NUM_INT_LEVELS            (16)
#define __N_IRQS__ ICB_INT_NO

#define PCR_VAL_LEON_OS  (00006587)
#define PCR_VAL_LEON_RT  (10006587)
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

// according to cpu_subsystem.v
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
#define IRQ_IIC3        13

#define IRQ_SPI1        14
#define IRQ_SPI2        15
#define IRQ_SPI3        16

#define IRQ_I2S_1       17
#define IRQ_I2S_2       18
#define IRQ_I2S_3       19

#define IRQ_ETH         20
#define IRQ_SDIO        21
#define IRQ_USB         22
#define IRQ_AHB_DMA     23
#define IRQ_LEON4_L2C   24
#define IRQ_ROIC        25
// 26 .. 29 unused
#define IRQ_USB_CTRL    30
// 31 unused 

/**
 * Shave vector engine general interrupt
 */
#define IRQ_SVE_0       32
#define IRQ_SVE_1       33
#define IRQ_SVE_2       34
#define IRQ_SVE_3       35
#define IRQ_SVE_4       36
#define IRQ_SVE_5       37
#define IRQ_SVE_6       38
#define IRQ_SVE_7       39

#define IRQ_SVE_8       40
#define IRQ_SVE_9       41
#define IRQ_SVE_10      42
#define IRQ_SVE_11      43
#define IRQ_SVE_12      44  // TODO rename;  must be somthing else 
#define IRQ_SVE_13      45  // TODO rename
#define IRQ_SVE_14      46  // TODO rename
#define IRQ_SVE_15      47  // TODO rename

#define IRQ_GPIO_0      48
#define IRQ_GPIO_1      49
#define IRQ_GPIO_2      50
#define IRQ_GPIO_3      51


// todo rename accordingly if special functionalitty is specified 
#define IRQ_RTOS_0      52
#define IRQ_RTOS_1      53
#define IRQ_RTOS_2      54
#define IRQ_RTOS_3      55
#define IRQ_RTOS_4      56
#define IRQ_RTOS_5      57
#define IRQ_RTOS_6      58
#define IRQ_RTOS_7      59
#define IRQ_RTOS_8      60
#define IRQ_RTOS_9      61
#define IRQ_RTOS_10     62
#define IRQ_RTOS_11     63

///@}

#endif
