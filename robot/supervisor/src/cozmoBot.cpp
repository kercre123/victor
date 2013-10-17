
#include "anki/cozmo/MessageProtocol.h"

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
//#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/mainExecution.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/vehicleMath.h"
#include "anki/cozmo/robot/vehicleSpeedController.h"

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
#include "anki/cozmo/robot/visionSystem.h"

namespace Anki {
  
  namespace Cozmo {
    
    // "Private Member Variables"
    namespace {
      
      // Parameters / Constants:
      const f32 ENCODER_FILTERING_COEFF = 0.9f;
      
      // Create Mailboxes for holding messages from the VisionSystem,
      // to be relayed up to the Basestation.
      VisionSystem::BlockMarkerMailbox blockMarkerMailbox_;
      VisionSystem::MatMarkerMailbox   matMarkerMailbox_;
      
      Robot::OperationMode mode_, nextMode_;
      
      // Localization:
      f32 currentMatX_, currentMatY_;
      Radians currentMatHeading_;
      
      // Encoder / Wheel Speed Filtering
      s32 leftWheelSpeed_mmps_;
      s32 rightWheelSpeed_mmps_;
      float filterSpeedL_;
      float filterSpeedR_;
      
      // Runs one step of the wheel encoder filter;
      void EncoderSpeedFilterIteration(void);
      
    } // Robot private namespace
    
    
    //
    // Accessors:
    //
    Robot::OperationMode Robot::GetOperationMode()
    { return mode_; }
    
    void Robot::SetOperationMode(Robot::OperationMode newMode)
    { mode_ = newMode; }
    
    s32 Robot::GetLeftWheelSpeed(void)
    { return leftWheelSpeed_mmps_; }
    
    s32 Robot::GetRightWheelSpeed(void)
    { return rightWheelSpeed_mmps_; }

    s32 Robot::GetLeftWheelSpeedFiltered(void)
    { return filterSpeedL_; }
    
    s32 Robot::GetRightWheelSpeedFiltered(void)
    { return filterSpeedR_; }

    
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
      
      leftWheelSpeed_mmps_ = 0;
      rightWheelSpeed_mmps_ = 0;
      filterSpeedL_ = 0.f;
      filterSpeedR_ = 0.f;
      
      if(PathFollower::Init() == EXIT_FAILURE) {
        fprintf(stdout, "PathFollower initialization failed.");
        return EXIT_FAILURE;
      }
      
      return EXIT_SUCCESS;
      
    } // Robot::Init()
    
    void Robot::Destroy()
    {
      VisionSystem::Destroy();
      HAL::Destroy();
    }
    
    
    void EncoderSpeedFilterIteration(void)
    {
      
      // Get true (gyro measured) speeds from robot model
      leftWheelSpeed_mmps_ = HAL::GetLeftWheelSpeed();
      rightWheelSpeed_mmps_ = HAL::GetRightWheelSpeed();
      
      filterSpeedL_ = (Robot::GetLeftWheelSpeed() * (1.0f - ENCODER_FILTERING_COEFF) +
                            (filterSpeedL_ * ENCODER_FILTERING_COEFF));
      filterSpeedR_ = (Robot::GetRightWheelSpeed() * (1.0f - ENCODER_FILTERING_COEFF) +
                            (filterSpeedR_ * ENCODER_FILTERING_COEFF));
      
    } // Robot::EncoderSpeedFilterIteration()
  
    
    ReturnCode Robot::step_MainExecution()
    {
      // If the hardware interface needs to be advanced (as in the case of
      // a Webots simulation), do that first.
      HAL::Step();
      
#if(EXECUTE_TEST_PATH)
      // TESTING
      static u32 startDriveTime_us = 1000000;
      static BOOL driving = FALSE;
      if (!driving && HAL::GetMicroCounter() > startDriveTime_us) {
        VehicleSpeedController::SetUserCommandedAcceleration( MAX(ONE_OVER_CONTROL_DT + 1, 500) );  // This can't be smaller than 1/CONTROL_DT!
        VehicleSpeedController::SetUserCommandedDesiredVehicleSpeed(160);
        fprintf(stdout, "Speed commanded: %d mm/s\n",
                VehicleSpeedController::GetUserCommandedDesiredVehicleSpeed() );
        
        
        // Create a path and follow it
        PathFollower::AppendPathSegment_Line(0, 0.0, 0.0, 0.3, -0.3);
        float arc1_radius = sqrt(0.005);  // Radius of sqrt(0.05^2 + 0.05^2)
        PathFollower::AppendPathSegment_Arc(0, 0.35, -0.25, arc1_radius, -0.75*PI, 0);
        PathFollower::AppendPathSegment_Line(0, 0.35 + arc1_radius, -0.25, 0.35 + arc1_radius, 0.2);
        float arc2_radius = sqrt(0.02); // Radius of sqrt(0.1^2 + 0.1^2)
        PathFollower::AppendPathSegment_Arc(0, 0.35 + arc1_radius - arc2_radius, 0.2, arc2_radius, 0, PIDIV2);
        PathFollower::StartPathTraversal();
        
        driving = TRUE;
      }
#endif //EXECUTE_TEST_PATH
      
      // Buffer any incoming data from basestation
      HAL::ManageRecvBuffer();
      
      // Check if connector attaches to anything
      HAL::ManageGripper();

      HAL::UpdateDisplay();
      
      // Check any messages from the vision system and pass them along to the
      // basestation as a message
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
      
    } // GetCurrentMatPose()
    
    
    void Robot::SetOpenLoopMotorSpeed(s16 speedl, s16 speedr)
    {
      // Convert PWM to rad/s
      // TODO: Do this properly.  For now assume MOTOR_PWM_MAXVAL achieves 1m/s lateral speed.
      
      // Radius ~= 15mm => circumference of ~95mm.
      // 1m/s == 10.5 rot/s == 66.1 rad/s
      f32 left_rad_per_s  = speedl * 66.1f / HAL::MOTOR_PWM_MAXVAL;
      f32 right_rad_per_s = speedr * 66.1f / HAL::MOTOR_PWM_MAXVAL;
      
      HAL::SetWheelAngularVelocity(left_rad_per_s, right_rad_per_s);
      
    } // Robot::SetOpenLoopMotorSpeed()
    
    
  } // namespace Cozmo
  
} // namespace Anki
