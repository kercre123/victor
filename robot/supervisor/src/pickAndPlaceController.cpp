#include "anki/common/robot/config.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "speedController.h"
#include "steeringController.h"
#include "visionSystem.h"


#define DEBUG_PAP_CONTROLLER 0

namespace Anki {
  namespace Cozmo {
    namespace PickAndPlaceController {
      
      namespace {
        
        // Constants
        
        // TODO: Need to be able to specify wheel motion by distance
        const u32 BACKOUT_TIME = 1500000;
        const f32 BACKOUT_SPEED_MMPS = -30;
        
        const f32 LOW_DOCKING_HEAD_ANGLE = DEG_TO_RAD_F32(-15);
        const f32 HIGH_DOCKING_HEAD_ANGLE = DEG_TO_RAD_F32(15);
        
        // Distance between the robot origin and the distance along the robot's x-axis
        // to the lift when it is in the low docking position.
        const f32 ORIGIN_TO_LOW_LIFT_DIST_MM = 28.f;
        const f32 ORIGIN_TO_HIGH_LIFT_DIST_MM = 20.f;
        const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_MM = 20.f;  // TODO: Technically, this should be the same as ORIGIN_TO_HIGH_LIFT_DIST_MM

        Mode mode_ = IDLE;
        
        DockAction_t action_ = DA_PICKUP_LOW;
        
        Vision::MarkerType dockToMarker_;
        f32 markerWidth_ = 0;
        f32 dockOffsetDistX_ = 0;
        f32 dockOffsetDistY_ = 0;
        f32 dockOffsetAng_ = 0;
        
        // Expected location of the desired dock marker in the image.
        // If not specified, marker may be located anywhere in the image.
        Embedded::Point2f markerCenter_;
        f32 pixelSearchRadius_;
        
        bool isCarryingBlock_ = false;
        bool lastActionSucceeded_ = false;
        
        // When to transition to the next state. Only some states use this.
        u32 transitionTime_ = 0;
        
#if(ALT_HIGH_BLOCK_DOCK_METHOD)
        const f32 ORIGIN_TO_HIGH_PICKUP_HEAD_LOWERING_DIST_MM = 47.f;
        
        ///// Memory stuff //////
        // For computing pose of high block wrt low block
        const int LOCAL_BUFFER_SIZE = 512;
        char localBuffer[LOCAL_BUFFER_SIZE];
        Embedded::MemoryStack localScratch_;
        
        // Pose of high block wrt camera
        Embedded::Array<f32> rotHighBlockWrtRobot;
        Embedded::Point3<f32> transHighBlockWrtRobot;
        
        // Pose of low block wrt camera
        Embedded::Array<f32> rotLowBlockWrtRobot;
        Embedded::Point3<f32> transLowBlockWrtRobot;
        Embedded::Array<f32> rotLowBlockWrtRobot_inv;
        
        // Pose of high block wrt low block
        Embedded::Array<f32> rotHighBlockWrtLowBlock;
        Embedded::Point3<f32> transHighBlockWrtLowBlock;

        
        // WARNING: ResetBuffers should be used with caution
        Result ResetBuffers()
        {
          localScratch_ = Embedded::MemoryStack(localBuffer, LOCAL_BUFFER_SIZE);
          rotHighBlockWrtRobot = Embedded::Array<f32>(3,3, localScratch_);
          rotLowBlockWrtRobot = Embedded::Array<f32>(3,3, localScratch_);
          rotLowBlockWrtRobot_inv = Embedded::Array<f32>(3,3, localScratch_);
          rotHighBlockWrtLowBlock = Embedded::Array<f32>(3,3, localScratch_);
          
          if(!localScratch_.IsValid()) {
            PRINT("Error: PAP::InitializeScratchBuffers\n");
            return RESULT_FAIL;
          }
          return RESULT_OK;
        }
#endif  // #if(ALT_HIGH_BLOCK_DOCK_METHOD)
        
      } // "private" namespace
      
      
      Mode GetMode() {
        return mode_;
      }

#if(ALT_HIGH_BLOCK_DOCK_METHOD)
      Result Init() {
        Reset();
        return ResetBuffers();
      }
#endif // #if(ALT_HIGH_BLOCK_DOCK_METHOD)
      
