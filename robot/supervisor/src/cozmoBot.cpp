#include "anki/common/robot/config.h"
#include "anki/common/shared/utilities_shared.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "anki/cozmo/robot/debug.h"
#include "messages.h"
#include "faceTrackingController.h"
#include "imuFilter.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "gripController.h"
#include "headController.h"
#include "liftController.h"
#include "testModeController.h"
#include "localization.h"
#include "pathFollower.h"
#include "speedController.h"
#include "steeringController.h"
#include "wheelController.h"
#include "visionSystem.h"
#include "animationController.h"
#include "proxSensors.h"

#include "anki/messaging/shared/utilMessaging.h"

///////// TESTING //////////

#if ANKICORETECH_EMBEDDED_USE_MATLAB && USING_MATLAB_VISION
#include "anki/embeddedCommon/matlabConverters.h"
#endif

///////// END TESTING //////

namespace Anki {
  namespace Cozmo {
    namespace Robot {
      
      // "Private Member Variables"
      namespace {
        
        // Parameters / Constants:
        
        // TESTING
        // Change this value to run different test modes
        const TestMode DEFAULT_TEST_MODE = TM_NONE;

        Robot::OperationMode mode_ = INIT_MOTOR_CALIBRATION;
        bool wasConnected_ = false;
        
        bool wasPickedUp_ = false;
        
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
        const u32 MAIN_TOO_LATE_TIME_THRESH = TIME_STEP * 1000 + 500;  // Normal cycle time plus some margin
        const u32 MAIN_TOO_LONG_TIME_THRESH = TIME_STEP * 1000 + 500;
        const u32 MAIN_CYCLE_ERROR_REPORTING_PERIOD = 1000000;

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
#if defined(HAVE_ACTIVE_GRIPPER) && HAVE_ACTIVE_GRIPPER
        GripController::DisengageGripper();
#endif
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

        lastResult = Localization::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "Localization System init failed.\n");
        
        // TODO: Get VisionSystem to work on robot
        lastResult = VisionSystem::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "Vision System init failed.\n");

        
        lastResult = PathFollower::Init();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "PathFollower System init failed.\n");

        
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

        lastResult = FaceTrackingController::Init(VisionSystem::GetFaceDetectionParams());
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "Robot::Init()", "FaceTrackingController init failed.\n");

        
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
        // Detect if it took too long in between mainExecution calls
        u32 cycleStartTime = HAL::GetMicroCounter();
        if (lastCycleStartTime_ != 0) {
          u32 timeBetweenCycles = cycleStartTime - lastCycleStartTime_;
          if (timeBetweenCycles > MAIN_TOO_LATE_TIME_THRESH) {
            ++mainTooLateCnt_;
            avgMainTooLateTime_ = (u32)((f32)(avgMainTooLateTime_ * (mainTooLateCnt_ - 1) + timeBetweenCycles)) / mainTooLateCnt_;
          }
        }
        
        // HACK: Manually setting timestamp here in mainExecution until
        // until Nathan implements this the correct way.
        HAL::SetTimeStamp(HAL::GetTimeStamp()+TIME_STEP);

// TBD - This should be moved to simulator just before step_MainExecution is called
#ifndef ROBOT_HARDWARE
        // If the hardware interface needs to be advanced (as in the case of
        // a Webots simulation), do that first.
        HAL::Step();
#endif
        
        //////////////////////////////////////////////////////////////
        // Test Mode
        //////////////////////////////////////////////////////////////
        TestModeController::Update();
        
        
        //////////////////////////////////////////////////////////////
        // Localization
        //////////////////////////////////////////////////////////////
        Localization::Update();
        
        
        //////////////////////////////////////////////////////////////
        // Communications
        //////////////////////////////////////////////////////////////

        // Check if there is a new or dropped connection to a basestation
        if (HAL::RadioIsConnected() && !wasConnected_) {
          PRINT("Robot radio is connected.\n");
          wasConnected_ = true;
        } else if (!HAL::RadioIsConnected() && wasConnected_) {
          PRINT("Radio disconnected\n");
          Messages::ResetInit();
          wasConnected_ = false;
        }

        // Process any messages from the basestation
        Messages::ProcessBTLEMessages();
        
        /*
        Messages::MatMarkerObserved matMsg;
        while( Messages::CheckMailbox(matMsg) )
        {
          HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::MatMarkerObserved), &matMsg);
        }
        
        Messages::BlockMarkerObserved blockMsg;
        while( Messages::CheckMailbox(blockMsg) )
        {
          HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::BlockMarkerObserved), &blockMsg);
          
        } // while blockMarkerMailbox has mail
        */      

        //////////////////////////////////////////////////////////////
        // Sensor updates
        //////////////////////////////////////////////////////////////
