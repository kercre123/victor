/**
 * File:        hal.cpp
 *
 * Description: Hardware Abstraction Layer for robot process
 *
 **/


// System Includes
#include <chrono>
#include <assert.h>

// Our Includes
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "../spine/spine_hal.h"
#include "../spine/cc_commander.h"

#include "schema/messages.h"
#include "clad/types/proxMessages.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"


namespace Anki {
namespace Cozmo {

BodyToHead* bodyData_; //buffers are owned by the code that fills them. Spine owns this one
HeadToBody headData_;  //-we own this one.

namespace { // "Private members"

  s32 robotID_ = -1;

  // TimeStamp offset
  std::chrono::steady_clock::time_point timeOffset_ = std::chrono::steady_clock::now();


#ifdef USING_ANDROID_PHONE
  BodyToHead dummyBodyData_ = {
    .cliffSense = {800, 800, 800, 800}
  };
#endif

  // update every tick of the robot:
  // some touch values are 0xFFFF, which we want to ignore
  // so we cache the last non-0xFFFF value and return this as the latest touch sensor reading
  u16 lastValidTouchIntensity_;

} // "private" namespace

// Forward Declarations
Result InitMotor();
Result InitRadio();
void InitIMU();
void ProcessIMUEvents();
void ProcessTouchLevel(void);
void PrintConsoleOutput(void);


#define MAX_RETRIES 4
#define SPINE_DATA_TIMEOUT_MS 500
Result GetSpineDataFrame(void)
{
  int_fast8_t retries = MAX_RETRIES;
  while (retries--) {
    const SpineMessageHeader* hdr = (const SpineMessageHeader*)
      hal_get_next_frame(SPINE_DATA_TIMEOUT_MS/MAX_RETRIES);
    if (!hdr) { continue; }
    if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
      bodyData_ = (BodyToHead*)(hdr + 1);
      return RESULT_OK;
    }
    else if (hdr->payload_type == PAYLOAD_CONT_DATA) {
      ccc_data_process( (ContactData*)(hdr+1) );
      return RESULT_OK;
    }
  }
  return RESULT_FAIL_IO_TIMEOUT;
}


Result HAL::Init()
{
  // Set ID
  robotID_ = Anki::Cozmo::DEFAULT_ROBOT_ID;

  InitMotor();

  InitIMU();

  if (InitRadio() != RESULT_OK) {
    AnkiError("HAL.Init.InitRadioFailed", "");
    return RESULT_FAIL;
  }

#ifndef USING_ANDROID_PHONE
  {
    AnkiInfo("HAL.Init.StartingSpineHAL", "");

    SpineErr_t error = hal_init(SPINE_TTY, SPINE_BAUD);

    if (error != err_OK) {
      return RESULT_FAIL;
    }
    AnkiDebug("HAL.Init.SettingRunMode", "");

    hal_set_mode(RobotMode_RUN);

    AnkiDebug("HAL.Init.WaitingForDataFrame", "");
    Result result;
    do {
      result = GetSpineDataFrame();
      //spin on good frame
      if (result == RESULT_FAIL_IO_TIMEOUT) {
        AnkiWarn("HAL.Init.SpineTimeout", "Kicking the body again!");
        hal_set_mode(RobotMode_RUN);
      }
    } while (result != RESULT_OK);

  }
#else
  bodyData_ = &dummyBodyData_;
#endif
  assert(bodyData_ != nullptr);


  for (int m = MOTOR_LIFT; m < MOTOR_COUNT; m++) {
    MotorResetPosition((MotorID)m);
  }
  AnkiInfo("HAL.Init.Success", "");

  return RESULT_OK;
}  // Init()

void ForwardMicData(void)
{
  // static_assert(MICDATA_SAMPLES_COUNT ==
  //               (sizeof(RobotInterface::MicData::data) / sizeof(RobotInterface::MicData::data[0])),
  //               "bad mic data sample count define");
  RobotInterface::MicData micData;
  micData.sequenceID = bodyData_->framecounter;
  micData.timestamp = HAL::GetTimeStamp();
#if MICDATA_ENABLED
  std::copy(bodyData_->audio, bodyData_->audio + MICDATA_SAMPLES_COUNT, micData.data);
  RobotInterface::SendMessage(micData);
#endif
}

Result HAL::Step(void)
{
  Result result = RESULT_OK;

#ifndef USING_ANDROID_PHONE
  {
    headData_.framecounter++;

    //check if the charge contact commander is active,
    //if so, override normal operation
    bool commander_is_active = ccc_commander_is_active();
    if (commander_is_active) {
      hal_send_frame(PAYLOAD_DATA_FRAME, ccc_data_get_response(), sizeof(HeadToBody));
    }
    else {
      hal_send_frame(PAYLOAD_DATA_FRAME, &headData_, sizeof(HeadToBody));

      // Process IMU while next frame is buffering in the background
      ProcessIMUEvents();

    }
    result =  GetSpineDataFrame();

    if (commander_is_active) {
      ccc_payload_process(bodyData_);
      return RESULT_FAIL;
    }

    ProcessTouchLevel(); // filter invalid values from touch sensor

    PrintConsoleOutput();
  }
#endif

  ForwardMicData();
  return result;
}

void ProcessTouchLevel(void)
{
  if(bodyData_->touchLevel[0] != 0xFFFF) {
    lastValidTouchIntensity_ = bodyData_->touchLevel[HAL::BUTTON_CAPACITIVE];
  }
}

// Get the number of microseconds since boot
u32 HAL::GetMicroCounter(void)
{
  auto currTime = std::chrono::steady_clock::now();
  return static_cast<TimeStamp_t>(std::chrono::duration_cast<std::chrono::microseconds>(currTime.time_since_epoch()).count());
}

void HAL::MicroWait(u32 microseconds)
{
  u32 now = GetMicroCounter();
  while ((GetMicroCounter() - now) < microseconds)
    ;
}

TimeStamp_t HAL::GetTimeStamp(void)
{
  auto currTime = std::chrono::steady_clock::now();
  return static_cast<TimeStamp_t>(std::chrono::duration_cast<std::chrono::milliseconds>(currTime.time_since_epoch()).count());
}

void HAL::SetTimeStamp(TimeStamp_t t)
{
  AnkiInfo("HAL.SetTimeStamp", "%d", t);
  timeOffset_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(t);
};


void HAL::SetLED(LEDId led_id, u32 color)
{
  assert(led_id >= 0 && led_id < LED_COUNT);

  const u32 ledIdx = (u32)led_id;

  uint8_t r = (color >> LED_RED_SHIFT) & LED_CHANNEL_MASK;
  uint8_t g = (color >> LED_GRN_SHIFT) & LED_CHANNEL_MASK;
  uint8_t b = (color >> LED_BLU_SHIFT) & LED_CHANNEL_MASK;
  headData_.ledColors[ledIdx * LED_CHANEL_CT + LED0_RED] = r;
  headData_.ledColors[ledIdx * LED_CHANEL_CT + LED0_GREEN] = g;
  headData_.ledColors[ledIdx * LED_CHANEL_CT + LED0_BLUE] = b;
}

u32 HAL::GetID()
{
  return robotID_;
}

inline u16 FlipBytes(u16 v) {
  return ((((v) & 0x00FF)<<8) | ((v)>>8));
}

ProxSensorData HAL::GetRawProxData()
{
  ProxSensorData proxData;
  proxData.rangeStatus = bodyData_->proximity.rangeStatus;
  proxData.distance_mm = FlipBytes(bodyData_->proximity.rangeMM);
  // Signal/Ambient Rate are fixed point 9.7, so convert to float:
  proxData.signalIntensity = static_cast<float>(FlipBytes(bodyData_->proximity.signalRate)) / 128.f;
  proxData.ambientIntensity = static_cast<float>(FlipBytes(bodyData_->proximity.ambientRate)) / 128.f;
  // SPAD count is fixed point 8.8, so convert to float:
  proxData.spadCount = static_cast<float>(FlipBytes(bodyData_->proximity.spadCount)) / 256.f;
  return proxData;
}

u16 HAL::GetButtonState(const ButtonID button_id)
{
  assert(button_id >= 0 && button_id < BUTTON_COUNT);
  if(button_id==HAL::BUTTON_CAPACITIVE) {
    return lastValidTouchIntensity_;
  }
  return bodyData_->touchLevel[button_id];
}

u16 HAL::GetRawCliffData(const CliffID cliff_id)
{
  assert(cliff_id < DROP_SENSOR_COUNT);
  return bodyData_->cliffSense[cliff_id];
}

u16 HAL::GetCliffOffLevel(const CliffID cliff_id)
{
  // This is not supported by V2 hardware
  return 0;
}

f32 HAL::BatteryGetVoltage()
{
  // On charger battery.battery reports ~2500 so scale it to 5v
  static const f32 kBatteryScale = 5.f/2500;
  return kBatteryScale * bodyData_->battery.battery;
}

bool HAL::BatteryIsCharging()
{
  // TEMP!! This should be fixed once syscon reports the correct flags for isCharging, etc.
  static bool isCharging = false;
  static bool wasAboveThresh = false;
  static u32 lastTransition_ms = HAL::GetTimeStamp();

  const int32_t thresh = 2000; // raw ADC value?
  const u32 debounceTime_ms = 200U;

  const bool isAboveThresh = bodyData_->battery.charger > thresh;

  if (isAboveThresh != wasAboveThresh) {
    lastTransition_ms = HAL::GetTimeStamp();
  }

  const bool canTransition = HAL::GetTimeStamp() > lastTransition_ms + debounceTime_ms;

  if (canTransition) {
    isCharging = isAboveThresh;
  }

  wasAboveThresh = isAboveThresh;
  return isCharging;
  //return bodyData_->battery.flags & isCharging;
}

bool HAL::BatteryIsOnCharger()
{
  // TEMP!! This should be fixed once syscon reports the correct flags for isCharging, etc.
  return HAL::BatteryIsCharging();
  //return bodyData_->battery.flags & isOnCharger;
}

bool HAL::BatteryIsChargerOOS()
{
  return bodyData_->battery.flags & chargerOOS;
}

u8 HAL::GetWatchdogResetCounter()
{
  // not (yet) implemented in HAL in V2
  return 0;//bodyData_->status.watchdogCount;
}

} // namespace Cozmo
} // namespace Anki


extern "C" {

  u64 steady_clock_now(void) {
    return std::chrono::steady_clock::now().time_since_epoch().count();
  }
}
