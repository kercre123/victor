#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoTypes.h"

#include "anki/common/shared/mailbox_impl.h"

#include "messages.h"
#include "localization.h"
#include "visionSystem.h"
#include "animationController.h"
#include "pathFollower.h"
#include "faceTrackingController.h"
#include "speedController.h"
#include "steeringController.h"
#include "wheelController.h"
#include "liftController.h"
#include "headController.h"
#include "imuFilter.h"
#include "dockingController.h"
#include "pickAndPlaceController.h"
#include "testModeController.h"
#include "animationController.h"
#include "proxSensors.h"

namespace Anki {
  namespace Cozmo {
    namespace Messages {
      
      namespace {
  
        // 4. Fill in the message information lookup table:
        typedef struct {
          u8 priority;
          u16 size;
          void (*dispatchFcn)(const u8* buffer);
        } TableEntry;
        
        const size_t NUM_TABLE_ENTRIES = NUM_MSG_IDS + 1;
        const TableEntry LookupTable_[NUM_TABLE_ENTRIES] = {
          {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/shared/MessageDefinitionsB2R.def"
          
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_NO_FUNC_MODE
#include "anki/cozmo/shared/MessageDefinitionsR2B.def"
          {0, 0, 0} // Final dummy entry without comma at end
        };
        

        u8 msgBuffer_[256];
        
        // For waiting for a particular message ID
        const u32 LOOK_FOR_MESSAGE_TIMEOUT = 1000000;
        ID lookForID_ = NO_MESSAGE_ID;
        u32 lookingStartTime_ = 0;
        
        
        // Mailboxes for different types of messages that the vision
        // system communicates to main execution:
        //MultiMailbox<Messages::BlockMarkerObserved, MAX_BLOCK_MARKER_MESSAGES> blockMarkerMailbox_;
        //Mailbox<Messages::MatMarkerObserved>    matMarkerMailbox_;
        Mailbox<Messages::DockingErrorSignal>   dockingMailbox_;
        
        MultiMailbox<Messages::FaceDetection,VisionSystem::FaceDetectionParameters::MAX_FACE_DETECTIONS> faceDetectMailbox_;

        static RobotState robotState_;
        
        // History of the last 2 RobotState messages that were sent to the basestation.
        // Used to avoid repeating a send.
        TimeStamp_t robotStateSendHist_[2];
        u8 robotStateSendHistIdx_ = 0;
       
        TimeStamp_t lastPingTime_ = 0;
        
        // Flag for receipt of Init message
        bool initReceived_ = false;
        
      } // private namespace
      

// #pragma mark --- Messages Method Implementations ---
      
      u16 GetSize(const ID msgID)
      {
        return LookupTable_[msgID].size;
      }
      
      void ProcessMessage(const ID msgID, const u8* buffer)
      {
        if(LookupTable_[msgID].dispatchFcn != NULL) {
          //PRINT("ProcessMessage(): Dispatching message with ID=%d.\n", msgID);
          
          (*LookupTable_[msgID].dispatchFcn)(buffer);
          
          // Treat any message as a ping
          lastPingTime_ = HAL::GetTimeStamp();
          //PRINT("Received message, kicking ping time at %d\n", lastPingTime_);
        }
        
        if(lookForID_ != NO_MESSAGE_ID) {
          
          // See if this was the message we were told to look for
          if(msgID == lookForID_) {
            lookForID_ = NO_MESSAGE_ID;
          }
        }
      } // ProcessMessage()
      
      void LookForID(const ID msgID) {
        lookForID_ = msgID;
        lookingStartTime_ = HAL::GetMicroCounter();
      }
      
