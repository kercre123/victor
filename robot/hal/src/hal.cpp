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
#include "anki/cozmo/robot/logEvent.h"
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

#ifdef HAL_DUMMY_BODY
  BodyToHead dummyBodyData_ = {
    .cliffSense = {800, 800, 800, 800}
  };
#endif

  // update every tick of the robot:
  // some touch values are 0xFFFF, which we want to ignore
  // so we cache the last non-0xFFFF value and return this as the latest touch sensor reading
  u16 lastValidTouchIntensity_;

  PowerState desiredPowerMode_;

  // Flag to prevent spamming of unexepected power mode warning
  bool reportUnexpectedPowerMode_ = false;

  // Time since the desired power mode was last set
  TimeStamp_t lastPowerSetModeTime_ms_ = 0;

  // Last time a HeadToBody frame was sent
  TimeStamp_t lastH2BSendTime_ms_ = 0;

  // The maximum time expected to elapse before we're sure that 
  // syscon should have changed to the desired power mode,
  // indexed by desired power mode. 
  static const TimeStamp_t MAX_POWER_MODE_SWITCH_TIME_MS[2] = {100,          // Calm->Active timeout
                                                               1000 + 100};  // Active->Calm timeout

  // Number of frames to skip sending to body when in calm power mode
  static const int NUM_CALM_MODE_SKIP_FRAMES = 12;  // Every 60ms
  int calmModeSkipFrameCount_ = 0;

  struct spine_ctx spine_;
  uint8_t frameBuffer_[SPINE_B2H_FRAME_LEN];
  uint8_t readBuffer_[4096];
  BodyToHead BootBodyData_ = { //dummy data for boot stub frames
    .framecounter = 0
  };
  static const f32 kBatteryScale = 2.8f / 2048.f;
} // "private" namespace

// Forward Declarations
Result InitRadio();
void InitIMU();
void StopIMU();
void ProcessIMUEvents();
void ProcessTouchLevel(void);
void PrintConsoleOutput(void);


extern "C" {
  ssize_t spine_write_frame(spine_ctx_t spine, PayloadId type, const void* data, int len);
  void record_body_version( const struct VersionInfo* info);
  void request_version(void) {
    spine_write_frame(&spine_, PAYLOAD_VERSION, NULL, 0);
  }
}

