#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/robot/cozmoBot.h"

#include "anki/common/robot/array2d.h"

#include "utilEmbedded/transport/IUnreliableTransport.h"
#include "utilEmbedded/transport/IReceiver.h"
#include "utilEmbedded/transport/reliableTransport.h"

#include "messages.h"
#include "localization.h"
#include "animationController.h"
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
#include "backpackLightController.h"
#include "anki/cozmo/shared/activeBlockTypes.h"


namespace Anki {
  namespace Cozmo {
    ReliableConnection connection;
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

        u8 pktBuffer_[2048];
        u8 msgBuff_[2048];

        // For waiting for a particular message ID
        const u32 LOOK_FOR_MESSAGE_TIMEOUT = 1000000;
        ID lookForID_ = NO_MESSAGE_ID;
        u32 lookingStartTime_ = 0;

        static RobotState robotState_;

        // History of the last 2 RobotState messages that were sent to the basestation.
        // Used to avoid repeating a send.
        TimeStamp_t robotStateSendHist_[2];
        u8 robotStateSendHistIdx_ = 0;

        TimeStamp_t lastPingTime_ = 0;

        // Flag for receipt of Init message
        bool initReceived_ = false;

        const int IMAGE_SEND_JPEG_COMPRESSION_QUALITY = 80; // 0 to 100

      } // private namespace


// #pragma mark --- Messages Method Implementations ---

      Result Init()
      {
        ReliableTransport_Init();
        ReliableConnection_Init(&connection, NULL); // We only have one connection so dest pointer is superfluous
        return RESULT_OK;
      }

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

        HAL::IMU_DataStructure imuData = IMUFilter::GetLatestRawData();
        robotState_.rawGyroZ = imuData.rate_z;
        robotState_.rawAccelY = imuData.acc_y;

        //ProxSensors::GetValues(robotState_.proxLeft, robotState_.proxForward, robotState_.proxRight);

        robotState_.lastPathID = PathFollower::GetLastPathID();

        robotState_.currPathSegment = PathFollower::GetCurrPathSegment();
        robotState_.numFreeSegmentSlots = PathFollower::GetNumFreeSegmentSlots();
        robotState_.battVolt10x = HAL::BatteryGetVoltage10x();

        robotState_.status = 0;

        // TODO: Make this a parameters somewhere?
        const f32 WHEEL_SPEED_STOPPED = 2.f;
        robotState_.status |= (HeadController::IsMoving() ||
                               LiftController::IsMoving() ||
                               fabs(robotState_.lwheel_speed_mmps) > WHEEL_SPEED_STOPPED ||
                               fabs(robotState_.rwheel_speed_mmps) > WHEEL_SPEED_STOPPED ? IS_MOVING : 0);
        robotState_.status |= (PickAndPlaceController::IsCarryingBlock() ? IS_CARRYING_BLOCK : 0);
        robotState_.status |= (PickAndPlaceController::IsBusy() ? IS_PICKING_OR_PLACING : 0);
        robotState_.status |= (IMUFilter::IsPickedUp() ? IS_PICKED_UP : 0);
        //robotState_.status |= (ProxSensors::IsForwardBlocked() ? IS_PROX_FORWARD_BLOCKED : 0);
        //robotState_.status |= (ProxSensors::IsSideBlocked() ? IS_PROX_SIDE_BLOCKED : 0);
        robotState_.status |= (AnimationController::IsPlaying() ? IS_ANIMATING : 0);
        robotState_.status |=  (LiftController::IsInPosition() ? LIFT_IN_POS : 0);
        robotState_.status |=  (HeadController::IsInPosition() ? HEAD_IN_POS : 0);
        robotState_.status |= (AnimationController::IsBufferFull() ? IS_ANIM_BUFFER_FULL : 0);
        
        //robotState_.numFreeAnimationFrames = AnimationController::GetNumFramesFree();
        robotState_.numAnimBytesFree = AnimationController::GetApproximateNumBytesFree();
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

        // Send ACK back to basestation
        Messages::SyncTimeAck stMsg;
        if(!HAL::RadioSendMessage(SyncTimeAck_ID, &stMsg)) {
          PRINT("Failed to send sync time ack message.\n");
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
        /*Result res =*/ Localization::UpdatePoseWithKeyframe(msg.pose_frame_id, msg.timestamp, currentMatX, currentMatY, currentMatHeading.ToFloat());
        //Localization::SetCurrentMatPose(currentMatX, currentMatY, currentMatHeading);
        //Localization::SetPoseFrameId(msg.pose_frame_id);

        /*
        PRINT("Robot %s localization update from "
              "basestation  at currTime=%d for frame at time=%d: (%.3f,%.3f) at %.1f degrees (frame = %d)\n",
              res == RESULT_OK ? "PROCESSED" : "IGNORED",
              HAL::GetTimeStamp(),
              msg.timestamp,
              currentMatX, currentMatY,
              currentMatHeading.getDegrees(),
              Localization::GetPoseFrameId());
         */
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
        DockingController::SetDockingErrorSignalMessage(msg);
      }

      void ProcessFaceDetectionMessage(const FaceDetection& msg)
      {
      }

      void ProcessBTLEMessages()
      {
        u32 dataLen;

        while((dataLen = HAL::RadioGetNextPacket(pktBuffer_)) > 0)
        {
          s16 res = ReliableTransport_ReceiveData(&connection, pktBuffer_, dataLen);
          if (res < 0)
          {
            PRINT("ERROR (%d): ReliableTransport didn't accept message %d[%d]\n", res, pktBuffer_[0], dataLen);
          }
        }

        if (HAL::RadioIsConnected())
        {
          if (ReliableTransport_Update(&connection) == false) // Connection has timed out
          {
            PRINT("WARN: Reliable transport has timed out\n");
            Receiver_OnDisconnect(&connection);
          }
        }
      }

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
                                                  msg.targetSpeed, msg.accel, msg.decel, msg.useShortestDir);
      }

      void ProcessTrimPathMessage(const TrimPath& msg) {
        PathFollower::TrimPath(msg.numPopFrontSegments, msg.numPopBackSegments);
      }

      void ProcessExecutePathMessage(const ExecutePath& msg) {
        PRINT("Starting path %d\n", msg.pathID);
        PathFollower::StartPathTraversal(msg.pathID, msg.useManualSpeed);
      }

      void ProcessDockWithObjectMessage(const DockWithObject& msg)
      {
          PRINT("RECVD DockToBlock (action %d, manualSpeed %d)\n", msg.dockAction, msg.useManualSpeed);

          PickAndPlaceController::DockToBlock(msg.useManualSpeed,
                                              static_cast<DockAction_t>(msg.dockAction));
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
        PRINT("Moving lift to %f\n", msg.height_mm);
        LiftController::SetMaxSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        LiftController::SetDesiredHeight(msg.height_mm);
      }

      void ProcessSetHeadAngleMessage(const SetHeadAngle& msg) {
        PRINT("Moving head to %f\n", msg.angle_rad);
        HeadController::SetMaxSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
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

      void ProcessImageRequestMessage(const ImageRequest& msg)
      {
        PRINT("Image requested (mode: %d, resolution: %d)\n", msg.imageSendMode, msg.resolution);

        ImageSendMode_t imageSendMode = static_cast<ImageSendMode_t>(msg.imageSendMode);
        Vision::CameraResolution imageSendResolution = static_cast<Vision::CameraResolution>(msg.resolution);
        
        HAL::SetImageSendMode(imageSendMode, imageSendResolution);

        // Send back camera calibration for this resolution
        const HAL::CameraInfo* headCamInfo = HAL::GetHeadCamInfo();

        // TODO: Just store CameraResolution in calibration data instead of height/width?

        if(headCamInfo == NULL) {
          PRINT("NULL HeadCamInfo retrieved from HAL.\n");
        }
        else {
          HAL::CameraInfo headCamInfoScaled(*headCamInfo);
          const s32 width  = Vision::CameraResInfo[msg.resolution].width;
          const s32 height = Vision::CameraResInfo[msg.resolution].height;
          const f32 xScale = static_cast<f32>(width/headCamInfo->ncols);
          const f32 yScale = static_cast<f32>(height/headCamInfo->nrows);

          if(xScale != 1.f || yScale != 1.f)
          {
            PRINT("Scaling [%dx%d] camera calibration by [%.1f %.1f] to match requested resolution.\n",
                  headCamInfo->ncols, headCamInfo->nrows, xScale, yScale);

            // Stored calibration info does not requested resolution, so scale it
            // accordingly and adjust the pointer so we send this scaled info below.
            headCamInfoScaled.focalLength_x *= xScale;
            headCamInfoScaled.focalLength_y *= yScale;
            headCamInfoScaled.center_x      *= xScale;
            headCamInfoScaled.center_y      *= yScale;
            headCamInfoScaled.nrows = height;
            headCamInfoScaled.ncols = width;

            headCamInfo = &headCamInfoScaled;
          }

          Messages::CameraCalibration headCalibMsg = {
            headCamInfo->focalLength_x,
            headCamInfo->focalLength_y,
            headCamInfo->center_x,
            headCamInfo->center_y,
            headCamInfo->skew,
            headCamInfo->nrows,
            headCamInfo->ncols,
#           ifdef SIMULATOR
            0 // This is NOT a real robot
#           else
            1 // This is a real robot
#           endif
          };

          if(!HAL::RadioSendMessage(CameraCalibration_ID, &headCalibMsg)) {
            PRINT("Failed to send camera calibration message.\n");
          }
        }
      } // ProcessImageRequestMessage()

      void ProcessSetHeadlightMessage(const SetHeadlight& msg) {
        HAL::SetHeadlights(msg.intensity > 0);
      }

      void ProcessSetDefaultLightsMessage(const SetDefaultLights& msg) {
        /*
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
         */
        for(s32 i=0; i<NUM_BACKPACK_LEDS; ++i) {
          HAL::SetLED((LEDId)i, msg.color>>8);
        }
      }

      void ProcessSetWheelControllerGainsMessage(const SetWheelControllerGains& msg) {
        WheelController::SetGains(msg.kpLeft, msg.kiLeft, msg.maxIntegralErrorLeft,
                                  msg.kpRight, msg.kiRight, msg.maxIntegralErrorRight);
      }

      void ProcessSetHeadControllerGainsMessage(const SetHeadControllerGains& msg) {
        HeadController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
      }

      void ProcessSetLiftControllerGainsMessage(const SetLiftControllerGains& msg) {
        LiftController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
      }

      void ProcessSetSteeringControllerGainsMessage(const SetSteeringControllerGains& msg) {
        SteeringController::SetGains(msg.k1, msg.k2);
      }

      void ProcessSetVisionSystemParamsMessage(const SetVisionSystemParams& msg) {
        PRINT("Deprecated SetVisionSystemParams message received!\n");
      }

      void ProcessSetFaceDetectParamsMessage(const SetFaceDetectParams& msg) {
        PRINT("Deprecated SetVisionSystemParams message received!\n");
      }

      void ProcessIMURequestMessage(const IMURequest& msg) {
        IMUFilter::RecordAndSend(msg.length_ms);
      }


      void ProcessFaceTrackingMessage(const FaceTracking& msg)
      {
      }

      void ProcessAbortDockingMessage(const AbortDocking& msg)
      {
        DockingController::ResetDocker();
      }

      //
      // Animation related:
      //

//      void ProcessPlayAnimationMessage(const PlayAnimation& msg) {
//        //PRINT("Processing play animation message\n");
//        AnimationController::Play((AnimationID_t)msg.animationID, msg.numLoops);
//      }

//      void ProcessTransitionToStateAnimationMessage(const TransitionToStateAnimation& msg) {
//        AnimationController::TransitionAndPlay(msg.transitionAnimID, msg.stateAnimID);
//      }

      void ProcessAbortAnimationMessage(const AbortAnimation& msg)
      {
        AnimationController::Clear();
      }
      
      template<typename KF_TYPE>
      static inline void ProcessAnimKeyFrameHelper(const KF_TYPE& msg)
      { 
        if(AnimationController::BufferKeyFrame(msg) != RESULT_OK) {
          PRINT("Failed to buffer a keyframe! Clearing Animation buffer!\n");
          AnimationController::Clear();
        }
      }
      
#     define DEFINE_PROCESS_KEYFRAME_METHOD(__MSG_TYPE__) \
void Process##__MSG_TYPE__##Message(const __MSG_TYPE__& msg) { ProcessAnimKeyFrameHelper(msg); }

      DEFINE_PROCESS_KEYFRAME_METHOD(AnimKeyFrame_AudioSample)
      DEFINE_PROCESS_KEYFRAME_METHOD(AnimKeyFrame_AudioSilence)
      DEFINE_PROCESS_KEYFRAME_METHOD(AnimKeyFrame_FaceImage)
      DEFINE_PROCESS_KEYFRAME_METHOD(AnimKeyFrame_FacePosition)
      DEFINE_PROCESS_KEYFRAME_METHOD(AnimKeyFrame_HeadAngle)
      DEFINE_PROCESS_KEYFRAME_METHOD(AnimKeyFrame_LiftHeight)
      DEFINE_PROCESS_KEYFRAME_METHOD(AnimKeyFrame_BackpackLights)
      DEFINE_PROCESS_KEYFRAME_METHOD(AnimKeyFrame_BodyMotion)
      DEFINE_PROCESS_KEYFRAME_METHOD(AnimKeyFrame_EndOfAnimation)

      void ProcessPanAndTiltHeadMessage(const PanAndTiltHead& msg)
      {
        // TODO: Move this to some kind of VisualInterestTrackingController or something
        
        HeadController::SetDesiredAngle(msg.headTiltAngle_rad, 0.1f, 0.1f, 0.1f);
        if(msg.bodyPanAngle_rad != 0.f) {
          SteeringController::ExecutePointTurn(msg.bodyPanAngle_rad, 50.f, 10.f, 10.f, true);
        }
      }

      void ProcessSetCarryStateMessage(const SetCarryState& msg)
      {
        PickAndPlaceController::SetCarryState((CarryState_t)msg.state);
      }

      void ProcessSetBackpackLightsMessage(const SetBackpackLights& msg)
      {
        for(s32 i=0; i<NUM_BACKPACK_LEDS; ++i) {
          BackpackLightController::SetParams((LEDId)i, msg.onColor[i], msg.offColor[i],
                                             msg.onPeriod_ms[i], msg.offPeriod_ms[i],
                                             msg.transitionOnPeriod_ms[i], msg.transitionOffPeriod_ms[i]);
        }
      }

      void ProcessTapBlockOnGroundMessage(const TapBlockOnGround& msg)
      {
        LiftController::TapBlockOnGround(msg.numTaps);
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
        HAL::SetBlockLight(msg.blockID, msg.onColor, msg.offColor, msg.onPeriod_ms, msg.offPeriod_ms,
                           msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms);
      }


      void ProcessSetBlockBeingCarriedMessage(const SetBlockBeingCarried& msg)
      {
        // TODO: need to add this hal.h and implement
        // HAL::SetBlockBeingCarried(msg.blockID, msg.isBeingCarried);
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

        if(HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::RobotState), m, false, false) == true) {
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


      bool ReceivedInit()
      {

        return initReceived_;
      }


      void ResetInit()
      {
        initReceived_ = false;
        lastPingTime_ = 0;

        HAL::SetImageSendMode(ISM_STREAM, Vision::CAMERA_RES_CVGA);
      }


#     ifdef SIMULATOR
      Result CompressAndSendImage(const Embedded::Array<u8> &img, const TimeStamp_t captureTime)
      {
        Messages::ImageChunk m;

        switch(img.get_size(0)) {
          case 240:
            AnkiConditionalErrorAndReturnValue(img.get_size(1)==320*3, RESULT_FAIL, "CompressAndSendImage",
                                               "Unrecognized resolution: %dx%d.\n", img.get_size(1)/3, img.get_size(0));
            m.resolution = Vision::CAMERA_RES_QVGA;
            break;

          case 296:
            AnkiConditionalErrorAndReturnValue(img.get_size(1)==400*3, RESULT_FAIL, "CompressAndSendImage",
                                               "Unrecognized resolution: %dx%d.\n", img.get_size(1)/3, img.get_size(0));
            m.resolution = Vision::CAMERA_RES_CVGA;
            break;
            
          case 480:
            AnkiConditionalErrorAndReturnValue(img.get_size(1)==640*3, RESULT_FAIL, "CompressAndSendImage",
                                               "Unrecognized resolution: %dx%d.\n", img.get_size(1)/3, img.get_size(0));
            m.resolution = Vision::CAMERA_RES_VGA;
            break;

          default:
            AnkiError("CompressAndSendImage", "Unrecognized resolution: %dx%d.\n", img.get_size(1)/3, img.get_size(0));
            return RESULT_FAIL;
        }

        static u32 imgID = 0;
        const cv::vector<int> compressionParams = {
          CV_IMWRITE_JPEG_QUALITY, IMAGE_SEND_JPEG_COMPRESSION_QUALITY
        };

        cv::Mat cvImg;
        cvImg = cv::Mat(img.get_size(0), img.get_size(1)/3, CV_8UC3, const_cast<void*>(img.get_buffer()));
        cvtColor(cvImg, cvImg, CV_BGR2RGB);

        cv::vector<u8> compressedBuffer;
        cv::imencode(".jpg",  cvImg, compressedBuffer, compressionParams);

        const u32 numTotalBytes = compressedBuffer.size();

        //PRINT("Sending frame with capture time = %d at time = %d\n", captureTime, HAL::GetTimeStamp());

        m.frameTimeStamp = captureTime;
        m.imageId = ++imgID;
        m.chunkId = 0;
        m.chunkSize = IMAGE_CHUNK_SIZE;
        m.imageChunkCount = ceilf((f32)numTotalBytes / IMAGE_CHUNK_SIZE);
        m.imageEncoding = Vision::IE_JPEG_COLOR;

        u32 totalByteCnt = 0;
        u32 chunkByteCnt = 0;

        for(s32 i=0; i<numTotalBytes; ++i)
        {
          m.data[chunkByteCnt] = compressedBuffer[i];

          ++chunkByteCnt;
          ++totalByteCnt;

          if (chunkByteCnt == IMAGE_CHUNK_SIZE) {
            //PRINT("Sending image chunk %d\n", m.chunkId);
            HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::ImageChunk), &m, false, true);
            ++m.chunkId;
            chunkByteCnt = 0;
          } else if (totalByteCnt == numTotalBytes) {
            // This should be the last message!
            //PRINT("Sending LAST image chunk %d\n", m.chunkId);
            m.chunkSize = chunkByteCnt;
            HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::ImageChunk), &m, false, true);
          }
        } // for each byte in the compressed buffer

        return RESULT_OK;
      } // CompressAndSendImage()

#     endif // SIMULATOR


    } // namespace Messages

    namespace HAL {
      bool RadioSendMessage(const int msgID, const void *buffer, const bool reliable, const bool hot)
      {
        const u32 size = Messages::GetSize((const Messages::ID)msgID);
        if (RadioIsConnected())
        {
          if (reliable)
          {
            if (ReliableTransport_SendMessage((const uint8_t*)buffer, size, &connection, eRMT_SingleReliableMessage, hot, msgID) == false) // failed to queue reliable message!
            {
              // Have to drop the connection
              //PRINT("Dropping connection because can't queue reliable messages\r\n");
              ReliableTransport_Disconnect(&connection);
              Receiver_OnDisconnect(&connection);
              return false;
            }
            else
            {
              return true;
            }
          }
          else
          {
            return ReliableTransport_SendMessage((const uint8_t*)buffer, size, &connection, eRMT_SingleUnreliableMessage, hot, msgID);
          }
        }
        else
        {
          return false;
        }
      }

#ifndef SIMULATOR
      bool RadioSendImageChunk(const void* chunkData, const uint16_t length)
      {
        bool result = ReliableTransport_SendMessage((uint8_t*)chunkData, length, &connection, eRMT_SingleUnreliableMessage, true, Messages::ImageChunk_ID);
        return result;
      }

      void FlashBlockIDs()
      {
        // THIS DOESN'T WORK FOR now
      }
#endif

    } // namespace HAL

  } // namespace Cozmo
} // namespace Anki

