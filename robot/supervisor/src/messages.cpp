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
#include "clad/types/activeObjectTypes.h"

#define SEND_TEXT_REDIRECT_TO_STDOUT 0

namespace Anki {
  namespace Cozmo {
    ReliableConnection connection;
    namespace Messages {

      namespace {

        u8 pktBuffer_[2048];

        // For waiting for a particular message ID
        const u32 LOOK_FOR_MESSAGE_TIMEOUT = 1000000;
        RobotInterface::EngineToRobot::Tag lookForID_ = RobotInterface::EngineToRobot::INVALID;
        u32 lookingStartTime_ = 0;

        static RobotState robotState_;

        // History of the last 2 RobotState messages that were sent to the basestation.
        // Used to avoid repeating a send.
        TimeStamp_t robotStateSendHist_[2];
        u8 robotStateSendHistIdx_ = 0;

        // Flag for receipt of Init message
        bool initReceived_ = false;

      } // private namespace

// #pragma mark --- Messages Method Implementations ---

      Result Init()
      {
        ReliableTransport_Init();
        ReliableConnection_Init(&connection, NULL); // We only have one connection so dest pointer is superfluous
        return RESULT_OK;
      }
      
      // Checks whitelist of all messages that are allowed to
      // be processed while the robot is picked up.
      bool IgnoreMessageDuringPickup(const RobotInterface::EngineToRobot::Tag msgID) {
        
        if (!IMUFilter::IsPickedUp()) {
          return false;
        }
        
        switch(msgID) {
          case RobotInterface::EngineToRobot::Tag_moveHead:
          case RobotInterface::EngineToRobot::Tag_headAngle:
          case RobotInterface::EngineToRobot::Tag_headAngleUpdate:
          case RobotInterface::EngineToRobot::Tag_stop:
          case RobotInterface::EngineToRobot::Tag_clearPath:
          case RobotInterface::EngineToRobot::Tag_absLocalizationUpdate:
          case RobotInterface::EngineToRobot::Tag_syncTime:
          case RobotInterface::EngineToRobot::Tag_imageRequest:
          case RobotInterface::EngineToRobot::Tag_setControllerGains:
          case RobotInterface::EngineToRobot::Tag_setCarryState:
          case RobotInterface::EngineToRobot::Tag_setBackpackLights:
          case RobotInterface::EngineToRobot::Tag_setBlockLights:
          case RobotInterface::EngineToRobot::Tag_flashBlockIDs:
          case RobotInterface::EngineToRobot::Tag_setBlockBeingCarried:
            return false;
          default:
            break;
        }
        
        return true;
      }
      
      void ProcessBadTag_EngineToRobot(RobotInterface::EngineToRobot::Tag badTag)
      {
        PRINT("Received message with bad tag %02x\n", badTag);
      }
      
      void ProcessMessage(RobotInterface::EngineToRobot& msg)
      {
        if (!IgnoreMessageDuringPickup(msg.tag))
        {
          #include "clad/robotInterface/messageEngineToRobot_switch.def"
        }
        if (lookForID_ != RobotInterface::EngineToRobot::INVALID)
        {
          if (msg.tag == lookForID_)
          {
            lookForID_ = RobotInterface::EngineToRobot::INVALID;
          }
        }
      } // ProcessMessage()

      void LookForID(const RobotInterface::EngineToRobot::Tag msgID) {
        lookForID_ = msgID;
        lookingStartTime_ = HAL::GetMicroCounter();
      }

      bool StillLookingForID(void) {
        if(lookForID_ == RobotInterface::EngineToRobot::INVALID) {
          return false;
        }
        else if(HAL::GetMicroCounter() - lookingStartTime_ > LOOK_FOR_MESSAGE_TIMEOUT) {
            PRINT("Timed out waiting for message ID %d.\n", lookForID_);
            lookForID_ = RobotInterface::EngineToRobot::INVALID;

          return false;
        }

        return true;

      }


      void UpdateRobotStateMsg()
      {
        robotState_.timestamp = HAL::GetTimeStamp();

        Radians poseAngle;

        robotState_.pose_frame_id = Localization::GetPoseFrameId();

        Localization::GetCurrentMatPose(robotState_.pose.x, robotState_.pose.y, poseAngle);
        robotState_.pose.z = 0;
        robotState_.pose.angle = poseAngle.ToFloat();
        robotState_.pose.pitch_angle = IMUFilter::GetPitch();

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
      }

      RobotState const& GetRobotStateMsg() {
        return robotState_;
      }


// #pragma --- Message Dispatch Functions ---


