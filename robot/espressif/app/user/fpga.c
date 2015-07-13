/** @file Espressif Interface to the Lattice FPGA on the head board implementation
 * @author Daniel Casner <daniel@anki.com>
 */

#include "fpga.h"
#include "gpio.h"
#include "driver/spi.h"

#define CRESET 0
#define NCS    15

#define SPI_FREQUENCY 20000

#define SPI_FLUSH_TIME ((1000000 * 32 * SPI_FIFO_DEPTH) / SPI_FREQUENCY)

int8_t ICACHE_FLASH_ATTR fpgaInit(void)
{
  // Setup the SPI bus to talk to the FPGA
  spi_master_init(HSPI, SPI_FREQUENCY);

  // Set the pin MUX for the reset pin
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);

  // Disable the FPGA
  fpgaDisable();
}

void ICACHE_FLASH_ATTR fpgaDisable(void)
{
  // Mux the CS pin to be GPIO
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
  GPIO_OUTPUT_SET(GPIO_ID_PIN(NCS),    0);
  GPIO_OUTPUT_SET(GPIO_ID_PIN(CRESET), 0);
}

void ICACHE_FLASH_ATTR fpgaEnable(void)
{
  GPIO_OUTPUT_SET(GPIO_ID_PIN(CRESET), 1);
  os_delay_us(300); // Delay for the FPGA to boot up in slave mode
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_HSPI); // Allow the CS pin to be used by the SPI peripheral
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
  os_delay_us(SPI_FLUSH_TIME);
  while (spi_mast_busy(HSPI));
  return true;
}
