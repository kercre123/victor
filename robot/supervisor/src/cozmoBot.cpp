
#include "anki/cozmo/messageProtocol.h"

// TODO: move more of these include files to "src"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "dockingController.h"
#include "gripController.h"
#include "headController.h"
#include "liftController.h"
#include "testModeController.h"
#include "anki/cozmo/robot/commandHandler.h"
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

// Frame buffers, for now.
static const u32 FRAMEBUFFER_WIDTH  = 640;
static const u32 FRAMEBUFFER_HEIGHT = 480;
static const u32 FRAMEBUFFER_SIZE   = FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT;

#ifdef SIMULATOR
static u8 frameBuffer[FRAMEBUFFER_SIZE];
#else
#define DDR_BUFFER    __attribute__((section(".ddr.text")))
static DDR_BUFFER u8 frameBuffer[FRAME_SIZE];
#endif

namespace Anki {
  namespace Cozmo {
    namespace Robot {
      
      // "Private Member Variables"
      namespace {
        
        // Parameters / Constants:
        
        // TESTING
        const TestModeController::TestMode DEFAULT_TEST_MODE = TestModeController::TM_NONE;
        
        
        // Create Mailboxes for holding messages from the VisionSystem,
        // to be relayed up to the Basestation.
        VisionSystem::BlockMarkerMailbox blockMarkerMailbox_;
        VisionSystem::MatMarkerMailbox   matMarkerMailbox_;
        
        Robot::OperationMode mode_ = INITIALIZING;
        
        bool continuousCaptureStarted_ = false;
        
        bool isCarryingBlock_ = false;
        
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
        
        // TODO: Get VisionSystem to work on robot
#ifdef SIMULATOR
        if(VisionSystem::Init(HAL::GetHeadCamInfo(),
                              HAL::GetMatCamInfo(),
                              &blockMarkerMailbox_,
                              &matMarkerMailbox_) == EXIT_FAILURE)
        {
          PRINT("Vision System initialization failed.");
          return EXIT_FAILURE;
        }
#endif
        
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
        CozmoMsg_RobotAvailable msg;
        msg.size = sizeof(CozmoMsg_RobotAvailable);
        msg.msgID = MSG_V2B_CORE_ROBOT_AVAILABLE;
        msg.robotID = HAL::GetRobotID();
        HAL::RadioToBase(reinterpret_cast<u8 *>(&msg), msg.size);
        
        
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
        CommandHandler::ProcessIncomingMessages();
        
        // Check for any messages from the vision system and pass them along to
        // the basestation
        while( matMarkerMailbox_.hasMail() )
        {
          const CozmoMsg_ObservedMatMarker matMsg = matMarkerMailbox_.getMessage();
          HAL::RadioToBase((u8*)(&matMsg), sizeof(CozmoMsg_ObservedMatMarker));
        }
        
        while( blockMarkerMailbox_.hasMail() )
        {
          const CozmoMsg_ObservedBlockMarker blockMsg = blockMarkerMailbox_.getMessage();
          
          HAL::RadioToBase((u8*)(&blockMsg), sizeof(CozmoMsg_ObservedBlockMarker));
    
#if DOCKING_TEST
          if(isCarryingBlock_)
          {
            // We are already carrying a block.  If we see BlockType YY,
            // then switch (back) to docking mode and start tracking it so we can
            // put the one we're carrying on top of it
            if(blockMsg.blockType == 60) {
              mode_ = DOCK;
            }
          }
          else {
            // We aren't yet carrying a block.  If we see a BlockType XX,
            // switch to docking mode and start tracking it so we can go
            // pick it up.
            if(blockMsg.blockType == 65) {
              mode_ = DOCK;
            }
          } // if/else carrying block
#endif
        } // while blockMarkerMailbox has mail
        
        //////////////////////////////////////////////////////////////
        // Head & Lift Position Updates
        //////////////////////////////////////////////////////////////
        
        HeadController::Update();
        LiftController::Update();
        GripController::Update();
        
        
        PathFollower::Update();
        //DockingController::Update();
        
        //////////////////////////////////////////////////////////////
        // State Machine
        //////////////////////////////////////////////////////////////

        switch(mode_)
        {
          case INITIALIZING:
          {
            MotorCalibrationUpdate();
            break;
          }
            /*
          case FOLLOW_PATH:
          {
            PathFollower::Update();
            break;
          }
           */
          case DOCK:
          {
            DockingController::Update();
            
            if(DockingController::IsDone())
            {
              mode_ = WAITING;
              
              if(DockingController::DidSucceed())
              {
                // TODO: send a message to basestation that we are carrying a block?
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHT);
              }
              
            } // if head and left are in position
            
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
      
      
      // For the "long executation" thread, i.e. the vision code, which
      // will be slower
      ReturnCode step_LongExecution()
      {
        // Only QQQVGA can be captured in SINGLE mode.
        // Other modes must be captured in CONTINUOUS mode otherwise you get weird
        // rolling sync effects.
        // NB: CONTINUOUS mode contains tears that could affect vision algorithms
        // if moving too fast.
        // NOTE: we are currently always capturing at 640x480.  The camera mode
        //       is just affecting how much we downsample before sending the
        //       frame out over UART.
        if (HAL::GetHeadCamMode() != HAL::CAMERA_MODE_QQQVGA) {
          
          if (!continuousCaptureStarted_) {
            CameraStartFrame(HAL::CAMERA_FRONT, frameBuffer, HAL::CAMERA_MODE_VGA,
                             HAL::CAMERA_UPDATE_CONTINUOUS, 0, false);
            continuousCaptureStarted_ = true;
          }
          
        }
        else {
          CameraStartFrame(HAL::CAMERA_FRONT, frameBuffer, HAL::CAMERA_MODE_VGA,
                           HAL::CAMERA_UPDATE_SINGLE, 0, false);
          continuousCaptureStarted_ = false;
        }
        
        
        while (!HAL::CameraIsEndOfFrame(HAL::CAMERA_FRONT))
        {
        }
        
#if defined(SERIAL_IMAGING)
        HAL::USBSendFrame(frameBuffer, FRAMEBUFFER_HEIGHT, FRAMEBUFFER_WIDTH);
        HAL::USBSendMessage();
#endif
        
        switch(mode_)
        {
          case DOCK:
          {
            
            break;
          }
            
        } // SWITCH(mode_)
        
        // TODO: Get VisionSystem to work on robot
#ifdef SIMULATOR
        if(VisionSystem::lookForBlocks() == EXIT_FAILURE) {
          PRINT("VisionSystem::lookForBLocks() failed.\n");
          return EXIT_FAILURE;
        }
        
        if(VisionSystem::localizeWithMat() == EXIT_FAILURE) {
          PRINT("VisionSystem::localizeWithMat() failed.\n");
          return EXIT_FAILURE;
        }
#endif
        
        return EXIT_SUCCESS;
        
      } // Robot::step_longExecution()
      
    } // namespace Robot
  } // namespace Cozmo
} // namespace Anki