      void Process_syncTime(const RobotInterface::SyncTime& msg)
      {

        PRINT("Robot received SyncTime message from basestation with ID=%d and syncTime=%d.\n",
              msg.robotID, msg.syncTime);

        initReceived_ = true;

        // TODO: Compare message ID to robot ID as a handshake?

        // Poor-man's time sync to basestation, for now.
        HAL::SetTimeStamp(msg.syncTime);

        // Reset pose history and frameID to zero
        Localization::ResetPoseFrame();

        // Reset number of bytes played in animation buffer
        AnimationController::ClearNumBytesPlayed();

      } // ProcessRobotInit()


      void Process_absLocalizationUpdate(const RobotInterface::AbsoluteLocalizationUpdate& msg)
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

      } // ProcessAbsLocalizationUpdateMessage()


      void Process_dockingErrorSignal(const DockingErrorSignal& msg)
      {
        DockingController::SetDockingErrorSignalMessage(msg);
      }

      void ProcessBTLEMessages()
      {
        u32 dataLen;

        //ReliableConnection_printState(&connection);

        while((dataLen = HAL::RadioGetNextPacket(pktBuffer_)) > 0)
        {
          s16 res = ReliableTransport_ReceiveData(&connection, pktBuffer_, dataLen);
          if (res < 0)
          {
#ifdef SIMULATOR
            printf("ReliableTransport didn't accept packet: %d\n", res);
#else
            HAL::BoardPrintf("ReliableTransport didn't accept packet: %d\n", res);
#endif
          }
        }

        if (HAL::RadioIsConnected())
        {
          if (ReliableTransport_Update(&connection) == false) // Connection has timed out
          {
            Receiver_OnDisconnect(&connection);
						// Can't print anything because we have no where to send it
					}
        }
      }

      void Process_clearPath(const RobotInterface::ClearPath& msg) {
        SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
        PathFollower::ClearPath();
        //SteeringController::ExecuteDirectDrive(0,0);
      }

      void Process_appendPathSegArc(const RobotInterface::AppendPathSegmentArc& msg) {
        PathFollower::AppendPathSegment_Arc(0, msg.x_center_mm, msg.y_center_mm,
                                            msg.radius_mm, msg.startRad, msg.sweepRad,
                                            msg.speed.target, msg.speed.accel, msg.speed.decel);
      }

      void Process_appendPathSegLine(const RobotInterface::AppendPathSegmentLine& msg) {
        PathFollower::AppendPathSegment_Line(0, msg.x_start_mm, msg.y_start_mm,
                                             msg.x_end_mm, msg.y_end_mm,
                                             msg.speed.target, msg.speed.accel, msg.speed.decel);
      }

      void Process_appendPathSegPointTurn(const RobotInterface::AppendPathSegmentPointTurn& msg) {
        PathFollower::AppendPathSegment_PointTurn(0, msg.x_center_mm, msg.y_center_mm, msg.targetRad,
                                                  msg.speed.target, msg.speed.accel, msg.speed.decel, msg.useShortestDir);
      }

      void Process_trimPath(const RobotInterface::TrimPath& msg) {
        PathFollower::TrimPath(msg.numPopFrontSegments, msg.numPopBackSegments);
      }

      void Process_executePath(const RobotInterface::ExecutePath& msg) {
        PRINT("Starting path %d\n", msg.pathID);
        PathFollower::StartPathTraversal(msg.pathID, msg.useManualSpeed);
      }

      void Process_dockWithObject(const DockWithObject& msg)
      {
          PRINT("RECVD DockToBlock (action %d, manualSpeed %d)\n", msg.action, msg.useManualSpeed);

          PickAndPlaceController::DockToBlock(msg.useManualSpeed,
                                              static_cast<DockAction>(msg.action));
      }

      void Process_placeObjectOnGround(const PlaceObjectOnGround& msg)
      {
        //PRINT("Received PlaceOnGround message.\n");
        PickAndPlaceController::PlaceOnGround(msg.rel_x_mm, msg.rel_y_mm, msg.rel_angle, msg.useManualSpeed);
      }

