#ifndef MOTORS_H
#define MOTORS_H

#include "portable.h"

#define MOTOR_COUNT 4

typedef s32 Fixed;

namespace Motors {
  // Initialize the PWM peripheral on the designated pins in the source file.
  void init();

  // Set the (unitless) power for a specified motor in the range [-798, 798].
  void setPower(u8 motorID, s16 power);
  Fixed getSpeed(u8 motorID);
  
  // Updates the PWM values for the timers in a safe manner
  void update();

  // Print the raw encoder input values over the UART
  void printEncodersRaw();

  // Print the motor encoder value in binary
  void printEncoder(u8 motorID);

  
  s32 debugWheelsGetTicks(u8 motorID);
}

#endif
