/**
 * File:        hal_motors.cpp
 *
 * Description: HAL motor interface
 *
 **/


// System Includes
#include <stdio.h>
#include <assert.h>

// Our Includes
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"

#include "schema/messages.h"


// Debugging Defines
#define REALTIME_CONSOLE_OUTPUT 0 //Print status to console
#define MOTOR_OF_INTEREST MOTOR_RIGHT_WHEEL  //print status of this motor
#define STR(s)  #s
#define DEFNAME(s) STR(s)


#if REALTIME_CONSOLE_OUTPUT > 0
#define SAVE_MOTOR_POWER(motor, power)  internalData_.motorPower[motor]=power
#define CONSOLE_DATA(decl) decl
#else
#define SAVE_MOTOR_POWER(motor, power)
#define CONSOLE_DATA(decl)
#endif

namespace Anki {
namespace Vector {

static_assert(EnumToUnderlyingType(MotorID::MOTOR_LEFT_WHEEL) == MOTOR_LEFT, "Robot/Spine CLAD Mimatch");
static_assert(EnumToUnderlyingType(MotorID::MOTOR_RIGHT_WHEEL) == MOTOR_RIGHT, "Robot/Spine CLAD Mimatch");
static_assert(EnumToUnderlyingType(MotorID::MOTOR_LIFT) == MOTOR_LIFT, "Robot/Spine CLAD Mimatch");
static_assert(EnumToUnderlyingType(MotorID::MOTOR_HEAD) == MOTOR_HEAD, "Robot/Spine CLAD Mimatch");

// From hal.cpp
extern BodyToHead* bodyData_; //buffers are owned by the code that fills them. Spine owns this one
extern HeadToBody headData_;  //-we own this one.

namespace { // "Private members"

  //map power -1.0 .. 1.0 to -32767 to 32767
  static const f32 HAL_MOTOR_POWER_SCALE = 0x7FFF;
  static const f32 HAL_MOTOR_POWER_OFFSET = 0;

  //convert per syscon tick -> /sec
  static const f32 HAL_SEC_PER_TICK = (1.0 / 256) / 48000000;

  //encoder counts -> mm or deg
  static f32 HAL_MOTOR_POSITION_SCALE[MOTOR_COUNT] = {
    ((       0.96 * 29.0 * 0.25 * 3.14159265359) / 172.3), //Left Tread mm (Magic 96% corrective fudge factor, 29mm wheel diameter)
    ((-1.0 * 0.96 * 29.0 * 0.25 * 3.14159265359) / 172.3), //Right Tread mm
    (0.25 * 3.14159265359) / 149.7,     //Lift radians
    (0.25 * 3.14159265359) / 366.211,   //Head radians
  };

  static f32 HAL_MOTOR_DIRECTION[MOTOR_COUNT] = {
    1.0, -1.0, 1.0, 1.0
  };

  static f32 HAL_HEAD_MOTOR_CALIB_POWER = -0.3f;

  static f32 HAL_LIFT_MOTOR_CALIB_POWER = -0.4f;

  struct {
    s32 motorOffset[MOTOR_COUNT];
    CONSOLE_DATA(f32 motorPower[MOTOR_COUNT]);
  } internalData_;
  
} // "private" namespace


// Returns the motor power used for calibration [-1.0, 1.0]
float HAL::MotorGetCalibPower(MotorID motor)
{
  f32 power = 0.f;
  switch (motor) 
  {
    case MotorID::MOTOR_LIFT:
      power = HAL_LIFT_MOTOR_CALIB_POWER;
      break;
    case MotorID::MOTOR_HEAD:
      power = HAL_HEAD_MOTOR_CALIB_POWER;
      break;
    default:
      AnkiError("HAL.MotorGetCalibPower.InvalidMotorType", "%s", EnumToString(motor));
      assert(false);
      break;
  }
  return power;
}

// Set the motor power in the unitless range [-1.0, 1.0]
void HAL::MotorSetPower(MotorID motor, f32 power)
{
  const auto m = EnumToUnderlyingType(motor);
  assert(m < MOTOR_COUNT);
  SAVE_MOTOR_POWER(m, power);
  headData_.motorPower[m] = HAL_MOTOR_POWER_OFFSET + HAL_MOTOR_POWER_SCALE * power * HAL_MOTOR_DIRECTION[m];
}

// Reset the internal position of the specified motor to 0
void HAL::MotorResetPosition(MotorID motor)
{
  const auto m = EnumToUnderlyingType(motor);
  assert(m < MOTOR_COUNT);
  internalData_.motorOffset[m] = bodyData_->motor[m].position;
}


// Returns units based on the specified motor type:
// Wheels are in mm/s, everything else is in degrees/s.
f32 HAL::MotorGetSpeed(MotorID motor)
{
  const auto m = EnumToUnderlyingType(motor);
  assert(m < MOTOR_COUNT);

  // Every frame, syscon sends the last detected speed as a two part number:
  // `delta` encoder counts, and `time` span for those counts.
  // syscon only changes the value when counts are detected
  // if no counts for ~125ms (i.e. MAX_ENCODER_FRAMES), will report 0/0
  if (bodyData_->motor[m].time != 0) {
    float countsPerTick = (float)bodyData_->motor[m].delta / bodyData_->motor[m].time;
    return (countsPerTick / HAL_SEC_PER_TICK) * HAL_MOTOR_POSITION_SCALE[m];
  }
  return 0.0; //if time is 0, it's not moving.
}

// Returns units based on the specified motor type:
// Wheels are in mm since reset, everything else is in degrees.
f32 HAL::MotorGetPosition(MotorID motor)
{
  const auto m = EnumToUnderlyingType(motor);
  assert(m < MOTOR_COUNT);
  return (bodyData_->motor[m].position - internalData_.motorOffset[m]) * HAL_MOTOR_POSITION_SCALE[m];
}

void PrintConsoleOutput(void)
{
#if REALTIME_CONSOLE_OUTPUT > 0
  {
    printf("FC:%d ", bodyData_->framecounter);
    // printf("%s: ", DEFNAME(MOTOR_OF_INTEREST));
    printf("Pos: %d %d %d %d", bodyData_->motor[(int)MotorID::MOTOR_HEAD].position,
                               bodyData_->motor[(int)MotorID::MOTOR_LIFT].position,
                               bodyData_->motor[(int)MotorID::MOTOR_LEFT_WHEEL].position,
                               bodyData_->motor[(int)MotorID::MOTOR_RIGHT_WHEEL].position);
    // printf("pos = %f ", HAL::MotorGetPosition(MotorID::MOTOR_OF_INTEREST));
    // printf("spd = %f ", HAL::MotorGetSpeed(MotorID::MOTOR_OF_INTEREST));
    // printf("pow = %f ", internalData_.motorPower[(int)MotorID::MOTOR_OF_INTEREST]);
    // printf("cliff = %d %d% d% d ",bodyData_->cliffSense[0],bodyData_->cliffSense[1],bodyData_->cliffSense[2],bodyData_->cliffSense[3]);
    // printf("prox = %d %d ",bodyData_->proximity.rangeStatus, bodyData_->proximity.rangeMM);
    printf("\n");
  }
#endif
}


} // namespace Vector
} // namespace Anki

