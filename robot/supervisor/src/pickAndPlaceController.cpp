#include "anki/common/robot/config.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"
#include "localization.h"
#include "imuFilter.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "speedController.h"
#include "steeringController.h"
#include "visionSystem.h"


#define DEBUG_PAP_CONTROLLER 0

// Whether or not to use "snapshots" for pick and place verification
#define USE_SNAPSHOT_VERIFICATION 0

// If you enable this, make sure image streaming is off
// and you enable the print-to-file code in the ImageChunk message handler
// on the basestation.
#define SEND_PICKUP_VERIFICATION_SNAPSHOTS 0


namespace Anki {
  namespace Cozmo {
    namespace PickAndPlaceController {
      
      namespace {
        
        // Constants
        
        // TODO: Need to be able to specify wheel motion by distance
        //const u32 BACKOUT_TIME = 1500000;
        //

        // The distance from the last-observed position of the target that we'd
        // like to be after backing out
        const f32 BACKOUT_DISTANCE_MM = 60.f;
        const f32 BACKOUT_SPEED_MMPS = 40;
        
        const f32 RAMP_TRAVERSE_SPEED_MMPS = 40;
        const f32 ON_RAMP_ANGLE_THRESH = 0.15;
        const f32 OFF_RAMP_ANGLE_THRESH = 0.05;
        
        const f32 BRIDGE_TRAVERSE_SPEED_MMPS = 40;
        
        const f32 LOW_DOCKING_HEAD_ANGLE  = DEG_TO_RAD_F32(-18);
        const f32 HIGH_DOCKING_HEAD_ANGLE = DEG_TO_RAD_F32(18);

        Mode mode_ = IDLE;
        
        DockAction_t action_ = DA_PICKUP_LOW;

        Embedded::Point2f ptStamp_;
        Radians angleStamp_;
        
        Vision::MarkerType dockToMarker_;
        Vision::MarkerType dockToMarker2_;  // 2nd marker used for bridge crossing only
        f32 markerWidth_ = 0;
        f32 dockOffsetDistX_ = 0;
        f32 dockOffsetDistY_ = 0;
        f32 dockOffsetAng_ = 0;
        
        // Last seen marker pose used for bridge crossing
        f32 relMarkerX_, relMarkerY_, relMarkerAng_;
        
        // Expected location of the desired dock marker in the image.
        // If not specified, marker may be located anywhere in the image.
        Embedded::Point2f markerCenter_;
        f32 pixelSearchRadius_;
        
        bool isCarryingBlock_ = false;
        bool lastActionSucceeded_ = false;
        
        // When to transition to the next state. Only some states use this.
        u32 transitionTime_ = 0;
        
        // Whether or not docking path should be traversed with manually controlled speed
        bool useManualSpeed_ = false;
        
#if USE_SNAPSHOT_VERIFICATION
        // "Snapshots" for visual verification of a successful block pick up
        // We'll need two mini-images (or snapshots), along with an associated
        // memory buffer, to hold low-res views from the camera before and after
        // trying to pickup (or place) a block.  The ready flag is so the state
        // machine can wait for the vision system to actually produce the
        // snapshot, since it's running on a separate (slower) "thread".
        bool isSnapshotReady_ = false;
        Embedded::Array<u8> pickupSnapshotBefore_;
        Embedded::Array<u8> pickupSnapshotAfter_;
        const s32 SNAPSHOT_SIZE = 16; // the snapshots will be a 2D array SNAPSHOT_SIZE x SNAPSHOT_SIZE in size
                                      // NOTE: Make sure this matches CAMERA_RES_VERIFICATION_SNAPSHOT
        const s32 SNAPSHOT_SUBSAMPLE = 8; // this is the spacing between samples taken from the original image resolution
        const s32 SNAPSHOT_ROI_SIZE = SNAPSHOT_SUBSAMPLE*SNAPSHOT_SIZE; // thus, this is the size of the ROI in the original image
        const Embedded::Rectangle<s32> snapShotRoiLow_((320-SNAPSHOT_ROI_SIZE)/2, (320+SNAPSHOT_ROI_SIZE)/2, (240-SNAPSHOT_ROI_SIZE)/2, (240+SNAPSHOT_ROI_SIZE)/2);
        const Embedded::Rectangle<s32> snapShotRoiHigh_((320-SNAPSHOT_ROI_SIZE)/2, (320+SNAPSHOT_ROI_SIZE)/2, (240-SNAPSHOT_ROI_SIZE)/2, (240+SNAPSHOT_ROI_SIZE)/2);
        const s32 SNAPSHOT_BUFFER_SIZE = 2*SNAPSHOT_SIZE*SNAPSHOT_SIZE + 64; // 2X (16x16) arrays + overhead
        u8 snapshotBuffer_[SNAPSHOT_BUFFER_SIZE];
        Embedded::MemoryStack snapshotMemory_;
        const s32 SNAPSHOT_PER_PIXEL_COMPARE_THRESHOLD = 100; //SNAPSHOT_SIZE*SNAPSHOT_SIZE*64*64; // average grayscale difference of 64
        const s32 SNAPSHOT_COMPARE_THRESHOLD = SNAPSHOT_PER_PIXEL_COMPARE_THRESHOLD*SNAPSHOT_PER_PIXEL_COMPARE_THRESHOLD*SNAPSHOT_SIZE*SNAPSHOT_SIZE;
        
