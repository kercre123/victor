
#include "anki/cozmo/MessageProtocol.h"

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
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
#define EXECUTE_TEST_PATH 0

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

#include "anki/cozmo/robot/hardwareInterface.h" // simulated or real!
#include "anki/cozmo/robot/visionSystem.h"

namespace Anki {
  
  namespace Cozmo {
    
    namespace Robot {
      
      // Parameters / Constants:
      const f32 ENCODER_FILTERING_COEFF = 0.9f;
     
      // "Member Variables"
      typedef struct {
        // Create Mailboxes for holding messages from the VisionSystem,
        // to be relayed up to the Basestation.
        VisionSystem::BlockMarkerMailbox blockMarkerMailbox;
        VisionSystem::MatMarkerMailbox   matMarkerMailbox;
        
        OperationMode mode, nextMode;
        
        // Localization:
        f32 currentMatX, currentMatY;
        Radians currentMatHeading;
        
        // Encoder / Wheel Speed Filtering
        s32 leftWheelSpeed_mmps;
        s32 rightWheelSpeed_mmps;
        float filterSpeedL;
        float filterSpeedR;
      } Members;
      
      static Members this_;

      
      // Runs one step of the wheel encoder filter;
      void EncoderSpeedFilterIteration(void);

    } // namespace Robot
    
    
    
    //
    // Accessors:
    //
    Robot::OperationMode Robot::GetOperationMode()
    { return this_.mode; }
    
    void Robot::SetOperationMode(Robot::OperationMode newMode)
    { this_.mode = newMode; }
    
    s32 Robot::GetLeftWheelSpeed(void)
    { return this_.leftWheelSpeed_mmps; }
    
    s32 Robot::GetRightWheelSpeed(void)
    { return this_.rightWheelSpeed_mmps; }

    s32 Robot::GetLeftWheelSpeedFiltered(void)
    { return this_.filterSpeedL; }
    
    s32 Robot::GetRightWheelSpeedFiltered(void)
    { return this_.filterSpeedR; }

    
    //
    // Methods:
    //

    ReturnCode Robot::Init(void)
    {
      if(HardwareInterface::Init() == EXIT_FAILURE) {
        fprintf(stdout, "Hardware Interface initialization failed!\n");
        return EXIT_FAILURE;
      }
      
      if(VisionSystem::Init(HardwareInterface::GetHeadFrameGrabber(),
                            HardwareInterface::GetMatFrameGrabber(),
                            HardwareInterface::GetHeadCamInfo(),
                            HardwareInterface::GetMatCamInfo(),
                            &this_.blockMarkerMailbox,
                            &this_.matMarkerMailbox) == EXIT_FAILURE)
      {
        fprintf(stdout, "Vision System initialization failed.");
        return EXIT_FAILURE;
      }
      
      this_.leftWheelSpeed_mmps = 0;
      this_.rightWheelSpeed_mmps = 0;
      this_.filterSpeedL = 0.f;
      this_.filterSpeedR = 0.f;
      
      if(PathFollower::Init() == EXIT_FAILURE) {
        fprintf(stdout, "PathFollower initialization failed.");
        return EXIT_FAILURE;
      }
      
      return EXIT_SUCCESS;
      
    } // Robot::Init()
    
    void Robot::Destroy()
    {
      VisionSystem::Destroy();
      HardwareInterface::Destroy();
    }
    
    
    void Robot::EncoderSpeedFilterIteration(void)
    {
      
      // Get true (gyro measured) speeds from robot model
      this_.leftWheelSpeed_mmps = HardwareInterface::GetLeftWheelSpeed();
      this_.rightWheelSpeed_mmps = HardwareInterface::GetRightWheelSpeed();
      
      this_.filterSpeedL = (GetLeftWheelSpeed() * (1.0f - ENCODER_FILTERING_COEFF) +
                            (this_.filterSpeedL * ENCODER_FILTERING_COEFF));
      this_.filterSpeedR = (GetRightWheelSpeed() * (1.0f - ENCODER_FILTERING_COEFF) +
                            (this_.filterSpeedR * ENCODER_FILTERING_COEFF));
      
    } // Robot::EncoderSpeedFilterIteration()
  
    
    ReturnCode Robot::step_MainExecution()
    {
      // If the hardware interface needs to be advanced (as in the case of
      // a Webots simulation), do that first.
      HardwareInterface::Step();
      
#if(EXECUTE_TEST_PATH)
      // TESTING
      static u32 startDriveTime_us = 1000000;
      static BOOL driving = FALSE;
      if (!IsKeyboardControllerEnabled() && !driving && getMicroCounter() > startDriveTime_us) {
        SetUserCommandedAcceleration( MAX(ONE_OVER_CONTROL_DT + 1, 500) );  // This can't be smaller than 1/CONTROL_DT!
        SetUserCommandedDesiredVehicleSpeed(160);
        fprintf(stdout, "Speed commanded: %d mm/s\n", GetUserCommandedDesiredVehicleSpeed() );
        
        
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
      HardwareInterface::ManageRecvBuffer();
      
      // Check if connector attaches to anything
      HardwareInterface::ManageGripper();

      HardwareInterface::UpdateDisplay();
      
      // Check any messages from the vision system and pass them along to the
      // basestation as a message
      while( this_.matMarkerMailbox.hasMail() )
      {
        const CozmoMsg_ObservedMatMarker matMsg = this_.matMarkerMailbox.getMessage();
        HardwareInterface::SendMessage(&matMsg, sizeof(CozmoMsg_ObservedMatMarker));
      }
      
      while( this_.blockMarkerMailbox.hasMail() )
      {
        const CozmoMsg_ObservedBlockMarker blockMsg = this_.blockMarkerMailbox.getMessage();
        HardwareInterface::SendMessage(&blockMsg, sizeof(CozmoMsg_ObservedBlockMarker));
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
      float left_rad_per_s  = speedl * 66.1f / MOTOR_PWM_MAXVAL;
      float right_rad_per_s = speedr * 66.1f / MOTOR_PWM_MAXVAL;
      
      HardwareInterface::SetWheelAngularVelocity(left_rad_per_s, right_rad_per_s);
      
    } // Robot::SetOpenLoopMotorSpeed()
    
    
  } // namespace Cozmo
  
} // namespace Anki
