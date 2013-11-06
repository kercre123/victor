
#include "anki/cozmo/messageProtocol.h"

// TODO: move more of these include files to "src"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"
#include "anki/cozmo/robot/commandHandler.h"
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/vehicleMath.h"
#include "anki/cozmo/robot/speedController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/wheelController.h"
#include "anki/cozmo/robot/visionSystem.h"

#include "anki/messaging/robot/utilMessaging.h"

//#include "cozmo_physics.h"

#include <cmath>
#include <cstdio>
#include <string>

///////// TESTING //////////
#define EXECUTE_TEST_PATH 0
#define USE_OVERLAY_DISPLAY 1


#if(USE_OVERLAY_DISPLAY)
#include "sim_overlayDisplay.h"
#endif

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
        
        // Create Mailboxes for holding messages from the VisionSystem,
        // to be relayed up to the Basestation.
        VisionSystem::BlockMarkerMailbox blockMarkerMailbox_;
        VisionSystem::MatMarkerMailbox   matMarkerMailbox_;
        
        Robot::OperationMode mode_ = INITIALIZING, nextMode_ = WAITING;
        
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
      
      ReturnCode Init(void)
      {
        if(HAL::Init() == EXIT_FAILURE) {
          PRINT("Hardware Interface initialization failed!\n");
          return EXIT_FAILURE;
        }
        
        if(VisionSystem::Init(HAL::GetHeadFrameGrabber(),
                              HAL::GetMatFrameGrabber(),
                              HAL::GetHeadCamInfo(),
                              HAL::GetMatCamInfo(),
                              &blockMarkerMailbox_,
                              &matMarkerMailbox_) == EXIT_FAILURE)
        {
          PRINT("Vision System initialization failed.");
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
         
         if(LiftController::Init() == EXIT_FAILURE) {
         PRINT("LiftController initialization failed.\n");
         return EXIT_FAILURE;
         }
         
         */
        
        // Lower the lift
        LiftController::SetDesiredHeight(LIFT_HEIGHT_LOW);
        
        // Once initialization is done, broadcast a message that this robot
        // is ready to go
        PRINT("Robot broadcasting availability message.\n");
        CozmoMsg_RobotAvailable msg;
        msg.size = sizeof(CozmoMsg_RobotAvailable);
        msg.msgID = MSG_V2B_CORE_ROBOT_AVAILABLE;
        msg.robotID = HAL::GetRobotID();
        HAL::SendMessage(reinterpret_cast<u8 *>(&msg), msg.size);
        
        mode_ = WAITING;
        
        return EXIT_SUCCESS;
        
      } // Robot::Init()
      
      
      void Destroy()
      {
        VisionSystem::Destroy();
        HAL::Destroy();
      }
      
      
      ReturnCode step_MainExecution()
      {
        // If the hardware interface needs to be advanced (as in the case of
        // a Webots simulation), do that first.
        HAL::Step();
        
        //////////////////////////////////////////////////////////////
        // Localization
        //////////////////////////////////////////////////////////////
        Localization::Update();
        
        
        //////////////////////////////////////////////////////////////
        // Communications
        //////////////////////////////////////////////////////////////
        
        // Buffer any incoming data from basestation
        HAL::ManageRecvBuffer();
        
        // Process any messages from the basestation
        CommandHandler::ProcessIncomingMessages();
        
        // Check for any messages from the vision system and pass them along to
        // the basestation
        while( matMarkerMailbox_.hasMail() )
        {
          const CozmoMsg_ObservedMatMarker matMsg = matMarkerMailbox_.getMessage();
          HAL::SendMessage(&matMsg, sizeof(CozmoMsg_ObservedMatMarker));
        }
        
        while( blockMarkerMailbox_.hasMail() )
        {
          const CozmoMsg_ObservedBlockMarker blockMsg = blockMarkerMailbox_.getMessage();
          HAL::SendMessage(&blockMsg, sizeof(CozmoMsg_ObservedBlockMarker));
        }
        
        //////////////////////////////////////////////////////////////
        // Head & Lift Position Updates
        //////////////////////////////////////////////////////////////
        
        HeadController::Update();
        LiftController::Update();
        
        //////////////////////////////////////////////////////////////
        // Path Following
        //////////////////////////////////////////////////////////////
        
#if(EXECUTE_TEST_PATH)
        // TESTING
        const u32 startDriveTime_us = 500000;
        static bool testPathStarted = false;
        if (not PathFollower::IsTraversingPath() &&
            HAL::GetMicroCounter() > startDriveTime_us &&
            !testPathStarted) {
          SpeedController::SetUserCommandedAcceleration( MAX(ONE_OVER_CONTROL_DT + 1, 500) );  // This can't be smaller than 1/CONTROL_DT!
          SpeedController::SetUserCommandedDesiredVehicleSpeed(160);
          PRINT("Speed commanded: %d mm/s\n",
                  SpeedController::GetUserCommandedDesiredVehicleSpeed() );
          
          // Create a path and follow it
          PathFollower::AppendPathSegment_Line(0, 0.0, 0.0, 0.3, -0.3);
          float arc1_radius = sqrt(0.005);  // Radius of sqrt(0.05^2 + 0.05^2)
          PathFollower::AppendPathSegment_Arc(0, 0.35, -0.25, arc1_radius, -0.75*PI, 0);
          PathFollower::AppendPathSegment_Line(0, 0.35 + arc1_radius, -0.25, 0.35 + arc1_radius, 0.2);
          float arc2_radius = sqrt(0.02); // Radius of sqrt(0.1^2 + 0.1^2)
          PathFollower::AppendPathSegment_Arc(0, 0.35 + arc1_radius - arc2_radius, 0.2, arc2_radius, 0, PIDIV2);
          PathFollower::StartPathTraversal();
          testPathStarted = true;
        }
#endif //EXECUTE_TEST_PATH
        
        //////////////////////////////////////////////////////////////
        // State Machine
        //////////////////////////////////////////////////////////////

        switch(mode_)
        {
          case INITIALIZING:
          {
            PRINT("Robot still initializing.\n");
            break;
          }
            
          case FOLLOW_PATH:
          {
            if(PathFollower::IsTraversingPath())
            {
              PathFollower::Update();
            }
            break;
          }
            
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
        
        if(VisionSystem::lookForBlocks() == EXIT_FAILURE) {
          fprintf(stdout, "VisionSystem::lookForBLocks() failed.\n");
          return EXIT_FAILURE;
        }
        
        if(VisionSystem::localizeWithMat() == EXIT_FAILURE) {
          fprintf(stdout, "VisionSystem::localizeWithMat() failed.\n");
          return EXIT_FAILURE;
        }
        
        
        return EXIT_SUCCESS;
        
      } // Robot::step_longExecution()
      
      
      void SetOpenLoopMotorSpeed(s16 speedl, s16 speedr)
      {
        // Convert PWM to rad/s
        // TODO: Do this properly.  For now assume MOTOR_PWM_MAXVAL achieves 1m/s lateral speed.
        
        // "FACTOR" is the converstion factor for computing radians/second
        // from commanded speed: 2*pi * (1 meter / wheel_circumference)
        const float FACTOR = ((2.f*M_PI) *
                              (1000.f / (Cozmo::WHEEL_DIAMETER_MM*M_PI)));
        f32 left_rad_per_s  = speedl * FACTOR / HAL::MOTOR_PWM_MAXVAL;
        f32 right_rad_per_s = speedr * FACTOR / HAL::MOTOR_PWM_MAXVAL;
        
        HAL::SetWheelAngularVelocity(left_rad_per_s, right_rad_per_s);
        
      } // Robot::SetOpenLoopMotorSpeed()
      
    } // namespace Robot
  } // namespace Cozmo
} // namespace Anki
