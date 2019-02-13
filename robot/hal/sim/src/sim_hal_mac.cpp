/**
 * File: sim_hal_mac.cpp
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/10/2013
 * Author: Daniel Casner <daniel@anki.com>
 * Revised for Cozmo 2: 02/27/2017
 *
 * Description:
 *
 *   Simulated HAL implementation for Cozmo 2.0
 *
 *   This is an "abstract class" defining an interface to lower level
 *   hardware functionality like getting an image from a camera,
 *   setting motor speeds, etc.  This is all the Robot should
 *   need to know in order to talk to its underlying hardware.
 *
 *   To avoid C++ class overhead running on a robot's embedded hardware,
 *   this is implemented as a namespace instead of a fullblown class.
 *
 *   This just defines the interface; the implementation (e.g., Real vs.
 *   Simulated) is given by a corresponding .cpp file.  Which type of
 *   is used for a given project/executable is decided by which .cpp
 *   file gets compiled in.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef WEBOTS

// System Includes
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>

// Our Includes
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "audioUtil/audioCaptureSystem.h"
#include "util/container/fixedCircularBuffer.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "messages.h"
#include "wheelController.h"

#include "simulator/robot/sim_overlayDisplay.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

#define DEBUG_GRIPPER 0

// Whether or not to simulate gyro bias and drift due to temperature
static const bool kSimulateGyroBias = false;

#ifndef MACOSX
#error MACOSX should be defined by any target using sim_hal.cpp
#endif

namespace Anki {
  namespace Vector {

    namespace { // "Private members"

      // Const paramters / settings
      // TODO: some of these should be defined elsewhere (e.g. comms)

#pragma mark --- Simulated HardwareInterface "Member Variables" ---

      bool isInitialized = false;

      u32 tickCnt_ = 0; // increments each robot step (ROBOT_TIME_STEP_MS)

      s32 robotID_ = 0;

      // Power
      HAL::PowerState powerState_ = HAL::POWER_MODE_ACTIVE;

      bool wasOnCharger_ = false;

      const u32 batteryUpdateRate_tics_ = 50; // How often to update the simulated battery voltage (in robot ticks)

      // MicData
      // Use the mac mic as input with AudioCaptureSystem
      constexpr uint32_t kSamplesPerChunk = 80;
      constexpr uint32_t kSampleRate_hz = 16000;
      AudioUtil::AudioCaptureSystem audioCaptureSystem_(kSamplesPerChunk, kSampleRate_hz);

      // Limit the number of messages that can be sent per robot tic
      // and toss the rest. Since audio data is generated in
      // real-time vs. robot time, if Webots processes slow down or
      // are paused (debugger) then we'll end up flooding the send buffer.
      constexpr uint32_t kMicSendWindow_tics = 6;   // Roughly equivalent to every anim process tic
      constexpr uint32_t kMaxNumMicMsgsAllowedPerSendWindow = 100;
      int _numMicMsgsSent      = 0;        // Num of mic msgs sent in the current window
      int _micSendWindowTicIdx = 0;        // Which tic of the current window we're in

      constexpr uint32_t kInterleavedSamplesPerChunk = kSamplesPerChunk * 4;
      using RawChunkArray = std::array<int16_t, kInterleavedSamplesPerChunk>;
      Util::FixedCircularBuffer<RawChunkArray, kMicSendWindow_tics * kMaxNumMicMsgsAllowedPerSendWindow> micData_;
      std::mutex micDataMutex_;

#pragma mark --- Simulated Hardware Interface "Private Methods" ---
      // Localization
      //void GetGlobalPose(f32 &x, f32 &y, f32& rad);

      void MotorUpdate()
      {
      }

      void UpdateSimBatteryVolts()
      {
      }

      void AudioInputCallback(const AudioUtil::AudioSample* data, uint32_t numSamples)
      {
        std::lock_guard<std::mutex> lock(micDataMutex_);
        micData_.push_back();
        auto* newData = micData_.back().data();

        // Duplicate our mono channel input across 4 interleaved channels to simulate 4 mics
        constexpr int kNumChannels = 4;
        for (int j=0; j<kSamplesPerChunk; j++)
        {
          auto* sampleStart = newData + (j * kNumChannels);
          const auto sample = data[j];

          for(int i=0; i<kNumChannels; ++i)
          {
            sampleStart[i] = sample;
          }
        }
      }

      void AudioInputUpdate()
      {
        // Reset send counter for next send window
        if (++_micSendWindowTicIdx >= kMicSendWindow_tics) {
          _micSendWindowTicIdx = 0;
          _numMicMsgsSent = 0;
        }
      }

    } // "private" namespace

#pragma mark --- Simulated Hardware Method Implementations ---

    // Forward Declaration
    Result InitRadio();

    Result HAL::Init(const int * shutdownSignal)
    {
      wasOnCharger_ = false;

      if (InitRadio() != RESULT_OK) {
        AnkiError("sim_hal.Init.InitRadioFailed", "");
        return RESULT_FAIL;
      }

      // Audio Input
      audioCaptureSystem_.SetCallback(std::bind(&AudioInputCallback, std::placeholders::_1, std::placeholders::_2));
      audioCaptureSystem_.Init();

      if (audioCaptureSystem_.IsValid())
      {
        audioCaptureSystem_.StartRecording();
      }
      else
      {
        PRINT_NAMED_WARNING("HAL.Init", "AudioCaptureSystem not supported on this platform. No mic data input available.");
      }

      isInitialized = true;
      return RESULT_OK;

    } // Init()

    void HAL::Stop()
    {
      
    }

    void HAL::Destroy()
    {
    } // Destroy()

    bool HAL::IsInitialized(void)
    {
      return isInitialized;
    }


    void HAL::GetGroundTruthPose(f32 &x, f32 &y, f32& rad)
    {
    } // GetGroundTruthPose()


    bool HAL::IsGripperEngaged() {
      return false;
    }

    void HAL::UpdateDisplay(void)
    {
    } // HAL::UpdateDisplay()


    bool HAL::IMUReadData(HAL::IMU_DataStructure &IMUData)
    {
      float gyroVals[3];
      float accelVals[3];
      for (int i=0 ; i<3 ; i++) {
        gyroVals[i] = (rand() / (float)RAND_MAX) * 0.006f - 0.003f;
        accelVals[i] = (rand() / (float)RAND_MAX) * 0.006f - 0.003f;
        IMUData.gyro[i]  = (f32)(gyroVals[i]);
        IMUData.accel[i] = (f32)(accelVals[i]);// * 1000);
      }

      // Compute estimated IMU temperature based on measured data from Victor prototype

      // Temperature dynamics approximated by:
      // dT/dt = k * (T_final - T) , with T(0) = T_initial
      // 1st order IVP. Solution:
      // T(t) = T_final - (T_final - T_initial) * exp(-k * t)
      const float T_initial = 20.f;
      const float T_final = 70.f;    // measured on Victor prototype
      const float k = .0032f;        // constant (measured), units sec^-1
      const float t = HAL::GetTimeStamp() / 1000.f; // current time in seconds

      IMUData.temperature_degC = T_final - (T_final - T_initial) * exp(-k * t);

      // Apply gyro bias based on temperature:
      if (kSimulateGyroBias) {
        // All worst case values are given in Section 1.2 of BMI160 datasheet
        const float initialBias_dps[3] = {0.f, 0.f, 0.f};     // inital zero-rate offset at startup. worst case is +/- 10 deg/sec
        const float biasChangeDueToTemp_dps_per_degC = 0.08f; // zero-rate offset change as temperature changes. worst case 0.08 deg/sec per degC
        const float biasDueToTemperature_dps = (IMUData.temperature_degC - T_initial) * biasChangeDueToTemp_dps_per_degC;

        for (int i=0 ; i<3 ; i++) {
          IMUData.gyro[i] += DEG_TO_RAD(initialBias_dps[i] + biasDueToTemperature_dps);
        }
      }

      // Return true if IMU was already read this timestamp
      static TimeStamp_t lastReadTimestamp = 0;
      bool newReading = lastReadTimestamp != HAL::GetTimeStamp();
      lastReadTimestamp = HAL::GetTimeStamp();
      return newReading;
    }

    // Returns the motor power used for calibration [-1.0, 1.0]
    float HAL::MotorGetCalibPower(MotorID motor)
    {
      float power = 0.f;
      switch (motor) {
        case MotorID::MOTOR_LIFT:
        case MotorID::MOTOR_HEAD:
          power = -0.4f;
          break;
        default:
          PRINT_NAMED_ERROR("simHAL.MotorGetCalibPower.UndefinedType", "%d", EnumToUnderlyingType(motor));
          break;
      }
      return power;
    }

    // Set the motor power in the unitless range [-1.0, 1.0]
    void HAL::MotorSetPower(MotorID motor, f32 power)
    {
    }

    // Reset the internal position of the specified motor to 0
    void HAL::MotorResetPosition(MotorID motor)
    {
    }

    // Returns units based on the specified motor type:
    // Wheels are in mm/s, everything else is in degrees/s.
    f32 HAL::MotorGetSpeed(MotorID motor)
    {
      return 0;
    }

    // Returns units based on the specified motor type:
    // Wheels are in mm since reset, everything else is in degrees.
    f32 HAL::MotorGetPosition(MotorID motor)
    {
      return 0;
    }


    void HAL::EngageGripper()
    {
    }

    void HAL::DisengageGripper()
    {
    }

    // Forward declaration
    void ActiveObjectsUpdate();

    Result HAL::Step(void)
    {
      MotorUpdate();
      AudioInputUpdate();

      // Check charging status (Debug)
      if (BatteryIsOnCharger() && !wasOnCharger_) {
        PRINT_NAMED_DEBUG("simHAL.Step.OnCharger", "");
        wasOnCharger_ = true;
      } else if (!BatteryIsOnCharger() && wasOnCharger_) {
        PRINT_NAMED_DEBUG("simHAL.Step.OffCharger", "");
        wasOnCharger_ = false;
      }

      if ((tickCnt_ % batteryUpdateRate_tics_) == 0) {
        UpdateSimBatteryVolts();
      }

      ++tickCnt_;
      return RESULT_OK;
    } // step()



    // Get the number of microseconds since boot
    u32 HAL::GetMicroCounter(void)
    {
      static u32 tick = 0;
      ++tick;
      return static_cast<u32>(tick);
    }

    void HAL::MicroWait(u32 microseconds)
    {
      u32 now = GetMicroCounter();
      while ((GetMicroCounter() - now) < microseconds)
        ;
    }

    TimeStamp_t HAL::GetTimeStamp(void)
    {
      static u32 tick = 0;
      ++tick;
      return static_cast<TimeStamp_t>(tick);
    }

    void HAL::SetLED(LEDId led_id, u32 color) {
    }

    void HAL::SetSystemLED(u32 color)
    {
    }

    u32 HAL::GetID()
    {
      return robotID_;
    }

    ProxSensorDataRaw HAL::GetRawProxData()
    {
      ProxSensorDataRaw proxData;
      return proxData;
    }

    u16 HAL::GetButtonState(const ButtonID button_id)
    {
      return 0;
    }

    u16 HAL::GetRawCliffData(const CliffID cliff_id)
    {
      assert(cliff_id < HAL::CLIFF_COUNT);
      return CLIFF_CALM_MODE_VAL;
    }

    bool HAL::HandleLatestMicData(SendDataFunction sendDataFunc)
    {
      // Check if our sim mic thread has delivered more audio for us to send out
      if(micDataMutex_.try_lock())
      {
        if (micData_.empty())
        {
          micDataMutex_.unlock();
          return false;
        }

        if (_numMicMsgsSent < kMaxNumMicMsgsAllowedPerSendWindow) {
          sendDataFunc(micData_.front().data(), kInterleavedSamplesPerChunk);
          ++_numMicMsgsSent;
        }

        micData_.pop_front();
        if (micData_.empty())
        {
          micDataMutex_.unlock();
          return false;
        }

        micDataMutex_.unlock();
        return true;
      }
      return false;
    }

    f32 HAL::BatteryGetVoltage()
    {
      return 3.45f;
    }

    bool HAL::BatteryIsCharging()
    {
      return true;
    }

    bool HAL::BatteryIsOnCharger()
    {
      // The _physical_ robot only knows that it's on the charger if
      // the charge contacts are powered, so treat this the same as
      // BatteryIsCharging()
      return HAL::BatteryIsCharging();
    }

    bool HAL::BatteryIsDisconnected()
    {
      // NOTE: This doesn't simulate syscon cutoff after 30 min
      return false;
    }

    bool HAL::BatteryIsOverheated()
    {
      // NOTE: This doesn't simulate syscon cutoff after 30 min
      return false;
    }

    u8 HAL::BatteryGetTemperature_C()
    {
      return 40;
    }

    f32 HAL::ChargerGetVoltage()
    {
      if (BatteryIsOnCharger()) {
        return 5.f;
      }
      return 0.f;
    }

    extern "C" {
    void EnableIRQ() {}
    void DisableIRQ() {}
    }


    u8 HAL::GetWatchdogResetCounter()
    {
      return 0; // Simulator never watchdogs
    }

    void HAL::PrintBodyData(u32 period_tics, bool motors, bool prox, bool battery)
    {
      AnkiWarn("HAL.PrintBodyData.NotSupportedInSim", "");
    }

    void HAL::Shutdown()
    {

    } 

    void HAL::PowerSetDesiredMode(const PowerState state)
    {
      powerState_ = state;
      if (powerState_ != POWER_MODE_ACTIVE) {
        AnkiWarn("HAL.PowerSetDesiredMode.UnsupportedMode", 
                 "Only POWER_MODE_ACTIVE behavior is actually supported in sim");
      }
    }

    HAL::PowerState HAL::PowerGetDesiredMode()
    {
      return powerState_;
    }

    HAL::PowerState HAL::PowerGetMode()
    {
      return powerState_;
    }

    bool HAL::AreEncodersDisabled()
    {
      return false;
    }

    bool HAL::IsHeadEncoderInvalid()
    {
      return false;
    }

    bool HAL::IsLiftEncoderInvalid()
    {
      return false;
    }

    const uint8_t* const HAL::GetSysconVersionInfo()
    {
      static const uint8_t arr[16] = {0};
      return arr;
    }
  
  } // namespace Vector
} // namespace Anki

#endif // ndef WEBOTS
