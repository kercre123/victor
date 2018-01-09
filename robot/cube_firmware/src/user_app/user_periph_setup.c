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

typedef struct {
  GPIO_PORT port;
  GPIO_PIN pin;
} LED_Pin;

static const LED_Pin LEDs[] = {
  { GPIOPP(D0) },
  { GPIOPP(D3) },
  { GPIOPP(D6) },
  { GPIOPP(D9) },

  { GPIOPP(D1) },
  { GPIOPP(D4) },
  { GPIOPP(D7) },
  { GPIOPP(D10) },

  { GPIOPP(D2) },
  { GPIOPP(D5) },
  { GPIOPP(D8) },
  { GPIOPP(D11) },
};

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

  // Disable things
  GPIO_INIT_PIN(BOOST_EN, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_1V );
  GPIO_INIT_PIN(ACC_CS,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D0, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D1, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D2, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D3, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D4, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D5, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D6, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D7, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D8, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D9, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D10, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D11, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );

  // Blink pattern
  for (int p = 0; p < 12; p++) {
    GPIO_SetInactive(LEDs[p].port, LEDs[p].pin);
    for (int i = 0; i < 200000; i++) __nop();
    GPIO_SetActive(LEDs[p].port, LEDs[p].pin);
  }
  GPIO_CLR(BOOST_EN);
}
