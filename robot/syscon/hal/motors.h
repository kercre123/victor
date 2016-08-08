#ifndef MOTORS_H
#define MOTORS_H

#include "portable.h"
#include <stdint.h>
#include <stddef.h>

enum MotorID
{
  MOTOR_LEFT_WHEEL,
  MOTOR_RIGHT_WHEEL,
  MOTOR_LIFT,
  MOTOR_HEAD,
  MOTOR_COUNT
};

typedef s32 Fixed;

namespace Motors {
  // Initialize the PWM peripheral on the designated pins in the source file.
  void init();
  
  void teardown(void);  // Disable motors safely and reconfigure GPIO
  void setup(void);
  
  void disable(bool disable);
  bool getChargeOkay();
  
  // Set the (unitless) power for a specified motor in the range [-798, 798].
  void setPower(u8 motorID, s16 power);
  Fixed getSpeed(u8 motorID);
  
  // Updates the PWM values for the timers in a safe manner
  void manage();

  // Print the raw encoder input values over the UART
  void printEncodersRaw();
  void getRawValues(uint32_t *positions);
  
  // Print the motor encoder value in binary
  void printEncoder(u8 motorID);

  
  s32 debugWheelsGetTicks(u8 motorID);
}

#endif
