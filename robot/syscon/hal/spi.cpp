#include "spi.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "spi_master.h"

namespace
{
  const u8 PIN_SPI_MISO = 1;
  const u8 PIN_SPI_MOSI = 0;
  const u8 PIN_SPI_SCK = 30;  // SPI_SCLK_HEAD
}

void SPIInit()
{
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
  NRF_SPI0->FREQUENCY = (0x02000000 << Freq_1Mbps);
 
  // Configure for SPI_MODE0 with LSB-first transmission
  NRF_SPI0->CONFIG = (SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos) |
    (SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos) |
    (SPI_CONFIG_ORDER_LsbFirst << SPI_CONFIG_ORDER_Pos);
  
  NRF_SPI0->EVENTS_READY = 0;
  
  // Enable the SPI peripheral
  NRF_SPI0->ENABLE = 1;
}

s32 SPITransmitReceive(u16 length, const u8* dataTX, u8* dataRX)
{
  using namespace Anki::Cozmo::HAL;
  
  u16 i;
  const u32 TIMEOUT = 4000;  // 4ms
  
  for (i = 0; i < length; i++)
  {
    NRF_SPI0->TXD = (u32)dataTX[i];
    
    while (!NRF_SPI0->EVENTS_READY)
      ;
    
    dataRX[i] = NRF_SPI0->RXD;
    
    NRF_SPI0->EVENTS_READY = 0;
  }
  
  return 0;
}