      void Process_drive(const RobotInterface::DriveWheels& msg) {

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

      void Process_driveCurvature(const RobotInterface::DriveWheelsCurvature& msg) {
        /*
        PathFollower::ClearPath();

        SpeedController::SetUserCommandedDesiredVehicleSpeed(msg.speed_mmPerSec);
        SpeedController::SetUserCommandedAcceleration(msg.accel_mmPerSec2);
        SpeedController::SetUserCommandedDeceleration(msg.decel_mmPerSec2);
        */
      }

      void Process_moveLift(const RobotInterface::MoveLift& msg) {
        LiftController::SetAngularVelocity(msg.speed_rad_per_sec);
      }

      void Process_moveHead(const RobotInterface::MoveHead& msg) {
        HeadController::SetAngularVelocity(msg.speed_rad_per_sec);
      }

      void Process_liftHeight(const RobotInterface::SetLiftHeight& msg) {
        PRINT("Moving lift to %f (maxSpeed %f, duration %f)\n", msg.height_mm, msg.max_speed_rad_per_sec, msg.duration_sec);
        LiftController::SetMaxSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        LiftController::SetDesiredHeight(msg.height_mm, 0.1f, 0.1f, msg.duration_sec);
      }

      void Process_headAngle(const RobotInterface::SetHeadAngle& msg) {
        PRINT("Moving head to %f (maxSpeed %f, duration %f)\n", msg.angle_rad, msg.max_speed_rad_per_sec, msg.duration_sec);
        HeadController::SetMaxSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        HeadController::SetDesiredAngle(msg.angle_rad, 0.1f, 0.1f, msg.duration_sec);
      }
      
      void Process_headAngleUpdate(const RobotInterface::HeadAngleUpdate& msg) {
        HeadController::SetAngleRad(msg.newAngle);
      }

      void Process_panAndTiltHead(const RobotInterface::PanAndTilt& msg)
      {
        // TODO: Move this to some kind of VisualInterestTrackingController or something

        HeadController::SetDesiredAngle(msg.headTiltAngle_rad, 0.1f, 0.1f, 0.1f);
        if(msg.bodyPanAngle_rad != 0.f) {
          SteeringController::ExecutePointTurn(msg.bodyPanAngle_rad, 50.f, 10.f, 10.f, true);
        }
      }
      
      void Process_setCarryState(const CarryState& state)
      {
        PickAndPlaceController::SetCarryState(state);
      }
      
      

      void Process_turnInPlaceAtSpeed(const RobotInterface::TurnInPlaceAtSpeed& msg) {
        PRINT("Turning in place at %f rad/s (%f rad/s2)\n", msg.speed_rad_per_sec, msg.accel_rad_per_sec2);
        SteeringController::ExecutePointTurn(msg.speed_rad_per_sec, msg.accel_rad_per_sec2);
      }
      
      void Process_stop(const RobotInterface::StopAllMotors& msg) {
        SteeringController::ExecuteDirectDrive(0,0);
        LiftController::SetAngularVelocity(0);
        HeadController::SetAngularVelocity(0);
      }

      void Process_startControllerTestMode(const StartControllerTestMode& msg)
      {
        if (msg.mode < TM_NUM_TESTS) {
          TestModeController::Start((TestMode)(msg.mode), msg.p1, msg.p2, msg.p3);
        } else {
          PRINT("Unknown test mode %d received\n", msg.mode);
        }
      }

      void Process_imageRequest(const RobotInterface::ImageRequest& msg)
      {
        PRINT("Image requested (mode: %d, resolution: %d)\n", msg.sendMode, msg.resolution);

        ImageSendMode imageSendMode = static_cast<ImageSendMode>(msg.sendMode);
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

          RobotInterface::CameraCalibration headCalibMsg = {
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

          if(RobotInterface::SendMessage(headCalibMsg)) {
            PRINT("Failed to send camera calibration message.\n");
          }
        }
      } // ProcessImageRequestMessage()

      void Process_setControllerGains(const Anki::Cozmo::RobotInterface::ControllerGains& msg) {
        switch (msg.controller)
        {
          case RobotInterface::controller_wheel:
          {
            WheelController::SetGains(msg.kp, msg.ki, msg.maxIntegralError,
                                      msg.kp, msg.ki, msg.maxIntegralError);
            break;
          }
          case RobotInterface::controller_head:
          {
            HeadController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
            break;
          }
          case RobotInterface::controller_lift:
          {
            LiftController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
            break;
          }
          case RobotInterface::controller_stearing:
          {
            SteeringController::SetGains(msg.kp, msg.ki); // Coopting structure
            break;
          }
          default:
          {
            PRINT("SetControllerGains invalid controller: %d\n", msg.controller);
          }
        }
      }

      void Process_abortDocking(const AbortDocking& msg)
      {
        DockingController::ResetDocker();
      }

      //
      // Animation related:
      //

//      void ProcessPlayAnimation(const PlayAnimation& msg) {
//        //PRINT("Processing play animation message\n");
//        AnimationController::Play((AnimationID_t)msg.animationID, msg.numLoops);
//      }

//      void ProcessTransitionToStateAnimation(const TransitionToStateAnimation& msg) {
//        AnimationController::TransitionAndPlay(msg.transitionAnimID, msg.stateAnimID);
//      }

      void Process_abortAnimation(const RobotInterface::AbortAnimation& msg)
      {
        AnimationController::Clear();
      }

      // Group processor for all animation key frame messages
      void Process_anim(const RobotInterface::EngineToRobot& msg)
      {
        if(AnimationController::BufferKeyFrame(msg) != RESULT_OK) {
          //PRINT("Failed to buffer a keyframe! Clearing Animation buffer!\n");
          AnimationController::Clear();
        }
      }

      void Process_setBackpackLights(const RobotInterface::BackpackLights& msg)
      {
        for(s32 i=0; i<NUM_BACKPACK_LEDS; ++i) {
          BackpackLightController::SetParams((LEDId)i, msg.lights[i].onColor, msg.lights[i].offColor,
                                             msg.lights[i].onPeriod_ms, msg.lights[i].offPeriod_ms,
                                             msg.lights[i].transitionOnPeriod_ms, msg.lights[i].transitionOffPeriod_ms);
        }
      }

      // --------- Block control messages ----------

      void Process_flashBlockIDs(const RobotInterface::FlashBlockIDs& msg)
      {
        // Start flash pattern on blocks
        HAL::FlashBlockIDs();
      }

      void Process_setBlockLights(const RobotInterface::BlockLights& msg)
      {
        HAL::SetBlockLight(msg.blockID, msg.lights);
      }


      void Process_setBlockBeingCarried(const RobotInterface::SetBlockBeingCarried& msg)
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

        if(RobotInterface::SendMessage(*m, false, false) == true) {
          // Update send history
          robotStateSendHist_[robotStateSendHistIdx_] = m->timestamp;
          if (++robotStateSendHistIdx_ > 1) robotStateSendHistIdx_ = 0;
          RobotInterface::AnimationState am;
          am.timestamp = m->timestamp;
          am.numAnimBytesPlayed = AnimationController::GetTotalNumBytesPlayed();
          RobotInterface::SendMessage(am, false, false);
          return RESULT_OK;
        } else {
          return RESULT_FAIL;
        }
      }