        // WARNING: ResetBuffers should be used with caution
        Result ResetBuffers()
        {
          snapshotMemory_ = Embedded::MemoryStack(snapshotBuffer_, SNAPSHOT_BUFFER_SIZE);
          AnkiConditionalErrorAndReturnValue(snapshotMemory_.IsValid(),
                                             RESULT_FAIL_MEMORY,
                                             "PAP::ResetBuffers()", "Failed to initialize snapshotMemory!\n")
          
          pickupSnapshotBefore_ = Embedded::Array<u8>(SNAPSHOT_SIZE, SNAPSHOT_SIZE, snapshotMemory_);
          AnkiConditionalErrorAndReturnValue(pickupSnapshotBefore_.IsValid(),
                                             RESULT_FAIL_MEMORY,
                                             "PAP::ResetBuffers()", "Failed to allocate pickupSnapshotBefore_!\n")
          
          pickupSnapshotAfter_ = Embedded::Array<u8>(SNAPSHOT_SIZE, SNAPSHOT_SIZE, snapshotMemory_);
          AnkiConditionalErrorAndReturnValue(pickupSnapshotAfter_.IsValid(),
                                             RESULT_FAIL_MEMORY,
                                             "PAP::ResetBuffers()", "Failed to allocate pickupSnapshotAfter_!\n")
          
          return RESULT_OK;
          
        } // ResetBuffers()
        
#endif // USE_SNAPSHOT_VERIFICATION
        
      } // "private" namespace
      
      
      Mode GetMode() {
        return mode_;
      }

      Result Init() {
        Reset();
#if USE_SNAPSHOT_VERIFICATION
        AnkiConditionalErrorAndReturnValue(snapShotRoiLow_.get_width()  == SNAPSHOT_SIZE*SNAPSHOT_SUBSAMPLE &&
                                           snapShotRoiLow_.get_height() == SNAPSHOT_SIZE*SNAPSHOT_SUBSAMPLE,
                                           RESULT_FAIL_INVALID_SIZE, "PAP::Init()",
                                           "Snapshot ROI Low not the expected size (%dx%d vs. %dx%d).",
                                           snapShotRoiLow_.get_width(), snapShotRoiLow_.get_height(),
                                           SNAPSHOT_SIZE, SNAPSHOT_SIZE);
        
        AnkiConditionalErrorAndReturnValue(snapShotRoiHigh_.get_width()  == SNAPSHOT_SIZE*SNAPSHOT_SUBSAMPLE &&
                                           snapShotRoiHigh_.get_height() == SNAPSHOT_SIZE*SNAPSHOT_SUBSAMPLE,
                                           RESULT_FAIL_INVALID_SIZE, "PAP::Init()",
                                           "Snapshot ROI High not the expected size (%dx%d vs. %dx%d).",
                                           snapShotRoiHigh_.get_width(), snapShotRoiHigh_.get_height(),
                                           SNAPSHOT_SIZE, SNAPSHOT_SIZE);
        
        return ResetBuffers();
#else 
        return RESULT_OK;
#endif // USE_SNAPSHOT_VERIFICATION

      }

      
      void Reset()
      {
        mode_ = IDLE;
        DockingController::ResetDocker();
      }

