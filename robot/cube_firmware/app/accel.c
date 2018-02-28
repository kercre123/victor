#include "datasheet.h"
#include "board.h"
#include "accel.h"

#include "protocol.h"

#include "BMA253.h"

extern void (*ble_send)(uint8_t length, const void* data);

static uint16_t* const SPI_READ  = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5));
static uint16_t* const SPI_SET   = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 2);
static uint16_t* const SPI_RESET = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 4);

static const uint16_t BIT_ACC_nCS = 1 << ACC_nCS_PIN;
static const uint16_t BIT_ACC_SCK = 1 << ACC_SCK_PIN;
static const uint16_t BIT_ACC_SDA = 1 << ACC_SDA_PIN;

void spi_write8(uint8_t value) {
  for (int i = 0x80; i; i >>= 1) {
    *SPI_RESET = BIT_ACC_SCK;
    *((value & i) ? SPI_SET : SPI_RESET) = BIT_ACC_SDA;
    *SPI_SET = BIT_ACC_SCK;
  }
}

uint8_t spi_read8() {
  uint8_t value = 0;
  for (int i = 0x80; i; i >>= 1) {
    *SPI_RESET = BIT_ACC_SCK;
    __nop(); __nop();
    *SPI_SET = BIT_ACC_SCK;
    if (*SPI_READ & BIT_ACC_SDA) value |= i;
  }
  return value;
}

void spi_write(uint8_t address, int length, const uint8_t* data) {
  *SPI_RESET = BIT_ACC_nCS;
  spi_write8(address);
  while (length-- > 0) spi_write8(*(data++));
  *SPI_SET = BIT_ACC_nCS;
}

void spi_read(uint8_t address, int length, uint8_t* data) {
  *SPI_RESET = BIT_ACC_nCS;
  spi_write8(address | 0x80);
  GPIO_PIN_FUNC(ACC_SDA, INPUT, PID_GPIO);
  while (length-- > 0) *(data++) = spi_read8();
  GPIO_PIN_FUNC(ACC_SDA, OUTPUT, PID_GPIO);
  *SPI_SET = BIT_ACC_nCS;
}

void hal_acc_init(void) {
  GPIO_INIT_PIN(ACC_PWR, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SDA, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SCK, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_nCS, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );

  static const uint8_t SOFT_RESET = 0xB6;
  spi_write(BGW_SOFTRESET, sizeof(SOFT_RESET), &SOFT_RESET);
}

void hal_acc_stop(void) {
  GPIO_PIN_FUNC(ACC_SCK, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(ACC_SDA, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(ACC_nCS, INPUT, PID_GPIO);
  GPIO_CLR(ACC_PWR);
}

void hal_acc_tick(void) {
  static int countdown = 200;
  if (countdown && !--countdown) {
    // Enter Half-duplex SPI mode
    static const uint8_t MODE_SPI3 = 0x01;
    uint8_t chip_id;

    spi_write(0x34, sizeof(MODE_SPI3), &MODE_SPI3);
    spi_read (BGW_CHIPID, sizeof(chip_id), &chip_id);

    ble_send(sizeof(chip_id), &chip_id);
  }
}
