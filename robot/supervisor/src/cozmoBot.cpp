#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "clad/types/imageTypes.h"
#include "timeProfiler.h"
#include "messages.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "liftController.h"
#include "headController.h"
#include "imuFilter.h"
#include "proxSensors.h"
#include "speedController.h"
#include "steeringController.h"
#include "wheelController.h"
#include "localization.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "pathFollower.h"
#include "testModeController.h"
#include "anki/cozmo/robot/logging.h"
#ifndef TARGET_K02
#include "animationController.h"
#include "anki/common/shared/utilities_shared.h"
#include "blockLightController.h"
#endif

#ifdef SIMULATOR
#include "anki/vision/CameraSettings.h"
#include "nvStorage.h"
#include <math.h>
#endif

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
      ImageResolution captureResolution_ = QVGA;
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
        
        // Wait flag for StartMotorCalibration that we expect to receive
        // from the body when wifi connects
        bool waitForFirstMotorCalibAfterConnect_ = true;

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
      Result Init(void)
      {
        Result lastResult = RESULT_OK;

        // Coretech setup
#ifndef TARGET_K02
#ifndef SIMULATOR
#if(DIVERT_PRINT_TO_RADIO)
        SetCoreTechPrintFunctionPtr(Messages::SendText);
#else
        SetCoreTechPrintFunctionPtr(0);
#endif
#elif(USING_UART_RADIO && DIVERT_PRINT_TO_RADIO)
        SetCoreTechPrintFunctionPtr(Messages::SendText);
#endif
#endif
        // HAL and supervisor init
#ifndef ROBOT_HARDWARE    // The HAL/Operating System cannot be Init()ed or Destroy()ed on a real robot
        lastResult = HAL::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, 219, "CozmoBot.InitFail.HAL", 305, "", 0);
#endif
        lastResult = Messages::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, 220, "CozmoBot.InitFail.Messages", 305, "", 0);

        lastResult = Localization::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, 221, "CozmoBot.InitFail.Localization", 305, "", 0);

        lastResult = PathFollower::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, 222, "CozmoBot.InitFail.PathFollower", 305, "", 0);
        
        lastResult = IMUFilter::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, 366, "CozmoBot.InitFail.IMUFilter", 305, "", 0);
        
        lastResult = DockingController::Init();;
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, 224, "CozmoBot.InitFail.DockingController", 305, "", 0);

        // Before liftController?!
        lastResult = PickAndPlaceController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, 225, "CozmoBot.InitFail.PickAndPlaceController", 305, "", 0);

        lastResult = LiftController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, 226, "CozmoBot.InitFail.LiftController", 305, "", 0);
#ifndef TARGET_K02
        lastResult = AnimationController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, 227, "CozmoBot.InitFail.AnimationController", 305, "", 0);
#endif

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
          AnkiInfo( 94, "CozmoBot.TicsPerSec", 347, "%d", 1, numTicsPerSec);
          startTime = cycleStartTime;
          cnt = 0;
        }
*/

        //////////////////////////////////////////////////////////////
        // Simulated NVStorage
        //////////////////////////////////////////////////////////////
#if SIMULATOR
        NVStorage::Update();
#endif

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
          AnkiEvent( 228, "CozmoBot.Radio.Connected", 305, "", 0);
          wasConnected_ = true;
          
#if SIMULATOR
          LiftController::Enable();
          HeadController::Enable();
          WheelController::Enable();
