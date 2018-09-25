#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/logEvent.h"
#include "anki/cozmo/robot/DAS.h"

#include "clad/types/motorTypes.h"
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
#include "powerModeManager.h"
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
  namespace Vector {
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
        u32 maxMainTooLongTime_usec_ = 0;
        u32 maxMainTooLateTime_usec_ = 0;
        u32 avgMainTooLongTime_usec_ = 0;
        u32 avgMainTooLateTime_usec_ = 0;
        u32 lastCycleStartTime_usec_ = 0;
        u32 nextMainCycleTimeErrorReportTime_usec_ = 0;
        const u32 MAIN_TOO_LATE_TIME_THRESH_USEC = ROBOT_TIME_STEP_MS * 1500;  // Normal cycle time plus 50% margin
        const u32 MAIN_TOO_LONG_TIME_THRESH_USEC = 2500;
        const u32 MAIN_CYCLE_ERROR_REPORTING_PERIOD_USEC = 1000000;

        // If there are more than this many TooLates in a reporting period
        // a warning is issued
        const u32 MIN_TOO_LATE_COUNT_PER_REPORTING_PERIOD = 5;

        // If a single tick is late by this amount in a reporting period
        // a warning is issued
        const u32 INSTANT_WARNING_TOO_LATE_TIME_THRESH_USEC = 15000;

        bool shutdownInProgress_ = false;
      } // Robot private namespace

      //
      // Methods:
      //
      Result Init(const int * shutdownSignal)
      {
        Result lastResult = RESULT_OK;

        // HAL and supervisor init
        lastResult = HAL::Init(shutdownSignal);
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

      void CheckForOverheatingBatteryShutdown()
      {
        if (!shutdownInProgress_ && HAL::BatteryIsOverheated()) {
          // Send a shutdown message to anim/engine
          AnkiInfo("CozmoBot.CheckForOverheatingBattery.Shutdown", "Sending PrepForShutdown");
          RobotInterface::PrepForShutdown msg;
          msg.reason = ShutdownReason::SHUTDOWN_BATTERY_CRITICAL_TEMP;
          RobotInterface::SendMessage(msg);

          shutdownInProgress_ = true;
        }
      }

      void CheckForCriticalBatteryShutdown()
      {        
        static const int   CRITICAL_BATTERY_THRESH_TICS = 20;
        static const float CRITICAL_BATTERY_THRESH_VOLTS = 3.45f;
        static const float HAL_SHUTDOWN_DELAY_MS = 2000;
        static TimeStamp_t shutdownTime_ms = 0;
        static int         numTicsBelowThresh = 0;

        const f32 battVoltage = HAL::BatteryGetVoltage();
        if (shutdownTime_ms == 0) {
          if (battVoltage < CRITICAL_BATTERY_THRESH_VOLTS) {
            ++numTicsBelowThresh;
            if (numTicsBelowThresh > CRITICAL_BATTERY_THRESH_TICS) {
              // Send a shutdown message to anim/engine
              AnkiInfo("CozmoBot.CheckForCriticalBattery.Shutdown", "Sending PrepForShutdown");
              RobotInterface::PrepForShutdown msg;
              msg.reason = ShutdownReason::SHUTDOWN_BATTERY_CRITICAL_VOLT;
              RobotInterface::SendMessage(msg);

              shutdownTime_ms = HAL::GetTimeStamp() + HAL_SHUTDOWN_DELAY_MS;
              shutdownInProgress_ = true;
            }
          } else {
            numTicsBelowThresh = 0;
          }
        } else if (HAL::GetTimeStamp() > shutdownTime_ms) {
          AnkiInfo("CozmoBot.CheckForCriticalBattery.HALShutdown","");
          HAL::Shutdown();
        }
      }

      void CheckForShutdown()
      {
        // Shutdown sequence will be begin if the button is held down for this
        // amount of time
        static const int SHUTDOWN_TIME_MS = 3000;

        // Wait this amount of time after sending PrepForShutdown message and calling
        // sync(). The assumption is that whoever is handling the PrepForShutdown message
        // is able to do so within this amount of time
        static const int TIME_TO_WAIT_UNTIL_SYNC_MS = 1350;

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
#if FACTORY_TEST
                // In factory, don't send PrepForShutdown because processes don't shutdown cleanly
                // and we get a fault code (i.e. 915 - NO_ENGINE) appear right before the power is pulled.
                // Instead just call sync and let syscon pull power.
                AnkiWarn("CozmoBot.CheckForButtonHeld.PossiblyShuttingDown", "Syncing");
                timeMark_ms = curTime_ms;
                sync();
#else
                timeMark_ms = curTime_ms;
                state = START;
                // Send a shutdown message to anim/engine
                AnkiInfo("CozmoBot.CheckForButtonHeld.Shutdown", "Sending PrepForShutdown");
                RobotInterface::PrepForShutdown msg;
                msg.reason = ShutdownReason::SHUTDOWN_BUTTON;
                RobotInterface::SendMessage(msg);
#endif                
                shutdownInProgress_ = true;
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
              AnkiInfo("CozmoBot.CheckForButtonHeld.Sync", "");
              #ifdef VICOS
              sync();
              #endif
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

      void CheckForGyroCalibShutdown()
      {
        static const int GYRO_NOT_CALIB_PREP_FOR_SHUTDOWN_MS = 57000;
        static const int GYRO_NOT_CALIB_SHUTDOWN_MS = 60000;
        
        static TimeStamp_t syncReceivedTime_ms = 0;

        enum ShutdownState
        {
          CHECK,
          PREP,
          SHUTDOWN,
          NONE,
        };
        static ShutdownState state = CHECK;

        switch(state) {
          case NONE:
            // Do nothing
            break;
          case CHECK:
          {
            // update syncReceivedTime once we receive sync
            if(Messages::ReceivedInit())
            {
              syncReceivedTime_ms = HAL::GetTimeStamp();
              state = PREP;
            }
            break; 
          }
          case PREP:
          {
            if(IMUFilter::IsBiasFilterComplete()) {
              state = NONE;
            } else {
              const TimeStamp_t timeDif_ms = HAL::GetTimeStamp() - syncReceivedTime_ms;
              if (timeDif_ms > GYRO_NOT_CALIB_PREP_FOR_SHUTDOWN_MS) {
                // Send a shutdown message to anim/engine
                AnkiInfo("CozmoBot.CheckForGyroCalibShutdown.Shutdown", "Sending PrepForShutdown");
                RobotInterface::PrepForShutdown msg;
                msg.reason = ShutdownReason::SHUTDOWN_GYRO_NOT_CALIBRATING;
                RobotInterface::SendMessage(msg);

                state = SHUTDOWN;
                shutdownInProgress_ = true;
              }
            }
            break;
          }
          case SHUTDOWN:
          {
            const TimeStamp_t timeDif_ms = HAL::GetTimeStamp() - syncReceivedTime_ms;
            if(timeDif_ms > GYRO_NOT_CALIB_SHUTDOWN_MS)
            {
              AnkiWarn("CozmoBot.CheckForGyroCalibShutdown.HALShutdown","");
              HAL::Shutdown();
            }
            break;
          }
        }
      }

      Result step_MainExecution()
      {
        EventStart(EventType::MAIN_STEP);

        START_TIME_PROFILE(CozmoBotMain, TOTAL);
        START_TIME_PROFILE(CozmoBot, HAL);

        // Detect if it took too long in between mainExecution calls
        u32 cycleStartTime = HAL::GetMicroCounter();
        if (lastCycleStartTime_usec_ != 0) {
          u32 timeBetweenCycles = cycleStartTime - lastCycleStartTime_usec_;
          if (timeBetweenCycles > MAIN_TOO_LATE_TIME_THRESH_USEC) {
            EventStart(EventType::MAIN_CYCLE_TOO_LATE);
            ++mainTooLateCnt_;
            EventStop(EventType::MAIN_CYCLE_TOO_LATE);
            avgMainTooLateTime_usec_ = (u32)((f32)(avgMainTooLateTime_usec_ * (mainTooLateCnt_ - 1) + timeBetweenCycles)) / mainTooLateCnt_;
            if (maxMainTooLateTime_usec_ < timeBetweenCycles) {
              maxMainTooLateTime_usec_ = timeBetweenCycles;
            }
          }
        }

        // Only do these checks if communicating with engine
        // or a shutdown is already in progress
        if (Messages::ReceivedInit() || shutdownInProgress_) {
          CheckForShutdown();
          CheckForCriticalBatteryShutdown();
          CheckForGyroCalibShutdown();
          CheckForOverheatingBatteryShutdown();
        }
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
          AnkiInfo("CozmoBot.Radio.Connected", "");
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
        // Power management
        //////////////////////////////////////////////////////////////
        PowerModeManager::Update();

        //////////////////////////////////////////////////////////////
        // Feedback / Display
        //////////////////////////////////////////////////////////////

        Messages::UpdateRobotStateMsg();
        ++robotStateMessageCounter_;
        const s32 messagePeriod = ( HAL::PowerGetMode() == HAL::POWER_MODE_CALM ) ? STATE_MESSAGE_FREQUENCY_CALM
                                                                                  : STATE_MESSAGE_FREQUENCY;
        if(robotStateMessageCounter_ >= messagePeriod) {
          Messages::SendRobotStateMsg();
          robotStateMessageCounter_ = 0;
        }

        // Now that the robot state msg has been updated, send mic data (which uses some of robot state)
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
          avgMainTooLongTime_usec_ = (u32)((f32)(avgMainTooLongTime_usec_ * (mainTooLongCnt_ - 1) + cycleTime)) / mainTooLongCnt_;
          if (maxMainTooLongTime_usec_ < cycleTime) {
            maxMainTooLongTime_usec_ = cycleTime;
          }
        }
        lastCycleStartTime_usec_ = cycleStartTime;          

        // Report main cycle time error
        if (nextMainCycleTimeErrorReportTime_usec_ > cycleEndTime) {

          const bool reportTooLate = (mainTooLateCnt_ > MIN_TOO_LATE_COUNT_PER_REPORTING_PERIOD) || 
                                     (maxMainTooLateTime_usec_ > INSTANT_WARNING_TOO_LATE_TIME_THRESH_USEC);
          const bool reportTooLong = (mainTooLongCnt_ > 0);
          if (reportTooLate || reportTooLong) {
            AnkiWarn( "CozmoBot.MainCycleTimeError", 
                      "TooLate: %d tics, avg: %d us, max: %d us, TooLong: %d tics, avg: %d us, max: %d us",
                      mainTooLateCnt_, 
                      avgMainTooLateTime_usec_, 
                      maxMainTooLateTime_usec_, 
                      mainTooLongCnt_, 
                      avgMainTooLongTime_usec_, 
                      maxMainTooLongTime_usec_);

            if (reportTooLate) {
              DASMSG(vectorbot_main_cycle_too_late,    "vectorbot.main_cycle_too_late", "Report upto once/sec when robot tick is too late");
              DASMSG_SET(i1, mainTooLateCnt_,          "Number of ticks that exceed MAIN_TOO_LATE_TIME_THRESH_USEC");
              DASMSG_SET(i2, avgMainTooLateTime_usec_, "Of the late ticks, average time since start of previous tick (usec)");
              DASMSG_SET(i3, maxMainTooLateTime_usec_, "Of the late ticks, maximum time since start of previous tick (usec)");
              DASMSG_SEND();
            }
            if (reportTooLong) {
              DASMSG(vectorbot_main_cycle_too_long,    "vectorbot.main_cycle_too_long", "Report upto once/sec when robot tick is too long");
              DASMSG_SET(i1, mainTooLongCnt_,          "Number of ticks that exceed MAIN_TOO_LONG_TIME_THRESH_USEC");
              DASMSG_SET(i2, avgMainTooLongTime_usec_, "Average duration of too long ticks (usec)");
              DASMSG_SET(i3, maxMainTooLongTime_usec_, "Maximum duration of too long ticks (usec)");
              DASMSG_SEND();
            }

          }

          mainTooLateCnt_ = 0;
          avgMainTooLateTime_usec_ = 0;
          maxMainTooLateTime_usec_ = 0;
          mainTooLongCnt_ = 0;
          avgMainTooLongTime_usec_ = 0;
          maxMainTooLongTime_usec_ = 0;

          nextMainCycleTimeErrorReportTime_usec_ += MAIN_CYCLE_ERROR_REPORTING_PERIOD_USEC;
        }

        EventStop(EventType::MAIN_STEP);

        return RESULT_OK;

      } // Robot::step_MainExecution()

    } // namespace Robot
  } // namespace Vector
} // namespace Anki
