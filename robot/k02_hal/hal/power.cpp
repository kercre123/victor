#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "MK02F12810.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {    
      // This powers up the camera, OLED, and ESP8266 while respecting power sequencing rules
      void PowerInit()
      {  
        // I/Os used during startup
        GPIO_PIN_SOURCE(POWEREN,    PTA, 2);
        GPIO_PIN_SOURCE(SCK,        PTE, 17);
        GPIO_PIN_SOURCE(MOSI,       PTE, 18);
        GPIO_PIN_SOURCE(WS,         PTD, 4);
        GPIO_PIN_SOURCE(CAM_PWDN,   PTA, 1);
       
        // Clear any I/Os that are default driven in K02
        // PTA0-3 are taken care of (SWD and POWER pins)
        // PTA18 and 19 are driven by default
        GPIO_IN(PTA, (1<<18) | (1<<19));
        SOURCE_SETUP(PTA, 18, SourceGPIO);
        SOURCE_SETUP(PTA, 19, SourceGPIO);
        
        // Drive CAM_PWDN once analog power comes on
        GPIO_SET(GPIO_CAM_PWDN, PIN_CAM_PWDN);
        GPIO_OUT(GPIO_CAM_PWDN, PIN_CAM_PWDN);
        SOURCE_SETUP(GPIO_CAM_PWDN, SOURCE_CAM_PWDN, SourceGPIO);
        
        // Pull-down SCK during ESP8266 boot
        GPIO_RESET(GPIO_SCK, PIN_SCK);
        GPIO_OUT(GPIO_SCK, PIN_SCK);
        SOURCE_SETUP(GPIO_SCK, SOURCE_SCK, SourceGPIO);
        
        // Weakly pull MOSI high to put ESP8266 into flash boot mode
        // We rrely on the fixture to pull this strongly low and enter bootloader mode
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
      }
    }
  }
}