      int SendText(const char *format, ...)
      {
        va_list argptr;
        va_start(argptr, format);
#if SEND_TEXT_REDIRECT_TO_STDOUT
        // print to console - works in webots environment.
        printf(format, argptr);
#else
        SendText(format, argptr);
#endif
        va_end(argptr);

        return 0;
      }

      int SendText(const char *format, va_list vaList)
      {
        RobotInterface::PrintText m;
        int len;
        m.logLevel = 0;
                
        #define MAX_SEND_TEXT_LENGTH 255 // uint_8 definition in messageRobotToEngine.clad
        memset(m.text, 0, MAX_SEND_TEXT_LENGTH);

        // Create formatted text
        len = vsnprintf(m.text, MAX_SEND_TEXT_LENGTH, format, vaList);
        
        if (len > 0)
        {
          m.text_length = len;
          RobotInterface::SendMessage(m);
        }

        return 0;
      }

      bool ReceivedInit()
      {

        return initReceived_;
      }


      void ResetInit()
      {
        initReceived_ = false;

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

        const u32 numTotalBytes = static_cast<u32>(compressedBuffer.size());

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
      bool RadioSendMessage(const void *buffer, const u16 size, const int msgID, const bool reliable, const bool hot)
      {
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
  Anki::Cozmo::RobotInterface::EngineToRobot msgBuf;
  
  // Copy into structured memory  
  memcpy(msgBuf.GetBuffer(), buffer, bufferSize);
  if (!msgBuf.IsValid())
  {
    Anki::Cozmo::PRINT("Receiver got %02x[%d] invald\n", buffer[0], bufferSize);
  }
  else if (msgBuf.Size() != bufferSize)
  {
    Anki::Cozmo::PRINT("Parsed message size error %d != %d\n", bufferSize, msgBuf.Size());
  }
  else
  {
    Anki::Cozmo::Messages::ProcessMessage(msgBuf);
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
  Anki::Cozmo::HAL::RadioUpdateState(0, 0);   // Must mark connection disconnected BEFORE trying to print
  Anki::Cozmo::PRINT("ReliableTransport disconnected\n");
  ReliableConnection_Init(connection, NULL); // Reset the connection
  Anki::Cozmo::HAL::RadioUpdateState(0, 0);
}