#if !defined(THIS_IS_PETES_BOARD)
        IMUFilter::Update();
#endif        
        
        ProxSensors::Update();
        
        //////////////////////////////////////////////////////////////
        // Head & Lift Position Updates
        //////////////////////////////////////////////////////////////

        AnimationController::Update();
        
        HeadController::Update();
        LiftController::Update();
#if defined(HAVE_ACTIVE_GRIPPER) && HAVE_ACTIVE_GRIPPER
        GripController::Update();
#endif
        
        PathFollower::Update();
        PickAndPlaceController::Update();
        DockingController::Update();
        FaceTrackingController::Update();
        
        //////////////////////////////////////////////////////////////
        // State Machine
        //////////////////////////////////////////////////////////////

        switch(mode_)
        {
          case INIT_MOTOR_CALIBRATION:
          {
            if(MotorCalibrationUpdate()) {
              // Once initialization is done, broadcast a message that this robot
              // is ready to go
              Messages::RobotAvailable msg;
              PRINT("Robot %d broadcasting availability message.\n", msg.robotID);
              HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::RobotAvailable), &msg);
         
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
        
        
        //////////////////////////////////////////////////////////////
        // Pickup reaction
        //////////////////////////////////////////////////////////////
        if (IMUFilter::IsPickedUp() && !wasPickedUp_) {
          // Stop wheels
          PathFollower::ClearPath();
          SteeringController::ExecuteDirectDrive(0, 0);
          SpeedController::SetBothDesiredAndCurrentUserSpeed(0);
        }
        wasPickedUp_ = IMUFilter::IsPickedUp();

        
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
        
        
        // Check if main took too long
        u32 cycleEndTime = HAL::GetMicroCounter();
        u32 cycleTime = cycleEndTime - cycleStartTime;
        if (cycleTime > MAIN_TOO_LONG_TIME_THRESH) {
          ++mainTooLongCnt_;
          avgMainTooLongTime_ = (u32)((f32)(avgMainTooLongTime_ * (mainTooLongCnt_ - 1) + cycleTime)) / mainTooLongCnt_;
        }
        lastCycleStartTime_ = cycleStartTime;
        
        
        // Report main cycle time error
        if ((mainTooLateCnt_ > 0 || mainTooLongCnt_ > 0) &&
            (cycleEndTime - lastMainCycleTimeErrorReportTime_ > MAIN_CYCLE_ERROR_REPORTING_PERIOD)) {
          Messages::MainCycleTimeError m;
          m.numMainTooLateErrors = mainTooLateCnt_;
          m.avgMainTooLateTime = avgMainTooLateTime_;
          m.numMainTooLongErrors = mainTooLongCnt_;
          m.avgMainTooLongTime = avgMainTooLongTime_;
          
          HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::MainCycleTimeError), &m);
          
          mainTooLateCnt_ = 0;
          avgMainTooLateTime_ = 0;
          mainTooLongCnt_ = 0;
          avgMainTooLongTime_ = 0;
          
          lastMainCycleTimeErrorReportTime_ = cycleEndTime;
        }
        
        
        return RESULT_OK;
        
      } // Robot::step_MainExecution()
      
      
      // For the "long execution" thread, i.e. the vision code, which
      // will be slower
      Result step_LongExecution()
      {
        Result retVal = RESULT_OK;
        
        // IMPORTANT: The static robot state message is being passed in here
        //   *by value*, NOT by reference.  This is because step_LongExecution()
        //   can be interupted by step_MainExecution().
        retVal = VisionSystem::Update(Messages::GetRobotStateMsg());
        
        return retVal;
        
      } // Robot::step_longExecution()
      
      
    } // namespace Robot
  } // namespace Cozmo
} // namespace Anki