      void UpdatePoseSnapshot()
      {
        Localization::GetCurrentMatPose(ptStamp_.x, ptStamp_.y, angleStamp_);
      }
      
      Result SendBlockPickUpMessage(const bool success)
      {
        Messages::BlockPickedUp msg;
        msg.timestamp = HAL::GetTimeStamp();
        msg.didSucceed = success;
        if(HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::BlockPickedUp), &msg)) {
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }
      
      Result SendBlockPlacedMessage(const bool success)
      {
        Messages::BlockPlaced msg;
        msg.timestamp = HAL::GetTimeStamp();
        msg.didSucceed = success;
        if(HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::BlockPlaced), &msg)) {
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }

#if USE_SNAPSHOT_VERIFICATION
      // Since auto exposure settings may not have stabilized by this point,
      // just do a software normalize by raising the brightest pixel to 255.
      // The most typical failure case seems to be that the before image
      // is darker than than the after since the robot casts a shadow on the
      // block when it first reaches it.
      static void NormalizeSnapshots(void)
      {
        const s32 nrows = pickupSnapshotBefore_.get_size(0);
        const s32 ncols = pickupSnapshotBefore_.get_size(1);
        
        AnkiAssert(pickupSnapshotAfter_.get_size(0) == nrows &&
                   pickupSnapshotAfter_.get_size(1) == ncols);
        
        #if(SEND_PICKUP_VERIFICATION_SNAPSHOTS)
        // Send snapshots (for debug)
        Messages::ImageChunk b[4];
        Messages::ImageChunk a[4];
        for(u8 i=0;i<4; ++i) {
          b[i].resolution = Vision::CAMERA_RES_VERIFICATION_SNAPSHOT;
          b[i].imageId = 100;
          b[i].chunkId = i;
          b[i].chunkSize = MIN(80, (nrows * ncols) - (80*i));
          
          a[i].resolution = Vision::CAMERA_RES_VERIFICATION_SNAPSHOT;
          a[i].imageId = 101;
          a[i].chunkId = i;
          a[i].chunkSize = MIN(80, (nrows * ncols) - (80*i));
        }
        u8 byteCnt = 0;
        #endif // SEND_PICKUP_VERIFICATION_SNAPSHOTS
        
        u8 maxValBefore = 0;
        u8 maxValAfter = 0;
        for(s32 i=0; i<nrows; ++i)
        {
          const u8 * restrict pBefore = pickupSnapshotBefore_.Pointer(i,0);
          const u8 * restrict pAfter  = pickupSnapshotAfter_.Pointer(i,0);

          for(s32 j=0; j<ncols; ++j)
          {
            if (pBefore[j] > maxValBefore) {
              maxValBefore = pBefore[j];
            }
            
            if (pAfter[j] > maxValAfter) {
              maxValAfter = pAfter[j];
            }
            
            #if(SEND_PICKUP_VERIFICATION_SNAPSHOTS)
            b[byteCnt / 80].data[byteCnt % 80] = pBefore[j];
            a[byteCnt / 80].data[byteCnt % 80] = pAfter[j];
            byteCnt++;
            #endif // SEND_PICKUP_VERIFICATION_SNAPSHOTS
            
          }
        }

        #if(SEND_PICKUP_VERIFICATION_SNAPSHOTS)
        for(u8 i=0; i<4; ++i) {
          HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::ImageChunk), &b[i]);
        }
        for(u8 i=0; i<4; ++i) {
          HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::ImageChunk), &a[i]);
        }
        #endif //SEND_PICKUP_VERIFICATION_SNAPSHOTS

        
        if (maxValBefore == 0) {
          PRINT("WARNING: NormalizeSnapShots Before image is blank!\n");
          return;
        }
        if (maxValAfter == 0) {
          PRINT("WARNING: NormalizeSnapShots After image is blank!\n");
          return;
        }
        
        f32 normScaleBefore = 255.f / maxValBefore;
        f32 normScaleAfter = 255.f / maxValAfter;
        
        PRINT("normScaleBefore: %f, normScaleAfter: %f\n", normScaleBefore, normScaleAfter);
        
        for(s32 i=0; i<nrows; ++i)
        {
          u8 * restrict pBefore = pickupSnapshotBefore_.Pointer(i,0);
          u8 * restrict pAfter  = pickupSnapshotAfter_.Pointer(i,0);
          
          for(s32 j=0; j<ncols; ++j)
          {
            pBefore[j] = static_cast<u8>(pBefore[j] * normScaleBefore);
            pAfter[j] = static_cast<u8>(pAfter[j] * normScaleAfter);
          }
        }
      }
      
      // Return sum-squared-difference between the before and after snapshots
      static s32 CompareSnapshots(void)
      {
        const s32 nrows = pickupSnapshotBefore_.get_size(0);
        const s32 ncols = pickupSnapshotBefore_.get_size(1);
        
        AnkiAssert(pickupSnapshotAfter_.get_size(0) == nrows &&
                   pickupSnapshotAfter_.get_size(1) == ncols);
        
        NormalizeSnapshots();
        
        s32 ssd = 0;
        for(s32 i=0; i<nrows; ++i)
        {
          const u8 * restrict pBefore = pickupSnapshotBefore_.Pointer(i,0);
          const u8 * restrict pAfter  = pickupSnapshotAfter_.Pointer(i,0);
          
          for(s32 j=0; j<ncols; ++j)
          {
            const s32 diff = static_cast<s32>(pAfter[j]) - static_cast<s32>(pBefore[j]);
            ssd += diff*diff;
          }
        }
        
        //pickupSnapshotBefore_.Show("Snapshot Before", false);
        //pickupSnapshotAfter_.Show("Snapshot After", true);
        
        return ssd;
        
      } // CompareSnapshots()
      
