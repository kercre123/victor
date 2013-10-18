
#include "anki/cozmo/MessageProtocol.h"

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
//#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/mainExecution.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/vehicleMath.h"
#include "anki/cozmo/robot/speedController.h"

#include "anki/messaging/robot/utilMessaging.h"

//#include "keyboardController.h"
#include "cozmo_physics.h"

#include <cmath>
#include <cstdio>
#include <string>

#include <webots/Display.hpp>

#if ANKICORETECH_EMBEDDED_USE_MATLAB && USING_MATLAB_VISION
#include "anki/embeddedCommon/matlabConverters.h"
#endif

///////// TESTING //////////
#define EXECUTE_TEST_PATH 1

///////// END TESTING //////

//Names of the wheels used for steering
#define WHEEL_FL "wheel_fl"
#define WHEEL_FR "wheel_fr"
#define GYRO_FL "wheel_gyro_fl"
#define GYRO_FR "wheel_gyro_fr"
#define PITCH "motor_head_pitch"
#define LIFT "lift_motor"
#define LIFT2 "lift_motor2"
#define CONNECTOR "connector"
#define PLUGIN_COMMS "cozmo_physics_comms"
#define RX "radio_rx"
#define TX "radio_tx"

#define DOWN_CAMERA "cam_down"
#define HEAD_CAMERA "cam_head"
#define LIFT_CAMERA "cam_lift"

#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/wheelController.h"
#include "anki/cozmo/robot/visionSystem.h"

namespace Anki {
  
  namespace Cozmo {
    
    // "Private Member Variables"
    namespace {
      
      // Parameters / Constants:

      
      // Create Mailboxes for holding messages from the VisionSystem,
      // to be relayed up to the Basestation.
      VisionSystem::BlockMarkerMailbox blockMarkerMailbox_;
      VisionSystem::MatMarkerMailbox   matMarkerMailbox_;
      
      Robot::OperationMode mode_, nextMode_;
      
      // Localization:
      f32 currentMatX_, currentMatY_;
      Radians currentMatHeading_;
      
    } // Robot private namespace
    
    
    //
    // Accessors:
    //
    Robot::OperationMode Robot::GetOperationMode()
    { return mode_; }
    
    void Robot::SetOperationMode(Robot::OperationMode newMode)
    { mode_ = newMode; }
        
    //
    // Methods:
    //

    ReturnCode Robot::Init(void)
    {
      if(HAL::Init() == EXIT_FAILURE) {
        fprintf(stdout, "Hardware Interface initialization failed!\n");
        return EXIT_FAILURE;
      }
      
      if(VisionSystem::Init(HAL::GetHeadFrameGrabber(),
                            HAL::GetMatFrameGrabber(),
                            HAL::GetHeadCamInfo(),
                            HAL::GetMatCamInfo(),
                            &blockMarkerMailbox_,
                            &matMarkerMailbox_) == EXIT_FAILURE)
      {
        fprintf(stdout, "Vision System initialization failed.");
        return EXIT_FAILURE;
      }
      
      if(PathFollower::Init() == EXIT_FAILURE) {
        fprintf(stdout, "PathFollower initialization failed.\n");
        return EXIT_FAILURE;
      }
      
      // Initialize subsystems if/when available:
      /*
      if(WheelController::Init() == EXIT_FAILURE) {
        fprintf(stdout, "WheelController initialization failed.\n");
        return EXIT_FAILURE;
      }
      
      if(SpeedController::Init() == EXIT_FAILURE) {
        fprintf(stdout, "SpeedController initialization failed.\n");
        return EXIT_FAILURE;
      }
      
      if(SteeringController::Init() == EXIT_FAILURE) {
        fprintf(stdout, "SteeringController initialization failed.\n");
        return EXIT_FAILURE;
      }
       */
      
      return EXIT_SUCCESS;
      
    } // Robot::Init()
    
    void Robot::Destroy()
    {
      VisionSystem::Destroy();
      HAL::Destroy();
    }
    
    
    
  
    
