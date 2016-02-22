#include "messages.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include <math.h>

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

#ifndef TARGET_K02
#include "../sim_hal/transport/IUnreliableTransport.h"
#include "../sim_hal/transport/IReceiver.h"
#include "../sim_hal/transport/reliableTransport.h"
#include "anki/vision/CameraSettings.h"
#include <string.h>
#endif

#include "liftController.h"
#include "headController.h"
#include "imuFilter.h"
#include "backpackLightController.h"
#include "blockLightController.h"
#include "speedController.h"
#include "steeringController.h"
#include "wheelController.h"
#include "localization.h"
#include "pathFollower.h"
#include "dockingController.h"
#include "pickAndPlaceController.h"
#include "testModeController.h"
#include "battery.h"
#ifndef TARGET_K02
#include "animationController.h"
#endif

#define SEND_TEXT_REDIRECT_TO_STDOUT 0

namespace Anki {
  namespace Cozmo {
#ifndef TARGET_K02
    ReliableConnection connection;
#endif
    namespace Messages {

      namespace {
#ifndef TARGET_K02
        u8 pktBuffer_[2048];
#endif
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
#ifndef TARGET_K02
        ReliableTransport_Init();
        ReliableConnection_Init(&connection, NULL); // We only have one connection so dest pointer is superfluous
#endif
        return RESULT_OK;
      }

      void ProcessBadTag_EngineToRobot(RobotInterface::EngineToRobot::Tag badTag)
      {
        AnkiWarn( 106, "Messages.ProcessBadTag_EngineToRobot.Recvd", 355, "Received message with bad tag %z", 1, badTag);
      }

      void ProcessMessage(RobotInterface::EngineToRobot& msg)
      {
        #ifdef TARGET_K02
        #include "clad/robotInterface/messageEngineToRobot_switch.def"
        #else
        #include "clad/robotInterface/messageEngineToRobot_switch_group_anim.def"
        #endif
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
            AnkiWarn( 107, "Messages.StillLookingForID.Timeout", 356, "Timed out waiting for message ID %d.", 1, lookForID_);
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
        robotState_.status |= (PathFollower::IsTraversingPath() ? IS_PATHING : 0);
        robotState_.status |= (LiftController::IsInPosition() ? LIFT_IN_POS : 0);
        robotState_.status |= (HeadController::IsInPosition() ? HEAD_IN_POS : 0);
        robotState_.status |= HAL::BatteryIsOnCharger() ? IS_ON_CHARGER : 0;
        robotState_.status |= HAL::BatteryIsCharging() ? IS_CHARGING : 0;
        robotState_.status |= HAL::IsCliffDetected() ? CLIFF_DETECTED : 0;
      }

      RobotState const& GetRobotStateMsg() {
        return robotState_;
      }


// #pragma --- Message Dispatch Functions ---


      void Process_syncTime(const RobotInterface::SyncTime& msg)
      {
        AnkiInfo( 100, "Messages.Process_syncTime.Recvd", 305, "", 0);

        RobotInterface::SyncTimeAck syncTimeAckMsg;
        if (!RobotInterface::SendMessage(syncTimeAckMsg)) {
          AnkiWarn( 102, "Messages.Process_syncTime.AckFailed", 352, "Failed to send syncTimeAckMsg", 0);
        }

        initReceived_ = true;

        // TODO: Compare message ID to robot ID as a handshake?

        // Poor-man's time sync to basestation, for now.
        HAL::SetTimeStamp(msg.syncTime);

        // Reset pose history and frameID to zero
        Localization::ResetPoseFrame();
#ifndef TARGET_K02
        // Reset number of bytes/audio frames played in animation buffer
        AnimationController::ClearNumBytesPlayed();
        AnimationController::ClearNumAudioFramesPlayed();
#endif
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
        AnkiInfo( 115, "Messages.Process_absLocalizationUpdate.Recvd", 363, "Result %d, currTime=%d, updated frame time=%d: (%.3f,%.3f) at %.1f degrees (frame = %d)\n", 7,
              res,
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

#ifndef TARGET_K02
      extern "C" void ProcessMessage(u8* buffer, u16 bufferSize)
      {
         //XXX  "Implement ProcessMessage"
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
#endif

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
        AnkiInfo( 103, "Messages.Process_executePath.StartingPath", 347, "%d", 1, msg.pathID);
        PathFollower::StartPathTraversal(msg.pathID, msg.useManualSpeed);
      }

      void Process_dockWithObject(const DockWithObject& msg)
      {
        AnkiInfo( 104, "Messages.Process_dockWithObject.Recvd", 353, "action %d, speed %f, acccel %f, manualSpeed %d", 4,
              msg.action, msg.speed_mmps, msg.accel_mmps2, msg.useManualSpeed);

        // Currently passing in default values for rel_x, rel_y, and rel_angle
        PickAndPlaceController::DockToBlock(msg.action,
                                            msg.speed_mmps,
                                            msg.accel_mmps2,
                                            0, 0, 0,
                                            msg.useManualSpeed);
      }

      void Process_placeObjectOnGround(const PlaceObjectOnGround& msg)
      {
        //AnkiInfo( 108, "Messages.Process_placeObjectOnGround.Recvd", 305, "", 0);
        PickAndPlaceController::PlaceOnGround(msg.speed_mmps,
                                              msg.accel_mmps2,
                                              msg.rel_x_mm,
                                              msg.rel_y_mm,
                                              msg.rel_angle,
                                              msg.useManualSpeed);
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
            AnkiInfo( 105, "Messages.Process_drive.Ignoring", 354, "Ignoring command because robot is currently following a path.", 0);
          }
          return;
        }

        //AnkiInfo( 116, "Messages.Process_drive.Executing", 364, "left=%f mm/s, right=%f mm/s", 2, msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);

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
        //AnkiInfo( 109, "Messages.Process_liftHeight.Recvd", 357, "height %f, maxSpeed %f, duration %f", 3, msg.height_mm, msg.max_speed_rad_per_sec, msg.duration_sec);
        LiftController::SetMaxSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        LiftController::SetDesiredHeight(msg.height_mm, 0.1f, 0.1f, msg.duration_sec);
      }