#endif
        } else if (!HAL::RadioIsConnected() && wasConnected_) {
          AnkiInfo( 229, "CozmoBot.Radio.Disconnected", 305, "", 0);
          Messages::ResetInit();
          SteeringController::ExecuteDirectDrive(0,0);
          PickAndPlaceController::Reset();
          PickAndPlaceController::SetCarryState(CARRY_NONE);
          ProxSensors::EnableStopOnCliff(true);
          waitForFirstMotorCalibAfterConnect_ = true;
          mode_ = INIT_MOTOR_CALIBRATION;

#if SIMULATOR
          LiftController::Disable();
          HeadController::Disable();
          WheelController::Disable();
          
          TestModeController::Start(TM_NONE);
          AnimationController::EnableTracks(ALL_TRACKS);
          HAL::FaceClear();
#endif
          wasConnected_ = false;
        }

        // Process any messages from the basestation
        MARK_NEXT_TIME_PROFILE(CozmoBot, MSG);
        Messages::Update();

        //////////////////////////////////////////////////////////////
        // Sensor updates
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, IMU);
        IMUFilter::Update();
        ProxSensors::Update();


        //////////////////////////////////////////////////////////////
        // Head & Lift Position Updates
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, ANIM);
#ifndef TARGET_K02
        if(AnimationController::Update() != RESULT_OK) {
          AnkiWarn( 230, "CozmoBot.Main.AnimationControllerUpdateFailed", 305, "", 0);
          AnimationController::Clear();
        }
#endif
        MARK_NEXT_TIME_PROFILE(CozmoBot, EYEHEADLIFT);
        HeadController::Update();
        LiftController::Update();
#ifndef TARGET_K02
        BlockLightController::Update();
#endif
        MARK_NEXT_TIME_PROFILE(CozmoBot, PATHDOCK);
        PathFollower::Update();
        PickAndPlaceController::Update();
        DockingController::Update();

#ifndef TARGET_K02
        //////////////////////////////////////////////////////////////
        // Audio Subsystem
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, AUDIO);
        Anki::Cozmo::HAL::AudioFill();
#endif

        //////////////////////////////////////////////////////////////
        // State Machine
        //////////////////////////////////////////////////////////////
        MARK_NEXT_TIME_PROFILE(CozmoBot, WHEELS);
        switch(mode_)
        {
          case INIT_MOTOR_CALIBRATION:
          {
            if (LiftController::IsCalibrated() && HeadController::IsCalibrated()) {
              // Once initialization is done, broadcast a message that this robot
              // is ready to go
              waitForFirstMotorCalibAfterConnect_ = false;
              
#ifndef TARGET_K02
              RobotInterface::RobotAvailable msg;
              msg.robotID = HAL::GetIDCard()->esn;
              AnkiInfo( 179, "CozmoBot.BroadcastingAvailability", 479, "RobotID: %d", 1, msg.robotID);
              RobotInterface::SendMessage(msg);
#endif
              
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
            AnkiWarn( 231, "CozmoBot.InvalidMode", 347, "%d", 1, mode_);

        } // switch(mode_)

        // Manage the various motion controllers:
        SpeedController::Manage();
        SteeringController::Manage();
        WheelController::Manage();


        //////////////////////////////////////////////////////////////
        // Feedback / Display
        //////////////////////////////////////////////////////////////

        Messages::UpdateRobotStateMsg();
#if(!STREAM_DEBUG_IMAGES)
        ++robotStateMessageCounter_;
        if(robotStateMessageCounter_ >= STATE_MESSAGE_FREQUENCY) {
          if (!waitForFirstMotorCalibAfterConnect_) Messages::SendRobotStateMsg();
          Messages::SendTestStateMsg();
          robotStateMessageCounter_ = 0;
        }
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
          TimeStamp_t currentImageTime = floor((currentTime-HAL::GetCameraStartTime())/VISION_TIME_STEP) * VISION_TIME_STEP + HAL::GetCameraStartTime() - VISION_TIME_STEP;

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

            HAL::CameraGetFrame(buffer,
                                HAL::captureResolution_, false);
            // Send the image, with its actual capture time (not the current system time)
            Messages::CompressAndSendImage(buffer, captureHeight, captureWidth, currentImageTime);

            //PRINT("Sending state message from time = %d to correspond to image at time = %d\n",
            //      robotState.timestamp, currentImageTime);

            // Mark that we've already sent the image for the current time
            lastImageSentTime = currentImageTime;
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
