#include "anki/common/robot/config.h"
#include "anki/common/shared/utilities_shared.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "clad/types/imageTypes.h"
#include "clad/types/powerMessages.h"
#include "anki/cozmo/robot/debug.h"
#include "messages.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "imuFilter.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"
#include "testModeController.h"
#include "localization.h"
#include "pathFollower.h"
#include "speedController.h"
#include "steeringController.h"
#include "wheelController.h"
#include "animationController.h"
#include "proxSensors.h"
#include "backpackLightController.h"
#include "blockLightController.h"
#include "timeProfiler.h"

#include "anki/messaging/shared/utilMessaging.h"

///////// TESTING //////////

#if ANKICORETECH_EMBEDDED_USE_MATLAB && USING_MATLAB_VISION
#include "anki/embeddedCommon/matlabConverters.h"
#endif

///////// END TESTING //////

namespace Anki {
  namespace Cozmo {
    
#ifdef SIMULATOR
    namespace HAL {
      ImageSendMode imageSendMode_;
      ImageResolution captureResolution_ = CVGA;
      void SetImageSendMode(const ImageSendMode mode, const ImageResolution res)
      {
        imageSendMode_ = mode;
        captureResolution_ = res;
      }
    }
#endif
    
    namespace Robot {

      // "Private Member Variables"
      namespace {

        // Parameters / Constants:

        // TESTING
        // Change this value to run different test modes
        const TestMode DEFAULT_TEST_MODE = TM_NONE;

        Robot::OperationMode mode_ = INIT_MOTOR_CALIBRATION;
        bool wasConnected_ = false;

        // For only sending robot state messages every STATE_MESSAGE_FREQUENCY
        // times through the main loop
        u32 robotStateMessageCounter_ = 0;

        // Main cycle time errors
        u32 mainTooLongCnt_ = 0;
        u32 mainTooLateCnt_ = 0;
        u32 avgMainTooLongTime_ = 0;
        u32 avgMainTooLateTime_ = 0;
        u32 lastCycleStartTime_ = 0;
        u32 lastMainCycleTimeErrorReportTime_ = 0;
        const u32 MAIN_TOO_LATE_TIME_THRESH_USEC = TIME_STEP * 1500;  // Normal cycle time plus 50% margin
        const u32 MAIN_TOO_LONG_TIME_THRESH_USEC = 700;
        const u32 MAIN_CYCLE_ERROR_REPORTING_PERIOD_USEC = 1000000;

      } // Robot private namespace

      //
      // Accessors:
      //
      OperationMode GetOperationMode()
      { return mode_; }

      void SetOperationMode(OperationMode newMode)
      { mode_ = newMode; }

      //
      // Methods:
      //

      void StartMotorCalibrationRoutine()
      {
        LiftController::StartCalibrationRoutine();
        HeadController::StartCalibrationRoutine();
        SteeringController::ExecuteDirectDrive(0,0);
      }


      // The initial "stretch" and reset motor positions routine
      // Returns true when done.
      bool MotorCalibrationUpdate()
      {
        bool isDone = false;

        if(
           LiftController::IsCalibrated()
           && HeadController::IsCalibrated()
           ) {
          PRINT("Motors calibrated\n");
          IMUFilter::Reset();
          isDone = true;
        }

        return isDone;
      }


      Result Init(void)
      {
        Result lastResult = RESULT_OK;

        // Coretech setup
#ifndef SIMULATOR
#if(DIVERT_PRINT_TO_RADIO)
        SetCoreTechPrintFunctionPtr(Messages::SendText);
#else
        SetCoreTechPrintFunctionPtr(0);
#endif
#elif(USING_UART_RADIO && DIVERT_PRINT_TO_RADIO)
        SetCoreTechPrintFunctionPtr(Messages::SendText);
#endif

        // HAL and supervisor init
#ifndef ROBOT_HARDWARE    // The HAL/Operating System cannot be Init()ed or Destroy()ed on a real robot
        lastResult = HAL::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "HAL init failed.\n");
#endif

        lastResult = Messages::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "Messages / Reliable Transport init failed.\n");


        lastResult = Localization::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "Localization System init failed.\n");

        /*
        lastResult = VisionSystem::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "Vision System init failed.\n");
         */

