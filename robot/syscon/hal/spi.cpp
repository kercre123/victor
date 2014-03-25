#include "spi.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "spi_master.h"

namespace
{
  const u8 PIN_SPI_MISO = 1;
  const u8 PIN_SPI_MOSI = 0;
  const u8 PIN_SPI_SCK = 30;  // SPI_SCLK_HEAD
}

void PowerInit()
{
  // Ensure MOSI is low
  nrf_gpio_pin_clear(PIN_SPI_MOSI);
  nrf_gpio_cfg_output(PIN_SPI_MOSI);
  
  const u8 PIN_VINs_EN = 11;
  nrf_gpio_pin_clear(PIN_VINs_EN);
  nrf_gpio_cfg_output(PIN_VINs_EN);
  
  const u8 PIN_VDDs_EN = 12;
  nrf_gpio_pin_set(PIN_VDDs_EN);
  nrf_gpio_cfg_output(PIN_VDDs_EN);
  
  MicroWait(50000);
  
  nrf_gpio_pin_clear(PIN_VDDs_EN);
  nrf_gpio_pin_set(PIN_VINs_EN);
  
  /*
  // Wait for MISO to go high (signaled from the head board)
  nrf_gpio_cfg_input(PIN_SPI_MISO, NRF_GPIO_PIN_PULLDOWN);
  MicroWait(10);  // Let pin settle
  while (!(NRF_GPIO->IN & (1 << PIN_SPI_MISO)))
    ;
  
  // Acknowledge the head board
  nrf_gpio_pin_set(PIN_SPI_MOSI);
  MicroWait(1000);  // Give headboard time to enable SPI
  */
}

void SPIInit()
{
  // Make sure the head board is alive and ready
  PowerInit();
  
  // Configure the pins
  nrf_gpio_cfg_output(PIN_SPI_SCK);
  nrf_gpio_cfg_output(PIN_SPI_MOSI);
  nrf_gpio_cfg_input(PIN_SPI_MISO, NRF_GPIO_PIN_NOPULL);
  
  // Not sure if it needs to be powered on or if it already is. Let's be safe.
  NRF_SPI0->POWER = 1;
  
  // Configure the peripheral for the physical pins are being used for SPI
  NRF_SPI0->PSELSCK = PIN_SPI_SCK;
  NRF_SPI0->PSELMISO = PIN_SPI_MISO;
  NRF_SPI0->PSELMOSI = PIN_SPI_MOSI;
  
  // Base frequency starts at 0x02000000
  NRF_SPI0->FREQUENCY = (0x02000000 << Freq_4Mbps);
 
  // Configure for SPI_MODE0 with LSB-first transmission
  // NOTE: Header does not match docs on these bits. Tread with care.
  NRF_SPI0->CONFIG = (SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos) |
    (SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos) |
    (SPI_CONFIG_ORDER_LsbFirst << SPI_CONFIG_ORDER_Pos);
  
  NRF_SPI0->EVENTS_READY = 0;
  
  // Enable the SPI peripheral
  NRF_SPI0->ENABLE = 1;
}

void SPITransmitReceive(u16 length, const u8* dataTX, u8* dataRX)
{
  u16 i;
  
  for (i = 0; i < length; i++)
  {
    NRF_SPI0->TXD = (u32)dataTX[i];
    
    while (!NRF_SPI0->EVENTS_READY)
      ;
    
    dataRX[i] = NRF_SPI0->RXD;
    
    NRF_SPI0->EVENTS_READY = 0;
  }
}
