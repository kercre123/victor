#include <stdint.h>
#include "MK02F12810.h"

#include "timer.h"
#include "power.h"
#include "spi.h"

#include "portable.h"
#include "../hal/hardware.h"

// This powers up the camera, OLED, and ESP8266 while respecting power sequencing rules
void Anki::Cozmo::HAL::Power::init()
{ 
  // Clear any I/Os that are default driven in K02
  // PTA0-3 are taken care of (SWD and POWER pins)
  // PTA18 and 19 are driven by default
  GPIO_IN(PTA, (1<<18) | (1<<19));
  SOURCE_SETUP(PTA, 18, SourceGPIO);
  SOURCE_SETUP(PTA, 19, SourceGPIO);

  // Initial state of the machine (Powered down, disabled)
  GPIO_SET(GPIO_CAM_PWDN, PIN_CAM_PWDN);
  GPIO_OUT(GPIO_CAM_PWDN, PIN_CAM_PWDN);
  SOURCE_SETUP(GPIO_CAM_PWDN, SOURCE_CAM_PWDN, SourceGPIO);

  GPIO_RESET(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);
  GPIO_OUT(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);
  SOURCE_SETUP(GPIO_CAM_OLED_RESET_N, SOURCE_CAM_OLED_RESET_N, SourceGPIO);

  #ifdef HEAD_1_0
  // Drive the OLED reset line low on startup
  GPIO_RESET(GPIO_OLED_RST, PIN_OLED_RST);
  GPIO_OUT(GPIO_OLED_RST, PIN_OLED_RST);
  SOURCE_SETUP(GPIO_OLED_RST, SOURCE_OLED_RST, SourceGPIO);
  #endif
  
  #ifndef HEAD_1_0
  GPIO_SET(GPIO_nV18_EN, PIN_nV18_EN);
  GPIO_OUT(GPIO_nV18_EN, PIN_nV18_EN);
  SOURCE_SETUP(GPIO_nV18_EN, SOURCE_nV18_EN, SourceGPIO);
  #endif
  GPIO_RESET(GPIO_POWEREN, PIN_POWEREN);
  GPIO_OUT(GPIO_POWEREN, PIN_POWEREN);
  SOURCE_SETUP(GPIO_POWEREN, SOURCE_POWEREN, SourceGPIO);

  // Configure spine communication
  GPIO_IN(GPIO_BODY_UART_RX, PIN_BODY_UART_RX);
  SOURCE_SETUP(GPIO_BODY_UART_RX, SOURCE_BODY_UART_RX, SourceGPIO);
  GPIO_RESET(GPIO_BODY_UART_TX, PIN_BODY_UART_TX);  // TX is inverted, idle = low
  GPIO_OUT(GPIO_BODY_UART_TX, PIN_BODY_UART_TX);
  SOURCE_SETUP(GPIO_BODY_UART_TX, SOURCE_BODY_UART_TX, SourceGPIO | SourcePullDown);

  GPIO_IN(GPIO_DEBUG_UART, PIN_DEBUG_UART);
  SOURCE_SETUP(GPIO_DEBUG_UART, SOURCE_DEBUG_UART, SourceGPIO | SourcePullUp);

  MicroWait(5000);

  // We are on likely on a test fixure, power up espressif
  if (!GPIO_READ(GPIO_DEBUG_UART)) {
    // Drive TX 
    GPIO_SET(GPIO_BODY_UART_TX, PIN_BODY_UART_TX);
    GPIO_OUT(GPIO_BODY_UART_TX, PIN_BODY_UART_TX);
    SOURCE_SETUP(GPIO_BODY_UART_TX, SOURCE_BODY_UART_TX, SourceGPIO); 
    
    enableEspressif(true);

    // XXX: For now, just wait - for 1.5, we have to run additional tests
    for(;;) ;
  }

  SOURCE_SETUP(GPIO_DEBUG_UART, SOURCE_DEBUG_UART, SourceGPIO);
}

void Anki::Cozmo::HAL::Power::enableEspressif(bool fixture)
{
  if (!fixture) {
    // Reset MISO - "recovery boot mode" signal
    GPIO_RESET(GPIO_MISO, PIN_MISO);
    GPIO_OUT(GPIO_MISO, PIN_MISO);
    SOURCE_SETUP(GPIO_MISO, SOURCE_MISO, SourceGPIO);

    // Weakly pull MOSI high to put ESP8266 into flash boot mode
    // We rely on the fixture to pull this strongly low and enter bootloader mode
    GPIO_IN(GPIO_MOSI, PIN_MOSI);
    SOURCE_SETUP(GPIO_MOSI, SOURCE_MOSI, SourceGPIO | SourcePullUp); 
  } else {
    // Pull down hard
    GPIO_RESET(GPIO_MOSI, PIN_MOSI);
    GPIO_OUT(GPIO_MOSI, PIN_MOSI);
    SOURCE_SETUP(GPIO_MOSI, SOURCE_MOSI, SourceGPIO);
    
    // Let UART work
    GPIO_IN(GPIO_MISO, PIN_MISO);
  }
  
  // Pull-down SCK during ESP8266 boot
  GPIO_RESET(GPIO_SCK, PIN_SCK);
  GPIO_OUT(GPIO_SCK, PIN_SCK);
  SOURCE_SETUP(GPIO_SCK, SOURCE_SCK, SourceGPIO);
  
  // Pull WS high to set correct boot mode on Espressif GPIO2 (flash or bootloader)
  GPIO_IN(GPIO_WS, PIN_WS);
  SOURCE_SETUP(GPIO_WS, SOURCE_WS, SourceGPIO | SourcePullUp);

  // Power sequencing
  #ifndef HEAD_1_0
  GPIO_RESET(GPIO_nV18_EN, PIN_nV18_EN);                    // Enable 1v8 rail
  MicroWait(1000);
  GPIO_SET(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);    // Release /RESET
  MicroWait(10);
  #endif
  GPIO_SET(GPIO_POWEREN, PIN_POWEREN);                      // Enable 2v8 / 3v3 rail
  MicroWait(1000);
  GPIO_RESET(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);  // Assert /RESET
  MicroWait(10);
  GPIO_RESET(GPIO_CAM_PWDN, PIN_CAM_PWDN);                  // Release camera /PWDN
  MicroWait(10);
  #ifdef HEAD_1_0
  GPIO_SET(GPIO_OLED_RST, PIN_OLED_RST);                    // Release /RESET (OLED)
  #endif
  GPIO_SET(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);    // Release /RESET
  MicroWait(5000);

  // Wait for Espressif to toggle out 4 words of I2SPI
  for (int i = 0; i < 32 * 512; i++)
  {
    while (GPIO_READ(GPIO_WS) & PIN_WS)     ;
    while (!(GPIO_READ(GPIO_WS) & PIN_WS))  ;
  }

  GPIO_IN(GPIO_MISO, PIN_MISO);
  GPIO_IN(GPIO_SCK, PIN_SCK);
}

void Anki::Cozmo::HAL::Power::disableEspressif(void)
{
  GPIO_RESET(GPIO_POWEREN, PIN_POWEREN);
}
