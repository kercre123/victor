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
          GPIO_SET(GPIO_CAM_PWDN, PIN_CAM_PWDN);
          GPIO_OUT(GPIO_CAM_PWDN, PIN_CAM_PWDN);
          SOURCE_SETUP(GPIO_CAM_PWDN, SOURCE_CAM_PWDN, SourceGPIO);

          GPIO_RESET(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);
          GPIO_OUT(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);
          SOURCE_SETUP(GPIO_CAM_OLED_RESET_N, SOURCE_CAM_OLED_RESET_N, SourceGPIO);

          // NOTE: THESE TWO ARE ACTUALLY IDENTICAL, ONLY DOING THIS INCASE
          // WE DECIDE TO CHANGE PIN MAPPING

          if (*HW_VERSION != 0x01050000) {
            // Drive the OLED reset line low on startup
            GPIO_RESET(GPIO_OLED_RST, PIN_OLED_RST);
            GPIO_OUT(GPIO_OLED_RST, PIN_OLED_RST);
            SOURCE_SETUP(GPIO_OLED_RST, SOURCE_OLED_RST, SourceGPIO);
          } else {
            // Disable V18 rail
            GPIO_SET(GPIO_nV18_EN, PIN_nV18_EN);
            GPIO_OUT(GPIO_nV18_EN, PIN_nV18_EN);
            SOURCE_SETUP(GPIO_nV18_EN, SOURCE_nV18_EN, SourceGPIO);
          }
          
          GPIO_RESET(GPIO_POWEREN, PIN_POWEREN);
          GPIO_OUT(GPIO_POWEREN, PIN_POWEREN);
          SOURCE_SETUP(GPIO_POWEREN, SOURCE_POWEREN, SourceGPIO);

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

          // Power sequencing
          if (*HW_VERSION == 0x01050000) {
            GPIO_RESET(GPIO_nV18_EN, PIN_nV18_EN);                  // Enable 1v8 rail
            MicroWait(1000);
            GPIO_SET(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);  // Release /RESET
            MicroWait(10);
          }
          GPIO_SET(GPIO_POWEREN, PIN_POWEREN);                      // Enable 2v8 / 3v3 rail
          MicroWait(1000);
          GPIO_RESET(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);  // Assert /RESET
          MicroWait(10);
          GPIO_RESET(GPIO_CAM_PWDN, PIN_CAM_PWDN);                  // Release camera /PWDN
          MicroWait(10);

          #ifndef FCC_TEST
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

          if (*HW_VERSION != 0x01050000) {
            GPIO_SET(GPIO_OLED_RST, PIN_OLED_RST);                  // Release /RESET (OLED)
          }

          GPIO_SET(GPIO_CAM_OLED_RESET_N, PIN_CAM_OLED_RESET_N);    // Release /RESET
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
