#include "anki/common/robot/config.h"
#include "anki/cozmo/robot/messages.h"

// TODO: move more of these include files to "src"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "gripController.h"
#include "headController.h"
#include "liftController.h"
#include "testModeController.h"
#include "anki/cozmo/robot/debug.h"
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/speedController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/wheelController.h"
#include "anki/cozmo/robot/visionSystem.h"

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
        const TestModeController::TestMode DEFAULT_TEST_MODE = TestModeController::TM_PICK_AND_PLACE;

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
        //HeadController::StartCalibrationRoutine();
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
           //&& HeadController::IsCalibrated()
           ) {
          PRINT("Motors calibrated\n");
          isDone = true;
        }
        
        return isDone;
      }
      
      
      ReturnCode Init(void)
      {
        if(HAL::Init() == EXIT_FAILURE) {
          PRINT("Hardware Interface initialization failed!\n");
          return EXIT_FAILURE;
        }
        
        if (Localization::Init() == EXIT_FAILURE) {
          PRINT("Localization System init failed.\n");
          return EXIT_FAILURE;
        }
        
        // TODO: Get VisionSystem to work on robot
        if(VisionSystem::Init() == EXIT_FAILURE)
        {
          PRINT("Vision System initialization failed.\n");
          return EXIT_FAILURE;
        }
        
        if(PathFollower::Init() == EXIT_FAILURE) {
          PRINT("PathFollower initialization failed.\n");
          return EXIT_FAILURE;
        }
        
        // Initialize subsystems if/when available:
        /*
         if(WheelController::Init() == EXIT_FAILURE) {
         PRINT("WheelController initialization failed.\n");
         return EXIT_FAILURE;
         }
         
         if(SpeedController::Init() == EXIT_FAILURE) {
         PRINT("SpeedController initialization failed.\n");
         return EXIT_FAILURE;
         }
         
         if(SteeringController::Init() == EXIT_FAILURE) {
         PRINT("SteeringController initialization failed.\n");
         return EXIT_FAILURE;
         }
         
         if(HeadController::Init() == EXIT_FAILURE) {
         PRINT("HeadController initialization failed.\n");
         return EXIT_FAILURE;
         }
         */
        
         if(LiftController::Init() == EXIT_FAILURE) {
         PRINT("LiftController initialization failed.\n");
         return EXIT_FAILURE;
         }
         
        // Setup test mode
        if(TestModeController::Init(DEFAULT_TEST_MODE) == EXIT_FAILURE) {
          PRINT("TestMode initialization failed.\n");
          return EXIT_FAILURE;
        }
        
        // Start calibration
        StartMotorCalibrationRoutine();

        // Set starting state
        mode_ = INIT_MOTOR_CALIBRATION;
        
        return EXIT_SUCCESS;
        
      } // Robot::Init()
      
      
      void Destroy()
      {
        VisionSystem::Destroy();
        HAL::Destroy();
      }
      
      
      ReturnCode step_MainExecution()
      {
#ifdef THIS_IS_PETES_BOARD      
        return EXIT_SUCCESS;
#endif

//#if(DEBUG_ANY && defined(SIMULATOR))
//        PRINT("\n==== FRAME START (time = %d us) ====\n", HAL::GetMicroCounter() );
//#endif
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
#ifndef USE_OFFBOARD_VISION
        // UART messages are handled during longExecution() when using
        // offboard vision processing.
        Messages::ProcessUARTMessages();
#endif
        
        // Check for any messages from the vision system and pass them along to
        // the basestation, update the docking controller, etc.
        Messages::VisionMarker markerMsg;
        while( Messages::CheckMailbox(markerMsg) )
        {
          HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::VisionMarker), &markerMsg);
        }
        
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
        
        Messages::SendRobotStateMsg();
        
        HAL::UpdateDisplay();
        
        
        return EXIT_SUCCESS;
        
      } // Robot::step_MainExecution()
      
      
      // For the "long execution" thread, i.e. the vision code, which
      // will be slower
      ReturnCode step_LongExecution()
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
        // IMPORTANT: The static robot state message is being passed in here
        //   *by value*, NOT by reference.  This is because step_LongExecution()
        //   can be interuppted by step_MainExecution().
        retVal = VisionSystem::Update(Messages::GetRobotStateMsg());
        
        HAL::USBSendPrintBuffer();
        
        return retVal;
        
      } // Robot::step_longExecution()
      
    } // namespace Robot
  } // namespace Cozmo
} // namespace Anki