      void Reset()
      {
        mode_ = IDLE;
        DockingController::ResetDocker();
      }
      
      
      Result SendBlockPickUpMessage(const bool success)
      {
        Messages::BlockPickUp msg;
        msg.timestamp = HAL::GetTimeStamp();
        msg.didSucceed = success;
        if(HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::BlockPickUp), &msg)) {
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }
      
      
      Result Update()
      {
        Result retVal = RESULT_OK;
        
        switch(mode_)
        {
          case IDLE:
            break;
            
          case SET_LIFT_PREDOCK:
          {
#if(DEBUG_PAP_CONTROLLER)
            PRINT("PAP: SETTING LIFT PREDOCK (action %d)\n", action_);
#endif
            mode_ = MOVING_LIFT_PREDOCK;
            HeadController::SetDesiredAngle(LOW_DOCKING_HEAD_ANGLE);
            switch(action_) {
              case DA_PICKUP_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                dockOffsetDistX_ = ORIGIN_TO_LOW_LIFT_DIST_MM;
                break;
              case DA_PICKUP_HIGH:
                // This action starts by lowering the lift and tracking the high block
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                HeadController::SetDesiredAngle(HIGH_DOCKING_HEAD_ANGLE);
#if(ALT_HIGH_BLOCK_DOCK_METHOD)
                dockOffsetDistX_ = ORIGIN_TO_HIGH_PICKUP_HEAD_LOWERING_DIST_MM;
#else
                dockOffsetDistX_ = ORIGIN_TO_HIGH_LIFT_DIST_MM;
#endif  //#if(ALT_HIGH_BLOCK_DOCK_METHOD)
                break;
              case DA_PLACE_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                break;
              case DA_PLACE_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                dockOffsetDistX_ = ORIGIN_TO_HIGH_PLACEMENT_DIST_MM;
                break;
              default:
                PRINT("ERROR: Unknown PickAndPlaceAction %d\n", action_);
                mode_ = IDLE;
                break;
            }
            break;
          }

          case MOVING_LIFT_PREDOCK:
#if(DEBUG_PAP_CONTROLLER)
            PERIODIC_PRINT(200, "PAP: MLP %d %d\n", LiftController::IsInPosition(), HeadController::IsInPosition());
#endif
            if (LiftController::IsInPosition() && HeadController::IsInPosition()) {
              
              if (action_ == DA_PLACE_LOW) {
                DockingController::StartDockingToRelPose(dockOffsetDistX_,
                                                         dockOffsetDistY_,
                                                         dockOffsetAng_);
              } else {
                if (pixelSearchRadius_ < 0) {
                  DockingController::StartDocking(dockToMarker_,
                                                  markerWidth_,
                                                  dockOffsetDistX_,
                                                  dockOffsetDistY_,
                                                  dockOffsetAng_);
                } else {
                  DockingController::StartDocking(dockToMarker_,
                                                  markerWidth_,
                                                  markerCenter_,
                                                  pixelSearchRadius_,
                                                  dockOffsetDistX_,
                                                  dockOffsetDistY_,
                                                  dockOffsetAng_);
                }
              }
              mode_ = DOCKING;
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: DOCKING\n");
#endif
              
              if (action_ == DA_PICKUP_HIGH) {
                DockingController::TrackCamWithLift(true);
              }
            }
            break;
           
#if(ALT_HIGH_BLOCK_DOCK_METHOD)
          case MOVING_LIFT_HIGH_PREDOCK:
          {
            // Robot has now stopped a short distance away from the high block.
            // Assess block pose relative to robot.
            // Move head down and lift up.
            // Assess pose of low block relative to high block.
            // Compute appropriate offset docking trajectory.
            // Follow it.
            if (LiftController::IsInPosition() && HeadController::IsInPosition()) {
              

              // Compute docking path relative to low block such that it is aligned with high block.
              f32 angle_x, angle_y, angle_z;
              
              // Get the nearest marker closest to where we expect the bottom block to be.
              bool markerFound = false;
              if (VisionSystem::GetVisionMarkerPoseNearestTo(DockingController::GetLastGoodMarkerPt(),
                                                             dockToMarker_,  // TODO: Does this come from BS?
                                                             markerWidth_ * 2, // TODO...
                                                             rotLowBlockWrtRobot,
                                                             transLowBlockWrtRobot,
                                                             markerFound
                                                             ) != RESULT_OK)
              {
                PRINT("GetVisionMarkerPoseNearestTo() failed (low)\n");
                mode_ = IDLE;
                break;
              }
              
              if (!markerFound){
                PRINT("Expected low marker not found\n");
                mode_ = IDLE;
                break;
              }
              
#if(DEBUG_PAP_CONTROLLER)
              Embedded::Matrix::GetEulerAngles(rotLowBlockWrtRobot, angle_x, angle_y, angle_z);
              PRINT("rotLWrtRobot: %f %f %f  transLowWrtRobot: %f %f %f\n",
                    angle_x, angle_y, angle_z,
                    transLowBlockWrtRobot.x, transLowBlockWrtRobot.y, transLowBlockWrtRobot.z);
              rotLowBlockWrtRobot.Print();
              transLowBlockWrtRobot.Print();
#endif
              
              // Compute pose of high block with respect to low block P_HwrtL
              // P_HwrtL = (P_LWrtRobot)^-1 * P_HWrtRobot
              //
              // P1 = (P_LWrtRobot)^-1 = | R1'   -R1'*t1 |
              //                       |  0       1    |
              //
              // P2 = P_HWrtRobot = | R2  t2 |
              //                  |  0   1 |
              //
              // P_HwrtL = P1 * P2
              // P_HwrtL = | R1'*R2   R1'*t2 + (-R1'*t1) |
              //           |   0           1             |
              //

              Embedded::Matrix::Transpose(rotLowBlockWrtRobot, rotLowBlockWrtRobot_inv);
              Embedded::Point3<f32> transLowBlockWrtRobot_inv = (rotLowBlockWrtRobot_inv * transLowBlockWrtRobot);
              transLowBlockWrtRobot_inv *= -1;  // -R1'*t1
              
              
              PRINT("\nPose inv\n");
              rotLowBlockWrtRobot_inv.Print();
              transLowBlockWrtRobot_inv.Print();
              
              Embedded::Matrix::Multiply(rotLowBlockWrtRobot_inv, rotHighBlockWrtRobot, rotHighBlockWrtLowBlock);
              transHighBlockWrtLowBlock = (rotLowBlockWrtRobot_inv * transHighBlockWrtRobot) + transLowBlockWrtRobot_inv;
              
              PRINT("\nPose H wrt L\n");
              rotHighBlockWrtLowBlock.Print();
              transHighBlockWrtLowBlock.Print();

              if (Embedded::Matrix::GetEulerAngles(rotHighBlockWrtLowBlock, angle_x, angle_y, angle_z) != RESULT_OK) {
                PRINT("GetEulerAngles() failed\n");
                mode_ = IDLE;
                break;
              }
              
              PRINT("High wrt Low: rot %f %f %f, trans %f %f %f\n",
                    angle_x, angle_y, angle_z,
                    transHighBlockWrtLowBlock.x, transHighBlockWrtLowBlock.y, transHighBlockWrtLowBlock.z);

              // Check that values are sane
              if (angle_x > 0.2f || angle_y > 0.2f) {
                PRINT("**** WARNING: block is not level! Aborting dock. *****\n");
                mode_ = IDLE;
              }
              
              dockOffsetDistX_ = transHighBlockWrtLowBlock.x;
              dockOffsetDistY_ = transHighBlockWrtLowBlock.y;
              

              
              DockingController::StartDocking(dockToMarker_, markerWidth_, dockOffsetDistX_, dockOffsetDistY_, angle_z);
              mode_ = DOCKING;
              #if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: DOCKING WRT LOW\n");
              #endif
            }

            break;
          }
#endif // #if(ALT_HIGH_BLOCK_DOCK_METHOD)
            
          case DOCKING:
             
            if (!DockingController::IsBusy()) {

              DockingController::TrackCamWithLift(false);
              
              if (DockingController::DidLastDockSucceed()) {
                
                // Docking is complete
                mode_ = SET_LIFT_POSTDOCK;
#if(DEBUG_PAP_CONTROLLER)
                PRINT("PAP: SET_LIFT_POSTDOCK\n");
#endif
                
#if(ALT_HIGH_BLOCK_DOCK_METHOD)
                switch(action_) {
                  case DA_PICKUP_LOW:
                  case DA_PLACE_HIGH:
                    // Docking is complete
                    mode_ = SET_LIFT_POSTDOCK;
                    #if(DEBUG_PAP_CONTROLLER)
                    PRINT("PAP: SET_LIFT_POSTDOCK\n");
                    #endif
                    break;
                  case DA_PICKUP_HIGH:
                  {
                    // If head is at high dock angle, now we should start the second phase
                    // of the dock by tracking the lower block. Otherwise, dock approach is
                    // complete so now lift up the block.
                    if (LiftController::GetLastCommandedHeightMM() == LIFT_HEIGHT_LOWDOCK) {
                      
                      // Get high block pose
                      
                      // Get the nearest marker closest to the block we were just docking to, i.e. the high block.
                      bool markerFound = false;
                      if (VisionSystem::GetVisionMarkerPoseNearestTo(DockingController::GetLastGoodMarkerPt(),
                                                                     dockToMarker_,
                                                                     markerWidth_ * 2, // TODO: Something is wrong with this function! Shouldn't need to do this.
                                                                     rotHighBlockWrtRobot,
                                                                     transHighBlockWrtRobot,
                                                                     markerFound
                                                                     ) != RESULT_OK)
                      {
                        PRINT("GetVisionMarkerPoseNearestTo() failed (high)\n");
                        mode_ = IDLE;
                        break;
                      }
                      
                      if (!markerFound){
                        PRINT("Expected high marker not found\n");
                        mode_ = IDLE;
                        break;
                      }
                      
                      
                      LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK);
                      HeadController::SetDesiredAngle(LOW_DOCKING_HEAD_ANGLE);
                      mode_ = MOVING_LIFT_HIGH_PREDOCK;
                      #if(DEBUG_PAP_CONTROLLER)
                      f32 ax, ay, az;
                      Embedded::Matrix::GetEulerAngles(rotHighBlockWrtRobot, ax, ay, az);
                      PRINT("rotHWrtRobot: %f %f %f  transHighWrtRobot: %f %f %f\n",
                            ax, ay, az,
                            transHighBlockWrtRobot.x, transHighBlockWrtRobot.y, transHighBlockWrtRobot.z);
                      rotHighBlockWrtRobot.Print();
                      transHighBlockWrtRobot.Print();
                      
                      PRINT("PAP: MOVING_LIFT_HIGH_PREDOCK\n");
                      #endif
                    } else {
                      // Docking is complete
                      mode_ = SET_LIFT_POSTDOCK;
                      #if(DEBUG_PAP_CONTROLLER)
                      PRINT("PAP: SET_LIFT_POSTDOCK (%f)\n", HeadController::GetLastCommandedAngle());
                      #endif
                      break;
                    }
                    break;
                  }
                  default:
                    PRINT("ERROR: Unknown PickAndPlaceAction %d\n", action_);
                    mode_ = IDLE;
                    break;
                }
#endif // #if(ALT_HIGH_BLOCK_DOCK_METHOD)
                
              } else {
                // Block is not being tracked.
                // Probably not visible.
#if(DEBUG_PAP_CONTROLLER)
                PRINT("WARN (PickAndPlaceController): Could not track block's marker\n");
#endif
                // TODO: Send BTLE message notifying failure
                mode_ = IDLE;
              }
            }
            break;

          case SET_LIFT_POSTDOCK:
          {
#if(DEBUG_PAP_CONTROLLER)
            PRINT("PAP: SETTING LIFT POSTDOCK\n");
#endif
            mode_ = MOVING_LIFT_POSTDOCK;
            switch(action_) {
              case DA_PLACE_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                break;
              case DA_PICKUP_LOW:
              case DA_PICKUP_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                break;
              case DA_PLACE_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK);
                break;
              default:
                PRINT("ERROR: Unknown PickAndPlaceAction %d\n", action_);
                mode_ = IDLE;
                break;
            }
            break;
          }
            
          case MOVING_LIFT_POSTDOCK:
            if (LiftController::IsInPosition()) {
              switch(action_) {
                case DA_PICKUP_LOW:
                  // TODO: Add visual verification of pickup here?
                  mode_ = IDLE;
                  lastActionSucceeded_ = true;
                  isCarryingBlock_ = true;
                  SendBlockPickUpMessage(true);
                  break;
                  
                case DA_PICKUP_HIGH:
                  // TODO: Add visual verification of pickup here?
                  isCarryingBlock_ = true;
                  SendBlockPickUpMessage(true);
                  // Note this falls through to next case!
                case DA_PLACE_LOW:
                case DA_PLACE_HIGH:
                  SteeringController::ExecuteDirectDrive(BACKOUT_SPEED_MMPS, BACKOUT_SPEED_MMPS);
                  transitionTime_ = HAL::GetMicroCounter() + BACKOUT_TIME;
                  mode_ = BACKOUT;
#if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: BACKING OUT\n");
#endif
                  break;
              }
            }
            break;
            
          case BACKOUT:
            if (HAL::GetMicroCounter() > transitionTime_) {

              SteeringController::ExecuteDirectDrive(0,0);
              
              switch(action_) {
                case DA_PLACE_LOW:
                case DA_PICKUP_LOW:
                case DA_PICKUP_HIGH:
                  mode_ = IDLE;
                  lastActionSucceeded_ = true;
                  isCarryingBlock_ = true;
                  break;
                case DA_PLACE_HIGH:
                  LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                  mode_ = LOWER_LIFT;
                  #if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: LOWERING LIFT\n");
                  #endif
                  break;
              }
            }
            break;
          case LOWER_LIFT:
            if (LiftController::IsInPosition()) {
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: IDLE\n");
#endif
              mode_ = IDLE;
              lastActionSucceeded_ = true;
              isCarryingBlock_ = false;
            }
            break;
            
          default:
            mode_ = IDLE;
            PRINT("Reached default case in DockingController "
                  "mode switch statement.(1)\n");
            break;
        }
        
        return retVal;
        
      } // Update()

      bool DidLastActionSucceed() {
        return lastActionSucceeded_;
      }

      bool IsBusy()
      {
        return mode_ != IDLE;
      }

      bool IsCarryingBlock()
      {
        return isCarryingBlock_;
      }
      
      
      void DockToBlock(const Vision::MarkerType blockMarker,
                       const f32 markerWidth_mm,
                       const DockAction_t action)
      {
#if(DEBUG_PAP_CONTROLLER)
        PRINT("PAP: DOCK TO BLOCK %d (action %d)\n", blockMarker, action);
#endif
				#warning fix me!
        /*if (action == DA_PLACE_LOW) {
          PRINT("Invalid action %d for DockToBlock()\n", action);
          return;
        }*/
        
        action_ = action;
        dockToMarker_ = blockMarker;
        markerWidth_  = markerWidth_mm;
        
        markerCenter_.x = -1.f;
        markerCenter_.y = -1.f;
        pixelSearchRadius_ = -1.f;
        
        dockOffsetDistX_ = 0;
        dockOffsetDistY_ = 0;
        dockOffsetAng_ = 0;
        
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;
      }
      
      void DockToBlock(const Vision::MarkerType blockMarker,
                       const f32 markerWidth_mm,
                       const Embedded::Point2f& markerCenter,
                       const f32 pixelSearchRadius,
                       const DockAction_t action)
      {
        DockToBlock(blockMarker, markerWidth_mm, action);
        
        markerCenter_ = markerCenter;
        pixelSearchRadius_ = pixelSearchRadius;
      }
      
      void PlaceOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle)
      {
        action_ = DA_PLACE_LOW;
        dockOffsetDistX_ = rel_x;
        dockOffsetDistY_ = rel_y;
        dockOffsetAng_ = rel_angle;
        
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;
      }
      
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki
