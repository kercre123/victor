#include <stdint.h>
#include "board.h"

//------------------------------------------------  
//    SPI
//------------------------------------------------

static uint16_t* const SPI_PIN_READ = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5));
static uint16_t* const SPI_PIN_SET = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 2);
static uint16_t* const SPI_PIN_RESET = (uint16_t*)(GPIO_BASE + (GPIO_PORT_0 << 5) + 4);

static uint16_t BIT_ACC_CS = 1 << ACC_CS_PIN;
static uint16_t BIT_ACC_PWR = 1 << ACC_PWR_PIN;
static uint16_t BIT_ACC_SCK = 1 << ACC_SCK_PIN;
static uint16_t BIT_ACC_SDA = 1 << ACC_SDA_PIN;

static void spi_write8(uint8_t value) {
  for (int i = 0x80; i; i >>= 1) {
    *SPI_PIN_RESET = BIT_ACC_SCK;
    *((value & i) ? SPI_PIN_SET : SPI_PIN_RESET) = BIT_ACC_SDA;
    *SPI_PIN_SET = BIT_ACC_SCK;
  }
}

static uint8_t spi_read8() {
  uint8_t value = 0;
  for (int i = 0x80; i; i >>= 1) {
    *SPI_PIN_RESET = BIT_ACC_SCK;
    __nop(); __nop();
    *SPI_PIN_SET = BIT_ACC_SCK;
    if (*SPI_PIN_READ & BIT_ACC_SDA) value |= i;
  }
  return value;
}

static void spi_write(uint8_t address, int length, const uint8_t* data) {
  *SPI_PIN_RESET = BIT_ACC_CS;
  spi_write8(address);
  while (length-- > 0) spi_write8(*(data++));
  *SPI_PIN_SET = BIT_ACC_CS;
}
static inline void spi_write1(uint8_t address, uint8_t dat) {
  spi_write(address, 1, &dat);
}

static void spi_read(uint8_t address, int length, uint8_t* data) {
  *SPI_PIN_RESET = BIT_ACC_CS;
  spi_write8(address | 0x80);
  GPIO_PIN_FUNC(ACC_SDA, INPUT, PID_GPIO);
  while (length-- > 0) *(data++) = spi_read8();
  GPIO_PIN_FUNC(ACC_SDA, OUTPUT, PID_GPIO);
  *SPI_PIN_SET = BIT_ACC_CS;
}
static inline uint8_t spi_read1(uint8_t address) {
  uint8_t dat;
  spi_read(address, 1, &dat);
  return dat;
}

//------------------------------------------------  
//    Accel
//------------------------------------------------

#include "accel.h"
#include "bma2x2.h" //from bosch bma253 'driver' (ignoring the obvious: bma2x2 != bma253)

#define ACCEL_DEBUG 1

#if ACCEL_DEBUG > 0
#include "hal_timer.h"
#include "hal_uart.h"
#define dbgprintf(...)   { hal_uart_printf(__VA_ARGS__); }
#else
#define dbgprintf(...)   {}
#endif

void accel_power_on(void) {
  *SPI_PIN_RESET = BIT_ACC_SDA;
  *SPI_PIN_SET = BIT_ACC_PWR;
  *SPI_PIN_SET = BIT_ACC_CS;
  *SPI_PIN_SET = BIT_ACC_SCK;
  hal_timer_wait(100);
}

void accel_power_off(void) {
  *SPI_PIN_RESET = BIT_ACC_SDA;
  *SPI_PIN_RESET = BIT_ACC_SCK;
  *SPI_PIN_RESET = BIT_ACC_CS;
  *SPI_PIN_RESET = BIT_ACC_PWR;
  hal_timer_wait(20);
}

bool inline accel_power_is_on(void) { return (*SPI_PIN_READ & BIT_ACC_PWR); }

bool accel_chipinit(void)
{
  dbgprintf("accel_chipinit():");
  /*if( !accel_power_is_on() ) {
    dbgprintf("  FAIL not powered\n");
    return 0;
  }*/
  
  spi_write1(BMA2x2_RST_ADDR, 0xB6); //softreset
  hal_timer_wait(1800); // Must wait 1.8ms after each reset
  
  spi_write1(BMA2x2_SERIAL_CTRL_ADDR, 0x01); // 3 wire mode
  spi_write1(BMA2x2_RANGE_SELECT_ADDR, BMA2x2_RANGE_4G);
  spi_write1(BMA2x2_BW_SELECT_ADDR, BMA2x2_BW_500HZ);
  
  // Read Chip ID
  #define CHIP_ID_BMA253  0xFA /*b'1111'1010*/
  uint8_t chipid = spi_read1(BMA2x2_CHIP_ID_ADDR);
  dbgprintf("  chipid = 0x%02x %s\n", chipid, chipid == CHIP_ID_BMA253 ? "(match)" : "(--MISMATCH--)" );
  
  //lowpower mode, sleep phase duration 0.5ms
  //spi_write1(BMA2x2_MODE_CTRL_ADDR, 0x40);
  //spi_write1(BMA2x2_LOW_POWER_MODE_REG, 0x40);
  
  //pull power on init failure (SPI funcs leave clk/cs driving high)
  if( chipid != CHIP_ID_BMA253 )
    accel_power_off();
  
  return chipid == CHIP_ID_BMA253;
}

int accel_read(void)
{
  if( accel_power_is_on() )
  {
    //XXX: read something
    return 1;
  }
  return -1;
}