        lastResult = PathFollower::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "PathFollower System init failed.\n");

        lastResult = BackpackLightController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "BackpackLightController init failed.\n");
        
        // Initialize subsystems if/when available:
        /*
         if(WheelController::Init() == RESULT_FAIL) {
         PRINT("WheelController initialization failed.\n");
         return RESULT_FAIL;
         }

         if(SpeedController::Init() == RESULT_FAIL) {
         PRINT("SpeedController initialization failed.\n");
         return RESULT_FAIL;
         }

         if(SteeringController::Init() == RESULT_FAIL) {
         PRINT("SteeringController initialization failed.\n");
         return RESULT_FAIL;
         }

         if(HeadController::Init() == RESULT_FAIL) {
         PRINT("HeadController initialization failed.\n");
         return RESULT_FAIL;
         }
         */

        lastResult = DockingController::Init();;
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "DockingController init failed.\n");

        // Before liftController?!
        lastResult = PickAndPlaceController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "PickAndPlaceController init failed.\n");

        lastResult = LiftController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "LiftController init failed.\n");

        lastResult = AnimationController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "AnimationController init failed.\n");

        // Start calibration
        StartMotorCalibrationRoutine();

        // Set starting state
        mode_ = INIT_MOTOR_CALIBRATION;

        robotStateMessageCounter_ = 0;

        return RESULT_OK;

      } // Robot::Init()


#ifndef ROBOT_HARDWARE    // The HAL/Operating System cannot be Init()ed or Destroy()ed on a real robot
      void Destroy()
      {
        HAL::Destroy();
      }
