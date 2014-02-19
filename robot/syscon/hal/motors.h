#ifndef MOTORS_H
#define MOTORS_H

#include "anki/cozmo/robot/hal.h"

// Initialize the PWM peripheral on the designated pins in the source file.
void MotorsInit();

// Set the (unitless) power for a specified motor
void MotorsSetPower(Anki::Cozmo::HAL::MotorID motorID, u16 power);

#endif
