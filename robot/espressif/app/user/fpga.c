/** @file Espressif Interface to the Lattice FPGA on the head board implementation
 * @author Daniel Casner <daniel@anki.com>
 */

#include "fpga.h"
#include "gpio.h"
#include "driver/spi.h"

#define CRESET 0

int8_t ICACHE_FLASH_ATTR fpgaInit(void)
{
  // Set the pin MUX for the reset pin
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
  // Disable the FPGA
  fpgaDisable();
}

void ICACHE_FLASH_ATTR fpgaDisable(void)
{
  GPIO_OUTPUT_SET(GPIO_ID_PIN(CRESET), 0);
}

void ICACHE_FLASH_ATTR fpgaEnable(void)
{
  GPIO_OUTPUT_SET(GPIO_ID_PIN(CRESET), 1);
}

bool ICACHE_FLASH_ATTR fpgaWriteBitstream(uint32* data)
{
  uint8_t i;
  for (i=0; i < SPI_FIFO_DEPTH; ++i)
  {
    if (!spi_mast_write(HSPI, data[i]))
    {
      os_printf("FPGA ERR: Couldn't add word to SPI FIFO %d\r\n", i);
      return false;
    }
  }
  spi_mast_start_transaction(HSPI, 0, 0, 0, 0, SPI_FIFO_DEPTH*32, 0, 0);
  os_delay_us(260); // Write time for full FIFO at 2MHz
  while (spi_mast_busy(HSPI));
  return true;
}