      bool StillLookingForID(void) {
        if(lookForID_ == NO_MESSAGE_ID) {
          return false;
        }
        else if(HAL::GetMicroCounter() - lookingStartTime_ > LOOK_FOR_MESSAGE_TIMEOUT) {
            PRINT("Timed out waiting for message ID %d.\n", lookForID_);
            lookForID_ = NO_MESSAGE_ID;
          
          return false;
        }
        
        return true;
        
      }

      
      void UpdateRobotStateMsg()
      {
        robotState_.timestamp = HAL::GetTimeStamp();
        
        Radians poseAngle;
        
        robotState_.pose_frame_id = Localization::GetPoseFrameId();
        
        Localization::GetCurrentMatPose(robotState_.pose_x, robotState_.pose_y, poseAngle);
        robotState_.pose_z = 0;
        robotState_.pose_angle = poseAngle.ToFloat();
        robotState_.pose_pitch_angle = IMUFilter::GetPitch();
        
        WheelController::GetFilteredWheelSpeeds(robotState_.lwheel_speed_mmps, robotState_.rwheel_speed_mmps);
        
        robotState_.headAngle  = HeadController::GetAngleRad();
        robotState_.liftAngle  = LiftController::GetAngleRad();
        robotState_.liftHeight = LiftController::GetHeightMM();

        ProxSensors::GetValues(robotState_.proxLeft, robotState_.proxForward, robotState_.proxRight);
        
        robotState_.lastPathID = PathFollower::GetLastPathID();
        
        robotState_.currPathSegment = PathFollower::GetCurrPathSegment();
        robotState_.numFreeSegmentSlots = PathFollower::GetNumFreeSegmentSlots();
        robotState_.battVolt10x = HAL::BatteryGetVoltage10x();
        
        robotState_.status = 0;
        robotState_.status |= (PickAndPlaceController::IsCarryingBlock() ? IS_CARRYING_BLOCK : 0);
        robotState_.status |= (PickAndPlaceController::IsBusy() ? IS_PICKING_OR_PLACING : 0);
        robotState_.status |= (IMUFilter::IsPickedUp() ? IS_PICKED_UP : 0);
        robotState_.status |= (ProxSensors::IsForwardBlocked() ? IS_PROX_FORWARD_BLOCKED : 0);
        robotState_.status |= (ProxSensors::IsSideBlocked() ? IS_PROX_SIDE_BLOCKED : 0);
        robotState_.status |= (AnimationController::IsPlaying() ? IS_ANIMATING : 0);
      }
      
      RobotState const& GetRobotStateMsg() {
        return robotState_;
      }
      
      
// #pragma --- Message Dispatch Functions ---
      
      
      void ProcessSyncTimeMessage(const SyncTime& msg)
      {
   
        PRINT("Robot received SyncTime message from basestation with ID=%d and syncTime=%d.\n",
              msg.robotID, msg.syncTime);
        
        initReceived_ = true;
        
        // TODO: Compare message ID to robot ID as a handshake?
        
        // Poor-man's time sync to basestation, for now.
        HAL::SetTimeStamp(msg.syncTime);
        
        // Reset pose history and frameID to zero
        Localization::ResetPoseFrame();
        
        // Send back camera calibration
        const HAL::CameraInfo* headCamInfo = HAL::GetHeadCamInfo();
        if(headCamInfo == NULL) {
          PRINT("NULL HeadCamInfo retrieved from HAL.\n");
        }
        else {
          Messages::CameraCalibration headCalibMsg = {
            headCamInfo->focalLength_x,
            headCamInfo->focalLength_y,
            headCamInfo->center_x,
            headCamInfo->center_y,
            headCamInfo->skew,
            headCamInfo->nrows,
            headCamInfo->ncols
          };
          
          
          if(!HAL::RadioSendMessage(CameraCalibration_ID,
                                    &headCalibMsg))
          {
            PRINT("Failed to send camera calibration message.\n");
          }
        }
        
      } // ProcessRobotInit()
      
      
      void ProcessAbsLocalizationUpdateMessage(const AbsLocalizationUpdate& msg)
      {
        // Don't modify localization while running path following test.
        // The point of the test is to see how well it follows a path
        // assuming perfect localization.
        if (TestModeController::GetMode() == TM_PATH_FOLLOW) {
          return;
        }
        
        // TODO: Double-check that size matches expected size?
        
        // TODO: take advantage of timestamp
        
        f32 currentMatX       = msg.xPosition;
        f32 currentMatY       = msg.yPosition;
        Radians currentMatHeading = msg.headingAngle;
        Localization::UpdatePoseWithKeyframe(msg.pose_frame_id, msg.timestamp, currentMatX, currentMatY, currentMatHeading.ToFloat());
        //Localization::SetCurrentMatPose(currentMatX, currentMatY, currentMatHeading);
        //Localization::SetPoseFrameId(msg.pose_frame_id);
        
        
        PRINT("Robot received localization update from "
              "basestation for time=%d: (%.3f,%.3f) at %.1f degrees (frame = %d)\n",
              msg.timestamp,
              currentMatX, currentMatY,
              currentMatHeading.getDegrees(),
              Localization::GetPoseFrameId());
#if(USE_OVERLAY_DISPLAY)
        {
          using namespace Sim::OverlayDisplay;
          SetText(CURR_POSE, "Pose: (x,y)=(%.4f,%.4f) at angle=%.1f\n",
                  currentMatX, currentMatY,
                  currentMatHeading.getDegrees());
        }
#endif
        
      } // ProcessAbsLocalizationUpdateMessage()
      
      
      void ProcessDockingErrorSignalMessage(const DockingErrorSignal& msg)
      {
        // Just pass the docking error signal along to the mainExecution to
        // deal with. Note that if the message indicates tracking failed,
        // the mainExecution thread should handle it, and put the vision
        // system back in LOOKING_FOR_BLOCKS mode.
        dockingMailbox_.putMessage(msg);
      }
      
