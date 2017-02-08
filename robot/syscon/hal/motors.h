#ifndef MOTORS_H
#define MOTORS_H

#include "portable.h"
#include <stdint.h>
#include <stddef.h>

namespace Motors {
  // Initialize the PWM peripheral on the designated pins in the source file.
  void init();
  
  void teardown(void);  // Disable motors safely and reconfigure GPIO
  void setup(void);
  
  void disable(bool disable);
  bool getChargeOkay();
  
  // Set the (unitless) power for a specified motor in the range [-798, 798].
  void setPower(u8 motorID, s16 power);
  
  // Returns speed in tics per second
  Fixed getSpeed(u8 motorID);
  
  // Updates the PWM values for the timers in a safe manner
  void manage();
}

#endif
