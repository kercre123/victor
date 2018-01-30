#include <stdint.h>
#include "board.h"
#include "hal_spi.h"
#include "hal_timer.h"
#include "bma222.h"

#define READ_BIT  0x80

static char* bma222e_reg_name(uint8_t addr)
{
  static char buf[5];
  switch(addr) {
    case BGW_SOFTRESET: return "BGW_SOFTRESET";
    case PMU_LOW_POWER: return "PMU_LOW_POWER";
    case PMU_LPW:       return "PMU_LPW";
    case BGW_SPI3_WDT:  return "BGW_SPI3_WDT";
    case PMU_RANGE:     return "PMU_RANGE";
    case PMU_BW:        return "PMU_BW";
    case FIFO_CONFIG_1: return "FIFO_CONFIG_1";
    case BGW_CHIPID:    return "BGW_CHIPID";
  }
  sprintf(buf, "0x%02x", addr);
  buf[4] = '\0';
  return buf;
}

uint8_t DataRead(uint8_t addr) {
  hal_spi_clr_cs();
  hal_spi_write( READ_BIT | addr );
  uint8_t dat = hal_spi_read();
  hal_spi_set_cs();
  hal_uart_printf("  %s (0x%02x) <- 0x%02x\n", bma222e_reg_name(addr), addr, dat);
  return dat;
}

void DataWrite(uint8_t addr, uint8_t dat) {
  hal_uart_printf("  %s (0x%02x) -> 0x%02x\n", bma222e_reg_name(addr), addr, dat);
  hal_spi_clr_cs();
  hal_spi_write( addr & ~(READ_BIT) );
  hal_spi_write( dat );
  hal_spi_set_cs();
}

void AccelInit()
{
  hal_spi_init();
  hal_uart_printf("AccelInit()\n");
  
  DataWrite(BGW_SOFTRESET, BGW_SOFTRESET_MAGIC); //Exit SUSPEND mode
  hal_timer_wait(1800); // Must wait 1.8ms after each reset
  
  // Enter STANDBY mode
  DataWrite(PMU_LOW_POWER, PMU_LOWPOWER_MODE); // 0x12 first
  DataWrite(PMU_LPW, PMU_SUSPEND);             // Then 0x11
  
  DataWrite(BGW_SPI3_WDT, 1); // 3 wire mode
  DataWrite(PMU_RANGE, RANGE_4G);
  DataWrite(PMU_BW, BW_500);
  
  // Throw out old FIFO data, reset errors, XYZ mode
  DataWrite(FIFO_CONFIG_1, FIFO_STREAM|FIFO_WORKAROUND); // Undocumented FIFO_WORKAROUND
  
  // From Bosch MIS-AN003: FIFO bug workarounds
  DataWrite(0x35, 0xCA);  // Undocumented sequence to turn off temperature sensor
  DataWrite(0x35, 0xCA);
  DataWrite(0x4F, 0x00);
  DataWrite(0x35, 0x0A);

  // Enter NORMAL mode
  DataWrite(PMU_LPW, PMU_NORMAL);
  
  // Read Chip ID
  int chipid = DataRead(BGW_CHIPID);
  hal_uart_printf("  chipid = 0x%02x %s\n", chipid, chipid == CHIPID ? "(match)" : "(--MISMATCH--)" );
}

