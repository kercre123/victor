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
#include "anki/cozmo/shared/cozmoConfig.h"

#include "../spine/spine_hal.h"
 
#include "clad/robotInterface/messageRobotToEngine.h"    //TODO: why do we still need these 2?
#include "clad/robotInterface/messageRobotToEngine_send_helper.h" 
#include "clad/spine/spine_protocol.h"

#define RADIO_IP "127.0.0.1"


// Debugging Defines
#define FRAMES_PER_RESPONSE  1  //send response every N input frames
#define REALTIME_CONSOLE_OUTPUT 0 //Print status to console

#if REALTIME_CONSOLE_OUTPUT > 0
#define SAVE_MOTOR_POWER(motor, power)  internalData_.motorPower[motor]=power
#define CONSOLE_DATA(decl) decl
#else
#define SAVE_MOTOR_POWER(motor, power)
#define CONSOLE_DATA(decl) 
#endif

namespace Anki {
  namespace Cozmo {

    static_assert(MOTOR_LEFT_WHEEL == RobotMotor_MOTOR_LEFT, "Robot/Spine CLAD Mimatch");
    static_assert(MOTOR_RIGHT_WHEEL == RobotMotor_MOTOR_RIGHT, "Robot/Spine CLAD Mimatch");
    static_assert(MOTOR_LIFT == RobotMotor_MOTOR_LIFT, "Robot/Spine CLAD Mimatch");
    static_assert(MOTOR_HEAD == RobotMotor_MOTOR_HEAD, "Robot/Spine CLAD Mimatch");
    

    namespace { // "Private members"

      //map power -1.0 .. 1.0 to -32767 to 32767
      static const f32 HAL_MOTOR_POWER_SCALE = 0x7FFF;
      static const f32 HAL_MOTOR_POWER_OFFSET = 0;
      
      //convert per syscon tick -> /sec
      static const f32 HAL_SEC_PER_TICK = (1.0/256)/48000000;

      //encoder counts -> mm or deg
      static const f32 HAL_MOTOR_POSITION_SCALE[RobotMotor_MOTOR_COUNT] = {
        ((0.948 * 0.125 * 0.0292 * 3.14159265359) / 149.7), //Left Tread mm
        ((0.948 * 0.125 * 0.0292 * 3.14159265359) / 149.7), //Right Tread mm
        (0.125 * 3.14159265359) / 172.68,   //Lift radians
        (0.25 * 3.14159265359) / 348.77,   //Head radians
      };

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

      struct {
        s32 motorOffset[RobotMotor_MOTOR_COUNT];
        CONSOLE_DATA(f32 motorSpeed[RobotMotor_MOTOR_COUNT]);
        CONSOLE_DATA(f32 motorPower[RobotMotor_MOTOR_COUNT]);
      } internalData_;
      
    } // "private" namespace

    // Forward Declarations
    Result InitRadio(const char* advertisementIP);
    void InitIMU();
    void ProcessIMUEvents();
    

    Result GetSpineDataFrame(void) {
      SpineMessageHeader* hdr = (SpineMessageHeader*) hal_get_frame(PayloadId_PAYLOAD_DATA_FRAME);
      if (hdr->payload_type != PayloadId_PAYLOAD_DATA_FRAME) {
        LOGE("Spine.c data corruption: payload does not match requested");
        return RESULT_FAIL_IO_UNSYNCHRONIZED;
      }
      bodyData_ = (BodyToHead*)(hdr+1);
      return RESULT_OK;
    }

    Result HAL::Init() {
      // Set ID
      robotID_ = 1;

      InitIMU();

      if (InitRadio(RADIO_IP) != RESULT_OK) {
        printf("Failed to initialize Radio.\n");
        return RESULT_FAIL;
      }

      printf("Starting spine hal\n");

      SpineErr_t result = hal_init(SPINE_TTY, SPINE_BAUD);

      if (result != err_OK) {
        return RESULT_FAIL;
      }

      hal_set_mode(RobotMode_RUN);
      
      while (GetSpineDataFrame() != RESULT_OK) {
        ; //spin on good frame
      }

      MotorID m;
      for (m=MOTOR_LIFT;m<MOTOR_COUNT;m++) {
        MotorResetPosition(m);
      }
      printf("Hal Init Success\n");

      return RESULT_OK;
    }  // Init()

    // Set the motor power in the unitless range [-1.0, 1.0]
    void HAL::MotorSetPower(MotorID motor, f32 power) {
      assert( motor < RobotMotor_MOTOR_COUNT );
      SAVE_MOTOR_POWER(motor,power);
      headData_.motorPower[motor] = HAL_MOTOR_POWER_OFFSET + HAL_MOTOR_POWER_SCALE * power;
    }

    // Reset the internal position of the specified motor to 0
    void HAL::MotorResetPosition(MotorID motor) {
      assert( motor < RobotMotor_MOTOR_COUNT );
      internalData_.motorOffset[motor]=bodyData_->motor[motor].position;
    }


