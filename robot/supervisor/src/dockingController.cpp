#include "anki/common/robot/config.h"
#include "anki/common/robot/trig_fast.h"
#include "dockingController.h"
#include "gripController.h"
#include "headController.h"
#include "liftController.h"

#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/cozmoTypes.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/visionSystem.h"
#include "anki/cozmo/robot/speedController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/pathFollower.h"

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
          APPROACH_FOR_DOCK,
          SET_LOW_LIFT,
          GRIP,
          DONE_DOCKING,
          SET_CARRY_LIFT_HEIGHT,
          APPROACH_FOR_PLACEMENT,
          SET_PLACEMENT_LIFT_HEIGHT,
          BACKOUT,
          LOWER_LIFT,
          DONE
        };

        // Turning radius of docking path
        f32 DOCK_PATH_START_RADIUS_M = 0.05;
        f32 DOCK_PATH_END_RADIUS_M = 0.05;
        
        // The length of the straight tail end of the dock path.
        // Should be roughly the length of the forks on the lift.
        f32 FINAL_APPROACH_STRAIGHT_SEGMENT_LENGTH_M = 0.03;
        
        // Distance between the robot origin and the distance along the robot's x-axis
        // to the lift when it is in the low docking position.
        const f32 ORIGIN_TO_LOW_LIFT_DIST_M = 0.02; // TODO: Measure this!
        const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_M = 0.017; // TODO: Measure this!
        
        u32 lastDockingErrorSignalRecvdTime_ = 0;
        const u32 STOPPED_TRACKING_TIMEOUT_US = 200000;
        
        const u16 DOCK_APPROACH_SPEED_MMPS = 20;
        const u16 DOCK_FAR_APPROACH_SPEED_MMPS = 50;
        const u16 DOCK_APPROACH_ACCEL_MMPS2 = 500;

        // HACK: Only using these because lift controller encoders not working.
        const u32 PRE_PLACEMENT_WAIT_TIME_US = 2000000;
        const u32 LIFT_MOTION_TIME_US = 7000000;
        const u32 LIFT_PLACEMENT_ADJUST_TIME_US = 200000;
        bool liftIsUp_ = true;
        const u32 BACKOUT_TIME = 3000000;
        
        const u16 BLOCK_TO_PICK_UP = 60;
        const u16 BLOCK_TO_PLACE_ON = 50;
        
        // TODO: set error tolerances in mm and convert to pixels based on camera resolution?
        const f32 VERTICAL_TARGET_ERROR_TOLERANCE = 1.f;   // in pixels
        const f32 HORIZONTAL_TARGET_ERROR_TOLERANCE = 1.f; // in pixels

#ifdef DOCK_BY_PATH
        Mode mode_ = IDLE;
#else
        Mode mode_ = DONE;
#endif
        bool success_  = false;
        
        // Whether or not the robot has a block in its grip
        bool isDocked_ = false;
        
        // When to transition to the next state. Only some states use this.
        u32 transitionTime_ = 0;
        
        
        // The pose of the robot at the start of docking.
        // While block tracking is maintained the robot follows
        // a path from this initial pose to the docking pose.
        Anki::Embedded::Pose2d approachStartPose_;

        // The pose of the block as we're docking
        Anki::Embedded::Pose2d blockPose_;

        // The docking pose
        Anki::Embedded::Pose2d dockPose_;
        
#ifndef SIMULATOR
        // Since the physical robot currently does not localize,
        // we need to store the transform from docking pose
        // to the approachStartPose, which we then use to compute
        // a new approachStartPose with every block pose update.
        // We're faking a different approachStartPose because without
        // localization it looks like the block is moving and not the robot.

        f32 approachPath_dx, approachPath_dy, approachPath_dtheta, approachPath_dOrientation;
