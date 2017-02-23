#include <stdint.h>
#include "MK02F12810.h"

#include "timer.h"
#include "power.h"
#include "spi.h"

#include "portable.h"
#include "../hal/hardware.h"

GPIO_PIN_SOURCE(PRE_CONFIGURED_1, PTA, 18);
GPIO_PIN_SOURCE(PRE_CONFIGURED_2, PTA, 19);

// This powers up the camera, OLED, and ESP8266 while respecting power sequencing rules
void Anki::Cozmo::HAL::Power::init()
{ 
  // Clear any I/Os that are default driven in K02
  // PTA0-3 are taken care of (SWD and POWER pins)
  // PTA18 and 19 are driven by default
  GPIO_IN(PRE_CONFIGURED_1);
  GPIO_IN(PRE_CONFIGURED_2);
  SOURCE_SETUP(PRE_CONFIGURED_1, SourceGPIO);
  SOURCE_SETUP(PRE_CONFIGURED_2, SourceGPIO);

  // Setup initial state for power on sequencing
  // Initial state of the machine (Powered down, disabled)
  GPIO_RESET(POWEREN);
  GPIO_SET(CAM_PWDN);
  GPIO_RESET(CAM_RESET_N);
  GPIO_RESET(OLED_RESET_N);

  // Configure pins as outputs
  GPIO_OUT(POWEREN);
  SOURCE_SETUP(POWEREN, SourceGPIO);
  GPIO_OUT(CAM_PWDN);
  SOURCE_SETUP(CAM_PWDN, SourceGPIO);
  GPIO_OUT(CAM_RESET_N);
  SOURCE_SETUP(CAM_RESET_N, SourceGPIO);
  GPIO_OUT(OLED_RESET_N);
  SOURCE_SETUP(OLED_RESET_N, SourceGPIO);
  MicroWait(3000);

  // Configure spine communication
  GPIO_IN(BODY_UART_RX);
  SOURCE_SETUP(BODY_UART_RX, SourceGPIO);
  GPIO_RESET(BODY_UART_TX);  // TX is inverted, idle = low
  GPIO_OUT(BODY_UART_TX);
  SOURCE_SETUP(BODY_UART_TX, SourceGPIO | SourcePullDown);

  GPIO_IN(DEBUG_UART);
  SOURCE_SETUP(DEBUG_UART, SourceGPIO | SourcePullUp);

  MicroWait(5000);

  // We are on likely on a test fixure, power up espressif
  if (!GPIO_READ(DEBUG_UART)) {
    // Drive TX 
    GPIO_SET(BODY_UART_TX);
    GPIO_OUT(BODY_UART_TX);
    SOURCE_SETUP(BODY_UART_TX, SourceGPIO); 
    
    enableEspressif(true);

    // XXX: For now, just wait - for 1.5, we have to run additional tests
    for(;;) ;
  }

  SOURCE_SETUP(DEBUG_UART, SourceGPIO);
}

void Anki::Cozmo::HAL::Power::enableEspressif(bool fixture)
{
  if (!fixture) {
    // Reset MISO - "recovery boot mode" signal
    GPIO_RESET(MISO);
    GPIO_OUT(MISO);
    SOURCE_SETUP(MISO, SourceGPIO);

    // Weakly pull MOSI high to put ESP8266 into flash boot mode
    // We rely on the fixture to pull this strongly low and enter bootloader mode
    GPIO_IN(MOSI);
    SOURCE_SETUP(MOSI, SourceGPIO | SourcePullUp); 
  } else {
    // Pull down hard
    GPIO_RESET(MOSI);
    GPIO_OUT(MOSI);
    SOURCE_SETUP(MOSI, SourceGPIO);
    
    // Let UART work
    GPIO_IN(MISO);
  }
  
  // Pull-down SCK during ESP8266 boot
  GPIO_RESET(SCK);
  GPIO_OUT(SCK);
  SOURCE_SETUP(SCK, SourceGPIO);
  
  // Pull WS high to set correct boot mode on Espressif GPIO2 (flash or bootloader)
  GPIO_IN(WS);
  SOURCE_SETUP(WS, SourceGPIO | SourcePullUp);

  // Power-up Sequence
  GPIO_SET(OLED_RESET_N);
  MicroWait(3000);

  GPIO_SET(POWEREN);
  MicroWait(10);

  GPIO_RESET(CAM_PWDN);
  MicroWait(10);

  GPIO_SET(CAM_RESET_N);
  MicroWait(10);

  // Wait for Espressif to toggle out 4 words of I2SPI
  for (int i = 0; i < 32 * 512; i++)
  {
    while (GPIO_READ(WS))     ;
    while (!GPIO_READ(WS))  ;
  }

  GPIO_IN(MISO);
  GPIO_IN(SCK);
}

void Anki::Cozmo::HAL::Power::disableEspressif(void)
{
  GPIO_RESET(POWEREN);
}