    ReturnCode Robot::step_MainExecution()
    {
      // If the hardware interface needs to be advanced (as in the case of
      // a Webots simulation), do that first.
      HAL::Step();
      
      //////////////////////////////////////////////////////////////
      // Communications
      //////////////////////////////////////////////////////////////
      
      // Buffer any incoming data from basestation
      HAL::ManageRecvBuffer();
      
      // Process any messages from the basestation
      u8 msgBuffer[255];
      u8 msgSize;
      while( (msgSize = HAL::RecvMessage(msgBuffer)) > 0 )
      {
        CozmoMsg_Command cmd = static_cast<CozmoMsg_Command>(msgBuffer[1]);
        switch(cmd)
        {
          case MSG_B2V_CORE_ABS_LOCALIZATION_UPDATE:
          {
            // TODO: Double-check that size matches expected size?
            
            const CozmoMsg_AbsLocalizationUpdate *msg = reinterpret_cast<const CozmoMsg_AbsLocalizationUpdate*>(msgBuffer);
            
            currentMatX_       = msg->xPosition;
            currentMatY_       = msg->yPosition;
            currentMatHeading_ = msg->headingAngle;
            
            fprintf(stdout, "Robot received localization update from "
                    "basestation: (%.1f,%.1f) at %.1f degrees\n",
                    currentMatX_, currentMatY_,
                    currentMatHeading_.getDegrees());
            
            break;
          }
          default:
            fprintf(stdout, "Unrecognized command in received message.\n");
            
        } // switch(cmd)
      }
      
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
      // Path Following
      //////////////////////////////////////////////////////////////
      
#if(EXECUTE_TEST_PATH)
      // TESTING
      const u32 startDriveTime_us = 500000;
      if (not PathFollower::IsTraversingPath() &&
          HAL::GetMicroCounter() > startDriveTime_us) {
        SpeedController::SetUserCommandedAcceleration( MAX(ONE_OVER_CONTROL_DT + 1, 500) );  // This can't be smaller than 1/CONTROL_DT!
        SpeedController::SetUserCommandedDesiredVehicleSpeed(160);
        fprintf(stdout, "Speed commanded: %d mm/s\n",
                SpeedController::GetUserCommandedDesiredVehicleSpeed() );
        
        // Create a path and follow it
        PathFollower::AppendPathSegment_Line(0, 0.0, 0.0, 0.3, -0.3);
        float arc1_radius = sqrt(0.005);  // Radius of sqrt(0.05^2 + 0.05^2)
        PathFollower::AppendPathSegment_Arc(0, 0.35, -0.25, arc1_radius, -0.75*PI, 0);
        PathFollower::AppendPathSegment_Line(0, 0.35 + arc1_radius, -0.25, 0.35 + arc1_radius, 0.2);
        float arc2_radius = sqrt(0.02); // Radius of sqrt(0.1^2 + 0.1^2)
        PathFollower::AppendPathSegment_Arc(0, 0.35 + arc1_radius - arc2_radius, 0.2, arc2_radius, 0, PIDIV2);
        PathFollower::StartPathTraversal();
      }
#endif //EXECUTE_TEST_PATH
      
      //////////////////////////////////////////////////////////////
      // Motion Control (Path following / Docking)
      //////////////////////////////////////////////////////////////
      if(PathFollower::IsTraversingPath())
      {
        PathFollower::Update();
      }
      
      //////////////////////////////////////////////////////////////
      // Gripper
      //////////////////////////////////////////////////////////////
      
      // Check if connector attaches to anything
      HAL::ManageGripper();

      
      //////////////////////////////////////////////////////////////
      // Feedback / Display
      //////////////////////////////////////////////////////////////
      
      HAL::UpdateDisplay();
      
      
      return EXIT_SUCCESS;

    } // Robot::step_MainExecution()
    
    
    ReturnCode Robot::step_LongExecution()
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
      
    } // Robot::step_MainExecution()
    
    
    void Robot::GetCurrentMatPose(f32& x, f32& y, Radians& angle)
    {
      x = currentMatX_;
      y = currentMatY_;
      angle = currentMatHeading_;
    } // GetCurrentMatPose()
    
    void Robot::SetOpenLoopMotorSpeed(s16 speedl, s16 speedr)
    {
      // Convert PWM to rad/s
      // TODO: Do this properly.  For now assume MOTOR_PWM_MAXVAL achieves 1m/s lateral speed.
      
      // Radius ~= 15mm => circumference of ~95mm.
      // 1m/s == 10.5 rot/s == 66.1 rad/s
      const float FACTOR = ((2.f*M_PI) *
                            (1000.f / (Cozmo::WHEEL_DIAMETER_MM*M_PI)));
      f32 left_rad_per_s  = speedl * FACTOR / HAL::MOTOR_PWM_MAXVAL;
      f32 right_rad_per_s = speedr * FACTOR / HAL::MOTOR_PWM_MAXVAL;
      
      HAL::SetWheelAngularVelocity(left_rad_per_s, right_rad_per_s);
      
    } // Robot::SetOpenLoopMotorSpeed()
    
    
  } // namespace Cozmo
  
} // namespace Anki
