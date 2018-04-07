/**
 ****************************************************************************************
 *
 * @file led.h
 *
 * @brief LED driver: control and defines for simple LED initialization and control.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev: 5768 $
 *
 ****************************************************************************************
 */

#ifndef LED_H_
#define LED_H_

/**
 ****************************************************************************************
 * @addtogroup LED
 * @ingroup DRIVERS
 *
 * @brief LED driver.
 *
 * This driver allows simple initialization of LED related registers and generating
 * a simple functionality for each passing in the main FW loop.
 *
 * @{
 ****************************************************************************************
 */

#include <stdint.h>          // standard integer definitions
#include "reg_auxio.h"       // register auxio

/*
 * MACROS
 ****************************************************************************************
 */
#define led_set(index)        //auxio_auxioa_out_set(auxio_auxioa_out_get() | (1 << (index-1)))
#define led_reset(index)      //auxio_auxioa_out_set(auxio_auxioa_out_get() & ~(1 << (index-1)))
#define led_toggle(index)     //auxio_auxioa_out_set(auxio_auxioa_out_get() ^ (1 << (index-1)))


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize LED.
 *
 ****************************************************************************************
 */
void led_init(void);

/**
 ****************************************************************************************
 * @brief Set LED.
 *
 * @param[in] value    LED settings
 *
 ****************************************************************************************
 */
void led_set_all(uint32_t value);

/// @} LED

#endif // LED_H_