#endif // USE_SNAPSHOT_VERIFICATION
      
      static void StartBackingOut()
      {
        static const f32 MIN_BACKOUT_DIST = 10.f;
        
        DockingController::GetLastMarkerPose(relMarkerX_, relMarkerY_, relMarkerAng_);
        
        f32 backoutDist_mm = MIN_BACKOUT_DIST;
        if(relMarkerX_ <= BACKOUT_DISTANCE_MM) {
          backoutDist_mm = MAX(MIN_BACKOUT_DIST, BACKOUT_DISTANCE_MM - relMarkerX_);
        }
        
        const f32 backoutTime_sec = backoutDist_mm / BACKOUT_SPEED_MMPS;

        PRINT("PAP: Last marker dist = %.1fmm. Starting %.1fmm backout (%.2fsec duration)\n",
              relMarkerX_, backoutDist_mm, backoutTime_sec);
        
        transitionTime_ = HAL::GetMicroCounter() + (backoutTime_sec*1e6);
        
        SteeringController::ExecuteDirectDrive(-BACKOUT_SPEED_MMPS, -BACKOUT_SPEED_MMPS);
        
        mode_ = BACKOUT;
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
            switch(action_) {
              case DA_PICKUP_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                HeadController::SetDesiredAngle(LOW_DOCKING_HEAD_ANGLE);
                dockOffsetDistX_ = ORIGIN_TO_LOW_LIFT_DIST_MM;
                break;
              case DA_PICKUP_HIGH:
                // This action starts by lowering the lift and tracking the high block
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                HeadController::SetDesiredAngle(HIGH_DOCKING_HEAD_ANGLE);
                dockOffsetDistX_ = ORIGIN_TO_HIGH_LIFT_DIST_MM;
                break;
              case DA_PLACE_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                HeadController::SetDesiredAngle(LOW_DOCKING_HEAD_ANGLE);
                break;
              case DA_PLACE_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                HeadController::SetDesiredAngle(LOW_DOCKING_HEAD_ANGLE);
                dockOffsetDistX_ = ORIGIN_TO_HIGH_PLACEMENT_DIST_MM;
                break;
              case DA_RAMP_ASCEND:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                HeadController::SetDesiredAngle(LOW_DOCKING_HEAD_ANGLE);
                dockOffsetDistX_ = 0;
                break;
              case DA_RAMP_DESCEND:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                HeadController::SetDesiredAngle(MIN_HEAD_ANGLE);
                dockOffsetDistX_ = 30; // can't wait until we are actually on top of the marker to say we're done!
                break;
              case DA_CROSS_BRIDGE:
                HeadController::SetDesiredAngle(MIN_HEAD_ANGLE);
                dockOffsetDistX_ = BRIDGE_ALIGNED_MARKER_DISTANCE;
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
                                                         dockOffsetAng_,
                                                         useManualSpeed_);
              } else {
                // When we are "docking" with a ramp or crossing a bridge, we
                // don't want to worry about the X angle being large (since we
                // _expect_ it to be large, since the markers are facing upward).
                const bool checkAngleX = !(action_ == DA_RAMP_ASCEND || action_ == DA_RAMP_DESCEND || action_ == DA_CROSS_BRIDGE);
                
                if (pixelSearchRadius_ < 0) {
                  DockingController::StartDocking(dockToMarker_,
                                                  markerWidth_,
                                                  dockOffsetDistX_,
                                                  dockOffsetDistY_,
                                                  dockOffsetAng_,
                                                  checkAngleX,
                                                  useManualSpeed_);
                } else {
                  DockingController::StartDocking(dockToMarker_,
                                                  markerWidth_,
                                                  markerCenter_,
                                                  pixelSearchRadius_,
                                                  dockOffsetDistX_,
                                                  dockOffsetDistY_,
                                                  dockOffsetAng_,
                                                  checkAngleX,
                                                  useManualSpeed_);
                }
              }
              mode_ = DOCKING;
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: DOCKING\n");
#endif
              
              //if (action_ == DA_PICKUP_HIGH) {
              //  DockingController::TrackCamWithLift(true);
              //}
            }
            break;
            
          case DOCKING:
             
            if (!DockingController::IsBusy()) {

              //DockingController::TrackCamWithLift(false);
              
              if (DockingController::DidLastDockSucceed())
              {
                // Docking is complete
                DockingController::ResetDocker();
                
                // Take snapshot of pose
                UpdatePoseSnapshot();
                
                if(action_ == DA_RAMP_DESCEND) {
                  #if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: TRAVERSE_RAMP_DOWN\n");
                  #endif
                  // Start driving forward (blindly) -- wheel guides!
                  SteeringController::ExecuteDirectDrive(RAMP_TRAVERSE_SPEED_MMPS, RAMP_TRAVERSE_SPEED_MMPS);
                  mode_ = TRAVERSE_RAMP_DOWN;
                } else if (action_ == DA_CROSS_BRIDGE) {
                  #if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: ENTER_BRIDGE\n");
                  #endif
                  // Start driving forward (blindly) -- wheel guides!
                  SteeringController::ExecuteDirectDrive(BRIDGE_TRAVERSE_SPEED_MMPS, BRIDGE_TRAVERSE_SPEED_MMPS);
                  mode_ = ENTER_BRIDGE;
                } else {
                  #if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: SET_LIFT_POSTDOCK\n");
                  #endif
                  mode_ = SET_LIFT_POSTDOCK;
                }
              } else {
                // Block is not being tracked.
                // Probably not visible.
                #if(DEBUG_PAP_CONTROLLER)
                PRINT("WARN (PickAndPlaceController): Could not track block's marker\n");
                #endif
                // TODO: Send BTLE message notifying failure
                
                PRINT("PAP: Docking failed while picking/placing high or low. Backing out.\n");
                VisionSystem::StopTracking();
                
                // Switch to BACKOUT mode:
                StartBackingOut();
                //mode_ = IDLE;
              }
            }
            else if (action_ == DA_RAMP_ASCEND && (ABS(IMUFilter::GetPitch()) > ON_RAMP_ANGLE_THRESH) )
            {
              DockingController::ResetDocker();
              SteeringController::ExecuteDirectDrive(RAMP_TRAVERSE_SPEED_MMPS, RAMP_TRAVERSE_SPEED_MMPS);
              mode_ = TRAVERSE_RAMP;
              Localization::SetOnRamp(true);
            }
            break;

          case SET_LIFT_POSTDOCK:
          {
#if(DEBUG_PAP_CONTROLLER)
            PRINT("PAP: SETTING LIFT POSTDOCK\n");
#endif
            switch(action_) {
              case DA_PLACE_LOW:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                mode_ = MOVING_LIFT_POSTDOCK;
                break;
              }
              case DA_PICKUP_LOW:
              case DA_PICKUP_HIGH:
              {
#if USE_SNAPSHOT_VERIFICATION
                // Take a snapshot before we try to pick up. We will compare a
                // post-pick up snapshot to this one to verify whether we
                // actually picked up the block
                if(isSnapshotReady_) {
                  LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                  isSnapshotReady_ = false;
                  mode_ = MOVING_LIFT_POSTDOCK;
                } else {
                  const Embedded::Rectangle<s32>* roi = &snapShotRoiLow_;
                  if(action_ == DA_PICKUP_HIGH) {
                    roi = &snapShotRoiHigh_;
                  }
                  Result lastResult = VisionSystem::TakeSnapshot(*roi, SNAPSHOT_SUBSAMPLE, pickupSnapshotBefore_, isSnapshotReady_);
                  if(lastResult != RESULT_OK) {
                    PRINT("ERROR: PickAndPlaceController: TakeSnapshot() failed in SET_LIFT_POSTDOCK:DA_PICKUP_LOW/HIGH!\n");
                    mode_ = IDLE;
                    return lastResult;
                  }
                }
#else
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                mode_ = MOVING_LIFT_POSTDOCK;
#endif // USE_SNAPSHOT_VERIFICATION
                break;
              }
                
              case DA_PLACE_HIGH:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK);
                mode_ = MOVING_LIFT_POSTDOCK;
                break;
              }
              default:
                PRINT("ERROR: Unknown PickAndPlaceAction %d\n", action_);
                mode_ = IDLE;
                break;
            }
            break;
          }
            
          case MOVING_LIFT_POSTDOCK:
            if (LiftController::IsInPosition()) {
#if USE_SNAPSHOT_VERIFICATION
              switch(action_) {
                case DA_PICKUP_LOW:
                  // Wait for visual verification
                  // TODO: add timeout?
                  if(isSnapshotReady_) {
                    mode_ = IDLE;
                    lastActionSucceeded_ = true;
                    
                    // Snapshots should differ if we actually lifted the block
                    const s32 SSD = CompareSnapshots();
                    PRINT("PickAndPlaceController: snapshot difference SSD = %d\n", SSD);
                    if(SSD > SNAPSHOT_COMPARE_THRESHOLD) {
                      isCarryingBlock_ = true;
                    } else {
                      isCarryingBlock_ = false;
                    }
                    isSnapshotReady_ = false;
                    SendBlockPickUpMessage(isCarryingBlock_);
                  } else {
                    Result lastResult = VisionSystem::TakeSnapshot(snapShotRoiLow_, SNAPSHOT_SUBSAMPLE, pickupSnapshotAfter_, isSnapshotReady_);
                    if(lastResult != RESULT_OK) {
                      PRINT("ERROR: PickAndPlaceController: TakeSnapshot() failed in MOVING_LIFT_POSTDOCK:DA_PICKUP_LOW!\n");
                      mode_ = IDLE;
                      return lastResult;
                    }
                  }
                  
                  break;
                  
                case DA_PICKUP_HIGH:
                  if(isSnapshotReady_) {
                    // Snapshots should differ if we actually lifted the block
                    const s32 SSD = CompareSnapshots();
                    if(SSD > SNAPSHOT_COMPARE_THRESHOLD) {
                      isCarryingBlock_ = true;
                    } else {
                      isCarryingBlock_ = false;
                    }
                    isSnapshotReady_ = false;
                    SendBlockPickUpMessage(isCarryingBlock_);
                    
                    // For now we backout even if the pickup failed, but maybe
                    // we wanna do something different
                    StartBackingOut();
                  } else {
                    Result lastResult = VisionSystem::TakeSnapshot(snapShotRoiHigh_, SNAPSHOT_SUBSAMPLE, pickupSnapshotAfter_, isSnapshotReady_);
                    if(lastResult != RESULT_OK) {
                      PRINT("ERROR: PickAndPlaceController: TakeSnapshot() failed in MOVING_LIFT_POSTDOCK:DA_PICKUP_HIGH!\n");
                      mode_ = IDLE;
                      return lastResult;
                    }
                    
                  }
                  break;
                  
                case DA_PLACE_LOW:
                case DA_PLACE_HIGH:
                  // TODO: Add visual verfication of placement here?
                  isCarryingBlock_ = false;
                  SendBlockPlacedMessage(true);
                  StartBackingOut();
#if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: BACKING OUT\n");
#endif
                  break;
                  
                default:
                  PRINT("ERROR: Reached default switch statement in MOVING_LIFT_POSTDOCK case.\n");
              }
#else 
              // If not using snapshots, just backup after picking or placing,
              // and move the head to a position to admire our work
              
              // Set head angle
              switch(action_)
              {
                case DA_PICKUP_HIGH:
                case DA_PLACE_HIGH:
                {
                  HeadController::SetDesiredAngle(DEG_TO_RAD(20));
                  break;
                } // HIGH
                case DA_PICKUP_LOW:
                case DA_PLACE_LOW:
                {
                  HeadController::SetDesiredAngle(DEG_TO_RAD(-5));
                  break;
                } // LOW
                default:
                  PRINT("ERROR: Reached default switch statement in MOVING_LIFT_POSTDOCK case.\n");
              } // switch(action_)
              
              // Send pickup or place message.  Assume success, let BaseStation
              // verify once we've backed out.
              switch(action_)
              {
                case DA_PICKUP_LOW:
                case DA_PICKUP_HIGH:
                {
                  SendBlockPickUpMessage(true);
                  break;
                } // PICKUP

                case DA_PLACE_LOW:
                case DA_PLACE_HIGH:
                {
                  SendBlockPlacedMessage(true);
                  break;
                } // PLACE
                default:
                  PRINT("ERROR: Reached default switch statement in MOVING_LIFT_POSTDOCK case.\n");
              } // switch(action_)
              
              // Switch to BACKOUT
              StartBackingOut();
              
#endif // USE_SNAPSHOT_VERIFICATION
            } // if (LiftController::IsInPosition())
            break;
            
          case BACKOUT:
            /*
            if(DockingController::IsBusy()) {
              // If the docking controller was not reset before switching to
              // backout phase, and we reacquire track, try re-docking
              PRINT("PAP: Reacquired tracking target while backing out. Retrying.\n");
              mode_ = MOVING_LIFT_PREDOCK;
            }
             
            else */if (HAL::GetMicroCounter() > transitionTime_)
            {
              SteeringController::ExecuteDirectDrive(0,0);
              
              if (HeadController::IsInPosition()) {
                switch(action_) {
                  case DA_PLACE_LOW:
                  case DA_PICKUP_LOW:
                  case DA_PICKUP_HIGH:
                    mode_ = IDLE;
                    lastActionSucceeded_ = true;
                    //isCarryingBlock_ = true;
                    break;
                  case DA_PLACE_HIGH:
                    LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                    mode_ = LOWER_LIFT;
                    #if(DEBUG_PAP_CONTROLLER)
                    PRINT("PAP: LOWERING LIFT\n");
                    #endif
                    break;
                  default:
                    PRINT("ERROR: Reached BACKUP unexpectedly (action = %d)\n", action_);
                    mode_ = IDLE;
                    break;
                }
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
            
          case TRAVERSE_RAMP_DOWN:
            if(IMUFilter::GetPitch() < -ON_RAMP_ANGLE_THRESH) {
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: Switching out of TRAVERSE_RAMP_DOWN to TRAVERSE_RAMP (angle = %f)\n", IMUFilter::GetPitch());
#endif
              Localization::SetOnRamp(true);
              mode_ = TRAVERSE_RAMP;
            }
            break;
            
          case TRAVERSE_RAMP:
            if ( ABS(IMUFilter::GetPitch()) < OFF_RAMP_ANGLE_THRESH ) {
              SteeringController::ExecuteDirectDrive(0, 0);
              #if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: IDLE (from TRAVERSE_RAMP)\n");
              #endif
              mode_ = IDLE;
              lastActionSucceeded_ = true;
              Localization::SetOnRamp(false);
            }
            break;
            
          case ENTER_BRIDGE:
            // Keep driving until the marker on the other side of the bridge is seen.
            if ( Localization::GetDistTo(ptStamp_.x, ptStamp_.y) > BRIDGE_ALIGNED_MARKER_DISTANCE) {
              // Set vision marker to look for marker
              DockingController::StartTrackingOnly(dockToMarker2_, markerWidth_);
              UpdatePoseSnapshot();
              mode_ = TRAVERSE_BRIDGE;
              #if(DEBUG_PAP_CONTROLLER)
              PRINT("TRAVERSE_BRIDGE\n");
              #endif
              Localization::SetOnBridge(true);
            }
            break;
          case TRAVERSE_BRIDGE:
            if (DockingController::IsBusy()) {
              if (DockingController::GetLastMarkerPose(relMarkerX_, relMarkerY_, relMarkerAng_) && relMarkerX_ < 100.f) {
                // We're tracking the end marker.
                // Keep driving until we're off.
                UpdatePoseSnapshot();
                mode_ = LEAVE_BRIDGE;
                #if(DEBUG_PAP_CONTROLLER)
                PRINT("LEAVING_BRIDGE: relMarkerX = %f\n", relMarkerX_);
                #endif
              }
            } else {
              // Marker tracking timedout. Start it again.
              DockingController::StartTrackingOnly(dockToMarker2_, markerWidth_);
              #if(DEBUG_PAP_CONTROLLER)
              PRINT("TRAVERSE_BRIDGE: Restarting tracking\n");
              #endif
            }
            break;
          case LEAVE_BRIDGE:
            if ( Localization::GetDistTo(ptStamp_.x, ptStamp_.y) > relMarkerX_ + MARKER_TO_OFF_BRIDGE_POSE_DIST) {
              SteeringController::ExecuteDirectDrive(0, 0);
              #if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: IDLE (from TRAVERSE_BRIDGE)\n");
              #endif
              mode_ = IDLE;
              lastActionSucceeded_ = true;
              Localization::SetOnBridge(false);
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
      
      
      void DockToBlock(const Vision::MarkerType markerType,
                       const Vision::MarkerType markerType2,
                       const f32 markerWidth_mm,
                       const bool useManualSpeed,
                       const DockAction_t action)
      {
#if(DEBUG_PAP_CONTROLLER)
        PRINT("PAP: DOCK TO BLOCK %d (action %d)\n", markerType, action);
#endif

        if (action == DA_PLACE_LOW) {
          PRINT("WARNING: Invalid action %d for DockToBlock(). Ignoring.\n", action);
          return;
        }
        
        action_ = action;
        dockToMarker_ = markerType;
        dockToMarker2_ = markerType2;
        markerWidth_  = markerWidth_mm;
        
        markerCenter_.x = -1.f;
        markerCenter_.y = -1.f;
        pixelSearchRadius_ = -1.f;
        
        dockOffsetDistX_ = 0;
        dockOffsetDistY_ = 0;
        dockOffsetAng_ = 0;
        
        useManualSpeed_ = useManualSpeed;
        
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;
      }
      
      void DockToBlock(const Vision::MarkerType markerType,
                       const Vision::MarkerType markerType2,
                       const f32 markerWidth_mm,
                       const Embedded::Point2f& markerCenter,
                       const f32 pixelSearchRadius,
                       const bool useManualSpeed,
                       const DockAction_t action)
      {
        DockToBlock(markerType, markerType2, markerWidth_mm, useManualSpeed, action);
        
        markerCenter_ = markerCenter;
        pixelSearchRadius_ = pixelSearchRadius;
      }
      
      void PlaceOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle, const bool useManualSpeed)
      {
        action_ = DA_PLACE_LOW;
        dockOffsetDistX_ = rel_x;
        dockOffsetDistY_ = rel_y;
        dockOffsetAng_ = rel_angle;
        
        useManualSpeed_ = useManualSpeed;
        
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;
      }
      
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki
