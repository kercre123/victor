#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "MK02F12810.h"

#include "hardware.h"
#include "power.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {    
      // This powers up the camera, OLED, and ESP8266 while respecting power sequencing rules
      namespace Power
      {  
        void enableEspressif(void)
        {
          // Pull-up MISO during ESP8266 boot
          GPIO_IN(GPIO_MISO, PIN_MISO);
          SOURCE_SETUP(GPIO_MISO, SOURCE_MISO, SourceGPIO | SourcePullUp);

          // Pull-down SCK during ESP8266 boot
          GPIO_RESET(GPIO_SCK, PIN_SCK);
          GPIO_OUT(GPIO_SCK, PIN_SCK);    // XXX: Driving SCK low here is bad for the ESP, why not pulldown?
          SOURCE_SETUP(GPIO_SCK, SOURCE_SCK, SourceGPIO);
          
          // Weakly pull MOSI high to put ESP8266 into flash boot mode
          // We rely on the fixture to pull this strongly low and enter bootloader mode
          GPIO_IN(GPIO_MOSI, PIN_MOSI);
          SOURCE_SETUP(GPIO_MOSI, SOURCE_MOSI, SourceGPIO | SourcePullUp); 

          // Pull WS high to set correct boot mode on Espressif GPIO2 (flash or bootloader)
          GPIO_IN(GPIO_WS, PIN_WS);
          SOURCE_SETUP(GPIO_WS, SOURCE_WS, SourceGPIO | SourcePullUp); 

          Anki::Cozmo::HAL::MicroWait(10000);
          
          // Turn on 2v8 and 3v3 rails
          GPIO_SET(GPIO_POWEREN, PIN_POWEREN);
          GPIO_OUT(GPIO_POWEREN, PIN_POWEREN);
          SOURCE_SETUP(GPIO_POWEREN, SOURCE_POWEREN, SourceGPIO);

          #ifndef ENABLE_FCC_TEST
          // Wait for Espressif to toggle out 4 words of I2SPI
          for (int i = 0; i < 32 * 512; i++)
          {
            while (GPIO_READ(GPIO_WS) & PIN_WS)     ;
            while (!(GPIO_READ(GPIO_WS) & PIN_WS))  ;
          }

          // Switch to 10MHz Espressif/external reference and 100MHz clock
          MCG_C1 &= ~MCG_C1_IREFS_MASK;
          // Wait for IREF to turn off
          while((MCG->S & MCG_S_IREFST_MASK))   ;
          // Wait for FLL to lock
          while((MCG->S & MCG_S_CLKST_MASK))    ;

          MicroWait(100);     // Because of erratum e7735: Wait 2 IRC cycles (or 2/32.768KHz)
          
          GPIO_IN(GPIO_SCK, PIN_SCK);   // XXX: Shouldn't we turn around SCK sooner? Are we driving against each other?
          #endif

          SOURCE_SETUP(GPIO_MISO, SOURCE_MISO, SourceGPIO);
        }
        
        void enterSleepMode(void)
        {
          NVIC_SystemReset();
        }
        
        void disableEspressif(void)
        {
          // TODO: CHANGE CLOCK TO INTERNAL OSCILLATOR
          // TODO: TURN OFF POWER TO ESPRESSIF
        }
      }
    }
  }
}