// Shim for reliable transport
bool UnreliableTransport_SendPacket(uint8_t* buffer, uint16_t bufferSize)
{
  return Anki::Cozmo::HAL::RadioSendPacket(buffer, bufferSize);
}

void Receiver_ReceiveData(uint8_t* buffer, uint16_t bufferSize, ReliableConnection* connection)
{
  const Anki::Cozmo::Messages::ID msgID = static_cast<Anki::Cozmo::Messages::ID>(buffer[0]);
  const u32 size = Anki::Cozmo::Messages::GetSize(msgID);

  if ((size + 1) == bufferSize)
  {
    memcpy(Anki::Cozmo::Messages::msgBuff_, buffer+1, bufferSize-1); // Copy message into aligned memory
    Anki::Cozmo::Messages::ProcessMessage(msgID, Anki::Cozmo::Messages::msgBuff_);
  }
  else
  {
    Anki::Cozmo::PRINT("Receiver got %d expeted len %d was %d\n", msgID, size, bufferSize);
  }
}

void Receiver_OnConnectionRequest(ReliableConnection* connection)
{
  Anki::Cozmo::PRINT("ReliableTransport new connection\n");
  ReliableTransport_FinishConnection(connection); // Accept the connection
  Anki::Cozmo::HAL::RadioUpdateState(1, 0);
}

void Receiver_OnConnected(ReliableConnection* connection)
{
  Anki::Cozmo::PRINT("ReliableTransport connection completed\n");
  Anki::Cozmo::HAL::RadioUpdateState(1, 0);
}

void Receiver_OnDisconnect(ReliableConnection* connection)
{
  Anki::Cozmo::PRINT("ReliableTransport disconnected\n");
  ReliableConnection_Init(connection, NULL); // Reset the connection
  Anki::Cozmo::HAL::RadioUpdateState(0, 0);
}
