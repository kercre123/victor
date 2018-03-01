/**
 ****************************************************************************************
 *
 * @file user_periph_setup.c
 *
 * @brief Peripherals setup and initialization.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration
#include "user_periph_setup.h"       // peripheral configuration
#include "global_io.h"
#include "gpio.h"
#include "uart.h"                    // UART initialization

#include "datasheet.h"

#include "../app/board.h"

/**
 ****************************************************************************************
 * @brief Each application reserves its own GPIOs here.
 *
 * @return void
 ****************************************************************************************
 */

#ifdef CFG_DEVELOPMENT_DEBUG

void GPIO_reservations(void)
{
/*
* Globally reserved GPIOs reservation
*/

/*
* Application specific GPIOs reservation. Used only in Development mode (#if DEVELOPMENT_DEBUG)

i.e.
    RESERVE_GPIO(DESCRIPTIVE_NAME, GPIO_PORT_0, GPIO_PIN_1, PID_GPIO);    //Reserve P_01 as Generic Purpose I/O
*/
}
#endif // CFG_DEVELOPMENT_DEBUG

void periph_init(void)
{
  // Power up peripherals' power domain
  SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
  while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));

  SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);

  //rom patch
  patch_func();

  // (Re)Initialize peripherals
  // i.e.
  //  uart_init(UART_BAUDRATE_115K2, 3);

  // Enable the pads
  SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);

  GPIO_INIT_PIN(BOOST_EN, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_1V );
  GPIO_INIT_PIN(D0, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D1, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D2, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D3, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D4, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D5, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D6, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D7, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D8, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D9, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D10, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D11, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );

  GPIO_INIT_PIN(ACC_PWR, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_nCS, INPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SCK, INPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SDA, INPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
}
