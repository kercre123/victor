#include "anki/common/robot/config.h"
#include "dockingController.h"
#include "gripController.h"
#include "headController.h"
#include "liftController.h"

#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/speedController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/visionSystem.h"

// Use PathFollower to follow a path generated from the relative docking pose
// specified by SetRelDockPose().
#define DOCK_BY_PATH


namespace Anki {
  namespace Cozmo {
    namespace DockingController {
      
      namespace {
        
        // Constants
        
        enum Mode {
          IDLE,
          APPROACH,
          SET_LIFT,
          GRIP,
          DONE
        };

        
        // Distance between the robot origin and the distance along the robot's x-axis
        // to the lift when it is in the low docking position.
        const f32 ORIGIN_TO_LOW_LIFT_DIST_M = 0.02; // TODO: Measure this!
        
        u32 lastDockingErrorSignalRecvdTime_ = 0;
        const u32 STOPPED_TRACKING_TIMEOUT_US = 100000;
        
        const u16 DOCK_APPROACH_SPEED_MMPS = 20;
        const u16 DOCK_FAR_APPROACH_SPEED_MMPS = 50;
        const u16 DOCK_APPROACH_ACCEL_MMPS2 = 500;
        
        
        // TODO: set error tolerances in mm and convert to pixels based on camera resolution?
        const f32 VERTICAL_TARGET_ERROR_TOLERANCE = 1.f;   // in pixels
        const f32 HORIZONTAL_TARGET_ERROR_TOLERANCE = 1.f; // in pixels

#ifdef DOCK_BY_PATH
        Mode mode_ = IDLE;
#else
        Mode mode_ = DONE;
#endif
        bool success_  = false;
        
        f32 liftDockHeight_ = LIFT_HEIGHT_LOWDOCK;
        
      } // "private" namespace
      
      void Reset()
      {
        mode_ = APPROACH;
        success_ = false;
      }
 
#if 0
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
        // TODO: Replacing obsolete SetWheelAngularVelocity(). This probably breaks everything since inputs are now in mm/s
        //       and the underlying controller is different.
        SteeringController::ExecuteDirectDrive(leftMotorVelocity, leftMotorVelocity);
        
      } // ApproachBlock()
#endif

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
        
#ifdef DOCK_BY_PATH
        switch(mode_)
        {
          case IDLE:
            success_ = false;
            break;
          case SET_LIFT:
            GripController::DisengageGripper();
            LiftController::SetDesiredHeight(liftDockHeight_);
            mode_ = APPROACH;
            break;
          case APPROACH:
          {
            // Stop if we haven't received error signal for a while
            if (HAL::GetMicroCounter() - lastDockingErrorSignalRecvdTime_ > STOPPED_TRACKING_TIMEOUT_US) {
              PathFollower::ClearPath();
              SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
              mode_ = IDLE;
              PRINT("Too long without block pose\n");
              break;
            }
            
            // If finished traversing path
            if (!PathFollower::IsTraversingPath()) {
              GripController::EngageGripper();
              mode_ = DONE;
              break;
            }
            
            break;
          }
          case DONE:
            success_ = true;
            
            // Go to IDLE when we lose tracking of the block
            if (HAL::GetMicroCounter() - lastDockingErrorSignalRecvdTime_ > STOPPED_TRACKING_TIMEOUT_US) {
              success_ = false;
              mode_ = IDLE;
            }
            break;
          default:
            mode_ = IDLE;
            success_ = false;
            PRINT("Reached default case in DockingController "
                  "mode switch statement.(1)\n");
            break;
        }
        
        
#else
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
              
              GripController::EngageGripper();
              
              if(GripController::IsGripperEngaged()) {
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
        
#endif // ifdef DOCK_BY_PATH
        
        if(success_ == false)
        {
          retVal = EXIT_FAILURE;
        }
        
        return retVal;
        
      } // Update()

      
      
      void SetRelDockPose(f32 rel_x, f32 rel_y, f32 rel_rad)
      {
        lastDockingErrorSignalRecvdTime_ = HAL::GetMicroCounter();
        
        // Set mode to approach if in idle
        if (mode_ == IDLE) {
          mode_ = SET_LIFT;
        }
        
        // Ignore if we've already finished approach
        if (mode_ == DONE) {
          return;
        }
        
        // Clear current path
        PathFollower::ClearPath();
        
        // Create new path that is aligned with the normal of the block we want to dock to.
        // End point: Where the robot origin should be by the time the robot has docked.
        // Start point: Projected from end point at specified rad.
        //              Just make length as long as distance to block.
        //
        //   ______
        //   |     |
        //   |     *  End ---------- Start              * == (rel_x, rel_y)
        //   |_____|      \ ) rad
        //    Block        \
        //                  \
        //                   \ Aligned with robot x axis (but opposite direction)
        //
        //
        //               \ +ve x axis
        //                \
        //                / ROBOT
        //               /
        //              +ve y-axis
        
        
        // Convert to m
        rel_x *= 0.001;
        rel_y *= 0.001;
        
        // Compute end point
        f32 dx = ORIGIN_TO_LOW_LIFT_DIST_M * cosf(rel_rad);
        f32 dy = ORIGIN_TO_LOW_LIFT_DIST_M * sinf(rel_rad);
        
        f32 x_end_m = rel_x - dx;
        f32 y_end_m = rel_y - dy;
        
        
        // Compute start point
        f32 distToBlock = sqrtf(rel_x * rel_x + rel_y * rel_y);
        dx *= distToBlock / ORIGIN_TO_LOW_LIFT_DIST_M;
        dy *= distToBlock / ORIGIN_TO_LOW_LIFT_DIST_M;
        f32 x_start_m = x_end_m - dx;
        f32 y_start_m = y_end_m - dy;
        
        
#if(DEBUG_DOCK_CONTROLLER)
        PERIODIC_PRINT(200, "SEG: x %f, y %f, rad %f => (%f, %f) to (%f, %f), dist %f\n",
                       rel_x, rel_y, rel_rad, x_start_m, y_start_m, x_end_m, y_end_m, distToBlock);
#endif
        
        // Create path segment
        PathFollower::AppendPathSegment_Line(0, x_start_m, y_start_m, x_end_m, y_end_m);
        
        // Set speed
        if (distToBlock < 0.15) {
          SpeedController::SetUserCommandedDesiredVehicleSpeed( DOCK_APPROACH_SPEED_MMPS );
        } else {
          SpeedController::SetUserCommandedDesiredVehicleSpeed( DOCK_FAR_APPROACH_SPEED_MMPS );
        }
        SpeedController::SetUserCommandedAcceleration( DOCK_APPROACH_ACCEL_MMPS2 );

        
        // Start following path
        PathFollower::StartPathTraversal();
        
      }
      

      } // namespace DockingController
    } // namespace Cozmo
  } // namespace Anki