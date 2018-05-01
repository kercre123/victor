#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/logEvent.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

#include "backpackLightController.h"
#include "dockingController.h"
#include "liftController.h"
#include "localization.h"
#include "headController.h"
#include "imuFilter.h"
#include "messages.h"
#include "pathFollower.h"
#include "pickAndPlaceController.h"
#include "proxSensors.h"
#include "speedController.h"
#include "steeringController.h"
#include "testModeController.h"
#include "timeProfiler.h"
#include "wheelController.h"

#ifndef SIMULATOR
#include <unistd.h>
#endif

namespace Anki {
  namespace Cozmo {
    namespace Robot {

      // "Private Member Variables"
      namespace {

        // Parameters / Constants:
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
        const u32 MAIN_TOO_LATE_TIME_THRESH_USEC = ROBOT_TIME_STEP_MS * 1500;  // Normal cycle time plus 50% margin
        const u32 MAIN_TOO_LONG_TIME_THRESH_USEC = 700;
        const u32 MAIN_CYCLE_ERROR_REPORTING_PERIOD_USEC = 1000000;
      } // Robot private namespace

      //
      // Methods:
      //
      Result Init(void)
      {
        Result lastResult = RESULT_OK;

        // HAL and supervisor init
        lastResult = HAL::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, "CozmoBot.InitFail.HAL", "");

        lastResult = BackpackLightController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, "CozmoBot.InitFail.BackpackLightController", "");
        
        lastResult = Messages::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, "CozmoBot.InitFail.Messages", "");

        lastResult = Localization::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, "CozmoBot.InitFail.Localization", "");

        lastResult = PathFollower::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, "CozmoBot.InitFail.PathFollower", "");
        
        lastResult = IMUFilter::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, "CozmoBot.InitFail.IMUFilter", "");
        
        lastResult = DockingController::Init();;
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, "CozmoBot.InitFail.DockingController", "");

        // Before liftController?!
        lastResult = PickAndPlaceController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, "CozmoBot.InitFail.PickAndPlaceController", "");

        lastResult = LiftController::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult, "CozmoBot.InitFail.LiftController", "");

        // Calibrate motors
        LiftController::StartCalibrationRoutine(1);
        HeadController::StartCalibrationRoutine(1);
        
        robotStateMessageCounter_ = 0;

        return RESULT_OK;

      } // Robot::Init()

      // Handler for cleaning up when this process is killed.
      // Note: Motors are disabled automatically by syscon after 25ms of spine sync loss
      void Destroy()
      {
        AnkiInfo("CozmoBot.Destroy", "");

        HAL::Stop();

        // Turn off lights
        BackpackLightController::TurnOffAll();
      }

      void CheckForShutdown()
      {
        // Shutdown sequence will be begin if the button is held down for this
        // amount of time
        static const int SHUTDOWN_TIME_MS = 4000;

        // Wait this amount of time after sending PrepForShutdown message and calling
        // sync(). The assumption is that whoever is handling the PrepForShutdown message
        // is able to do so within this amount of time
        static const int TIME_TO_WAIT_UNTIL_SYNC_MS = 350;

        // Wait this amount of time after syncing before sending the shutdown
        // command to syscon
        static const int TIME_TO_WAIT_AFTER_SYNC_MS = 100;
        
        enum ShutdownState
        {
          NONE,
          START,
          SYNC,
        };
        static ShutdownState state = NONE;
        static bool buttonWasPressed = false;
        static TimeStamp_t timeMark_ms = 0;
        static bool buttonReleasedAfterShutdownStarted = false;
        
        const TimeStamp_t curTime_ms = HAL::GetTimeStamp();
        const bool buttonIsPressed = HAL::GetButtonState(HAL::ButtonID::BUTTON_POWER);

        if(state != NONE && !buttonIsPressed)
        {
          buttonReleasedAfterShutdownStarted = true;
        }
        
        switch(state)
        {
          case NONE:
            if(buttonIsPressed)
            {
              // If button has just been pressed, record time
              if(!buttonWasPressed)
              {
                timeMark_ms = curTime_ms;
              }
              // If button has been held down for more than SHUTDOWN_TIME_MS
              else if(curTime_ms - timeMark_ms > SHUTDOWN_TIME_MS)
              {
                timeMark_ms = curTime_ms;
                state = START;
                // Send a shutdown message to anim/engine
                AnkiWarn("CozmoBot.CheckForButtonHeld.Shutdown", "Sending PrepForShutdown");
                RobotInterface::PrepForShutdown msg;
                RobotInterface::SendMessage(msg);
              }
            }
            else
            {
              timeMark_ms = 0;
            }
            break;

          case START:
            // If it has been more than TIME_TO_WAIT_UNTIL_SYNC_MS
            // then we should sync()
            if(curTime_ms - timeMark_ms > TIME_TO_WAIT_UNTIL_SYNC_MS)
            {
              AnkiWarn("CozmoBot.CheckForButtonHeld.Sync", "");
              sync();
              state = SYNC;
              timeMark_ms = curTime_ms;
            }
            break;
 
          case SYNC:
            // Waiting for either the button to be released
            // or syscon's power down time to be reached
            if(buttonReleasedAfterShutdownStarted &&
               curTime_ms - timeMark_ms > TIME_TO_WAIT_AFTER_SYNC_MS)
            {
              AnkiWarn("CozmoBot.CheckForButtonHeld.HALShutdown","");
              HAL::Shutdown();
            }
            break;
        }

        buttonWasPressed = buttonIsPressed;
      }

      
      Result step_MainExecution()
      {
        EventStart(EventType::MAIN_STEP);

        START_TIME_PROFILE(CozmoBotMain, TOTAL);
        START_TIME_PROFILE(CozmoBot, HAL);

        // Detect if it took too long in between mainExecution calls
        u32 cycleStartTime = HAL::GetMicroCounter();
        if (lastCycleStartTime_ != 0) {
          u32 timeBetweenCycles = cycleStartTime - lastCycleStartTime_;
          if (timeBetweenCycles > MAIN_TOO_LATE_TIME_THRESH_USEC) {
            EventStart(EventType::MAIN_CYCLE_TOO_LATE);
            ++mainTooLateCnt_;
            EventStop(EventType::MAIN_CYCLE_TOO_LATE);
            avgMainTooLateTime_ = (u32)((f32)(avgMainTooLateTime_ * (mainTooLateCnt_ - 1) + timeBetweenCycles)) / mainTooLateCnt_;
          }
        }

        CheckForShutdown();
/*
        // Test code for measuring number of mainExecution tics per second
        static u32 cnt = 0;
        static u32 startTime = 0;
        const u32 interval_seconds = 5;

        if (++cnt == (200 * interval_seconds)) {
          u32 numTicsPerSec = (cnt * 1000000) / (cycleStartTime - startTime);
          AnkiInfo( "CozmoBot.TicsPerSec", "%d", numTicsPerSec);
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
          AnkiEvent( "CozmoBot.Radio.Connected", "");
          wasConnected_ = true;
          
#ifdef SIMULATOR
          LiftController::Enable();
          HeadController::Enable();
          WheelController::Enable();
#endif
        } else if (!HAL::RadioIsConnected() && wasConnected_) {
          AnkiInfo( "CozmoBot.Radio.Disconnected", "");
          Messages::ResetInit();
          PathFollower::Init();
          SteeringController::ExecuteDirectDrive(0,0);
          PickAndPlaceController::Reset();
          PickAndPlaceController::SetCarryState(CarryState::CARRY_NONE);
          ProxSensors::EnableStopOnCliff(true);
          ProxSensors::SetAllCliffDetectThresholds(CLIFF_SENSOR_THRESHOLD_DEFAULT);

          TestModeController::Start(TestMode::TM_NONE);

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

        MARK_NEXT_TIME_PROFILE(CozmoBot, EYEHEADLIFT);
        HeadController::Update();
        LiftController::Update();
        
        MARK_NEXT_TIME_PROFILE(CozmoBot, LIGHTS);
        BackpackLightController::Update();
        
        MARK_NEXT_TIME_PROFILE(CozmoBot, PATHDOCK);
        PathFollower::Update();
        PickAndPlaceController::Update();
        DockingController::Update();

       

        // Manage the various motion controllers:
        SpeedController::Manage();
        SteeringController::Manage();
        WheelController::Manage();


        //////////////////////////////////////////////////////////////
        // Feedback / Display
        //////////////////////////////////////////////////////////////

        Messages::UpdateRobotStateMsg();
        ++robotStateMessageCounter_;
        if(robotStateMessageCounter_ >= STATE_MESSAGE_FREQUENCY) {
          Messages::SendRobotStateMsg();
          robotStateMessageCounter_ = 0;
        }

        // Now that the robot state msg has been udpated, send mic data (which uses some of robot state)
        Messages::SendMicDataMsgs();

        // Print time profile stats
        END_TIME_PROFILE(CozmoBot);
        END_TIME_PROFILE(CozmoBotMain);
        PERIODIC_PRINT_AND_RESET_TIME_PROFILE(CozmoBot, 400);
        PERIODIC_PRINT_AND_RESET_TIME_PROFILE(CozmoBotMain, 400);

        // Check if main took too long
        u32 cycleEndTime = HAL::GetMicroCounter();
        u32 cycleTime = cycleEndTime - cycleStartTime;
        if (cycleTime > MAIN_TOO_LONG_TIME_THRESH_USEC) {
          EventStart(EventType::MAIN_CYCLE_TOO_LONG);
          ++mainTooLongCnt_;
          EventStop(EventType::MAIN_CYCLE_TOO_LONG);
          avgMainTooLongTime_ = (u32)((f32)(avgMainTooLongTime_ * (mainTooLongCnt_ - 1) + cycleTime)) / mainTooLongCnt_;
        }
        lastCycleStartTime_ = cycleStartTime;


        // Report main cycle time error
        if ((mainTooLateCnt_ > 0 || mainTooLongCnt_ > 0) &&
            (cycleEndTime - lastMainCycleTimeErrorReportTime_ > MAIN_CYCLE_ERROR_REPORTING_PERIOD_USEC)) {
          
          AnkiWarn( "CozmoBot.MainCycleTimeError", "TooLateCount: %d, avgTooLateTime: %d us, tooLongCount: %d, avgTooLongTime: %d us",
                   mainTooLateCnt_, avgMainTooLateTime_, mainTooLongCnt_, avgMainTooLongTime_);

          mainTooLateCnt_ = 0;
          avgMainTooLateTime_ = 0;
          mainTooLongCnt_ = 0;
          avgMainTooLongTime_ = 0;

          lastMainCycleTimeErrorReportTime_ = cycleEndTime;
        }
        
        EventStop(EventType::MAIN_STEP);
        
        return RESULT_OK;

      } // Robot::step_MainExecution()

    } // namespace Robot
  } // namespace Cozmo
} // namespace Anki
