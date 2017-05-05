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

// Our Includes
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

namespace Anki {
  namespace Cozmo {

    namespace { // "Private members"

      s32 robotID_ = -1;
      
      // TimeStamp offset
      std::chrono::steady_clock::time_point timeOffset_ = std::chrono::steady_clock::now();
      
      // Audio
      // (Can't actually play sound in simulator, but proper handling of audio frames is still
      // necessary for proper animation timing)
      TimeStamp_t audioEndTime_ = 0;    // Expected end of audio
      u32 AUDIO_FRAME_TIME_MS = 33;     // Duration of single audio frame
      bool audioReadyForFrame_ = true;  // Whether or not ready to receive another audio frame
      
    } // "private" namespace



    // Forward Declaration.  This is implemented in sim_radio.cpp
    Result InitRadio(const char* advertisementIP);

    Result HAL::Init()
    {
      // Set ID
      robotID_ = 1;

      if(InitRadio("127.0.0.1") == RESULT_FAIL) {
        printf("Failed to initialize Radio.\n");
        return RESULT_FAIL;
      }

      return RESULT_OK;

    } // Init()
    

   
    bool HAL::IMUReadData(HAL::IMU_DataStructure &IMUData)
    {
      // Only one reading allowed per tic
      // Real hardware may differ
      
      IMUData.acc_x = 0;
      IMUData.acc_y = 0;
      IMUData.acc_z = 9810;
      IMUData.rate_x = 0;
      IMUData.rate_y = 0;
      IMUData.rate_z = 0;
      
      static TimeStamp_t lastReadTime = 0;
      bool readSuccess = lastReadTime < HAL::GetTimeStamp();
      lastReadTime = HAL::GetTimeStamp();
      return readSuccess;
    }
    
    void HAL::IMUReadRawData(int16_t* accel, int16_t* gyro, uint8_t* timestamp)
    {
      // Just storing junk values since this function exists purely for HW debug
      *timestamp = HAL::GetTimeStamp() % u8_MAX;
      accel[0] = accel[1] = accel[2] = 0;
      gyro[0] = gyro[1] = gyro[2] = 0;
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

    
    Result HAL::Step(void)
    {
      
      // Send block connection state when engine connects
      static bool wasConnected = false;
      if (!wasConnected && HAL::RadioIsConnected()) {
        
        // Send RobotAvailable indicating sim robot
        RobotInterface::RobotAvailable idMsg;
        idMsg.robotID = 0;
        idMsg.hwRevision = 0;
        RobotInterface::SendMessage(idMsg);
        
        // send firmware info indicating simulated robot
        {
          std::string firmwareJson{"{\"version\":0,\"time\":0,\"sim\":1}"};
          RobotInterface::FirmwareVersion msg;
          msg.RESRVED = 0;
          msg.json_length = firmwareJson.size() + 1;
          std::memcpy(msg.json, firmwareJson.c_str(), firmwareJson.size() + 1);
          RobotInterface::SendMessage(msg);
        }
        
        wasConnected = true;
      } else if (wasConnected && !HAL::RadioIsConnected()) {
        wasConnected = false;
      }
      
      return RESULT_OK;
      
    } // step()
    

    
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

    void HAL::SetLED(LEDId led_id, u16 color) {
    }

    u32 HAL::GetID()
    {
      return robotID_;
    }
    
    u16 HAL::GetRawProxData()
    {
      return 200;
    }

    u16 HAL::GetRawCliffData(const CliffID cliff_id)
    {
      return 800;
    }
    
    u16 HAL::GetCliffOffLevel(const CliffID cliff_id)
    {
      return 0;
    }
    
    f32 HAL::BatteryGetVoltage()
    {
      return 5;
    }

    bool HAL::BatteryIsCharging()
    {
      return false;
    }

    bool HAL::BatteryIsOnCharger()
    {
      return false;
    }
    
    bool HAL::BatteryIsChargerOOS()
    {
      return false;
    }

    Result HAL::SetBlockLight(const u32 activeID, const u16* colors)
    {
      return RESULT_OK;
    }
    
    Result HAL::StreamObjectAccel(const u32 activeID, const bool enable)
    {
      return RESULT_OK;
    }
    
    Result HAL::AssignSlot(u32 slot_id, u32 factory_id)
    {
      return RESULT_OK;
    }
    
    u8 HAL::GetWatchdogResetCounter()
    {
      return 0;
    }
    
    
    // @return true if the audio clock says it is time for the next frame
    bool HAL::AudioReady()
    {
      return audioReadyForFrame_;
    }
    
    void HAL::AudioPlaySilence() {
      AudioPlayFrame(nullptr);
    }
    
    // Play one frame of audio or silence
    // @param frame - a pointer to an audio frame or NULL to play one frame of silence
    void HAL::AudioPlayFrame(AnimKeyFrame::AudioSample *msg)
    {
      if (audioEndTime_ == 0) {
        audioEndTime_ = HAL::GetTimeStamp();
      }
      audioEndTime_ += AUDIO_FRAME_TIME_MS;
      audioReadyForFrame_ = false;
    }
    
    void HAL::FaceAnimate(u8* frame, const u16 length)
    {}
    
  } // namespace Cozmo
} // namespace Anki