ssize_t robot_io(spine_ctx_t spine)
{
  int fd = spine_get_fd(spine);

  EventStart(EventType::ROBOT_IO_READ);

  ssize_t r = read(fd, readBuffer_, sizeof(readBuffer_));

  EventAddToMisc(EventType::ROBOT_IO_READ, (uint32_t)r);
  EventStop(EventType::ROBOT_IO_READ);

  if (r > 0)
  {
    EventStart(EventType::ROBOT_IO_RECEIVE);
    r = spine_receive_data(spine, (const void*)readBuffer_, r);
    EventStop(EventType::ROBOT_IO_RECEIVE);
  }
  else if (r < 0)
  {
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
      else if (hdr->payload_type == PAYLOAD_VERSION) {
        record_body_version( (VersionInfo*)(hdr+1) );
      }
      else {
        LOGD("Unknown Frame Type %x\n", hdr->payload_type);
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

    robot_io(&spine_);
    read_count++;
  }

  return (initialized ? RESULT_OK : RESULT_FAIL_IO_TIMEOUT);
}

Result HAL::Init(const int * shutdownSignal)
{
  using Result = Anki::Result;

  // Set ID
  robotID_ = Anki::Cozmo::DEFAULT_ROBOT_ID;

  InitIMU();

  if (InitRadio() != RESULT_OK) {
    AnkiError("HAL.Init.InitRadioFailed", "");
    return RESULT_FAIL;
  }

#ifndef HAL_DUMMY_BODY
  {
    AnkiInfo("HAL.Init.StartingSpineHAL", "");

    desiredPowerMode_ = POWER_MODE_ACTIVE;

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
    request_version();  //get version so we have it when we need it.
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

void handle_payload_data(const uint8_t frame_buffer[]) {

  memcpy(frameBuffer_, frame_buffer, sizeof(frameBuffer_));
  bodyData_ = (BodyToHead*)(frameBuffer_ + sizeof(struct SpineMessageHeader));

  if (ccc_commander_is_active()) {
    ccc_payload_process(bodyData_);
  }

}


Result spine_get_frame() {
  Result result = RESULT_FAIL_IO_TIMEOUT;
  uint8_t frame_buffer[SPINE_B2H_FRAME_LEN];

  ssize_t r = 0;
  do {
    EventStart(EventType::PARSE_FRAME);
    r = spine_parse_frame(&spine_, frame_buffer, sizeof(frame_buffer), NULL);
    EventStop(EventType::PARSE_FRAME);

    if (r < 0)
    {
      continue;
    }
    else if (r > 0)
    {
      const struct SpineMessageHeader* hdr = (const struct SpineMessageHeader*)frame_buffer;
      if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
        handle_payload_data(frame_buffer);  //payload starts immediately after header
        result = RESULT_OK;
      }
      else if (hdr->payload_type == PAYLOAD_CONT_DATA) {
         LOGD("Handling CD payload type %x\n", hdr->payload_type);
        ccc_data_process( (ContactData*)(hdr+1) );
        result = RESULT_OK;
      }
      else if (hdr->payload_type == PAYLOAD_VERSION) {
         LOGD("Handling VR payload type %x\n", hdr->payload_type);
        record_body_version( (VersionInfo*)(hdr+1) );
      }
      else if (hdr->payload_type == PAYLOAD_BOOT_FRAME) {
        //extract button data from stub packet and put in fake full packet
        uint8_t button_pressed = ((struct MicroBodyToHead*)(hdr+1))->buttonPressed;
        BootBodyData_.touchLevel[1] = button_pressed ? 0xFFFF : 0x0000;
        BootBodyData_.battery.flags = POWER_ON_CHARGER;

        BootBodyData_.battery.battery = (int16_t)(5.0/kBatteryScale);
        BootBodyData_.battery.charger = (int16_t)(5.0/kBatteryScale);
        bodyData_ = &BootBodyData_;
      }
      else {
        LOGD("Unknown Frame Type %x\n", hdr->payload_type);
      }
    }
    else
    {
      // get more data
      EventStart(EventType::ROBOT_IO);
      robot_io(&spine_);
      EventStop(EventType::ROBOT_IO);
    }

    if(result == RESULT_OK)
    {
      break;
    }

  } while (r != 0);

  return result;
}


extern "C"  ssize_t spine_write_ccc_frame(spine_ctx_t spine, const struct ContactData* ccc_payload);
#define MIN_CCC_XMIT_SPACING_US 5000

Result HAL::Step(void)
{
  EventStep();
  EventStart(EventType::HAL_STEP);

  static uint32_t last_packet_send = 0;
  uint32_t now = GetMicroCounter();

  Result result = RESULT_OK;
  bool commander_is_active = false;

#ifndef HAL_DUMMY_BODY

  headData_.framecounter++;

  //Packet throttle.
  if (now-last_packet_send >= MIN_CCC_XMIT_SPACING_US ) {
    //check if the charge contact commander is active,
    //if so, override normal operation
    // commander_is_active = ccc_commander_is_active();
    // struct HeadToBody* h2bp = (commander_is_active) ? ccc_data_get_response() : &headData_;
    struct HeadToBody* h2bp =  &headData_;

    EventStart(EventType::WRITE_SPINE);
    const TimeStamp_t now_ms = GetTimeStamp();
    if (desiredPowerMode_ == POWER_MODE_CALM) {
      if (++calmModeSkipFrameCount_ > NUM_CALM_MODE_SKIP_FRAMES) {
        spine_set_lights(&spine_, &(h2bp->lightState));
        calmModeSkipFrameCount_ = 0;
      }
    } else {
      spine_write_h2b_frame(&spine_, h2bp);
      lastH2BSendTime_ms_ = now_ms;
    }
    EventStop(EventType::WRITE_SPINE);

    // Print warning if power mode is unexpected
    const HAL::PowerState currPowerMode = PowerGetMode();
    if (currPowerMode != desiredPowerMode_) {
      if ( ((lastPowerSetModeTime_ms_ == 0) && reportUnexpectedPowerMode_) || 
           ((lastPowerSetModeTime_ms_ > 0) && ((now_ms - lastPowerSetModeTime_ms_ > MAX_POWER_MODE_SWITCH_TIME_MS[desiredPowerMode_])))
           ) {
        AnkiWarn("HAL.Step.UnexpectedPowerMode", 
                 "Curr mode: %u, Desired mode: %u, now: %ums, lastSetModeTime: %ums, lastH2BSendTime: %ums",
                 currPowerMode, desiredPowerMode_, now_ms, lastPowerSetModeTime_ms_, lastH2BSendTime_ms_);
        lastPowerSetModeTime_ms_ = 0;  // Reset time to avoid spamming warning
        reportUnexpectedPowerMode_ = false;
      }
    } else {
      reportUnexpectedPowerMode_ = true;
    }

    struct ContactData* ccc_response = ccc_text_response();
    if (ccc_response) {
      spine_write_ccc_frame(&spine_, ccc_response);
    }
    last_packet_send = now;
  }

#if !PROCESS_IMU_ON_THREAD
  ProcessIMUEvents();
#endif

  EventStart(EventType::READ_SPINE);

  do {
    result = spine_get_frame();
  } while(result != RESULT_OK);

  EventStop(EventType::READ_SPINE);

#else // else have dummy body

#if !PROCESS_IMU_ON_THREAD
  ProcessIMUEvents();
#endif

#endif // #ifndef HAL_DUMMY_BODY

  ProcessTouchLevel(); // filter invalid values from touch sensor

  PrintConsoleOutput();

  EventStop(EventType::HAL_STEP);

  //return a fail code if commander is active to prevent robotics from getting confused
  return (commander_is_active) ? RESULT_FAIL_IO_UNSYNCHRONIZED : result;
}

void HAL::Stop()
{
  StopIMU();
  DisconnectRadio();
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

void HAL::SetLED(LEDId led_id, u32 color)
{
  assert(led_id >= 0 && led_id < LED_COUNT);

  // Light order is swapped in syscon
  const u32 ledIdx = LED_COUNT - led_id - 1;

  uint8_t r = (color >> LED_RED_SHIFT) & LED_CHANNEL_MASK;
  uint8_t g = (color >> LED_GRN_SHIFT) & LED_CHANNEL_MASK;
  uint8_t b = (color >> LED_BLU_SHIFT) & LED_CHANNEL_MASK;
  headData_.lightState.ledColors[ledIdx * LED_CHANEL_CT + LED0_RED] = r;
  headData_.lightState.ledColors[ledIdx * LED_CHANEL_CT + LED0_GREEN] = g;
  headData_.lightState.ledColors[ledIdx * LED_CHANEL_CT + LED0_BLUE] = b;
}

void HAL::SetSystemLED(u32 color)
{
  uint8_t r = (color >> LED_RED_SHIFT) & LED_CHANNEL_MASK;
  uint8_t g = (color >> LED_GRN_SHIFT) & LED_CHANNEL_MASK;
  uint8_t b = (color >> LED_BLU_SHIFT) & LED_CHANNEL_MASK;
  headData_.lightState.ledColors[LED3_RED] = r;
  // Technically have no control over green, it is always on
  headData_.lightState.ledColors[LED3_GREEN] = g;
  headData_.lightState.ledColors[LED3_BLUE] = b;
}

u32 HAL::GetID()
{
  return robotID_;
}

inline u16 FlipBytes(u16 v) {
  return ((((v) & 0x00FF)<<8) | ((v)>>8));
}

ProxSensorDataRaw HAL::GetRawProxData()
{
  ProxSensorDataRaw proxData;
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

bool HAL::BatteryIsDisconnected()
{
  // The POWER_BATTERY_DISCONNECTED flag is set whenever the robot is on
  // the charge base, but the battery has been disconnected from the
  // charging circuit.
  return bodyData_->battery.flags & POWER_BATTERY_DISCONNECTED;
}

u8 HAL::GetWatchdogResetCounter()
{
  // not (yet) implemented in HAL in V2
  return 0;//bodyData_->status.watchdogCount;
}

void HAL::Shutdown()
{
  spine_shutdown(&spine_);
}


void HAL::PowerSetMode(const PowerState state)
{
  AnkiInfo("HAL.PowerSetMode", "%d", state);
  desiredPowerMode_ = state;
  lastPowerSetModeTime_ms_ = HAL::GetTimeStamp();
}

HAL::PowerState HAL::PowerGetMode()
{
  return (bodyData_->flags & RUNNING_FLAGS_SENSORS_VALID) ? POWER_MODE_ACTIVE : POWER_MODE_CALM;
}


} // namespace Cozmo
} // namespace Anki


extern "C" {

  u64 steady_clock_now(void) {
    return std::chrono::steady_clock::now().time_since_epoch().count();
  }

  void hal_terminate(void) {
    Anki::Cozmo::HAL::Shutdown();
  }

}