    // Returns units based on the specified motor type:
    // Wheels are in mm/s, everything else is in degrees/s.
    f32 HAL::MotorGetSpeed(MotorID motor) {
      assert( motor < RobotMotor_MOTOR_COUNT );

      // Every frame, syscon sends the last detected speed as a two part number:
      // `delta` encoder counts, and `time` span for those counts.
      // syscon only changes the value when counts are detected
      // if no counts for ~25ms, will report 0/0
      if (bodyData_->motor[motor].time != 0) {
        float countsPerTick = (float)bodyData_->motor[motor].delta / bodyData_->motor[motor].time;
        return (countsPerTick / HAL_SEC_PER_TICK) * HAL_MOTOR_POSITION_SCALE[motor];
      }
      return 0.0; //if time is 0, it's not moving.
    }

    // Returns units based on the specified motor type:
    // Wheels are in mm since reset, everything else is in degrees.
    f32 HAL::MotorGetPosition(MotorID motor) {
      assert( motor < RobotMotor_MOTOR_COUNT );
      return (bodyData_->motor[motor].position - internalData_.motorOffset[motor]) * HAL_MOTOR_POSITION_SCALE[motor];
    }

    void PrintConsoleOutput(void)
    {
      #if REALTIME_CONSOLE_OUTPUT > 0
      {
        printf("FC = %d ", bodyData_->framecounter);
        printf("Lraw = %d ", bodyData_->motor[MOTOR_LIFT].position);
        printf("Lpos = %f ", HAL::MotorGetPosition(MOTOR_LIFT));
        printf("Lspd = %f ", internalData_.motorSpeed[MOTOR_LIFT]);
        printf(": Lpow = %f ",internalData_.motorPower[MOTOR_LIFT]);
        printf("\r");
      }
      #endif
    }
    

    Result HAL::MonitorConnectionState(void)
    {
      ProcessIMUEvents();
      
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
          std::string firmwareJson{"{\"version\":0,\"time\":0,\"sim\":0}"};
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


    
    Result HAL::Step(void)
    {
      static int repeater = FRAMES_PER_RESPONSE;
      if (--repeater <= 0) {
        repeater = FRAMES_PER_RESPONSE;
        headData_.framecounter++;
        hal_send_frame(PayloadId_PAYLOAD_DATA_FRAME, &headData_, sizeof(HeadToBody));
      }
      Result result =  GetSpineDataFrame();

      PrintConsoleOutput();
      
      MonitorConnectionState();
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

    void HAL::SetLED(LEDId led_id, u16 color) {
    }

    u32 HAL::GetID()
    {
      return robotID_;
    }
    
    u16 HAL::GetRawProxData()
    {
      return bodyData_->proximity.rangeMM;
    }

    u16 HAL::GetRawCliffData(const CliffID cliff_id) {
      assert (cliff_id < DropSensor_DROP_SENSOR_COUNT);
      return bodyData_->cliffSense[cliff_id];
    }

    u16 HAL::GetCliffOffLevel(const CliffID cliff_id) {
      // This is not supported by V2 hardware
      return 0;
    }

    f32 HAL::BatteryGetVoltage() {
      return bodyData_->battery.batteryVolts;
    }

    bool HAL::BatteryIsCharging() {
      return bodyData_->battery.flags & BatteryFlags_isCharging;
    }

    bool HAL::BatteryIsOnCharger() {
      return bodyData_->battery.flags & BatteryFlags_isOnCharger;
    }

    bool HAL::BatteryIsChargerOOS() {
      return bodyData_->battery.flags & BatteryFlags_chargerOOS;
    }

    Result HAL::SetBlockLight(const u32 activeID, const u16* colors) {
      // Not implemented in HAL in V2
      return RESULT_OK;
    }

    Result HAL::StreamObjectAccel(const u32 activeID, const bool enable) {
      // Not implemented in HAL in V2
      return RESULT_OK;
    }

    Result HAL::AssignSlot(u32 slot_id, u32 factory_id) {
      // Not implemented in HAL in V2
      return RESULT_OK;
    }

    u8 HAL::GetWatchdogResetCounter() {
      // not (yet) implemented in HAL in V2
      return 0;//bodyData_->status.watchdogCount;
    }

    // @return true if the audio clock says it is time for the next frame
    bool HAL::AudioReady() {
      // Not implemented in HAL in V2
      return audioReadyForFrame_;
    }

    void HAL::AudioPlaySilence() {
      // Not implemented in HAL in V2
      AudioPlayFrame(nullptr);
    }

    // Play one frame of audio or silence
    // @param frame - a pointer to an audio frame or NULL to play one frame of silence
    void HAL::AudioPlayFrame(AnimKeyFrame::AudioSample *msg) {
      // Not implemented in HAL in V2
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