      void ProcessFaceDetectionMessage(const FaceDetection& msg)
      {
        // Just pass the face detection along to mainExecution to deal with.
        faceDetectMailbox_.putMessage(msg);
      }
      
      void ProcessBTLEMessages()
      {
        ID msgID;
        
        while((msgID = HAL::RadioGetNextMessage(msgBuffer_)) != NO_MESSAGE_ID)
        {
          ProcessMessage(msgID, msgBuffer_);
        }
        
        // If no messages received for PING_DISCONNECT_TIMEOUT_MS, then set disconnected state
        if ((lastPingTime_ != 0) && (lastPingTime_ + B2R_PING_DISCONNECT_TIMEOUT_MS < HAL::GetTimeStamp())) {
          PRINT("WARN: Disconnecting radio due to ping timeout\n");
          HAL::DisconnectRadio();
        }
        
      } // ProcessBTLEMessages()
      
      void ProcessClearPathMessage(const ClearPath& msg) {
        SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
        PathFollower::ClearPath();
        //SteeringController::ExecuteDirectDrive(0,0);
      }

      void ProcessAppendPathSegmentArcMessage(const AppendPathSegmentArc& msg) {
        PathFollower::AppendPathSegment_Arc(0, msg.x_center_mm, msg.y_center_mm,
                                            msg.radius_mm, msg.startRad, msg.sweepRad,
                                            msg.targetSpeed, msg.accel, msg.decel);
      }
      
      void ProcessAppendPathSegmentLineMessage(const AppendPathSegmentLine& msg) {
        PathFollower::AppendPathSegment_Line(0, msg.x_start_mm, msg.y_start_mm,
                                             msg.x_end_mm, msg.y_end_mm,
                                             msg.targetSpeed, msg.accel, msg.decel);
      }

      void ProcessAppendPathSegmentPointTurnMessage(const AppendPathSegmentPointTurn& msg) {
        PathFollower::AppendPathSegment_PointTurn(0, msg.x_center_mm, msg.y_center_mm, msg.targetRad,
                                                  msg.targetSpeed, msg.accel, msg.decel);
      }
      
      void ProcessTrimPathMessage(const TrimPath& msg) {
        PathFollower::TrimPath(msg.numPopFrontSegments, msg.numPopBackSegments);
      }
      
      void ProcessExecutePathMessage(const ExecutePath& msg) {
        PathFollower::StartPathTraversal(msg.pathID, msg.useManualSpeed);
      }
      
