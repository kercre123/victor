/**
 ****************************************************************************************
 *
 * @file intc.h
 *
 * @brief Declaration of the Interrupt Controller API.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev: 5769 $
 *
 ****************************************************************************************
 */

#ifndef _INTC_H_
#define _INTC_H_

/**
 ****************************************************************************************
 * @addtogroup INTC INTC
 * @ingroup DRIVERS
 *
 * @brief Declaration of the Interrupt Controller API.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "compiler.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/** @name Mapping of the peripherals interrupts in the interrupt controller.
 * @{
 */
#define INTC_UNUSED31   (31)
#define INTC_UNUSED30   (30)
#define INTC_UNUSED29   (29)
#define INTC_UNUSED28   (28)
#define INTC_UNUSED27   (27)
#define INTC_UNUSED26   (26)
#define INTC_UNUSED25   (25)
#define INTC_UNUSED24   (24)
#define INTC_UNUSED23   (23)
#define INTC_UNUSED22   (22)
#define INTC_UNUSED21   (21)
#define INTC_UNUSED20   (20)
#define INTC_UNUSED19   (19)
#define INTC_UNUSED18   (18)
#define INTC_UNUSED17   (17)
#define INTC_UNUSED16   (16)
#define INTC_UNUSED15   (15)
#define INTC_UNUSED14   (14)
#define INTC_UNUSED13   (13)
#define INTC_UNUSED12   (12)
#define INTC_DMA        (12)
#define INTC_UNUSED11   (11)
#define INTC_USB        (11)
#define INTC_UNUSED10   (10)
#define INTC_PRBS       (10)
#define INTC_UNUSED9    (9)
#define INTC_GPIO       (9)
#define INTC_UNUSED8    (8)
#define INTC_UNUSED7    (7)
#define INTC_APB2DPVIRQ (7)
#define INTC_UNUSED6    (6)
#define INTC_TIMER2     (6)
#define INTC_UNUSED5    (5)
#define INTC_TIMER1     (5)
#define INTC_UNUSED4    (5)
#define INTC_BLE        (4)
#define INTC_UNUSED3    (3)
#define INTC_UART       (3)
#define INTC_UNUSED2    (2)
#define INTC_UNUSED1    (1)
#define INTC_PCM        (1)
#define INTC_UNUSED0    (0)
#define INTC_BT         (0)
/// @} INTC mapping

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize and configure the reference interrupt controller.
 *
 * This function configures the interrupt controller according to the system needs.
 *
 ****************************************************************************************
 */
void intc_init(void);

/**
 ****************************************************************************************
 * @brief Clear status of interrupts.
 *
 * This function clear interrupt status.
 *
 ****************************************************************************************
 */
void intc_stat_clear(void);

/**
 ****************************************************************************************
 * @brief IRQ Handler.
 *
 ****************************************************************************************
 */
__IRQ void intc_irq(void);

/**
 ****************************************************************************************
 * @brief FIQ Handler.
 *
 ****************************************************************************************
 */
__FIQ void intc_fiq(void);

/// @} INTC

#endif // _INTC_H_
