#include "datasheet.h"
#include "board.h"
#include "accel.h"

#include "protocol.h"

#include "BMA253.h"

extern void (*ble_send)(uint8_t length, const void* data);

static volatile uint16_t* const SPI_READ  = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5));
static volatile uint16_t* const SPI_SET   = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 2);
static volatile uint16_t* const SPI_RESET = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 4);

static const uint16_t BIT_ACC_nCS = 1 << ACC_nCS_PIN;
static const uint16_t BIT_ACC_SCK = 1 << ACC_SCK_PIN;
static const uint16_t BIT_ACC_SDA = 1 << ACC_SDA_PIN;

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
    __nop(); __nop();
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

  for (int i = 70; i > 0; i--) __nop();  // About 32us
  spi_write8(BGW_SOFTRESET, 0xB6);  // Soft Reset Accelerometer

  for (int i = 4800; i > 0; i--) __nop();  // About 1.8ms
  spi_write8(BGW_SPI3_WDT, 0x01);   // Enter three wire SPI mode

  spi_write8(PMU_LOW_POWER, 0x40);  // Low power mode
  //spi_write8(PMU_LPW, 0x80);        // Suspend

  spi_write8(PMU_LPW, 0x00);        // Normal operating mode

  spi_write8(PMU_RANGE, 0x05);      // +4g range
  spi_write8(PMU_BW, 0x0D);         // 250hz BW (1000 samples per second)
  spi_write8(FIFO_CONFIG_1, 0x40);  // FIFO mode (XYZ)
}

void hal_acc_stop(void) {
  GPIO_PIN_FUNC(ACC_SCK, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(ACC_SDA, INPUT, PID_GPIO);
  GPIO_PIN_FUNC(ACC_nCS, INPUT, PID_GPIO);
  GPIO_CLR(ACC_PWR);
}

#define TAP_DEBOUNCE  90        // 90ms before tap is recognized
#define TAP_DURATION  10        // 10ms is max length of a tap (anything slower is rejected)
#define TAP_THRESH    (90*32)   // Equivalent to EP1/2G 10 (since I shift less and use 4G)

uint8_t   tap_count;

void hal_acc_tick(void) {
  int16_t axis[3];

  int frames = spi_read8(FIFO_STATUS) & 0x7F;
  while (frames-- > 0) {
    spi_read(FIFO_DATA, sizeof(axis), (uint8_t*)&axis);

    static int16_t last = 0;
    static uint8_t debounce = 0;
    static int16_t tapPos;
    static int16_t tapNeg;
    static bool    posFirst;
    
    int16_t current = axis[2];
    int16_t diff = current - last;
    last = current;

    if (debounce > 0) { // If debouncing...
      if (debounce > (TAP_DEBOUNCE-TAP_DURATION) && (diff < (posFirst ? -TAP_THRESH : TAP_DEBOUNCE)))
      {
        tap_count++;

        debounce = TAP_DEBOUNCE - TAP_DURATION;
      }
      debounce--;
    } else { // If not debouncing, look for a tap
      // Look for something big enough to start the debounce
      if (diff > TAP_THRESH)
        posFirst = 1;
      else if (diff < -TAP_THRESH)
        posFirst = 0;
      else
        continue;   // Nothing, just continue looking
      
      debounce = TAP_DEBOUNCE;
      tapPos = tapNeg = 0;      // New potential tap, reset min/max
    }

    // If we get here, we are in a potential tap, track min/max absolute acceleration
    if (tapNeg < diff)
      tapNeg = diff;
    if (tapPos > diff)
      tapPos = diff;
  }
}
