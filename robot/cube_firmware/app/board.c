#include <string.h>
#include <stdarg.h>
#include "board.h"
#include "datasheet.h"

/*
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

static const uint8_t SLEEP_ACC = 0x40;
static const uint8_t MODE_SPI3 = 0x01;
uint8_t chip_id;

spi_write(0x34, sizeof(chip_id), &MODE_SPI3);
spi_read (0x00, sizeof(chip_id), &chip_id);
spi_write(0x11, sizeof(SLEEP_ACC), &SLEEP_ACC);
spi_write(0x12, sizeof(SLEEP_ACC), &SLEEP_ACC);
*/

//------------------------------------------------
//    Board
//------------------------------------------------

void board_init(void)
{
  //Init boost regulator control (NOTE: 1V-BAT logic level)
  GPIO_INIT_PIN(BOOST_EN, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_1V );

  // Init LEDs
  GPIO_INIT_PIN(D0,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D1,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D2,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D3,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D4,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D5,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D6,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D7,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D8,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D9,  OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D10, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D11, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );

  //SPI
  GPIO_INIT_PIN(ACC_SCK, OUTPUT,          PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SDA, INPUT_PULLDOWN , PID_GPIO, 0, GPIO_POWER_RAIL_3V );

  //UART
  GPIO_INIT_PIN(UTX, INPUT_PULLUP, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(URX, INPUT_PULLUP, PID_GPIO, 0, GPIO_POWER_RAIL_3V );

  //OTHER
  GPIO_INIT_PIN(CAPI,     INPUT_PULLDOWN, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(CAPO,     INPUT_PULLDOWN, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(NC_P12,   INPUT_PULLDOWN, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(NC_P13,   INPUT_PULLDOWN, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
}
