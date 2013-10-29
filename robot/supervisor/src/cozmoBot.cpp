
#include "anki/cozmo/messageProtocol.h"

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
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
        
        Robot::OperationMode mode_, nextMode_;
        
        // Localization:
        f32 currentMatX_, currentMatY_;
        Radians currentMatHeading_;
        
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
        
        // Once initialization is done, broadcast a message that this robot
        // is ready to go
        fprintf(stdout, "Robot broadcasting availability message.\n");
        CozmoMsg_RobotAvailable msg;
        msg.size = sizeof(CozmoMsg_RobotAvailable);
        msg.msgID = MSG_V2B_CORE_ROBOT_AVAILABLE;
        msg.robotID = HAL::GetRobotID();
        HAL::SendMessage(reinterpret_cast<u8 *>(&msg), msg.size);
        
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
            case MSG_B2V_CORE_ROBOT_ADDED_TO_WORLD:
            {
              const CozmoMsg_RobotAdded* msg = reinterpret_cast<const CozmoMsg_RobotAdded*>(msgBuffer);
              
              if(msg->robotID != HAL::GetRobotID()) {
                fprintf(stdout, "Robot received ADDED_TO_WORLD handshake with "
                        " wrong robotID (%d instead of %d).\n",
                        msg->robotID, HAL::GetRobotID());
              }
              
              fprintf(stdout, "Robot received handshake from basestation, "
                      "sending camera calibration.\n");
              const HAL::CameraInfo *matCamInfo  = HAL::GetMatCamInfo();
              const HAL::CameraInfo *headCamInfo = HAL::GetHeadCamInfo();
              
              CozmoMsg_CameraCalibration calibMsg;
              calibMsg.size = sizeof(CozmoMsg_CameraCalibration);
              
              //
              // Send mat camera calibration
              //
              calibMsg.msgID = MSG_V2B_CORE_MAT_CAMERA_CALIBRATION;
              // TODO: do we send x or y focal length, or both?
              calibMsg.focalLength_x = matCamInfo->focalLength_x;
              calibMsg.focalLength_y = matCamInfo->focalLength_y;
              calibMsg.fov           = matCamInfo->fov_ver;
              calibMsg.nrows         = matCamInfo->nrows;
              calibMsg.ncols         = matCamInfo->ncols;
              calibMsg.center_x      = matCamInfo->center_x;
              calibMsg.center_y      = matCamInfo->center_y;
              
              HAL::SendMessage(reinterpret_cast<const u8*>(&calibMsg),
                               calibMsg.size);
              //
              // Send head camera calibration
              //
              calibMsg.msgID = MSG_V2B_CORE_HEAD_CAMERA_CALIBRATION;
              // TODO: do we send x or y focal length, or both?
              calibMsg.focalLength_x = headCamInfo->focalLength_x;
              calibMsg.focalLength_y = headCamInfo->focalLength_y;
              calibMsg.fov           = headCamInfo->fov_ver;
              calibMsg.nrows         = headCamInfo->nrows;
              calibMsg.ncols         = headCamInfo->ncols;
              calibMsg.center_x      = headCamInfo->center_x;
              calibMsg.center_y      = headCamInfo->center_y;
              
              HAL::SendMessage(reinterpret_cast<const u8*>(&calibMsg),
                               calibMsg.size);
              
              break;
            }
            case MSG_B2V_CORE_ABS_LOCALIZATION_UPDATE:
            {
              // TODO: Double-check that size matches expected size?
              
              const CozmoMsg_AbsLocalizationUpdate *msg = reinterpret_cast<const CozmoMsg_AbsLocalizationUpdate*>(msgBuffer);
              
              // Note the conversion to meters
              SetCurrentMatPose(msg->xPosition * .001f,
                                msg->yPosition * .001f,
                                msg->headingAngle);
              
              fprintf(stdout, "Robot received localization update from "
                      "basestation: (%.3f,%.3f) at %.1f degrees\n",
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
      
      
      void GetCurrentMatPose(f32& x, f32& y, Radians& angle)
      {
        x = currentMatX_;
        y = currentMatY_;
        angle = currentMatHeading_;
      } // GetCurrentMatPose()
      
      void SetCurrentMatPose(f32  x, f32  y, Radians  angle)
      {
        currentMatX_ = x;
        currentMatY_ = y;
        currentMatHeading_ = angle;
        
#if(USE_OVERLAY_DISPLAY)
        {
          using namespace Sim::OverlayDisplay;
          SetText(CURR_EST_POSE, "Est. Pose: (x,y)=(%.4f, %.4f) at angle=%.1f",
                  currentMatX_, currentMatY_,
                  currentMatHeading_.getDegrees());
          f32 xTrue, yTrue, angleTrue;
          HAL::GetGroundTruthPose(xTrue, yTrue, angleTrue);
          Radians angleRad(angleTrue);
          
          SetText(CURR_TRUE_POSE, "True Pose: (x,y)=(%.4f, %.4f) at angle=%.1f",
                  xTrue, yTrue, angleRad.getDegrees());
        }
#endif
      }
      
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