#endif
        
        
      } // "private" namespace
      
      
      void StartPlacement();
      
      
      void Reset()
      {
        mode_ = APPROACH_FOR_DOCK;
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
          mode_ = SET_LOW_LIFT;
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
      
      bool IsDocked()
      {
        return isDocked_;
      }
      
    bool IsDockingOrPlacing()
      {
        return (mode_ != IDLE && mode_ != DONE_DOCKING && mode_ != DONE);
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
          case SET_LOW_LIFT:
            if (HAL::GetMicroCounter() > transitionTime_) {
              GripController::DisengageGripper();
              //LiftController::SetDesiredHeight(LIFT_HEIGHT_LOW);
              HAL::MotorSetPower(HAL::MOTOR_LIFT,0);
              mode_ = APPROACH_FOR_DOCK;
            }
            break;
          case APPROACH_FOR_DOCK:
          {
            // Stop if we haven't received error signal for a while
            if (HAL::GetMicroCounter() - lastDockingErrorSignalRecvdTime_ > STOPPED_TRACKING_TIMEOUT_US) {
              PathFollower::ClearPath();
              SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
              mode_ = IDLE;
              PRINT("Too long without block pose (currTime %d, lastErrSignal %d)\n", HAL::GetMicroCounter(), lastDockingErrorSignalRecvdTime_);
              break;
            }
            
            // If finished traversing path
            if (!PathFollower::IsTraversingPath()) {
              PRINT("GRIPPING\n");
              PathFollower::PrintPathSegment(0);
              GripController::EngageGripper();
              
              transitionTime_ = HAL::GetMicroCounter() + PRE_PLACEMENT_WAIT_TIME_US;
              mode_ = DONE_DOCKING;
              break;
            }
            
            break;
          }
          case DONE_DOCKING:
            success_ = true;
            isDocked_ = true;
            
            
            if (HAL::GetMicroCounter() > transitionTime_) {
              PRINT("DONE DOCKING\n");
              VisionSystem::SetDockingBlock(BLOCK_TO_PLACE_ON);
              StartPlacement();
            }
            
            break;
          case SET_CARRY_LIFT_HEIGHT:
            if (HAL::GetMicroCounter() > transitionTime_) {
              HAL::MotorSetPower(HAL::MOTOR_LIFT,0);
              mode_ = APPROACH_FOR_PLACEMENT;
            }
            break;
            
          case APPROACH_FOR_PLACEMENT:
          {
            // Stop if we haven't received error signal for a while
            if (HAL::GetMicroCounter() - lastDockingErrorSignalRecvdTime_ > STOPPED_TRACKING_TIMEOUT_US) {
              PathFollower::ClearPath();
              SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
              mode_ = IDLE;
              PRINT("Too long without block pose (currTime %d, lastErrSignal %d)\n", HAL::GetMicroCounter(), lastDockingErrorSignalRecvdTime_);
              break;
            }
            
            // If finished traversing path
            if (!PathFollower::IsTraversingPath()) {
              PRINT("PLACING\n");
              PathFollower::PrintPathSegment(0);
              GripController::DisengageGripper();
              
              transitionTime_ = HAL::GetMicroCounter() + LIFT_PLACEMENT_ADJUST_TIME_US;
              HAL::MotorSetPower(HAL::MOTOR_LIFT, -0.4);
              mode_ = SET_PLACEMENT_LIFT_HEIGHT;
              
              break;
            }
            
            break;
          }
            
          case SET_PLACEMENT_LIFT_HEIGHT:
            if (HAL::GetMicroCounter() > transitionTime_) {
              HAL::MotorSetPower(HAL::MOTOR_LIFT,0);
              SteeringController::ExecuteDirectDrive(-30, -30);
              transitionTime_ = HAL::GetMicroCounter() + BACKOUT_TIME;
              mode_ = BACKOUT;
              isDocked_ = false;
              PRINT("DONE PLACEMENT\n");
            }
            break;
          case BACKOUT:
            if (HAL::GetMicroCounter() > transitionTime_) {
              SteeringController::ExecuteDirectDrive(0,0);
              
              HAL::MotorSetPower(HAL::MOTOR_LIFT, -0.3);
              transitionTime_ = HAL::GetMicroCounter() + LIFT_MOTION_TIME_US;
              
              mode_ = LOWER_LIFT;
              PRINT("DONE BACKOUT\n");
            }
            break;
          case LOWER_LIFT:
            if (HAL::GetMicroCounter() > transitionTime_) {
              HAL::MotorSetPower(HAL::MOTOR_LIFT, 0);
              PRINT("DONE LOWER LIFT\n");
              mode_ = DONE;
            }
            break;
          case DONE:
            if (HAL::GetMicroCounter() - lastDockingErrorSignalRecvdTime_ > STOPPED_TRACKING_TIMEOUT_US) {
              PathFollower::ClearPath();
              SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
              mode_ = IDLE;
              PRINT("IDLE\n");
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
            case APPROACH_FOR_DOCK:
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
            
            case SET_LOW_LIFT:
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

      
      // HACK
      void StartPlacement()
      {
        PRINT("STARTING APPROACH FOR PLACEMENT (currTime: %d)\n", HAL::GetMicroCounter());
        mode_ = SET_CARRY_LIFT_HEIGHT;
        
        if (!liftIsUp_) {
          HAL::MotorSetPower(HAL::MOTOR_LIFT, 0.5);
          transitionTime_ = HAL::GetMicroCounter() + LIFT_MOTION_TIME_US;
          liftIsUp_ = true;
        }
      }
      

      // Set mode to approach if in idle
      void StartPicking() {
        PRINT("STARTING APPROACH FOR DOCK\n");
        isDocked_ = false;
        mode_ = SET_LOW_LIFT;
        
        if (liftIsUp_) {
          HAL::MotorSetPower(HAL::MOTOR_LIFT, -0.3);
          transitionTime_ = HAL::GetMicroCounter() + LIFT_MOTION_TIME_US;
          liftIsUp_ = false;
        }
      }

      
      
      void SetRelDockPose(f32 rel_x, f32 rel_y, f32 rel_rad)
      {
        lastDockingErrorSignalRecvdTime_ = HAL::GetMicroCounter();
        
        // Convert to m
        rel_x *= 0.001;
        rel_y *= 0.001;
        
        
        // Set mode to approach if in idle
        if (mode_ == IDLE) {
          if (isDocked_) {
            StartPlacement();
          } else {
            StartPicking();
          }
          
          
          // Set approach start pose
          Localization::GetCurrentMatPose(approachStartPose_.x(), approachStartPose_.y(), approachStartPose_.angle);
#ifndef SIMULATOR
          approachPath_dx = rel_x;
          approachPath_dx = rel_y;
          approachPath_dtheta = atan2_acc(rel_y, rel_x);
          approachPath_dOrientation = rel_rad;
#endif
        }
        
        
        // Return early if we've already finished approach or we're moving the lift in which case
        // we shouldn't drive just yet.
        if (mode_ == DONE_DOCKING
            || mode_ == SET_PLACEMENT_LIFT_HEIGHT
            || mode_ == BACKOUT
            || mode_ == LOWER_LIFT
            || mode_ == DONE) {
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
        
        
        if (rel_x <= -0.0f) {
          //PRINT("DOCK POSE REACHED\n");
          return;
        }
        
        Anki::Embedded::Pose2d currPose;
        Localization::GetCurrentMatPose(currPose.x(), currPose.y(), currPose.angle);
        
        // Compute absolute block pose
        f32 distToBlock = sqrtf((rel_x * rel_x) + (rel_y * rel_y));
        f32 rel_angle_to_block = atan2_acc(rel_y, rel_x);
        blockPose_.x() = currPose.x() + distToBlock * cosf(rel_angle_to_block + currPose.angle.ToFloat());
        blockPose_.y() = currPose.y() + distToBlock * sinf(rel_angle_to_block + currPose.angle.ToFloat());
        blockPose_.angle = currPose.angle + distToBlock + rel_rad;
        
        
#ifndef SIMULATOR
        // Adjust approachStartPose to compensate for lack of localization
        f32 cosTheta = cosf(approachPath_dtheta);
        f32 sinTheta = sinf(approachPath_dtheta);
        approachStartPose_.x() = blockPose_.x() * cosTheta - blockPose_.y() * sinTheta + approachPath_dx;
        approachStartPose_.y() = blockPose_.x() * sinTheta + blockPose_.y() * cosTheta + approachPath_dy;
        approachStartPose_.angle = blockPose_.angle - approachPath_dOrientation;
#endif

        
        
        // Set docking distance
        f32 dockOffsetDist = ORIGIN_TO_LOW_LIFT_DIST_M;
        if (mode_ == APPROACH_FOR_PLACEMENT) {
          dockOffsetDist = ORIGIN_TO_HIGH_PLACEMENT_DIST_M;
        }
        
        // Compute dock pose
        dockPose_.x() = blockPose_.x() - dockOffsetDist * cosf(blockPose_.angle.ToFloat());
        dockPose_.y() = blockPose_.y() - dockOffsetDist * sinf(blockPose_.angle.ToFloat());
        dockPose_.angle = blockPose_.angle;
        
        
        f32 path_length;
        u8 numPathSegments = PathFollower::GenerateDubinsPath(approachStartPose_.x(),
                                                              approachStartPose_.y(),
                                                              approachStartPose_.angle.ToFloat(),
                                                              dockPose_.x(),
                                                              dockPose_.y(),
                                                              dockPose_.angle.ToFloat(),
                                                              DOCK_PATH_START_RADIUS_M,
                                                              DOCK_PATH_END_RADIUS_M,
                                                              FINAL_APPROACH_STRAIGHT_SEGMENT_LENGTH_M,
                                                              &path_length);
        
        // No reasonable Dubins path exists.
        // Either try again with smaller radii or just let the controller
        // attempt to get on to a straight line normal path.
        if (numPathSegments == 0 || path_length > 2 * distToBlock) {
          
          // Compute new starting point for path
          // HACK: Feeling lazy, just multiplying path by some scalar so that it's likely to be behind the current robot pose.
          f32 x_start_m = dockPose_.x() - 5 * distToBlock * cosf(dockPose_.angle.ToFloat());
          f32 y_start_m = dockPose_.y() - 5 * distToBlock * sinf(dockPose_.angle.ToFloat());
          
          PathFollower::ClearPath();
          PathFollower::AppendPathSegment_Line(0, x_start_m, y_start_m, dockPose_.x(), dockPose_.y());
        }

        
        // Set speed
        // TODO: Add hysteresis
        if (distToBlock < 0.15) {
          SpeedController::SetUserCommandedDesiredVehicleSpeed( DOCK_APPROACH_SPEED_MMPS );
        } else {
          SpeedController::SetUserCommandedDesiredVehicleSpeed( DOCK_FAR_APPROACH_SPEED_MMPS );
        }
        SpeedController::SetUserCommandedAcceleration( DOCK_APPROACH_ACCEL_MMPS2 );

        
        // Start following path
        PathFollower::StartPathTraversal();
        
      }
      


      void ResetDocker() {
        
        // Don't interrupt backing out when it's done placingexit.
        // TODO: This should move to higher level controller.
        if (mode_ != SET_PLACEMENT_LIFT_HEIGHT &&
            mode_ != LOWER_LIFT &&
            mode_ != BACKOUT) {
          SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
          //PathFollower::ClearPath();
          SteeringController::ExecuteDirectDrive(0,0);
          mode_ = IDLE;
        }
        
        if (isDocked_) {
          VisionSystem::SetDockingBlock(BLOCK_TO_PLACE_ON);
        } else {
          VisionSystem::SetDockingBlock(BLOCK_TO_PICK_UP);
        }

        
      }
      

      } // namespace DockingController
    } // namespace Cozmo
  } // namespace Anki
