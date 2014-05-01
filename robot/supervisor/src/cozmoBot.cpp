#include "anki/common/robot/config.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "anki/cozmo/robot/debug.h"
#include "messages.h"
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
        if(HAL::Init() == RESULT_FAIL) {
          PRINT("Hardware Interface initialization failed!\n");
          return RESULT_FAIL;
        }
        
        if (Localization::Init() == RESULT_FAIL) {
          PRINT("Localization System init failed.\n");
          return RESULT_FAIL;
        }
        
        // TODO: Get VisionSystem to work on robot
        if(VisionSystem::Init() == RESULT_FAIL)
        {
          PRINT("Vision System initialization failed.\n");
          return RESULT_FAIL;
        }
        
        if(PathFollower::Init() == RESULT_FAIL) {
          PRINT("PathFollower initialization failed.\n");
          return RESULT_FAIL;
        }
        
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
        
         if(LiftController::Init() == RESULT_FAIL) {
         PRINT("LiftController initialization failed.\n");
         return RESULT_FAIL;
         }
        
        // Start calibration
        StartMotorCalibrationRoutine();

        // Set starting state
        mode_ = INIT_MOTOR_CALIBRATION;
        
        return RESULT_OK;
        
      } // Robot::Init()
      
      
      void Destroy()
      {
        HAL::Destroy();
      }
      
      
      Result step_MainExecution()
      {

        // If the hardware interface needs to be advanced (as in the case of
        // a Webots simulation), do that first.
        HAL::Step();

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
          PRINT("Robot %d's radio is connected.\n", HAL::GetRobotID());
          wasConnected_ = true;
        } else if (!HAL::RadioIsConnected() && wasConnected_) {
          PRINT("Radio disconnected\n");
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
        IMUFilter::Update();
        
        
        //////////////////////////////////////////////////////////////
        // Head & Lift Position Updates
        //////////////////////////////////////////////////////////////

        HeadController::Update();
        LiftController::Update();
#if defined(HAVE_ACTIVE_GRIPPER) && HAVE_ACTIVE_GRIPPER
        GripController::Update();
#endif
        
        PathFollower::Update();
        PickAndPlaceController::Update();
        DockingController::Update();
        
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
              msg.robotID = HAL::GetRobotID();
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
        // Feedback / Display
        //////////////////////////////////////////////////////////////
        
        Messages::UpdateRobotStateMsg();
#if(!STREAM_DEBUG_IMAGES)
        Messages::SendRobotStateMsg();
#endif
        
        HAL::UpdateDisplay();
        
        
        return RESULT_OK;
        
      } // Robot::step_MainExecution()
      
      
      // For the "long execution" thread, i.e. the vision code, which
      // will be slower
      Result step_LongExecution()
      {
        Result retVal = RESULT_OK;
        
        // IMPORTANT: The static robot state message is being passed in here
        //   *by value*, NOT by reference.  This is because step_LongExecution()
        //   can be interuppted by step_MainExecution().
        retVal = VisionSystem::Update(Messages::GetRobotStateMsg());
        
        return retVal;
        
      } // Robot::step_longExecution()
      
      
    } // namespace Robot
  } // namespace Cozmo
} // namespace Anki