#endif


      Result step_MainExecution()
      {
        START_TIME_PROFILE(CozmoBotMain, TOTAL);
        START_TIME_PROFILE(CozmoBot, HAL);
        
        // HACK: Manually setting timestamp here in mainExecution until
        // until Nathan implements this the correct way.
        HAL::SetTimeStamp(HAL::GetTimeStamp()+TIME_STEP);
        
        // TBD - This should be moved to simulator just before step_MainExecution is called
#ifndef ROBOT_HARDWARE
        // If the hardware interface needs to be advanced (as in the case of
        // a Webots simulation), do that first.
        HAL::Step();
#endif
        
        // Detect if it took too long in between mainExecution calls
        u32 cycleStartTime = HAL::GetMicroCounter();
        if (lastCycleStartTime_ != 0) {
          u32 timeBetweenCycles = cycleStartTime - lastCycleStartTime_;
          if (timeBetweenCycles > MAIN_TOO_LATE_TIME_THRESH_USEC) {
            ++mainTooLateCnt_;
            avgMainTooLateTime_ = (u32)((f32)(avgMainTooLateTime_ * (mainTooLateCnt_ - 1) + timeBetweenCycles)) / mainTooLateCnt_;
          }
        }
 

/*
        // Test code for measuring number of mainExecution tics per second
        static u32 cnt = 0;
        static u32 startTime = 0;
        const u32 interval_seconds = 5;
        
        if (++cnt == (200 * interval_seconds)) {
          u32 numTicsPerSec = (cnt * 1000000) / (cycleStartTime - startTime);
          PRINT("TicsPerSec %d\n", numTicsPerSec);
          startTime = cycleStartTime;
          cnt = 0;
        }
*/
        
        //////////////////////////////////////////////////////////////
        // Test Mode
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, TEST);
        TestModeController::Update();


        //////////////////////////////////////////////////////////////
        // Localization
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, LOC);
        Localization::Update();


        //////////////////////////////////////////////////////////////
        // Communications
        //////////////////////////////////////////////////////////////
        
        // Check if there is a new or dropped connection to a basestation
        if (HAL::RadioIsConnected() && !wasConnected_) {
          PRINT("Robot radio is connected.\n");
          wasConnected_ = true;
          BackpackLightController::TurnOffAll();
        } else if (!HAL::RadioIsConnected() && wasConnected_) {
          PRINT("Radio disconnected\n");
          Messages::ResetInit();
          TestModeController::Start(TM_NONE);
          SteeringController::ExecuteDirectDrive(0,0);
          LiftController::SetAngularVelocity(0);
          HeadController::SetAngularVelocity(0);
          PickAndPlaceController::Reset();
          PickAndPlaceController::SetCarryState(CARRY_NONE);
          BackpackLightController::TurnOffAll();
          BackpackLightController::SetParams(LED_BACKPACK_LEFT, LED_RED, LED_OFF, 1000, 1000, 0, 0);
          wasConnected_ = false;
        }

        // Process any messages from the basestation
        MARK_NEXT_TIME_PROFILE(CozmoBot, MSG);
        Messages::ProcessBTLEMessages();

        //////////////////////////////////////////////////////////////
        // Sensor updates
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, IMU);
        IMUFilter::Update();
        ProxSensors::Update();

        
        //////////////////////////////////////////////////////////////
        // Power management
        //////////////////////////////////////////////////////////////
        static bool _wasCharging = false;
        if (HAL::BatteryIsOnCharger() && !_wasCharging) {
          ChargerEvent msg;
          msg.onCharger = _wasCharging = true;
          RobotInterface::SendMessage(msg);
          
          // Stop whatever we were doing
          AnimationController::Clear();
          PickAndPlaceController::Reset();
          
        } else if (!HAL::BatteryIsOnCharger() && _wasCharging) {
          ChargerEvent msg;
          msg.onCharger = _wasCharging = false;
          RobotInterface::SendMessage(msg);
        }
        
        //////////////////////////////////////////////////////////////
        // Head & Lift Position Updates
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, ANIM);
        if(AnimationController::Update() != RESULT_OK) {
          PRINT("Failed updating AnimationController. Clearing.\n");
          AnimationController::Clear();
        }
        MARK_NEXT_TIME_PROFILE(CozmoBot, EYEHEADLIFT);
        HeadController::Update();
        LiftController::Update();

        MARK_NEXT_TIME_PROFILE(CozmoBot, PATHDOCK);
        PathFollower::Update();
        PickAndPlaceController::Update();
        DockingController::Update();
        BackpackLightController::Update();
        BlockLightController::Update();

        //////////////////////////////////////////////////////////////
        // Audio Subsystem
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, AUDIO);
        Anki::Cozmo::HAL::AudioFill();
        
        //////////////////////////////////////////////////////////////
        // State Machine
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, WHEELS);
        switch(mode_)
        {
          case INIT_MOTOR_CALIBRATION:
          {
            if(MotorCalibrationUpdate()) {
              // Once initialization is done, broadcast a message that this robot
              // is ready to go
              RobotInterface::RobotAvailable msg;
              msg.robotID = HAL::GetIDCard()->esn;
              PRINT("Robot %d broadcasting availability message.\n", msg.robotID);
              RobotInterface::SendMessage(msg);

              // Start test mode
              if (DEFAULT_TEST_MODE != TM_NONE) {
                if(TestModeController::Start(DEFAULT_TEST_MODE) == RESULT_FAIL) {
                  PRINT("TestMode %d failed to start.\n", DEFAULT_TEST_MODE);
                  return RESULT_FAIL;
                }
              }

              mode_ = WAITING;
            }

            break;
          }
          case WAITING:
          {
            // Idle.  Nothing to do yet...

            break;
          }

          default:
            PRINT("Unrecognized CozmoBot mode.\n");

        } // switch(mode_)

        // Manage the various motion controllers:
        SpeedController::Manage();
        SteeringController::Manage();
        WheelController::Manage();
        
        MARK_NEXT_TIME_PROFILE(CozmoBot, CUBES);
        Anki::Cozmo::HAL::ManageCubes();
        

        //////////////////////////////////////////////////////////////
        // Feedback / Display
        //////////////////////////////////////////////////////////////

        Messages::UpdateRobotStateMsg();
#if(!STREAM_DEBUG_IMAGES)
        ++robotStateMessageCounter_;
        if(robotStateMessageCounter_ >= STATE_MESSAGE_FREQUENCY) {
          Messages::SendRobotStateMsg();
          robotStateMessageCounter_ = 0;
        }
#endif

// TBD - This should be moved to simulator just after step_MainExecution is called
#ifndef ROBOT_HARDWARE
        HAL::UpdateDisplay();
