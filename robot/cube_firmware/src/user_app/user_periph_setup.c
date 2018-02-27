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

static uint16_t* const SPI_PIN_READ = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5));
static uint16_t* const SPI_PIN_SET = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 2);
static uint16_t* const SPI_PIN_RESET = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 4);

static uint16_t BIT_ACC_CS = 1 << ACC_CS_PIN;
static uint16_t BIT_ACC_SCK = 1 << ACC_SCK_PIN;
static uint16_t BIT_ACC_SDA = 1 << ACC_SDA_PIN;

void spi_write8(uint8_t value) {
  for (int i = 0x80; i; i >>= 1) {
    *SPI_PIN_RESET = BIT_ACC_SCK;
    *((value & i) ? SPI_PIN_SET : SPI_PIN_RESET) = BIT_ACC_SDA;
    *SPI_PIN_SET = BIT_ACC_SCK;
  }
}

uint8_t spi_read8() {
  uint8_t value = 0;
  for (int i = 0x80; i; i >>= 1) {
    *SPI_PIN_RESET = BIT_ACC_SCK;
    __nop(); __nop();
    *SPI_PIN_SET = BIT_ACC_SCK;
    if (*SPI_PIN_READ & BIT_ACC_SDA) value |= i;
  }
  return value;
}

void spi_write(uint8_t address, int length, const uint8_t* data) {
  *SPI_PIN_RESET = BIT_ACC_CS;
  spi_write8(address);
  while (length-- > 0) spi_write8(*(data++));
  *SPI_PIN_SET = BIT_ACC_CS;
}

void spi_read(uint8_t address, int length, uint8_t* data) {
  *SPI_PIN_RESET = BIT_ACC_CS;
  spi_write8(address | 0x80);
  GPIO_PIN_FUNC(ACC_SDA, INPUT, PID_GPIO);
  while (length-- > 0) *(data++) = spi_read8();
  GPIO_PIN_FUNC(ACC_SDA, OUTPUT, PID_GPIO);
  *SPI_PIN_SET = BIT_ACC_CS;
}

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

  static bool first_boot = true;
  if (first_boot) {
    first_boot = false ;
    __nop(); __nop(); __nop(); __nop(); __nop();

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

    GPIO_INIT_PIN(ACC_CS, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
    GPIO_INIT_PIN(ACC_SCK, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
    GPIO_INIT_PIN(ACC_SDA, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );

    static const uint8_t SLEEP_ACC = 0x40;
    static const uint8_t MODE_SPI3 = 0x01;
    uint8_t chip_id;

    spi_write(0x34, sizeof(chip_id), &MODE_SPI3);
    spi_read (0x00, sizeof(chip_id), &chip_id);
    spi_write(0x11, sizeof(SLEEP_ACC), &SLEEP_ACC);
    spi_write(0x12, sizeof(SLEEP_ACC), &SLEEP_ACC);

    // Blink pattern
    for (int p = 0; p < 12; p++) {
      GPIO_SetInactive(LEDs[p].port, LEDs[p].pin);
      for (int i = 0; i < 200000; i++) __nop();
      GPIO_SetActive(LEDs[p].port, LEDs[p].pin);
    }
  }

  GPIO_CLR(BOOST_EN);
  GPIO_PIN_FUNC(D0, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D1, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D2, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D3, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D4, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D5, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D6, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D7, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D8, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D9, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D10, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(D11, INPUT, PID_GPIO);
}
