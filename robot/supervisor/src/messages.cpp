#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "messages.h"
#include "localization.h"
#include "visionSystem.h"
#include "pathFollower.h"
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
          u8 size;
          void (*dispatchFcn)(const u8* buffer);
        } TableEntry;
        
        const size_t NUM_TABLE_ENTRIES = NUM_MSG_IDS + 1;
        const TableEntry LookupTable_[NUM_TABLE_ENTRIES] = {
          {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/shared/MessageDefinitionsB2R.h"
          
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_NO_FUNC_MODE
#include "anki/cozmo/shared/MessageDefinitionsR2B.h"
          {0, 0, 0} // Final dummy entry without comma at end
        };
        

        u8 msgBuffer_[256];
        
        // For waiting for a particular message ID
        const u32 LOOK_FOR_MESSAGE_TIMEOUT = 1000000;
        ID lookForID_ = NO_MESSAGE_ID;
        u32 lookingStartTime_ = 0;
        
        //
        // Mailboxes
        //
        //   "Mailboxes" are used for leaving messages from slower
        //   vision processing in LongExecution to be retrieved and acted upon
        //   by the faster MainExecution.
        //
        
        // Single-message Mailbox Class
        template<typename MsgType>
        class Mailbox
        {
        public:
          
          Mailbox();
          
          bool putMessage(const MsgType newMsg);
          bool getMessage(MsgType& msgOut);
          
        protected:
          MsgType message_;
          bool    beenRead_;
          bool    isLocked_;
        };
        
        // Multiple-message Mailbox Class
        template<typename MSG_TYPE, u8 NUM_BOXES>
        class MultiMailbox
        {
        public:
        
          bool putMessage(const MSG_TYPE newMsg);
          bool getMessage(MSG_TYPE& msg);
        
        protected:
          Mailbox<MSG_TYPE> mailboxes_[NUM_BOXES];
          u8 readIndex_, writeIndex_;
          
          void advanceIndex(u8 &index);
        };
        
        
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
        
        // Flag for receipt of Init message
        bool initReceived_ = false;
        
      } // private namespace
      

