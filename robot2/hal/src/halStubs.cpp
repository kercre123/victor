/**
 * File:        halStubs.cpp
 *
 * Description: Placeholder HAL implementation for android robot process
 *
 **/


// System Includes
#include <stdio.h>
#include <string>
#include <chrono>
#include <assert.h>

// Our Includes
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/hal_config.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "../spine/spine_hal.h"

#include "clad/robotInterface/messageRobotToEngine.h"    //TODO: why do we still need these 2?
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "schema/messages.h"
#include "clad/types/proxMessages.h"


#define RADIO_IP "127.0.0.1"

#define IMU_WORKING 1

// Debugging Defines
#define FRAMES_PER_RESPONSE  1  //send response every N input frames

#define REALTIME_CONSOLE_OUTPUT 0 //Print status to console
#define MOTOR_OF_INTEREST MOTOR_LIFT  //print status of this motor
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
  namespace Cozmo {

    static_assert(EnumToUnderlyingType(MotorID::MOTOR_LEFT_WHEEL) == MOTOR_LEFT, "Robot/Spine CLAD Mimatch");
    static_assert(EnumToUnderlyingType(MotorID::MOTOR_RIGHT_WHEEL) == MOTOR_RIGHT, "Robot/Spine CLAD Mimatch");
    static_assert(EnumToUnderlyingType(MotorID::MOTOR_LIFT) == MOTOR_LIFT, "Robot/Spine CLAD Mimatch");
    static_assert(EnumToUnderlyingType(MotorID::MOTOR_HEAD) == MOTOR_HEAD, "Robot/Spine CLAD Mimatch");
    
    namespace { // "Private members"

      //map power -1.0 .. 1.0 to -32767 to 32767
      static const f32 HAL_MOTOR_POWER_SCALE = 0x7FFF;
      static const f32 HAL_MOTOR_POWER_OFFSET = 0;

      //convert per syscon tick -> /sec
      static const f32 HAL_SEC_PER_TICK = (1.0 / 256) / 48000000;

      //encoder counts -> mm or deg
      static f32 HAL_MOTOR_POSITION_SCALE[MOTOR_COUNT] = {
        ((0.948 * 0.125 * 29.2 * 3.14159265359) / 173.43), //Left Tread mm
        ((0.948 * 0.125 * 29.2 * 3.14159265359) / 173.43), //Right Tread mm
        (0.25 * 3.14159265359) / 149.7,    //Lift radians
        (0.25 * 3.14159265359) / 348.77,   //Head radians
      };

      static f32 HAL_MOTOR_DIRECTION[MOTOR_COUNT] = {
        1.0,1.0,1.0,1.0
      };

      static f32 HAL_HEAD_MOTOR_CALIB_POWER = -0.4f;

      static f32 HAL_LIFT_MOTOR_CALIB_POWER = -0.4f;

      s32 robotID_ = -1;

      // TimeStamp offset
      std::chrono::steady_clock::time_point timeOffset_ = std::chrono::steady_clock::now();

      // Audio
      // (Can't actually play sound in simulator, but proper handling of audio frames is still
      // necessary for proper animation timing)
      TimeStamp_t audioEndTime_ = 0;    // Expected end of audio
      u32 AUDIO_FRAME_TIME_MS = 33;     // Duration of single audio frame
      bool audioReadyForFrame_ = true;  // Whether or not ready to receive another audio frame

      BodyToHead* bodyData_; //buffers are owned by the code that fills them. Spine owns this one
      HeadToBody headData_;  //-we own this one.

#ifdef USING_ANDROID_PHONE
      BodyToHead dummyBodyData_ = {
        .cliffSense = {800, 800, 800, 800}
      };
#endif

      struct {
        s32 motorOffset[MOTOR_COUNT];
        CONSOLE_DATA(f32 motorPower[MOTOR_COUNT]);
      } internalData_;

      static const char* HAL_INI_PATH = "/data/persist/hal.conf";
      const HALConfig::Item  configitems_[]  = {
        {"LeftTread mm/count",  HALConfig::FLOAT, &HAL_MOTOR_POSITION_SCALE[MOTOR_LEFT]},
        {"RightTread mm/count", HALConfig::FLOAT, &HAL_MOTOR_POSITION_SCALE[MOTOR_RIGHT]},
        {"Lift rad/count",      HALConfig::FLOAT, &HAL_MOTOR_POSITION_SCALE[MOTOR_LIFT]},
        {"Head rad/count",      HALConfig::FLOAT, &HAL_MOTOR_POSITION_SCALE[MOTOR_HEAD]},
        {"LeftTread Motor Direction",  HALConfig::FLOAT, &HAL_MOTOR_DIRECTION[MOTOR_LEFT]},
        {"RightTread Motor Direction", HALConfig::FLOAT, &HAL_MOTOR_DIRECTION[MOTOR_RIGHT]},
        {"Lift Motor Direction",       HALConfig::FLOAT, &HAL_MOTOR_DIRECTION[MOTOR_LIFT]},
        {"Head Motor Direction",       HALConfig::FLOAT, &HAL_MOTOR_DIRECTION[MOTOR_HEAD]},
        {"Lift Motor Calib Power",     HALConfig::FLOAT, &HAL_LIFT_MOTOR_CALIB_POWER},
        {"Head Motor Calib Power",     HALConfig::FLOAT, &HAL_HEAD_MOTOR_CALIB_POWER},
        {0} //Need zeros as end-of-list marker
      };
      
    } // "private" namespace

    // Forward Declarations
    Result InitRadio(const char* advertisementIP);
    void InitIMU();
    void ProcessIMUEvents();

    
    inline u16 FlipBytes(u16 v) {
      return ((((v) & 0x00FF)<<8) | ((v)>>8));
    }

    Result GetSpineDataFrame(void)
    {
      SpineMessageHeader* hdr = (SpineMessageHeader*) hal_get_frame(PAYLOAD_DATA_FRAME, 500);
      if (!hdr) {
        LOGE("Spine timeout!");
        return RESULT_FAIL_IO_TIMEOUT;
      }
      if (hdr->payload_type != PAYLOAD_DATA_FRAME) {
        LOGE("Spine.c data corruption: payload does not match requested");
        return RESULT_FAIL_IO_UNSYNCHRONIZED;
      }
      bodyData_ = (BodyToHead*)(hdr + 1);
      return RESULT_OK;
    }


    Result HAL::Init()
    {
      Result res = HALConfig::ReadConfigFile(HAL_INI_PATH, configitems_);
      if (res != RESULT_OK) {
        printf("Failed to read hal.conf file (result 0x%08X)\n", res);
        return res;
      }

      // Set ID
      robotID_ = 1;

//#if IMU_WORKING
      InitIMU();
//#endif

      if (InitRadio(RADIO_IP) != RESULT_OK) {
        printf("Failed to initialize Radio.\n");
        return RESULT_FAIL;
      }

#ifndef USING_ANDROID_PHONE
      {
        printf("Starting spine hal\n");

        SpineErr_t error = hal_init(SPINE_TTY, SPINE_BAUD);

        if (error != err_OK) {
          return RESULT_FAIL;
        }
        printf("hal Init OK\nSetting RUN mode\n");

        hal_set_mode(RobotMode_RUN);

        printf("Waiting for Data Frame\n");
        Result result;
        do {
          result = GetSpineDataFrame();
          //spin on good frame
          if (result == RESULT_FAIL_IO_TIMEOUT) {
            printf("Kicking the body again!\n");
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
      printf("Hal Init Success\n");

      return RESULT_OK;
    }  // Init()

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
          printf("HAL::MotorGetCalibPower.InvalidMotorType - No calib power for %s\n", EnumToString(motor));
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
      // if no counts for ~25ms, will report 0/0
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
        printf("FC = %d ", bodyData_->framecounter);
        printf("%s: ", DEFNAME(MOTOR_OF_INTEREST));
        printf("raw = %d ", bodyData_->motor[MOTOR_OF_INTEREST].position);
        printf("pos = %f ", HAL::MotorGetPosition(MOTOR_OF_INTEREST));
        printf("spd = %f ", HAL::MotorGetSpeed(MOTOR_OF_INTEREST));
        printf("pow = %f ", internalData_.motorPower[MOTOR_OF_INTEREST]);
        printf("cliff = %d %d% d% d ",bodyData_->cliffSense[0],bodyData_->cliffSense[1],bodyData_->cliffSense[2],bodyData_->cliffSense[3]);
        printf("prox = %d %d ",bodyData_->proximity.rangeStatus, bodyData_->proximity.rangeMM);
        printf("\r");
      }
#endif
    }


    // TODO: This is now being handled in animation process
    //       Is there still a need to maintain connection state in robot process?
    Result HAL::MonitorConnectionState(void)
    {
      /*
      // Send block connection state when engine connects
      static bool wasConnected = false;
      if (!wasConnected && HAL::RadioIsConnected()) {

        // Send RobotAvailable indicating sim robot
        RobotInterface::RobotAvailable idMsg;
        idMsg.hwRevision = 0;
        RobotInterface::SendMessage(idMsg);

        // send firmware info indicating physical robot
        {
          std::string firmwareJson{"{\"version\":0,\"time\":0}"};
          RobotInterface::FirmwareVersion msg;
          msg.RESRVED = 0;
          msg.json_length = firmwareJson.size() + 1;
          std::memcpy(msg.json, firmwareJson.c_str(), firmwareJson.size() + 1);
          RobotInterface::SendMessage(msg);
        }

        wasConnected = true;
      }
      else if (wasConnected && !HAL::RadioIsConnected()) {
        wasConnected = false;
      }
      */
      return RESULT_OK;

    } // step()

    void ForwardAudioInput(void)
    {
      // Takes advantage of the data in bodyData being ordered such that the required members of AudioInput are already
      // laid correctly.
      // TODO(Al/Lee): Put back once mics and camera can co-exist
//      const auto* latestAudioInput = reinterpret_cast<const RobotInterface::AudioInput*>(&bodyData_->audio);
//      RobotInterface::SendMessage(*latestAudioInput);
    }

    Result HAL::Step(void)
    {
      Result result = RESULT_OK;
      TimeStamp_t now = HAL::GetTimeStamp();

      // Check if audio frame is done
      if (now >= audioEndTime_) {
        audioReadyForFrame_ = true;
      }

#ifndef USING_ANDROID_PHONE
      {
        static int repeater = FRAMES_PER_RESPONSE;
        if (--repeater <= 0) {
          repeater = FRAMES_PER_RESPONSE;
          headData_.framecounter++;
          hal_send_frame(PAYLOAD_DATA_FRAME, &headData_, sizeof(HeadToBody));
        }

        // Process IMU while next frame is buffering in the background
#if IMU_WORKING
        ProcessIMUEvents();
#endif
        
        result =  GetSpineDataFrame();
        PrintConsoleOutput();
      }
#endif

      //MonitorConnectionState();

      ForwardAudioInput();
      return result;
    }

    // Get the number of microseconds since boot
    u32 HAL::GetMicroCounter(void)
    {
      auto currTime = std::chrono::steady_clock::now();
      return static_cast<TimeStamp_t>(std::chrono::duration_cast<std::chrono::microseconds>(currTime - timeOffset_).count());
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
      return static_cast<TimeStamp_t>(std::chrono::duration_cast<std::chrono::milliseconds>(currTime - timeOffset_).count());
    }

    void HAL::SetTimeStamp(TimeStamp_t t)
    {
      printf("HAL.SetTimeStamp %d\n", t);
      timeOffset_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(t);

      //      using namespace Anki::Cozmo::RobotInterface;
      //      AdjustTimestamp msg;
      //      msg.timestamp = t;
      //      RobotInterface::SendMessage(msg);
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
      return bodyData_->battery.battery;
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

    Result HAL::SetBlockLight(const u32 activeID, const u32* colors)
    {
      // Not implemented in HAL in V2
      return RESULT_OK;
    }

    Result HAL::StreamObjectAccel(const u32 activeID, const bool enable)
    {
      // Not implemented in HAL in V2
      return RESULT_OK;
    }

    Result HAL::AssignSlot(u32 slot_id, u32 factory_id)
    {
      // Not implemented in HAL in V2
      return RESULT_OK;
    }

    u8 HAL::GetWatchdogResetCounter()
    {
      // not (yet) implemented in HAL in V2
      return 0;//bodyData_->status.watchdogCount;
    }

    // @return true if the audio clock says it is time for the next frame
    bool HAL::AudioReady()
    {
      // Not implemented in HAL in V2
      return audioReadyForFrame_;
    }

    void HAL::AudioPlaySilence()
    {
      // Not implemented in HAL in V2
      AudioPlayFrame(nullptr);
    }

    // Play one frame of audio or silence
    // @param frame - a pointer to an audio frame or NULL to play one frame of silence
    void HAL::AudioPlayFrame(AnimKeyFrame::AudioSample *msg)
    {
      // Not implemented in HAL in V2
      if (audioEndTime_ == 0) {
        audioEndTime_ = HAL::GetTimeStamp();
      }
      audioEndTime_ += AUDIO_FRAME_TIME_MS;
      audioReadyForFrame_ = false;
    }


  } // namespace Cozmo
} // namespace Anki


extern "C" {

  u64 steady_clock_now(void) {
    return std::chrono::steady_clock::now().time_since_epoch().count();
  }
}
