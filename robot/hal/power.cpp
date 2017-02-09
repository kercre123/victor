#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "MK02F12810.h"

#include "hardware.h"
#include "power.h"
#include "anki/cozmo/robot/buildTypes.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {    
      // This powers up the camera, OLED, and ESP8266 while respecting power sequencing rules
      namespace Power
      {  
        void enableExternal(void)
        {
          // Setup initial state for power on sequencing
          // Initial state of the machine (Powered down, disabled)
          GPIO_SET(CAM_PWDN);
          GPIO_OUT(CAM_PWDN);
          SOURCE_SETUP(CAM_PWDN, SourceGPIO);

          GPIO_RESET(CAM_OLED_RESET_N);
          GPIO_OUT(CAM_OLED_RESET_N);
          SOURCE_SETUP(CAM_OLED_RESET_N, SourceGPIO);

          // NOTE: THESE TWO ARE ACTUALLY IDENTICAL, ONLY DOING THIS INCASE
          // WE DECIDE TO CHANGE PIN MAPPING

          if (*HW_VERSION != 0x01050000) {
            // Drive the OLED reset line low on startup
            GPIO_RESET(OLED_RST);
            GPIO_OUT(OLED_RST);
            SOURCE_SETUP(OLED_RST, SourceGPIO);
          } else {
            // Disable V18 rail
            GPIO_SET(nV18_EN);
            GPIO_OUT(nV18_EN);
            SOURCE_SETUP(nV18_EN, SourceGPIO);
          }
          
          GPIO_RESET(POWEREN);
          GPIO_OUT(POWEREN);
          SOURCE_SETUP(POWEREN, SourceGPIO);

          // Pull-up MISO during ESP8266 boot
          GPIO_IN(MISO);
          SOURCE_SETUP(MISO, SourceGPIO | SourcePullUp);

          // Pull-down SCK during ESP8266 boot
          GPIO_RESET(SCK);
          GPIO_OUT(SCK);    // XXX: Driving SCK low here is bad for the ESP, why not pulldown?
          SOURCE_SETUP(SCK, SourceGPIO);
          
          // Weakly pull MOSI high to put ESP8266 into flash boot mode
          // We rely on the fixture to pull this strongly low and enter bootloader mode
          GPIO_IN(MOSI);
          SOURCE_SETUP(MOSI, SourceGPIO | SourcePullUp); 

          // Pull WS high to set correct boot mode on Espressif GPIO2 (flash or bootloader)
          GPIO_IN(WS);
          SOURCE_SETUP(WS, SourceGPIO | SourcePullUp); 

          Anki::Cozmo::HAL::MicroWait(10000);

          // Power sequencing
          if (*HW_VERSION == 0x01050000) {
            GPIO_RESET(nV18_EN);                  // Enable 1v8 rail
            MicroWait(1000);
            GPIO_SET(CAM_OLED_RESET_N);  // Release /RESET
            MicroWait(10);
          }
          GPIO_SET(POWEREN);                      // Enable 2v8 / 3v3 rail
          MicroWait(1000);
          GPIO_RESET(CAM_OLED_RESET_N);  // Assert /RESET
          MicroWait(10);
          GPIO_RESET(CAM_PWDN);                  // Release camera /PWDN
          MicroWait(10);

          #ifndef FCC_TEST
          // Wait for Espressif to toggle out 4 words of I2SPI
          for (int i = 0; i < 32 * 512; i++)
          {
            while (GPIO_READ(WS))     ;
            while (!GPIO_READ(WS))  ;
          }

          // Switch to 10MHz Espressif/external reference and 100MHz clock
          MCG_C1 &= ~MCG_C1_IREFS_MASK;
          // Wait for IREF to turn off
          while((MCG->S & MCG_S_IREFST_MASK))   ;
          // Wait for FLL to lock
          while((MCG->S & MCG_S_CLKST_MASK))    ;

          MicroWait(100);     // Because of erratum e7735: Wait 2 IRC cycles (or 2/32.768KHz)
          
          GPIO_IN(SCK);   // XXX: Shouldn't we turn around SCK sooner? Are we driving against each other?
          #endif

          SOURCE_SETUP(MISO, SourceGPIO);

          if (*HW_VERSION != 0x01050000) {
            GPIO_SET(OLED_RST);                  // Release /RESET (OLED)
          }

          GPIO_SET(CAM_OLED_RESET_N);    // Release /RESET
          MicroWait(5000);
        }

        void enterSleepMode(void)
        {
          NVIC_SystemReset();
        }
        
        void disableExternal(void)
        {
          // TODO: CHANGE CLOCK TO INTERNAL OSCILLATOR
          // TODO: TURN OFF POWER TO ESPRESSIF
        }
      }
    }
  }
}