// #pragma mark --- Messages Method Implementations ---
      
      u8 GetSize(const ID msgID)
      {
        return LookupTable_[msgID].size;
      }
      
      void ProcessMessage(const ID msgID, const u8* buffer)
      {
        if(LookupTable_[msgID].dispatchFcn != NULL) {
          //PRINT("ProcessMessage(): Dispatching message with ID=%d.\n", msgID);
          
          (*LookupTable_[msgID].dispatchFcn)(buffer);
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
        
        robotState_.status = 0;
        robotState_.status |= (PickAndPlaceController::IsCarryingBlock() ? IS_CARRYING_BLOCK : 0);
        robotState_.status |= (PickAndPlaceController::IsBusy() ? IS_PICKING_OR_PLACING : 0);
        robotState_.status |= (IMUFilter::IsPickedUp() ? IS_PICKED_UP : 0);
        robotState_.status |= (ProxSensors::IsForwardBlocked() ? IS_PROX_FORWARD_BLOCKED : 0);
        robotState_.status |= (ProxSensors::IsSideBlocked() ? IS_PROX_SIDE_BLOCKED : 0);        
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
              "basestation: (%.3f,%.3f) at %.1f degrees (frame = %d)\n",
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
        PathFollower::StartPathTraversal(msg.pathID);
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
                                              static_cast<DockAction_t>(msg.dockAction));
        } else {
          
          PickAndPlaceController::DockToBlock(static_cast<Vision::MarkerType>(msg.markerType),
                                              static_cast<Vision::MarkerType>(msg.markerType2),
                                              msg.markerWidth_mm,
                                              static_cast<DockAction_t>(msg.dockAction));
        }
      }
      
      void ProcessPlaceObjectOnGroundMessage(const PlaceObjectOnGround& msg)
      {
        //PRINT("Received PlaceOnGround message.\n");
        PickAndPlaceController::PlaceOnGround(msg.rel_x_mm, msg.rel_y_mm, msg.rel_angle);
      }

      void ProcessDriveWheelsMessage(const DriveWheels& msg) {
        
        // Do not process external drive commands if following a test path
        if (PathFollower::IsTraversingPath()) {
          PRINT("Ignoring DriveWheels message because robot is currently following a path.\n");
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
          TestModeController::Start((TestMode)(msg.mode));
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
        HAL::SetLED(HAL::LED_LEFT_EYE_TOP, lColor);
        HAL::SetLED(HAL::LED_LEFT_EYE_RIGHT, lColor);
        HAL::SetLED(HAL::LED_LEFT_EYE_BOTTOM, lColor);
        HAL::SetLED(HAL::LED_LEFT_EYE_LEFT, lColor);

        u32 rColor = msg.eye_right_color;
        HAL::SetLED(HAL::LED_RIGHT_EYE_TOP, rColor);
        HAL::SetLED(HAL::LED_RIGHT_EYE_RIGHT, rColor);
        HAL::SetLED(HAL::LED_RIGHT_EYE_BOTTOM, rColor);
        HAL::SetLED(HAL::LED_RIGHT_EYE_LEFT, rColor);
      }
      
      void ProcessSetHeadControllerGainsMessage(const SetHeadControllerGains& msg) {
        HeadController::SetGains(msg.kp, msg.ki, msg.maxIntegralError);
      }
      
      void ProcessSetLiftControllerGainsMessage(const SetLiftControllerGains& msg) {
        LiftController::SetGains(msg.kp, msg.ki, msg.maxIntegralError);
      }
      
      void ProcessSetVisionSystemParamsMessage(const SetVisionSystemParams& msg) {
        VisionSystem::SetParams(msg.integerCountsIncrement,
                                msg.minExposureTime,
                                msg.maxExposureTime,
                                msg.highValue,
                                msg.percentileToMakeHigh);
      }
      
      void ProcessPlayAnimationMessage(const PlayAnimation& msg) {
        AnimationController::Play((AnimationID_t)msg.animationID, msg.numLoops);
      }

      void ProcessIMURequestMessage(const IMURequest& msg) {
        IMUFilter::RecordAndSend(msg.length_ms);
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
      
#if 0
#pragma mark --- VisionSystem::Mailbox Template Implementations ---
#endif
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
      
      //
      // Templated Mailbox Implementations
      //
      template<typename MSG_TYPE>
      Mailbox<MSG_TYPE>::Mailbox()
      : beenRead_(true)
      {
      
      }
      
      template<typename MSG_TYPE>
      bool Mailbox<MSG_TYPE>::putMessage(const MSG_TYPE newMsg)
      {
        if(isLocked_) {
          return false;
        }
        else {
          isLocked_ = true;    // Lock
          message_  = newMsg;
          beenRead_ = false;
          isLocked_ = false;   // Unlock
          return true;
        }
      }
      
      template<typename MSG_TYPE>
      bool Mailbox<MSG_TYPE>::getMessage(MSG_TYPE& msgOut)
      {
        if(isLocked_ || beenRead_) {
          return false;
        }
        else {
          isLocked_ = true;   // Lock
          msgOut = message_;
          beenRead_ = true;
          isLocked_ = false;  // Unlock
          return true;
        }
      }
      
      
      //
      // Templated MultiMailbox Implementations
      //
      
      template<typename MSG_TYPE, u8 NUM_BOXES>
      bool MultiMailbox<MSG_TYPE,NUM_BOXES>::putMessage(const MSG_TYPE newMsg)
      {
        if(mailboxes_[writeIndex_].putMessage(newMsg) == true) {
          advanceIndex(writeIndex_);
          return true;
        }
        else {
          return false;
        }
      }
      
      template<typename MSG_TYPE, u8 NUM_BOXES>
      bool MultiMailbox<MSG_TYPE,NUM_BOXES>::getMessage(MSG_TYPE& msg)
      {
        if(mailboxes_[readIndex_].getMessage(msg) == true) {
          // we got a message out of the mailbox (it wasn't locked and there
          // was something in it), so move to the next mailbox
          advanceIndex(readIndex_);
          return true;
        }
        else {
          return false;
        }
      }
   
      template<typename MSG_TYPE, u8 NUM_BOXES>
      void MultiMailbox<MSG_TYPE,NUM_BOXES>::advanceIndex(u8 &index)
      {
        ++index;
        if(index == NUM_BOXES) {
          index = 0;
        }
      }

      
      bool ReceivedInit()
      {
        return initReceived_;
      }
      

      void ResetInit()
      {
        initReceived_ = false;
      }
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
