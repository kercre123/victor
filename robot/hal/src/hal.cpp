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

#include "../spine/spine.h"
#include "../spine/cc_commander.h"

#include "schema/messages.h"
#include "clad/types/proxMessages.h"

#include <errno.h>

namespace Anki {
namespace Cozmo {

BodyToHead* bodyData_; //buffers are owned by the code that fills them. Spine owns this one
HeadToBody headData_;  //-we own this one.

namespace { // "Private members"

  s32 robotID_ = -1;

  // TimeStamp offset
  std::chrono::steady_clock::time_point timeOffset_ = std::chrono::steady_clock::now();


#ifdef HAL_DUMMY_BODY
  BodyToHead dummyBodyData_ = {
    .cliffSense = {800, 800, 800, 800}
  };
#endif

  // update every tick of the robot:
  // some touch values are 0xFFFF, which we want to ignore
  // so we cache the last non-0xFFFF value and return this as the latest touch sensor reading
  u16 lastValidTouchIntensity_;

  struct spine_ctx spine_;
  uint8_t frameBuffer_[SPINE_B2H_FRAME_LEN];
  uint8_t readBuffer_[4096];
} // "private" namespace

// Forward Declarations
Result InitRadio();
void InitIMU();
void ProcessIMUEvents();
void ProcessTouchLevel(void);
void PrintConsoleOutput(void);

ssize_t robot_io(spine_ctx_t spine, uint32_t sleepTimeMicroSec = 1000)
{
  usleep(sleepTimeMicroSec);

  int fd = spine_get_fd(spine);
  ssize_t r = read(fd, readBuffer_, sizeof(readBuffer_));
  if (r > 0) {
    r = spine_receive_data(spine, (const void*)readBuffer_, r);
  } else if (r < 0) {
    if (errno == EAGAIN) {
      r = 0;
    }
  }
  return r;
}

Result spine_wait_for_first_frame(spine_ctx_t spine, const int * shutdownSignal)
{
  bool initialized = false;
  int read_count = 0;

  assert(shutdownSignal != nullptr);

  while (!initialized && *shutdownSignal == 0) {
    ssize_t r = spine_parse_frame(spine, &frameBuffer_, sizeof(frameBuffer_), NULL);

    if (r < 0) {
      continue;
    } else if (r > 0) {
      const struct SpineMessageHeader* hdr = (const struct SpineMessageHeader*)frameBuffer_;
      if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
        initialized = true;
        const struct spine_frame_b2h* frame = (const struct spine_frame_b2h*)frameBuffer_;
        bodyData_ = (BodyToHead*)&frame->payload;
      }
      else if (hdr->payload_type == PAYLOAD_CONT_DATA) {
        ccc_data_process( (ContactData*)(hdr+1) );
        continue;
      }
      else {
        LOGD("Unknown Frame Type\n");
      }

    } else {
      // r == 0 (waiting)
      if (read_count > 50) {
        if (!initialized) {
          spine_set_mode(&spine_, RobotMode_RUN);
        }
        read_count = 0;
      }
    }

    robot_io(&spine_, 1000);
    read_count++;
  }