#endif
        
        // Print time profile stats
        END_TIME_PROFILE(CozmoBot);
        END_TIME_PROFILE(CozmoBotMain);
        PERIODIC_PRINT_AND_RESET_TIME_PROFILE(CozmoBot, 400);
        PERIODIC_PRINT_AND_RESET_TIME_PROFILE(CozmoBotMain, 400);

        // Check if main took too long
        u32 cycleEndTime = HAL::GetMicroCounter();
        u32 cycleTime = cycleEndTime - cycleStartTime;
        if (cycleTime > MAIN_TOO_LONG_TIME_THRESH_USEC) {
          ++mainTooLongCnt_;
          avgMainTooLongTime_ = (u32)((f32)(avgMainTooLongTime_ * (mainTooLongCnt_ - 1) + cycleTime)) / mainTooLongCnt_;
        }
        lastCycleStartTime_ = cycleStartTime;
        
        // Report main cycle time error
        if ((mainTooLateCnt_ > 0 || mainTooLongCnt_ > 0) &&
            (cycleEndTime - lastMainCycleTimeErrorReportTime_ > MAIN_CYCLE_ERROR_REPORTING_PERIOD_USEC)) {
          RobotInterface::MainCycleTimeError m;
          m.numMainTooLateErrors = mainTooLateCnt_;
          m.avgMainTooLateTime = avgMainTooLateTime_;
          m.numMainTooLongErrors = mainTooLongCnt_;
          m.avgMainTooLongTime = avgMainTooLongTime_;

          RobotInterface::SendMessage(m);

          mainTooLateCnt_ = 0;
          avgMainTooLateTime_ = 0;
          mainTooLongCnt_ = 0;
          avgMainTooLongTime_ = 0;

          lastMainCycleTimeErrorReportTime_ = cycleEndTime;
        }


        return RESULT_OK;

      } // Robot::step_MainExecution()




      // Long Execution now just captures image
      Result step_LongExecution()
      {
        Result retVal = RESULT_OK;
        
#       ifdef SIMULATOR
        
        if (!HAL::IsVideoEnabled()) {
          return retVal;
        }

        if (HAL::imageSendMode_ != Off) {
        
          TimeStamp_t currentTime = HAL::GetTimeStamp();

          // This computation is based on Cyberbotics support's explaination for how to compute
          // the actual capture time of the current available image from the simulated
          // camera, *except* I seem to need the extra "- VISION_TIME_STEP" for some reason.
          // (The available frame is still one frame behind? I.e. we are just *about* to capture
          //  the next one?)
          TimeStamp_t currentImageTime = std::floor((currentTime-HAL::GetCameraStartTime())/VISION_TIME_STEP) * VISION_TIME_STEP + HAL::GetCameraStartTime() - VISION_TIME_STEP;

          // Keep up with the capture time of the last image we sent
          static TimeStamp_t lastImageSentTime = 0;

          // Have we already sent the currently-available image?
          if(lastImageSentTime != currentImageTime)
          {
            // Nope, so get the (new) available frame from the camera:
            const s32 captureHeight = Vision::CameraResInfo[HAL::captureResolution_].height;
            const s32 captureWidth  = Vision::CameraResInfo[HAL::captureResolution_].width * 3; // The "*3" is a hack to get enough room for color

            static const int bufferSize = 1000000;
            static u8 buffer[bufferSize];
            Embedded::MemoryStack scratch(buffer, bufferSize);
            Embedded::Array<u8> image(captureHeight, captureWidth,
                                      scratch, Embedded::Flags::Buffer(false,false,false));

            HAL::CameraGetFrame(reinterpret_cast<u8*>(image.get_buffer()),
                                HAL::captureResolution_, false);
            if(!image.IsValid()) {
              retVal = RESULT_FAIL;
            } else {

              // Send the image, with its actual capture time (not the current system time)
              Messages::CompressAndSendImage(image, currentImageTime);

              //PRINT("Sending state message from time = %d to correspond to image at time = %d\n",
              //      robotState.timestamp, currentImageTime);

              // Mark that we've already sent the image for the current time
              lastImageSentTime = currentImageTime;
            }
          } // if(lastImageSentTime != currentImageTime)
          
          
          if (HAL::imageSendMode_ == SingleShot) {
            HAL::imageSendMode_ = Off;
          }
          
        } // if (HAL::imageSendMode_ != ISM_OFF)

#       endif // ifdef SIMULATOR

        return retVal;

      } // Robot::step_longExecution()


    } // namespace Robot
  } // namespace Cozmo
} // namespace Anki
