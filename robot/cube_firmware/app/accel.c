#include "datasheet.h"
#include "board.h"
#include "accel.h"

#include "protocol.h"

#include "BMA253.h"

extern void (*ble_send)(uint8_t length, const void* data);

static volatile uint16_t* const SPI_READ  = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5));
static volatile uint16_t* const SPI_SET   = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 2);
static volatile uint16_t* const SPI_RESET = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 4);

#define BIT_ACC_nCS (1 << ACC_nCS_PIN)
#define BIT_ACC_SCK (1 << ACC_SCK_PIN)
#define BIT_ACC_SDA (1 << ACC_SDA_PIN)

static void spi_write_byte(uint8_t value) {
  for (int i = 0x80; i; i >>= 1) {
    *SPI_RESET = BIT_ACC_SCK;
    *((value & i) ? SPI_SET : SPI_RESET) = BIT_ACC_SDA;
    *SPI_SET = BIT_ACC_SCK;
  }
}

static uint8_t spi_read_byte() {
  uint8_t value = 0;
  for (int i = 0x80; i; i >>= 1) {
    *SPI_RESET = BIT_ACC_SCK;
    __asm("nop"); __asm("nop");
    *SPI_SET = BIT_ACC_SCK;
    if (BIT_ACC_SDA & *SPI_READ) value |= i;
  }

  return value;
}

static void spi_write(uint8_t address, int length, const uint8_t* data) {
  *SPI_RESET = BIT_ACC_nCS;
  spi_write_byte(address);
  while (length-- > 0) spi_write_byte(*(data++));
  *SPI_SET = BIT_ACC_nCS;
}

static void spi_read(uint8_t address, int length, uint8_t* data) {
  *SPI_RESET = BIT_ACC_nCS;
  spi_write_byte(address | 0x80);
  GPIO_PIN_FUNC(ACC_SDA, INPUT, PID_GPIO);
  while (length-- > 0) *(data++) = spi_read_byte();
  GPIO_PIN_FUNC(ACC_SDA, OUTPUT, PID_GPIO);
  *SPI_SET = BIT_ACC_nCS;
}

static uint8_t spi_read8(uint8_t address) {
  uint8_t data;
  spi_read(address, sizeof(data), &data);
  return data;
}

static void spi_write8(uint8_t address, uint8_t data) {
  spi_write(address, sizeof(data), &data);
}

void hal_acc_init(void) {
  GPIO_INIT_PIN(ACC_PWR, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SDA, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_SCK, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(ACC_nCS, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );

  for (int i = 70; i > 0; i--) __asm("nop");  // About 32us
  //spi_write8(BGW_SOFTRESET, 0xB6);  // Soft Reset Accelerometer

  for (int i = 4800; i > 0; i--) __asm("nop");  // About 1.8ms
  spi_write8(BGW_SPI3_WDT, 0x01);   // Enter three wire SPI mode

  //spi_write8(PMU_LOW_POWER, 0x40);  // Low power mode
  //spi_write8(PMU_LPW, 0x80);        // Suspend

  //spi_write8(PMU_RANGE, 0x05);      // +4g range
  //spi_write8(PMU_BW, 0x0E);         // 500hz BW
  //spi_write8(FIFO_CONFIG_1, 0x41);  // FIFO mode (XYZ)

  //spi_write8(PMU_LPW, 0x00);        // Normal operating mode

  uint8_t chip_id = spi_read8(BGW_CHIPID);
  if (chip_id != 0xFA)
  {
    Payload p;
    p.command = COMMAND_ACCEL_FAILURE;
    p.flags = chip_id;
    ble_send(sizeof(p), &p);
  }
}

void hal_acc_stop(void) {
  GPIO_PIN_FUNC(ACC_SCK, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(ACC_SDA, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(ACC_nCS, INPUT, PID_GPIO);
  GPIO_CLR(ACC_PWR);
}

void hal_acc_tick(void) {
  static uint8_t data[18];
  static int bytes;

  while (spi_read8(FIFO_STATUS) & 0x7F) 
  {
    data[bytes++] = spi_read8(FIFO_DATA);

    if (bytes == sizeof(data)) {
      ble_send(sizeof(data), data);
      bytes = 0;
    }
  }
}