  return (initialized ? RESULT_OK : RESULT_FAIL_IO_TIMEOUT);
}


Result HAL::Init(const int * shutdownSignal)
{
  using Result = Anki::Result;

  // Set ID
  robotID_ = Anki::Cozmo::DEFAULT_ROBOT_ID;

//#if IMU_WORKING
  InitIMU();

  if (InitRadio() != RESULT_OK) {
    AnkiError("HAL.Init.InitRadioFailed", "");
    return RESULT_FAIL;
  }

#ifndef HAL_DUMMY_BODY
  {
    AnkiInfo("HAL.Init.StartingSpineHAL", "");

    spine_init(&spine_);
    struct spine_params params = {
      .devicename = SPINE_TTY,
      .baudrate = SPINE_BAUD
    };
    int errCode = spine_open(&spine_, params);

    if (errCode != err_OK) {
      return RESULT_FAIL;
    }
    AnkiDebug("HAL.Init.SettingRunMode", "");

    spine_set_mode(&spine_, RobotMode_RUN);

    AnkiDebug("HAL.Init.WaitingForDataFrame", "");
    const Result result = spine_wait_for_first_frame(&spine_, shutdownSignal);
    if (RESULT_OK != result) {
      AnkiError("HAL.Init.SpineWait", "Unable to synchronize spine (result %d)", result);
      return result;
    }

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

Result spine_get_frame() {
  Result result = RESULT_FAIL_IO_TIMEOUT;
  uint8_t frame_buffer[SPINE_B2H_FRAME_LEN];

  ssize_t r = 0;
  do {
    r = spine_parse_frame(&spine_, frame_buffer, sizeof(frame_buffer), NULL);

    if (r < 0) {
      continue;
    } else if (r > 0) {
      const struct SpineMessageHeader* hdr = (const struct SpineMessageHeader*)frame_buffer;
      if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
        memcpy(frameBuffer_, frame_buffer, sizeof(frameBuffer_));
        bodyData_ = (BodyToHead*)(frameBuffer_ + sizeof(struct SpineMessageHeader));
        result = RESULT_OK;
        continue;
      }
      else {
        LOGD("Unknown Frame Type\n");
      }

    } else {
      // get more data
      robot_io(&spine_, 1000);
    }
  } while (r != 0);

  return result;
}


extern "C"  ssize_t spine_write_ccc_frame(spine_ctx_t spine, const struct ContactData* ccc_payload);

Result HAL::Step(void)
{
  Result result = RESULT_OK;

#ifndef HAL_DUMMY_BODY
  headData_.framecounter++;

  //check if the charge contact commander is active,
  //if so, override normal operation
  // bool commander_is_active = ccc_commander_is_active();
  // struct HeadToBody* h2bp = (commander_is_active) ? ccc_data_get_response() : &headData_;
  struct HeadToBody* h2bp =  &headData_;

  spine_write_h2b_frame(&spine_, h2bp);

  struct ContactData* ccc_response = ccc_text_response();
  if (ccc_response) {
    spine_write_ccc_frame(&spine_, ccc_response);
  }
#endif // #ifndef HAL_DUMMY_BODY

  ProcessIMUEvents();

#ifndef HAL_DUMMY_BODY
  do {
    result = spine_get_frame();
  } while(result != RESULT_OK);
#else
  dummyBodyData_.framecounter++;
#endif // #ifndef HAL_DUMMY_BODY

  ProcessTouchLevel(); // filter invalid values from touch sensor

  PrintConsoleOutput();

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

  // Light order is swapped in syscon
  const u32 ledIdx = LED_COUNT - led_id - 1;

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

bool HAL::HandleLatestMicData(SendDataFunction sendDataFunc)
{
  #if MICDATA_ENABLED
  {
    sendDataFunc(bodyData_->audio, MICDATA_SAMPLES_COUNT);
  }
  #endif
  return false;
}

f32 HAL::BatteryGetVoltage()
{
  // scale raw ADC counts to voltage (conversion factor from Vandiver)
  static const f32 kBatteryScale = 2.8f / 2048.f;
  return kBatteryScale * bodyData_->battery.battery;
}

bool HAL::BatteryIsCharging()
{
  // The POWER_IS_CHARGING flag is set whenever syscon has the charging
  // circuitry enabled. It does not necessarily mean the charging circuit
  // is actually charging the battery. It may remain true even after the
  // battery is fully charged.
  return bodyData_->battery.flags & POWER_IS_CHARGING;
}

bool HAL::BatteryIsOnCharger()
{
  // The POWER_ON_CHARGER flag is set whenever there is sensed voltage on
  // the charge contacts.
  return bodyData_->battery.flags & POWER_ON_CHARGER;
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
