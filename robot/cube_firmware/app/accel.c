#include "datasheet.h"
#include "board.h"
#include "accel.h"

#include "BMA253.h"

static uint16_t* const SPI_PIN_READ  = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5));
static uint16_t* const SPI_PIN_SET   = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 2);
static uint16_t* const SPI_PIN_RESET = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 4);

static uint16_t BIT_ACC_CS  = 1 << ACC_nCS_PIN;
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

void hal_acc_init(void) {
  GPIO_INIT_PIN(ACC_PWR, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_nCS, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SCK, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SDA, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );

  // Enter Half-duplex SPI mode
  static const uint8_t MODE_SPI3 = 0x01;
  spi_write(0x34, sizeof(MODE_SPI3), &MODE_SPI3);
}

void hal_acc_stop(void) {
  GPIO_INIT_PIN(ACC_PWR, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_nCS, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SCK, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SDA, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
}
