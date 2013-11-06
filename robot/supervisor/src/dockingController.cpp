#include "dockingController.h"
#include "headController.h"
#include "liftController.h"

#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/visionSystem.h"

namespace Anki {
  namespace Cozmo {
    namespace DockingController {
      
      namespace {
        
        // Phases of docking: approach the block using visual servoing,
        // set the lift for gripping, grip the block.
        
        enum Mode {
          APPROACH,
          SET_LIFT,
          GRIP,
          DONE
        };
        
        // TODO: set error tolerances in mm and convert to pixels based on camera resolution?
        const f32 VERTICAL_TARGET_ERROR_TOLERANCE = 1.f;   // in pixels
        const f32 HORIZONTAL_TARGET_ERROR_TOLERANCE = 1.f; // in pixels

        Mode mode_ = DONE;
        bool success_  = false;
        
        f32 liftDockHeight_ = -1.f;
        VisionSystem::DockingTarget goalDockTarget_, obsDockTarget_;
        
      } // "private" namespace
      
      void Reset()
      {
        mode_ = APPROACH;
        success_ = false;
      }
      
      ReturnCode SetGoals(const CozmoMsg_InitiateDock* msg)
      {
        Reset();
        
        // Move the head to the position computed by the basestation
        HeadController::SetDesiredAngle(msg->headAngle);
        
        // Store the lift dock height the basestation computed, and drop
        // the lift all the way down, out of the way of the vision system.
        // We will raise it once APPROACH mode is complete and we are in
        // position.
        LiftController::SetDesiredHeight(LIFT_HEIGHT_LOW);
        liftDockHeight_ = msg->liftHeight;
        
        for(u8 i=0; i<4; ++i) {
          goalDockTarget_.dotX[i] = msg->dotX[i];
          goalDockTarget_.dotY[i] = msg->dotY[i];
        }
        
        // Use where the message says to start looking for the
        // target in the image
        VisionSystem::setDockingWindow(msg->winX, msg->winY,
                                       msg->winWidth, msg->winHeight);
        
        return EXIT_SUCCESS;
        
      } // SetGoals()
     
      // TODO: Use a real controller
      // Compute the wheel velocities from the difference between
      // the observed target and the goal target
      void ApproachBlock()
      {
        
        /*
         % High-level:
         % - If the observed target is to the right of the goal location for that
         %   target, we need to turn left, meaning we need the right motor to rotate
         %   forwards and the left motor to rotate backwards.
         % - If the observed target is above the goal location for that target, we
         %   need to back up, so we need to rotate both motors backwards.
         */
        
        // TODO: incorporate heading error?
        
        const f32 K_turn  = 0.03;
        const f32 K_dist  = 0.05;
        const f32 maxSpeed = 8;
        
        // Note that we're only comparing the centroid of the targets!
        f32 obsMeanX  = 0.25f*(obsDockTarget_.dotX[0] + obsDockTarget_.dotX[1] +
                               obsDockTarget_.dotX[2] + obsDockTarget_.dotX[3]);
        f32 obsMeanY  = 0.25f*(obsDockTarget_.dotY[0] + obsDockTarget_.dotY[1] +
                               obsDockTarget_.dotY[2] + obsDockTarget_.dotY[3]);
        
        f32 goalMeanX = 0.25f*(goalDockTarget_.dotX[0] + goalDockTarget_.dotX[1] +
                               goalDockTarget_.dotX[2] + goalDockTarget_.dotX[3]);
        f32 goalMeanY = 0.25f*(goalDockTarget_.dotY[0] + goalDockTarget_.dotY[1] +
                               goalDockTarget_.dotY[2] + goalDockTarget_.dotY[3]);
        
        f32 verticalError = 0.f;
        f32 horizontalError = obsMeanX - goalMeanX;
        f32 turnVelocityLeft  = K_turn*horizontalError;
        f32 turnVelocityRight = -turnVelocityLeft;
        
        // HACK: Only update distance if our heading is decent. I.e., turn towards
        // the target and _then_ start driving to it.
        f32 distanceVelocity = 0;
        if(fabs(horizontalError) < 10.f) {
          verticalError = obsMeanY - goalMeanY;
          distanceVelocity = -K_dist*verticalError;
        }
        
        f32 leftMotorVelocity  = 0.f;
        f32 rightMotorVelocity = 0.f;
        
        if(fabs(verticalError)   < VERTICAL_TARGET_ERROR_TOLERANCE &&
           fabs(horizontalError) < HORIZONTAL_TARGET_ERROR_TOLERANCE)
        {
          // We have reached the block. Stop moving and set mode to
          // lift gripper to desired docking height
          mode_ = SET_LIFT;
          leftMotorVelocity = 0.f;
          rightMotorVelocity = 0.f;
        }
        else {
          leftMotorVelocity  = MAX(-maxSpeed, MIN(maxSpeed, turnVelocityLeft  + distanceVelocity));
          rightMotorVelocity = MAX(-maxSpeed, MIN(maxSpeed, turnVelocityRight + distanceVelocity));
        }
        
        // Command the speeds
        // TODO: convert from desired velocity to power and use SetOpenLoopMotorSpeed()
        HAL::SetWheelAngularVelocity(leftMotorVelocity, leftMotorVelocity);
        
      } // ApproachBlock()
      
      bool IsDone()
      {
        return mode_ == DONE;
      }
      
      bool DidSucceed()
      {
        return success_;
      }
      
      ReturnCode Update()
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
        // Wait until head and lift are in position before proceeding
        if(HeadController::IsInPosition() &&
           LiftController::IsInPosition())
        {
          switch(mode_)
          {
            case APPROACH:
            {
              // Find the docking target and put it in obsTarget_
              if(VisionSystem::findDockingTarget(obsDockTarget_) == EXIT_SUCCESS)
              {
                // This will switch us to SET_LIFT mode when it's done
                ApproachBlock();
                
              } else {
                PRINT("Failed to find docking target.\n");
                mode_ = DONE;
                success_ = false;
                
              } // if found docking target
              
              break;
            } // case APPROACH
            
            case SET_LIFT:
            {
              // This will switch us to GRIP mode once it's done
              LiftController::SetDesiredHeight(liftDockHeight_);
            } // case SET_LIFT
              
            case GRIP:
            {
              
              HAL::EngageGripper();
              
              if(HAL::IsGripperEngaged()) {
                mode_ = DONE;
                success_ = true;
              }
              
              break;
            } // case GRIP
            
            default:
            {
              mode_ = DONE;
              success_ = false;
              PRINT("Reached default case in DockingController "
                      "mode switch statement.\n");
            } // default case
              
          } // switch(mode)
          
        } // if head and left are in position
        
        if(success_ == false)
        {
          retVal = EXIT_FAILURE;
        }
        
        return retVal;
        
      } // Update()

      } // namespace DockingController
    } // namespace Cozmo
  } // namespace Anki