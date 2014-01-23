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

#include "anki/messaging/robot/utilMessaging.h"

///////// TESTING //////////

#if ANKICORETECH_EMBEDDED_USE_MATLAB && USING_MATLAB_VISION
#include "anki/embeddedCommon/matlabConverters.h"
#endif

#define DOCKING_TEST 1

///////// END TESTING //////

namespace Anki {
  namespace Cozmo {
    namespace Robot {
      
      // "Private Member Variables"
      namespace {
        
        // Parameters / Constants:
        
#if(FREE_DRIVE_DUBINS_TEST)
        const TestModeController::TestMode DEFAULT_TEST_MODE = TestModeController::TM_NONE;
#else
        // TESTING
        // Change this value to run different test modes
        const TestModeController::TestMode DEFAULT_TEST_MODE = TestModeController::TM_NONE;
#endif
        Robot::OperationMode mode_ = INITIALIZING;
        
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
        GripController::DisengageGripper();
        SteeringController::ExecuteDirectDrive(0,0);

#ifdef SIMULATOR
        // Convenient for docking to set head angle at -15 degrees.
        // TODO: Move this somewhere else.
        HeadController::SetDesiredAngle(-0.26);
#endif
      }
      
      
      // The initial "stretch" and reset motor positions routine
      void MotorCalibrationUpdate()
      {
        if (LiftController::IsCalibrated()) {
          PRINT("Motors calibrated\n");
          mode_ = WAITING;
        }
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
      
        
        // Once initialization is done, broadcast a message that this robot
        // is ready to go
        PRINT("Robot broadcasting availability message.\n");
        Messages::RobotAvailable msg;
        msg.robotID = HAL::GetRobotID();
        HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::RobotAvailable), &msg);
        
        mode_ = INITIALIZING;

        
        return EXIT_SUCCESS;
        
      } // Robot::Init()
      
      
      void Destroy()
      {
        VisionSystem::Destroy();
        HAL::Destroy();
      }
      
      
      ReturnCode step_MainExecution()
      {


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

        // Process any messages from the basestation
        Messages::ProcessBTLEMessages();
#ifndef USE_OFFBOARD_VISION
        // UART messages are handled during longExecution() when using
        // offboard vision processing.
        Messages::ProcessUARTMessages();
#endif
        
        // Check for any messages from the vision system and pass them along to
        // the basestation, update the docking controller, etc.
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
                
        
        //////////////////////////////////////////////////////////////
        // Head & Lift Position Updates
        //////////////////////////////////////////////////////////////

        HeadController::Update();
        LiftController::Update();
        GripController::Update();
                
        PathFollower::Update();
        PickAndPlaceController::Update();
        DockingController::Update();
        
        //////////////////////////////////////////////////////////////
        // State Machine
        //////////////////////////////////////////////////////////////

        switch(mode_)
        {
          case INITIALIZING:
          {
            MotorCalibrationUpdate(); // switches mode_ to WAITING
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
        
        HAL::UpdateDisplay();
        
        
        return EXIT_SUCCESS;
        
      } // Robot::step_MainExecution()
      
      
      // For the "long execution" thread, i.e. the vision code, which
      // will be slower
      ReturnCode step_LongExecution()
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
        retVal = VisionSystem::Update();
        
        HAL::USBSendPrintBuffer();
        
        return retVal;
        
      } // Robot::step_longExecution()
      
    } // namespace Robot
  } // namespace Cozmo
} // namespace Anki