      void ProcessDockWithObjectMessage(const DockWithObject& msg)
      {
        if (msg.pixel_radius < u8_MAX) {
          Embedded::Point2f markerCenter(static_cast<f32>(msg.image_pixel_x), static_cast<f32>(msg.image_pixel_y));
          
          PickAndPlaceController::DockToBlock(static_cast<Vision::MarkerType>(msg.markerType),
                                              static_cast<Vision::MarkerType>(msg.markerType2),
                                              msg.markerWidth_mm,
                                              markerCenter,
                                              msg.pixel_radius,
                                              msg.useManualSpeed,
                                              static_cast<DockAction_t>(msg.dockAction));
        } else {
          
          PickAndPlaceController::DockToBlock(static_cast<Vision::MarkerType>(msg.markerType),
                                              static_cast<Vision::MarkerType>(msg.markerType2),
                                              msg.markerWidth_mm,
                                              msg.useManualSpeed,
                                              static_cast<DockAction_t>(msg.dockAction));
        }
      }
      
      void ProcessPlaceObjectOnGroundMessage(const PlaceObjectOnGround& msg)
      {
        //PRINT("Received PlaceOnGround message.\n");
        PickAndPlaceController::PlaceOnGround(msg.rel_x_mm, msg.rel_y_mm, msg.rel_angle, msg.useManualSpeed);
      }

