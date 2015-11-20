#include <stdint.h>
#include "MK02F12810.h"
#include "timer.h"

#include "portable.h"
#include "../../k02_hal/hal/hardware.h"

// This powers up the camera, OLED, and ESP8266 while respecting power sequencing rules
extern void PowerInit()
{   
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
  
  // Pull MOSI low to put ESP8266 into bootloader mode
  // XXX: Normally, you should drive this high and rely on the cable to enter debug mode!
  GPIO_IN(GPIO_MOSI, PIN_MOSI);
  SOURCE_SETUP(GPIO_MOSI, SOURCE_MOSI, SourceGPIO | SourcePullUp);  
  
  MicroWait(10000);
  
  // Turn on 2v8 and 3v3 rails
  GPIO_SET(GPIO_POWEREN, PIN_POWEREN);
  GPIO_OUT(GPIO_POWEREN, PIN_POWEREN);
  SOURCE_SETUP(GPIO_POWEREN, SOURCE_POWEREN, SourceGPIO);
}