      void Process_headAngle(const RobotInterface::SetHeadAngle& msg) {
        //AnkiInfo( 117, "Messages.Process_headAngle.Recvd", 365, "angle %f, maxSpeed %f, duration %f", 3, msg.angle_rad, msg.max_speed_rad_per_sec, msg.duration_sec);
        HeadController::SetMaxSpeedAndAccel(msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        HeadController::SetDesiredAngle(msg.angle_rad, 0.1f, 0.1f, msg.duration_sec);
      }

      void Process_headAngleUpdate(const RobotInterface::HeadAngleUpdate& msg) {
        HeadController::SetAngleRad(msg.newAngle);
      }

      void Process_setBodyAngle(const RobotInterface::SetBodyAngle& msg)
      {
        SteeringController::ExecutePointTurn(msg.angle_rad, msg.max_speed_rad_per_sec,
                                             msg.accel_rad_per_sec2,
                                             msg.accel_rad_per_sec2, true);
      }

      void Process_setCarryState(const CarryStateUpdate& update)
      {
        PickAndPlaceController::SetCarryState(update.state);
      }

      void Process_imuRequest(const Anki::Cozmo::RobotInterface::ImuRequest& msg)
      {
        IMUFilter::RecordAndSend(msg.length_ms);
      }

      void Process_turnInPlaceAtSpeed(const RobotInterface::TurnInPlaceAtSpeed& msg) {
        //AnkiInfo( 118, "Messages.Process_turnInPlaceAtSpeed.Recvd", 366, "speed %f rad/s, accel %f rad/s2", 2, msg.speed_rad_per_sec, msg.accel_rad_per_sec2);
        SteeringController::ExecutePointTurn(msg.speed_rad_per_sec, msg.accel_rad_per_sec2);
      }

      void Process_stop(const RobotInterface::StopAllMotors& msg) {
        LiftController::SetAngularVelocity(0);
        HeadController::SetAngularVelocity(0);
        SteeringController::ExecuteDirectDrive(0,0);
      }

      void Process_startControllerTestMode(const StartControllerTestMode& msg)
      {
        TestModeController::Start((TestMode)(msg.mode), msg.p1, msg.p2, msg.p3);
      }

      void Process_imageRequest(const RobotInterface::ImageRequest& msg)
      {
        AnkiInfo( 110, "Messages.Process_imageRequest.Recvd", 358, "mode: %d, resolution: %d", 2, msg.sendMode, msg.resolution);
#ifndef TARGET_K02
        HAL::SetImageSendMode(msg.sendMode, msg.resolution);

        // Send back camera calibration for this resolution
        const HAL::CameraInfo* headCamInfo = HAL::GetHeadCamInfo();

        // TODO: Just store CameraResolution in calibration data instead of height/width?

        if(headCamInfo == NULL) {
          AnkiWarn( 111, "Messages.Process_imageRequest.CalibNotFound", 359, "NULL HeadCamInfo retrieved from HAL.", 0);
        }
        else {
          HAL::CameraInfo headCamInfoScaled(*headCamInfo);
          const s32 width  = Vision::CameraResInfo[msg.resolution].width;
          const s32 height = Vision::CameraResInfo[msg.resolution].height;
          const f32 xScale = static_cast<f32>(width/headCamInfo->ncols);
          const f32 yScale = static_cast<f32>(height/headCamInfo->nrows);

          if(xScale != 1.f || yScale != 1.f)
          {
            AnkiInfo( 112, "Messages.Process_imageRequest.ScalingCalib", 360, "Scaling [%dx%d] camera calibration by [%.1f %.1f] to match requested resolution.", 4,
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

          if(!RobotInterface::SendMessage(headCalibMsg)) {
            AnkiWarn( 113, "Messages.Process_imageRequest.SendCalibFailed", 361, "Failed to send camera calibration message.", 0);
          }
        }
#endif
      } // ProcessImageRequestMessage()

      void Process_setControllerGains(const Anki::Cozmo::RobotInterface::ControllerGains& msg) {
        switch (msg.controller)
        {
          case RobotInterface::controller_wheel:
          {
            WheelController::SetGains(msg.kp, msg.ki, msg.maxIntegralError);
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
          case RobotInterface::controller_steering:
          {
            SteeringController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError); // Coopting structure
            break;
          }
          default:
          {
            AnkiWarn( 114, "Messages.Process_setControllerGains.InvalidController", 362, "controller: %d", 1, msg.controller);
          }
        }
      }

      void Process_setMotionModelParams(const RobotInterface::SetMotionModelParams& msg)
      {
        Localization::SetMotionModelParams(msg.slipFactor);
      }
      
      void Process_abortDocking(const AbortDocking& msg)
      {
        DockingController::ResetDocker();
      }

      void Process_abortAnimation(const RobotInterface::AbortAnimation& msg)
      {
#ifndef TARGET_K02
        AnimationController::Clear();
#endif
      }

      void Process_disableAnimTracks(const AnimKeyFrame::DisableAnimTracks& msg)
      {
#ifndef TARGET_K02
        AnimationController::DisableTracks(msg.whichTracks);
#endif
      }

      void Process_enableAnimTracks(const AnimKeyFrame::EnableAnimTracks& msg)
      {
#ifndef TARGET_K02
        AnimationController::EnableTracks(msg.whichTracks);
#endif
      }

#ifndef TARGET_K02
      // Group processor for all animation key frame messages
      void Process_anim(const RobotInterface::EngineToRobot& msg)
      {
        if(AnimationController::BufferKeyFrame(msg) != RESULT_OK) {
          //PRINT("Failed to buffer a keyframe! Clearing Animation buffer!\n");
          AnimationController::Clear();
        }
      }
#endif

      void Process_enablePickupParalysis(const RobotInterface::EnablePickupParalysis& msg)
      {
        IMUFilter::EnablePickupParalysis(msg.enable);
      }

      void Process_enableLiftPower(const RobotInterface::EnableLiftPower& msg)
      {
        if (msg.enable) {
          LiftController::Enable();
        } else {
          LiftController::Disable();
        }
      }


      // --------- Block control messages ----------

      void Process_flashObjectIDs(const  FlashObjectIDs& msg)
      {
        // Start flash pattern on blocks
        HAL::FlashBlockIDs();
      }

      void Process_setObjectBeingCarried(const ObjectBeingCarried& msg)
      {
        // TODO: need to add this hal.h and implement
        // HAL::SetBlockBeingCarried(msg.blockID, msg.isBeingCarried);
      }

      // ---------- Animation Key frame messages -----------
      void Process_animBlink(const Anki::Cozmo::AnimKeyFrame::Blink& msg)
      {
        // Hangled by the Espressif
      }
      void Process_animFaceImage(const Anki::Cozmo::AnimKeyFrame::FaceImage& msg)
      {
        // Handled by the Espressif
      }
      void Process_animHeadAngle(const Anki::Cozmo::AnimKeyFrame::HeadAngle& msg)
      {
        HeadController::SetDesiredAngle(DEG_TO_RAD(static_cast<f32>(msg.angle_deg)), 0.1f, 0.1f,
                                                static_cast<f32>(msg.time_ms)*.001f);
      }
      void Process_animBodyMotion(const Anki::Cozmo::AnimKeyFrame::BodyMotion& msg)
      {
        f32 leftSpeed=0, rightSpeed=0;
        if(msg.speed == 0) {
          // Stop
          leftSpeed = 0.f;
          rightSpeed = 0.f;
        } else if(msg.curvatureRadius_mm == s16_MAX ||
                  msg.curvatureRadius_mm == s16_MIN) {
          // Drive straight
          leftSpeed  = static_cast<f32>(msg.speed);
          rightSpeed = static_cast<f32>(msg.speed);
        } else if(msg.curvatureRadius_mm == 0) {
          SteeringController::ExecutePointTurn(DEG_TO_RAD_F32(msg.speed), 50);
          return;

        } else {
          // Drive an arc

          //if speed is positive, the left wheel should turn slower, so
          // it becomes the INNER wheel
          leftSpeed = static_cast<f32>(msg.speed) * (1.0f - WHEEL_DIST_HALF_MM / static_cast<f32>(msg.curvatureRadius_mm));

          //if speed is positive, the right wheel should turn faster, so
          // it becomes the OUTER wheel
          rightSpeed = static_cast<f32>(msg.speed) * (1.0f + WHEEL_DIST_HALF_MM / static_cast<f32>(msg.curvatureRadius_mm));
        }

        SteeringController::ExecuteDirectDrive(leftSpeed, rightSpeed);
      }
      void Process_animLiftHeight(const Anki::Cozmo::AnimKeyFrame::LiftHeight& msg)
      {
        LiftController::SetDesiredHeight(static_cast<f32>(msg.height_mm), 0.1f, 0.1f,
                                         static_cast<f32>(msg.time_ms)*.001f);

      }
      void Process_animAudioSample(const Anki::Cozmo::AnimKeyFrame::AudioSample&)
      {
        // Handled on the Espressif
      }
      void Process_animAudioSilence(const Anki::Cozmo::AnimKeyFrame::AudioSilence&)
      {
        // Handled on the Espressif
      }
      void Process_animFacePosition(const Anki::Cozmo::AnimKeyFrame::FacePosition&)
      {
        // Handled on the Espressif
      }
      void Process_animBackpackLights(const Anki::Cozmo::AnimKeyFrame::BackpackLights& msg)
      {
        for(s32 iLED=0; iLED<NUM_BACKPACK_LEDS; ++iLED) {
          HAL::SetLED(static_cast<LEDId>(iLED), msg.colors[iLED]);
        }
      }
      void Process_animEndOfAnimation(const Anki::Cozmo::AnimKeyFrame::EndOfAnimation&)
      {
        // Handled on the Espressif
      }
      void Process_animStartOfAnimation(const Anki::Cozmo::AnimKeyFrame::StartOfAnimation&)
      {
        // Handled on the Espressif
      }

      // ---------- Firmware over the air stubs for espressif -----------
      void Process_eraseFlash(Anki::Cozmo::RobotInterface::EraseFlash const&)
      {
        // Nothing to do here
      }
      void Process_writeFlash(RobotInterface::WriteFlash &)
      {
        // Nothing to do here
      }
      void Process_bodyUpgradeData(Anki::Cozmo::RobotInterface::BodyUpgradeData const&)
      {
        // Nothing to do here, handled in body
      }
      void Process_powerState(const PowerState& msg)
      {
        HAL::Battery::HandlePowerStateUpdate(msg);
      }
      void Process_getPropState(const PropState& msg)
      {
        HAL::GetPropState(msg.slot, msg.x, msg.y, msg.z, msg.shockCount);
      }
      void Process_enterBootloader(Anki::Cozmo::RobotInterface::EnterBootloader const&)
      {
				// Nothing to do here, handled in spi
      }
      void Process_triggerOTAUpgrade(Anki::Cozmo::RobotInterface::OTAUpgrade const&)
      {
        // Nothing to do here
      }
      void Process_writeNV(Anki::Cozmo::NVStorage::NVStorageBlob const&)
      {
        // Nothing to do here
      }
      void Process_readNV(Anki::Cozmo::NVStorage::NVStorageRead const&)
      {
        // Nothing to do here
      }
      void Process_setRawPWM(Anki::Cozmo::RawPWM const&)
      {
        // Not used here
      }
      void Process_radioConnected(const Anki::Cozmo::RobotInterface::RadioState& state)
      {
        HAL::RadioUpdateState(state.wifiConnected, false);
      }
			void Process_rtipVersion(const Anki::Cozmo::RobotInterface::RTIPVersionInfo&)
			{
				// Not processed here
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

        if(RobotInterface::SendMessage(*m) == true) {
          // Update send history
          robotStateSendHist_[robotStateSendHistIdx_] = m->timestamp;
          if (++robotStateSendHistIdx_ > 1) robotStateSendHistIdx_ = 0;
          #ifndef TARGET_K02
            AnimationController::SendAnimStateMessage();
          #endif
          return RESULT_OK;
        } else {
          return RESULT_FAIL;
        }
      }

#ifndef TARGET_K02
      int SendText(const char *format, ...)
      {
        va_list argptr;
        va_start(argptr, format);
#if SEND_TEXT_REDIRECT_TO_STDOUT
        // print to console - works in webots environment.
        vprintf(format, argptr);
#else
        SendText(format, argptr);
#endif
        va_end(argptr);

        return 0;
      }

      int SendText(const RobotInterface::LogLevel level, const char *format, va_list vaList)
      {
        RobotInterface::PrintText m;
        int len;

        #define MAX_SEND_TEXT_LENGTH 255 // uint_8 definition in messageRobotToEngine.clad
        memset(m.text, 0, MAX_SEND_TEXT_LENGTH);

        // Create formatted text
        len = vsnprintf(m.text, MAX_SEND_TEXT_LENGTH, format, vaList);

        if (len > 0)
        {
          m.text_length = len;
          m.level = level;
          RobotInterface::SendMessage(m);
        }

        return 0;
      }

      int SendText(const char *format, va_list vaList)
      {
        return SendText(RobotInterface::ANKI_LOG_LEVEL_PRINT, format, vaList);
      }
#endif

      bool ReceivedInit()
      {

        return initReceived_;
      }


      void ResetInit()
      {
        initReceived_ = false;
#ifndef TARGET_K02
        HAL::SetImageSendMode(Stream, QVGA);
#endif
      }

    } // namespace Messages


    namespace RobotInterface {
      int SendLog(const LogLevel level, const uint16_t name, const uint16_t formatId, const uint8_t numArgs, ...)
      {
        static u32 missedMessages = 0;
        PrintTrace m;
        if (missedMessages > 0)
        {
          m.level = ANKI_LOG_LEVEL_WARN;
          m.name  = 1;
          m.stringId = 1;
          m.value_length = 1;
          m.value[0] = missedMessages + 1;
        }
        else
        {
          m.level = level;
          m.name  = name;
          m.stringId = formatId;
          va_list argptr;
          va_start(argptr, numArgs);
          for (m.value_length=0; m.value_length < numArgs; m.value_length++)
          {
            m.value[m.value_length] = va_arg(argptr, int);
          }
          va_end(argptr);
        }
        if (SendMessage(m))
        {
          missedMessages = 0;
        }
        else
        {
          missedMessages++;
        }
        return 0;
      }
    } // namespace RobotInterface

    namespace HAL {
#ifndef TARGET_K02
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID, const bool reliable, const bool hot)
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
#endif

#ifndef SIMULATOR
      void FlashBlockIDs()
      {
        // THIS DOESN'T WORK FOR now
      }
#endif

    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki

#ifndef TARGET_K02
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
    AnkiWarn( 119, "Receiver.ReceiveData.Invalid", 367, "Receiver got %02x[%d] invalid", 2, buffer[0], bufferSize);
  }
  else if (msgBuf.Size() != bufferSize)
  {
    AnkiWarn( 120, "Receiver.ReceiveData.SizeError", 368, "Parsed message size error %d != %d", 2, bufferSize, msgBuf.Size());
  }
  else
  {
    Anki::Cozmo::Messages::ProcessMessage(msgBuf);
  }
}

void Receiver_OnConnectionRequest(ReliableConnection* connection)
{
  AnkiInfo( 121, "Receiver_OnConnectionRequest", 369, "ReliableTransport new connection", 0);
  ReliableTransport_FinishConnection(connection); // Accept the connection
  Anki::Cozmo::HAL::RadioUpdateState(1, 0);
}

void Receiver_OnConnected(ReliableConnection* connection)
{
  AnkiInfo( 122, "Receiver_OnConnected", 370, "ReliableTransport connection completed", 0);
  Anki::Cozmo::HAL::RadioUpdateState(1, 0);
}

void Receiver_OnDisconnect(ReliableConnection* connection)
{
  Anki::Cozmo::HAL::RadioUpdateState(0, 0);   // Must mark connection disconnected BEFORE trying to print
  AnkiInfo( 123, "Receiver_OnDisconnect", 371, "ReliableTransport disconnected", 0);
  ReliableConnection_Init(connection, NULL); // Reset the connection
  Anki::Cozmo::HAL::RadioUpdateState(0, 0);
}
#endif