      void ProcessDriveWheelsMessage(const DriveWheels& msg) {
        
        // Do not process external drive commands if following a test path
        if (PathFollower::IsTraversingPath()) {
          if (PathFollower::IsInManualSpeedMode()) {
            // TODO: Maybe want to set manual speed via a different message?
            //       For now, using average wheel speed.
            f32 manualSpeed = 0.5f * (msg.lwheel_speed_mmps + msg.rwheel_speed_mmps);
            PRINT("Commanding manual path speed %f mm/s.\n", manualSpeed);
            PathFollower::SetManualPathSpeed(manualSpeed, 1000, 1000);
          } else {
            PRINT("Ignoring DriveWheels message because robot is currently following a path.\n");
          }
          return;
        }
        
        //PRINT("Executing DriveWheels message: left=%f, right=%f\n",
        //      msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
        
        //PathFollower::ClearPath();
        SteeringController::ExecuteDirectDrive(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
      }
      
      void ProcessDriveWheelsCurvatureMessage(const DriveWheelsCurvature& msg) {
        /*
        PathFollower::ClearPath();
        
        SpeedController::SetUserCommandedDesiredVehicleSpeed(msg.speed_mmPerSec);
        SpeedController::SetUserCommandedAcceleration(msg.accel_mmPerSec2);
        SpeedController::SetUserCommandedDeceleration(msg.decel_mmPerSec2);
        */
      }
      
      void ProcessMoveLiftMessage(const MoveLift& msg) {
        LiftController::SetAngularVelocity(msg.speed_rad_per_sec);
      }
      
      void ProcessMoveHeadMessage(const MoveHead& msg) {
        HeadController::SetAngularVelocity(msg.speed_rad_per_sec);
      }
      
      void ProcessSetLiftHeightMessage(const SetLiftHeight& msg) {
        LiftController::SetSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        LiftController::SetDesiredHeight(msg.height_mm);
      }
      
      void ProcessSetHeadAngleMessage(const SetHeadAngle& msg) {
        HeadController::SetSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        HeadController::SetDesiredAngle(msg.angle_rad);
      }
      
      void ProcessStopAllMotorsMessage(const StopAllMotors& msg) {
        SteeringController::ExecuteDirectDrive(0,0);
        LiftController::SetAngularVelocity(0);
        HeadController::SetAngularVelocity(0);
      }
      
      void ProcessHeadAngleUpdateMessage(const HeadAngleUpdate& msg) {
        HeadController::SetAngleRad(msg.newAngle);
      }
      
      void ProcessStartTestModeMessage(const StartTestMode& msg)
      {
        if (msg.mode < TM_NUM_TESTS) {
          TestModeController::Start((TestMode)(msg.mode), msg.p1, msg.p2, msg.p3);
        } else {
          PRINT("Unknown test mode %d received\n", msg.mode);
        }
      }
      
      void ProcessImageRequestMessage(const ImageRequest& msg) {
        PRINT("Image requested (mode: %d, resolution: %d)\n", msg.imageSendMode, msg.resolution);
        VisionSystem::SetImageSendMode((ImageSendMode_t)msg.imageSendMode, (Vision::CameraResolution)msg.resolution);
      }
      
      void ProcessSetHeadlightMessage(const SetHeadlight& msg) {
        HAL::SetHeadlights(msg.intensity > 0);
      }

      void ProcessSetDefaultLightsMessage(const SetDefaultLights& msg) {
        u32 lColor = msg.eye_left_color;
        HAL::SetLED(LED_LEFT_EYE_TOP, lColor);
        HAL::SetLED(LED_LEFT_EYE_RIGHT, lColor);
        HAL::SetLED(LED_LEFT_EYE_BOTTOM, lColor);
        HAL::SetLED(LED_LEFT_EYE_LEFT, lColor);

        u32 rColor = msg.eye_right_color;
        HAL::SetLED(LED_RIGHT_EYE_TOP, rColor);
        HAL::SetLED(LED_RIGHT_EYE_RIGHT, rColor);
        HAL::SetLED(LED_RIGHT_EYE_BOTTOM, rColor);
        HAL::SetLED(LED_RIGHT_EYE_LEFT, rColor);
      }
      
      void ProcessSetHeadControllerGainsMessage(const SetHeadControllerGains& msg) {
        HeadController::SetGains(msg.kp, msg.ki, msg.maxIntegralError);
      }
      
      void ProcessSetLiftControllerGainsMessage(const SetLiftControllerGains& msg) {
        LiftController::SetGains(msg.kp, msg.ki, msg.maxIntegralError);
      }
      
      void ProcessSetVisionSystemParamsMessage(const SetVisionSystemParams& msg) {
        VisionSystem::SetParams(msg.autoexposureOn,
                                msg.exposureTime,
                                msg.integerCountsIncrement,
                                msg.minExposureTime,
                                msg.maxExposureTime,
                                msg.highValue,
                                msg.percentileToMakeHigh,
                                msg.limitFramerate);
      }

      void ProcessSetFaceDetectParamsMessage(const SetFaceDetectParams& msg) {
        VisionSystem::SetFaceDetectParams(msg.scaleFactor,
                                          msg.minNeighbors,
                                          msg.minObjectHeight, msg.minObjectWidth,
                                          msg.maxObjectHeight, msg.maxObjectWidth);
      }

      void ProcessIMURequestMessage(const IMURequest& msg) {
        IMUFilter::RecordAndSend(msg.length_ms);
      }
      
      
      void ProcessFaceTrackingMessage(const FaceTracking& msg)
      {
        if(msg.enabled) {
          PRINT("Starting face tracking with timeout = %dsec.\n", msg.timeout_sec);
          FaceTrackingController::StartTracking(FaceTrackingController::LARGEST, msg.timeout_sec);
        } else {
          PRINT("Stopping face tracking.\n");
          FaceTrackingController::Reset();
        }
      }
      
      void ProcessPingMessage(const Ping& msg)
      {
        lastPingTime_ = HAL::GetTimeStamp();
      }
      
      
      void ProcessAbortDockingMessage(const AbortDocking& msg)
      {
        DockingController::ResetDocker();
      }
      
      //
      // Animation related:
      //
      
      void ProcessPlayAnimationMessage(const PlayAnimation& msg) {
        //PRINT("Processing play animation message\n");
        AnimationController::Play((AnimationID_t)msg.animationID, msg.numLoops);
      }
      
      void ProcessTransitionToStateAnimationMessage(const TransitionToStateAnimation& msg) {
        AnimationController::TransitionAndPlay(msg.transitionAnimID, msg.stateAnimID);
      }
      
      void ProcessAbortAnimationMessage(const AbortAnimation& msg)
      {
        AnimationController::Stop();
      }
      
      void ProcessClearCannedAnimationMessage(const ClearCannedAnimation& msg)
      {
        AnimationController::ClearCannedAnimation(msg.animationID);
      }
      
      
      // Adds common message members to keyframe and adds it to the animation
      // specified by the message
      template<typename MSG_TYPE>
      static void AddKeyFrameHelper(const MSG_TYPE& msg,
                                    KeyFrame& kf)
      {
        PRINT("Adding keyframe with type %d to animation %d\n", kf.type, msg.animationID);
        
        kf.relTime_ms    = msg.relTime_ms;
        
        AnkiConditionalWarn(msg.transitionIn >= 0 && msg.transitionIn <= 100,
                            "Messages.AddKeyFrameHelper.InvalidTransitionPercentage",
                            "TransitionIn should be a percentage between 0 and 100.\n");
        
        kf.transitionIn  = CLIP(0, 100, msg.transitionIn);
        
        AnkiConditionalWarn(msg.transitionOut >= 0 && msg.transitionOut <= 100,
                            "Messages.AddKeyFrameHelper.InvalidTransitionPercentage",
                            "TransitionOut should be a percentage between 0 and 100.\n");
        
        kf.transitionOut = CLIP(0, 100, msg.transitionOut);
        
        AnimationController::AddKeyFrameToCannedAnimation(kf, static_cast<AnimationID_t>(msg.animationID));
      } // SetCommonKeyFrameMembers()
      
      
      void ProcessAddAnimKeyFrame_SetHeadAngleMessage(const AddAnimKeyFrame_SetHeadAngle& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::HEAD_ANGLE;
        kf.SetHeadAngle.angle_deg       = msg.angle_deg;
        kf.SetHeadAngle.variability_deg = msg.variability_deg;
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_StartHeadNodMessage(const AddAnimKeyFrame_StartHeadNod& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::START_HEAD_NOD;
        kf.StartHeadNod.highAngle_deg = msg.highAngle_deg;
        kf.StartHeadNod.lowAngle_deg  = msg.lowAngle_deg;
        kf.StartHeadNod.period_ms = msg.period_ms;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_StopHeadNodMessage(const AddAnimKeyFrame_StopHeadNod& msg)
      {
        KeyFrame kf;
        kf.type = KeyFrame::STOP_HEAD_NOD;
        kf.StopHeadNod.finalAngle_deg = msg.finalAngle_deg;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_SetLiftHeightMessage(const AddAnimKeyFrame_SetLiftHeight& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::LIFT_HEIGHT;
        kf.SetLiftHeight.targetHeight = msg.height_mm;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_StartLiftNodMessage(const AddAnimKeyFrame_StartLiftNod& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::START_LIFT_NOD;
        kf.StartLiftNod.lowHeight  = msg.lowHeight_mm;
        kf.StartLiftNod.highHeight = msg.highHeight_mm;
        kf.StartLiftNod.period_ms  = msg.period_ms;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_StopLiftNodMessage(const AddAnimKeyFrame_StopLiftNod& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::STOP_LIFT_NOD;
        kf.StopLiftNod.finalHeight  = msg.finalHeight_mm;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_DriveLineMessage(const AddAnimKeyFrame_DriveLine& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::DRIVE_LINE_SEGMENT;
        kf.DriveLineSegment.relativeDistance = msg.relativeDistance_mm;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_TurnInPlaceMessage(const AddAnimKeyFrame_TurnInPlace& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::POINT_TURN;
        kf.TurnInPlace.relativeAngle_deg = msg.relativeAngle_deg;
        kf.TurnInPlace.variability_deg   = msg.variability_deg;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_HoldHeadAngleMessage(const AddAnimKeyFrame_HoldHeadAngle& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::HOLD_HEAD_ANGLE;
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_HoldLiftHeightMessage(const AddAnimKeyFrame_HoldLiftHeight& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::HOLD_LIFT_HEIGHT;
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_HoldPoseMessage(const AddAnimKeyFrame_HoldPose& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::HOLD_POSE;
        AddKeyFrameHelper(msg, kf);
      }
      
      static inline u32 GetU32ColorFromRGB(const u8 rgb[3])
      {
        return (rgb[0]<<16) + (rgb[1]<<8) + rgb[2];
      }
      
      /*
      void ProcessAddAnimKeyFrame_SetLEDColorsMessage(const AddAnimKeyFrame_SetLEDColors& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::SET_LED_COLORS;
        kf.SetLEDcolors.led[0] = GetU32ColorFromRGB(msg.rightEye_top);
        kf.SetLEDcolors.led[1] = GetU32ColorFromRGB(msg.rightEye_right);
        kf.SetLEDcolors.led[2] = GetU32ColorFromRGB(msg.rightEye_bottom);
        kf.SetLEDcolors.led[3] = GetU32ColorFromRGB(msg.rightEye_left);

        kf.SetLEDcolors.led[4] = GetU32ColorFromRGB(msg.leftEye_top);
        kf.SetLEDcolors.led[5] = GetU32ColorFromRGB(msg.leftEye_right);
        kf.SetLEDcolors.led[6] = GetU32ColorFromRGB(msg.leftEye_bottom);
        kf.SetLEDcolors.led[7] = GetU32ColorFromRGB(msg.leftEye_left);
        
        AddKeyFrameHelper(msg, kf);
      }
       */
      
      void ProcessAddAnimKeyFrame_PlaySoundMessage(const AddAnimKeyFrame_PlaySound& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::PLAY_SOUND;
        kf.PlaySound.soundID  = msg.soundID;
        kf.PlaySound.numLoops = msg.numLoops;
        kf.PlaySound.volume   = msg.volume;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_WaitForSoundMessage(const AddAnimKeyFrame_WaitForSound& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::WAIT_FOR_SOUND;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_StopSoundMessage(const AddAnimKeyFrame_StopSound& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::STOP_SOUND;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_SetEyeShapeAndColorMessage(const AddAnimKeyFrame_SetEyeShapeAndColor& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::SET_EYE;
        kf.SetEye.whichEye = static_cast<WhichEye>(msg.whichEye);
        kf.SetEye.color    = GetU32ColorFromRGB(msg.color);
        kf.SetEye.shape    = static_cast<EyeShape>(msg.shape);
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_StartBlinkingMessage(const AddAnimKeyFrame_StartBlinking& msg)
      {
        KeyFrame kf;
        
        kf.type = KeyFrame::BLINK_EYES;
        kf.BlinkEyes.color      = GetU32ColorFromRGB(msg.color);
        kf.BlinkEyes.timeOn_ms  = msg.onPeriod_ms;
        kf.BlinkEyes.timeOff_ms = msg.offPeriod_ms;
        kf.BlinkEyes.variability_ms = msg.variability_ms;

        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_StartFlashingEyesMessage(const AddAnimKeyFrame_StartFlashingEyes& msg)
      {
        KeyFrame kf;
        kf.type = KeyFrame::FLASH_EYES;
        kf.FlashEyes.color      = GetU32ColorFromRGB(msg.color);
        kf.FlashEyes.timeOn_ms  = msg.onPeriod_ms;
        kf.FlashEyes.timeOff_ms = msg.offPeriod_ms;
        kf.FlashEyes.shape      = static_cast<EyeShape>(msg.shape);
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_StartSpinningEyesMessage(const AddAnimKeyFrame_StartSpinningEyes& msg)
      {
        KeyFrame kf;
        kf.type = KeyFrame::SPIN_EYES;
        kf.SpinEyes.color          = GetU32ColorFromRGB(msg.color);
        kf.SpinEyes.period_ms      = msg.period_ms;
        kf.SpinEyes.leftClockwise  = msg.leftClockwise;
        kf.SpinEyes.rightClockWise = msg.rightClockwise;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_StopEyeAnimationMessage(const AddAnimKeyFrame_StopEyeAnimation& msg)
      {
        KeyFrame kf;
        kf.type = KeyFrame::STOP_EYES;
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessAddAnimKeyFrame_TriggerAnimationMessage(const AddAnimKeyFrame_TriggerAnimation& msg)
      {
        KeyFrame kf;
        kf.type = KeyFrame::TRIGGER_ANIMATION;
        kf.TriggerAnimation.animID   = msg.animToPlay;
        kf.TriggerAnimation.numLoops = msg.numLoops;
        
        AddKeyFrameHelper(msg, kf);
      }
      
      void ProcessPanAndTiltHeadMessage(const PanAndTiltHead& msg)
      {
        // TODO: Move this to some kind of VisualInterestTrackingController or something
        
        HeadController::SetDesiredAngle(msg.relativeHeadTiltAngle_rad + HeadController::GetAngleRad());
        
        const f32 turnVelocity = (msg.relativePanAngle_rad < 0 ? -50.f : 50.f);
        SteeringController::ExecutePointTurn(msg.relativePanAngle_rad + Localization::GetCurrentMatOrientation().ToFloat(),
                                             turnVelocity, 5, -5);

        
      }
      
      
      // --------- Block control messages ----------
      
      void ProcessFlashBlockIDsMessage(const FlashBlockIDs& msg)
      {
        // Start flash pattern on blocks
        HAL::FlashBlockIDs();
        
        // Send timestamp of when flash message was sent to blocks
        Messages::BlockIDFlashStarted m;
        m.timestamp = HAL::GetTimeStamp();
        HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::BlockIDFlashStarted), &m);
      }
      
      void ProcessSetBlockLightsMessage(const SetBlockLights& msg)
      {
        HAL::SetBlockLight(msg.blockID, msg.color, msg.onPeriod_ms, msg.offPeriod_ms);
      }
      
// ----------- Send messages -----------------
      
      
      Result SendRobotStateMsg(const RobotState* msg)
      {
        
        // Don't send robot state updates unless the init message was received
        if (!initReceived_) {
          return RESULT_FAIL;
        }
        
        
        const RobotState* m = &robotState_;
        if (msg) {
          m = msg;
        }
        
        // Check if a state message with this timestamp was already sent
        for (u8 i=0; i < 2; ++i) {
          if (robotStateSendHist_[i] == m->timestamp) {
            return RESULT_FAIL;
          }
        }
        
        if(HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::RobotState), m) == true) {
          // Update send history
          robotStateSendHist_[robotStateSendHistIdx_] = m->timestamp;
          if (++robotStateSendHistIdx_ > 1) robotStateSendHistIdx_ = 0;
          return RESULT_OK;
        } else {
          return RESULT_FAIL;
        }
      }
      
      
      int SendText(const char *format, ...)
      {
        va_list argptr;
        va_start(argptr, format);
        SendText(format, argptr);
        va_end(argptr);
        
        return 0;
      }

      int SendText(const char *format, va_list vaList)
      {
        #define MAX_SEND_TEXT_LENGTH 512
        char text[MAX_SEND_TEXT_LENGTH];
        memset(text, 0, MAX_SEND_TEXT_LENGTH);

        // Create formatted text
        vsnprintf(text, MAX_SEND_TEXT_LENGTH, format, vaList);
        
        // Breakup and send in multiple messages if necessary
        Messages::PrintText m;
        const size_t tempBytesLeftToSend = strlen(text);
        AnkiAssert(tempBytesLeftToSend < s32_MAX);
        s32 bytesLeftToSend = static_cast<s32>(tempBytesLeftToSend);
        u8 numMsgs = 0;
        while(bytesLeftToSend > 0) {
          memset(m.text, 0, PRINT_TEXT_MSG_LENGTH);
          size_t currPacketBytes = MIN(PRINT_TEXT_MSG_LENGTH, bytesLeftToSend);
          memcpy(m.text, text + numMsgs*PRINT_TEXT_MSG_LENGTH, currPacketBytes);
          
          bytesLeftToSend -= PRINT_TEXT_MSG_LENGTH;
          
          HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::PrintText), &m);
          numMsgs++;
        }
        
        return 0;
      }
      
      /*
      bool CheckMailbox(BlockMarkerObserved& msg)
      {
        return blockMarkerMailbox_.getMessage(msg);
      }
      
      bool CheckMailbox(MatMarkerObserved&   msg)
      {
        return matMarkerMailbox_.getMessage(msg);
      }
       */
      
      
      
      bool CheckMailbox(DockingErrorSignal&  msg)
      {
        return dockingMailbox_.getMessage(msg);
      }
      
      bool CheckMailbox(FaceDetection&       msg)
      {
        return faceDetectMailbox_.getMessage(msg);
      }

      
      bool ReceivedInit()
      {
        return initReceived_;
      }
      

      void ResetInit()
      {
        initReceived_ = false;
        lastPingTime_ = 0;
      }
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
