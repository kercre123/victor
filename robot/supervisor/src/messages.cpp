#include "messages.h"
#include "anki/cozmo/robot/hal.h"
#include <math.h>

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

#ifdef SIMULATOR
#include "anki/cozmo/transport/IUnreliableTransport.h"
#include "anki/cozmo/transport/IReceiver.h"
#include "anki/cozmo/transport/reliableTransport.h"
#include "anki/vision/CameraSettings.h"
#include "../sim_hal/sim_nvStorage.h"
#endif
#include <string.h>

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
#include "proxSensors.h"
#ifdef TARGET_K02
#include "hal/uart.h"
#include "hal/imu.h"
#include "hal/wifi.h"
#include "hal/spine.h"
#else
#include "animationController.h"
#endif
#include "anki/cozmo/robot/logging.h"

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

        bool sendTestStateMessages = false;

        // Flag for receipt of Init message
        bool initReceived_ = false;
        // Cache for power state information
        float vExt_;
        bool onCharger_;
        bool isCharging_;
        BodyRadioMode bodyRadioMode_ = BODY_IDLE_OPERATING_MODE;
        
#ifdef SIMULATOR
        bool isForcedDelocalizing_ = false;
#endif
        uint32_t cubeID_;
        u8 rotationPeriod_;
        bool cubIDSet_ = false;
      } // private namespace

// #pragma mark --- Messages Method Implementations ---

      Result Init()
      {
#ifndef TARGET_K02
        ReliableTransport_Init();
        ReliableConnection_Init(&connection, NULL); // We only have one connection so dest pointer is superfluous
        
        
        // Store camera calibration in nvStorage
        const HAL::CameraInfo* headCamInfo = HAL::GetHeadCamInfo();
        if(headCamInfo == NULL) {
          AnkiWarn( 163, "Messages.Init.CalibNotFound", 359, "NULL HeadCamInfo retrieved from HAL.", 0);
        }
        else {

          CameraCalibration headCalib{
            headCamInfo->focalLength_x,
            headCamInfo->focalLength_y,
            headCamInfo->center_x,
            headCamInfo->center_y,
            headCamInfo->skew,
            headCamInfo->nrows,
            headCamInfo->ncols
          };
          
          for(s32 iCoeff=0; iCoeff<NUM_RADIAL_DISTORTION_COEFFS; ++iCoeff) {
            headCalib.distCoeffs[iCoeff] = headCamInfo->distortionCoeffs[iCoeff];
          }
          
          NVStorage::NVStorageWrite nvWrite;
          nvWrite.entry.tag = NVStorage::NVEntry_CameraCalib;
          nvWrite.entry.blob_length = headCalib.Size();
          memcpy(nvWrite.entry.blob, headCalib.GetBuffer(), headCalib.Size());
          nvWrite.reportDone = false;
          nvWrite.reportEach = false;
          nvWrite.rangeEnd = NVStorage::NVEntry_Invalid;
          nvWrite.writeNotErase = true;
          nvWrite.reportTo = NVStorage::RTIP;
          SimNVStorageSpace::Write(nvWrite);
          
        }
        
        // In sim we don't expect to get the PowerState message which normally sets this
        bodyRadioMode_ = BODY_ACCESSORY_OPERATING_MODE;

#endif
        return RESULT_OK;
      }

      void ProcessMessage(RobotInterface::EngineToRobot& msg)
      {
        switch(msg.tag)
        {
#ifdef TARGET_K02
          #include "clad/robotInterface/messageEngineToRobot_switch_from_0x30_to_0x7f.def"
          // Need to add additional messages for special cases handled both on the Espressif and K02
          case RobotInterface::EngineToRobot::Tag_setBackpackLightsMiddle:  // This one is actually handled only on K02 and body
            Process_setBackpackLightsMiddle(msg.setBackpackLightsMiddle);
            break;
          case RobotInterface::EngineToRobot::Tag_setBackpackLightsTurnSignals:
            Process_setBackpackLightsTurnSignals(msg.setBackpackLightsTurnSignals);
            break;
          case RobotInterface::EngineToRobot::Tag_animHeadAngle:
            Process_animHeadAngle(msg.animHeadAngle);
            break;
          case RobotInterface::EngineToRobot::Tag_animBodyMotion:
            Process_animBodyMotion(msg.animBodyMotion);
            break;
          case RobotInterface::EngineToRobot::Tag_animLiftHeight:
            Process_animLiftHeight(msg.animLiftHeight);
            break;
          case RobotInterface::EngineToRobot::Tag_animEventToRTIP:
            Process_animEventToRTIP(msg.animEventToRTIP);
            break;
          case RobotInterface::EngineToRobot::Tag_animBackpackLights:
            Process_animBackpackLights(msg.animBackpackLights);
            break;
          case RobotInterface::EngineToRobot::Tag_animEndOfAnimation:
            Process_animEndOfAnimation(msg.animEndOfAnimation);
            break;
#else
            #include "clad/robotInterface/messageEngineToRobot_switch_group_anim.def"
#endif
            
          default:
            AnkiWarn( 106, "Messages.ProcessBadTag_EngineToRobot.Recvd", 355, "Received message with bad tag %x", 1, msg.tag);
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
        robotState_.pose_origin_id = Localization::GetPoseOriginId();

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

        robotState_.cliffDataRaw = HAL::GetRawCliffData();
        
        robotState_.currPathSegment = PathFollower::GetCurrPathSegment();
        robotState_.numFreeSegmentSlots = PathFollower::GetNumFreeSegmentSlots();

        robotState_.status = 0;
        // TODO: Make this a parameters somewhere?
        robotState_.status |= (WheelController::AreWheelsMoving() ||
                              SteeringController::GetMode() == SteeringController::SM_POINT_TURN ? ARE_WHEELS_MOVING : 0);
        robotState_.status |= (HeadController::IsMoving() ||
                               LiftController::IsMoving() ||
                               (robotState_.status & ARE_WHEELS_MOVING) ? IS_MOVING : 0);
        robotState_.status |= (PickAndPlaceController::IsCarryingBlock() ? IS_CARRYING_BLOCK : 0);
        robotState_.status |= (PickAndPlaceController::IsBusy() ? IS_PICKING_OR_PLACING : 0);
        robotState_.status |= (IMUFilter::IsPickedUp() ? IS_PICKED_UP : 0);
        robotState_.status |= (PathFollower::IsTraversingPath() ? IS_PATHING : 0);
        robotState_.status |= (LiftController::IsInPosition() ? LIFT_IN_POS : 0);
        robotState_.status |= (HeadController::IsInPosition() ? HEAD_IN_POS : 0);
        robotState_.status |= HAL::BatteryIsOnCharger() ? IS_ON_CHARGER : 0;
        robotState_.status |= HAL::BatteryIsCharging() ? IS_CHARGING : 0;
        robotState_.status |= HAL::IsCliffDetected() ? CLIFF_DETECTED : 0;
        robotState_.status |= bodyRadioMode_ == BODY_ACCESSORY_OPERATING_MODE ? IS_BODY_ACC_MODE : 0;
#ifdef  SIMULATOR
        robotState_.batteryVoltage = HAL::BatteryGetVoltage();
        if(isForcedDelocalizing_)
        {
          robotState_.status |= IS_PICKED_UP;
        }
#endif
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

        // Set drive center offset
        Localization::SetDriveCenterOffset(msg.driveCenterOffset);
        
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
        /*Result res =*/ Localization::UpdatePoseWithKeyframe(msg.origin_id, msg.pose_frame_id, msg.timestamp, currentMatX, currentMatY, currentMatHeading.ToFloat());
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

      void Process_forceDelocalizeSimulatedRobot(const RobotInterface::ForceDelocalizeSimulatedRobot& msg)
      {
#ifdef SIMULATOR
        isForcedDelocalizing_ = true;
#endif
      }

      void Process_dockingErrorSignal(const DockingErrorSignal& msg)
      {
        DockingController::SetDockingErrorSignalMessage(msg);
      }

      void ProcessBTLEMessages()
      {
#ifndef TARGET_K02
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
#endif
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
                                                  msg.speed.target, msg.speed.accel, msg.speed.decel,
                                                  msg.angleTolerance, msg.useShortestDir);
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
        AnkiInfo( 104, "Messages.Process_dockWithObject.Recvd", 353, "action %d, speed %f, acccel %f, decel %f, manualSpeed %d", 5,
              msg.action, msg.speed_mmps, msg.accel_mmps2, msg.decel_mmps2, msg.useManualSpeed);

        DockingController::SetDockingMethod(msg.dockingMethod);

        // Currently passing in default values for rel_x, rel_y, and rel_angle
        PickAndPlaceController::DockToBlock(msg.action,
                                            msg.speed_mmps,
                                            msg.accel_mmps2,
                                            msg.decel_mmps2,
                                            0, 0, 0,
                                            msg.useManualSpeed,
                                            msg.numRetries);
      }

      void Process_placeObjectOnGround(const PlaceObjectOnGround& msg)
      {
        //AnkiInfo( 108, "Messages.Process_placeObjectOnGround.Recvd", 305, "", 0);
        PickAndPlaceController::PlaceOnGround(msg.speed_mmps,
                                              msg.accel_mmps2,
                                              msg.decel_mmps2,
                                              msg.rel_x_mm,
                                              msg.rel_y_mm,
                                              msg.rel_angle,
                                              msg.useManualSpeed);
      }

      void Process_startMotorCalibration(const RobotInterface::StartMotorCalibration& msg) {
        if (msg.calibrateHead) {
          HeadController::StartCalibrationRoutine();
        }
        
        if (msg.calibrateLift) {
          LiftController::StartCalibrationRoutine();
        }
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
            AnkiInfo( 338, "Messages.Process_drive.IgnoringBecauseAlreadyOnPath", 305, "", 0);
          }
          return;
        }

        //AnkiInfo( 116, "Messages.Process_drive.Executing", 364, "left=%f mm/s, right=%f mm/s", 2, msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);

        //PathFollower::ClearPath();
        SteeringController::ExecuteDirectDrive(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps,
                                               msg.lwheel_accel_mmps2, msg.rwheel_accel_mmps2);
      }

      void Process_driveCurvature(const RobotInterface::DriveWheelsCurvature& msg) {
        SteeringController::ExecuteDriveCurvature(msg.speed_mmPerSec,
                                                  msg.curvatureRadius_mm);
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
                                             msg.accel_rad_per_sec2,
                                             msg.angle_tolerance,
                                             true);
      }

      void Process_setCarryState(const CarryStateUpdate& update)
      {
        PickAndPlaceController::SetCarryState(update.state);
      }

      void Process_imuRequest(const Anki::Cozmo::RobotInterface::ImuRequest& msg)
      {
        IMUFilter::RecordAndSend(msg.length_ms);
      }
      
      void Process_setIMUCalibration(const Anki::Cozmo::RobotInterface::IMUCalibrationData& msg)
      {
        HAL::IMUSetCalibrationOffsets(msg.acc, msg.gyro);
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
/*
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

          CameraCalibration headCalibMsg = {
            headCamInfo->focalLength_x,
            headCamInfo->focalLength_y,
            headCamInfo->center_x,
            headCamInfo->center_y,
            headCamInfo->skew,
            headCamInfo->nrows,
            headCamInfo->ncols
          };

          if(!RobotInterface::SendMessage(headCalibMsg)) {
            AnkiWarn( 113, "Messages.Process_imageRequest.SendCalibFailed", 361, "Failed to send camera calibration message.", 0);
          }
        }
 */
#endif
      } // ProcessImageRequestMessage()

      void Process_rollActionParams(const RobotInterface::RollActionParams& msg) {
        PickAndPlaceController::SetRollActionParams(msg.liftHeight_mm,
                                                    msg.driveSpeed_mmps,
                                                    msg.driveAccel_mmps2,
                                                    msg.driveDuration_ms,
                                                    msg.backupDist_mm);
      }
      
      
      void Process_setControllerGains(const RobotInterface::ControllerGains& msg) {
        switch (msg.controller)
        {
          case controller_wheel:
          {
            WheelController::SetGains(msg.kp, msg.ki, msg.maxIntegralError);
            break;
          }
          case controller_head:
          {
            HeadController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
            break;
          }
          case controller_lift:
          {
            LiftController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
            break;
          }
          case controller_steering:
          {
            SteeringController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError); // Coopting structure
            break;
          }
          case controller_pointTurn:
          {
            SteeringController::SetPointTurnGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
            break;
          }
          default:
          {
            AnkiWarn( 114, "Messages.Process_setControllerGains.InvalidController", 362, "controller: %d", 1, msg.controller);
          }
        }
      }

      void Process_enterSleepMode(const EnterSleepMode& msg) {
        #ifdef TARGET_K02
        Anki::Cozmo::HAL::Power::enterSleepMode();
        #endif
      }
      
      void Process_setMotionModelParams(const RobotInterface::SetMotionModelParams& msg)
      {
        Localization::SetMotionModelParams(msg.slipFactor);
      }
      
      void Process_abortDocking(const AbortDocking& msg)
      {
        DockingController::StopDocking();
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
        if(AnimationController::BufferKeyFrame(msg.GetBuffer(), msg.Size()) != RESULT_OK) {
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

      void Process_enableReadToolCodeMode(const RobotInterface::EnableReadToolCodeMode& msg)
      {
        AnkiDebug( 162, "ReadToolCodeMode", 449, "enabled: %d, liftPower: %f, headPower: %f", 3, msg.enable, msg.liftPower, msg.headPower);
        if (msg.enable) {
          HeadController::Disable();
          f32 p = CLIP(msg.headPower, -0.5f, 0.5f);
          HAL::MotorSetPower(MOTOR_HEAD, p);
          
          LiftController::Disable();
          p = CLIP(msg.liftPower, -0.5f, 0.5f);
          HAL::MotorSetPower(MOTOR_LIFT, p);
          
        } else {
          
          HAL::MotorSetPower(MOTOR_HEAD, 0);
          HeadController::Enable();
          
          HAL::MotorSetPower(MOTOR_LIFT, 0);
          LiftController::Enable();
        }
      }
      
      void Process_enableStopOnCliff(const RobotInterface::EnableStopOnCliff& msg)
      {
        ProxSensors::EnableStopOnCliff(msg.enable);
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
      void Process_animFaceImage(const Anki::Cozmo::AnimKeyFrame::FaceImage& msg)
      {
        // Handled by the Espressif
      }
      void Process_animHeadAngle(const Anki::Cozmo::AnimKeyFrame::HeadAngle& msg)
      {
        HeadController::SetDesiredAngle(DEG_TO_RAD_F32(static_cast<f32>(msg.angle_deg)), 0.1f, 0.1f,
                                                static_cast<f32>(msg.time_ms)*.001f);
      }
      void Process_animBodyMotion(const Anki::Cozmo::AnimKeyFrame::BodyMotion& msg)
      {
        SteeringController::ExecuteDriveCurvature(msg.speed, msg.curvatureRadius_mm);
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
      void Process_animEvent(const Anki::Cozmo::AnimKeyFrame::Event& msg)
      {
        // Handled on the Espressif
      }
      void Process_diffieHellmanResults(Anki::Cozmo::DiffieHellmanResults const&)
      {
        // Handled on the NRF
      }
      void Process_calculateDiffieHellman(Anki::Cozmo::CalculateDiffieHellman const&)
      {
        // Handled on the Espressif
      }
      void Process_readBodyStorage(Anki::Cozmo::ReadBodyStorage const&) {
        // Handled on the NRF
      }

      void Process_writeBodyStorage(Anki::Cozmo::WriteBodyStorage const&) {
        // Handled on the NRF
      }
      
      void Process_bodyStorageContents(Anki::Cozmo::BodyStorageContents const&) {
        // Handled on the Espressif
      }
      
      void Process_animEventToRTIP(const RobotInterface::AnimEventToRTIP& msg)
      {
        RobotInterface::AnimationEvent emsg;
        emsg.timestamp = HAL::GetTimeStamp();
        emsg.event_id = msg.event_id;
        emsg.tag = msg.tag;
        SendMessage(emsg);
      }

      void Process_animBackpackLights(const Anki::Cozmo::AnimKeyFrame::BackpackLights& msg)
      {
        for(s32 iLED=0; iLED<NUM_BACKPACK_LEDS; ++iLED) {
          u16 color = msg.colors[iLED];
          BackpackLightController::SetParams(BackpackLightController::BPL_ANIM, iLED, color, color, 0xff, 0, 0, 0);
        }
        BackpackLightController::EnableLayer(BackpackLightController::BPL_ANIM, true);
      }

      void Process_animEndOfAnimation(const AnimKeyFrame::EndOfAnimation& msg)
      {
        // Set backpack lights back to user layer
        BackpackLightController::EnableLayer(BackpackLightController::BPL_USER);
      }
      
      void Process_powerState(const PowerState& msg)
      {
        robotState_.batteryVoltage = static_cast<float>(msg.VBatFixed)/65536.0f;
        vExt_ = static_cast<float>(msg.VExtFixed)/65536.0f;
        onCharger_  = msg.onCharger;
        isCharging_ = msg.isCharging;

        if (bodyRadioMode_ != msg.operatingMode) {
          bodyRadioMode_ = msg.operatingMode;
          if (bodyRadioMode_ == BODY_ACCESSORY_OPERATING_MODE ) {
            LiftController::Enable();
            LiftController::StartCalibrationRoutine();
            HeadController::Enable();
            HeadController::StartCalibrationRoutine();
            WheelController::Enable();
          } else {
            LiftController::Disable();
            LiftController::ClearCalibration();
            HeadController::Disable();
            HeadController::ClearCalibration();
            WheelController::Disable();
          }
        }
      }
      void Process_getPropState(const PropState& msg)
      {
#ifdef TARGET_K02
        // Remapped for EP3
        HAL::GetPropState(msg.slot, -msg.x, msg.z, msg.y, msg.shockCount,
                          msg.tapTime, msg.tapNeg, msg.tapPos);
#endif
      }
      
      void Process_objectConnectionStateToRobot(const ObjectConnectionStateToRobot& msg)
      {
        if (msg.connected) {
          // Clear motion data for the object on connect so that
          // ObjectUpAxisChanged will be sent up soon after.
          HAL::ClearActiveObjectData(msg.objectID);
        }
        
        // Forward on the 'to-engine' version of this message
        ObjectConnectionState newMsg;
        newMsg.objectID = msg.objectID;
        newMsg.device_type = msg.device_type;
        newMsg.factoryID = msg.factoryID;
        newMsg.connected = msg.connected;
        RobotInterface::SendMessage(newMsg);
      }
      
      void Process_enableTestStateMessage(const RobotInterface::EnableTestStateMessage& msg)
      {
        sendTestStateMessages = msg.enable;
      }
      
      void Process_enterRecoveryMode(const RobotInterface::OTA::EnterRecoveryMode& msg)
      {
        // Handled directly in spi to bypass main execution.
      }
      
      void Process_wifiState(const WiFiState& state)
      {
#ifndef SIMULATOR
        if (state.rtCount > 0) HAL::SetImageSendMode(Stream, QVGA);
        else if (state.rtCount == 0) HAL::SetImageSendMode(Off, QVGA);
#endif
        HAL::RadioUpdateState(state.rtCount, false);
      }
      
      void Process_setBackpackLightsTurnSignals(const RobotInterface::BackpackLightsTurnSignals& msg)
      {
        u8 led = LED_BACKPACK_LEFT;
        BackpackLightController::SetParams(BackpackLightController::BPL_USER, (LEDId)led,
                                             msg.lights[0].onColor, msg.lights[0].offColor,
                                             msg.lights[0].onFrames, msg.lights[0].offFrames,
                                             msg.lights[0].transitionOnFrames, msg.lights[0].transitionOffFrames);
        
        led = LED_BACKPACK_RIGHT;
        BackpackLightController::SetParams(BackpackLightController::BPL_USER, (LEDId)led,
                                           msg.lights[1].onColor, msg.lights[1].offColor,
                                           msg.lights[1].onFrames, msg.lights[1].offFrames,
                                           msg.lights[1].transitionOnFrames, msg.lights[1].transitionOffFrames);
        
        BackpackLightController::EnableLayer(BackpackLightController::BPL_USER, true);
      }
      
      void Process_setBackpackLightsMiddle(const RobotInterface::BackpackLightsMiddle& msg)
      {
        u8 led = LED_BACKPACK_FRONT;
        BackpackLightController::SetParams(BackpackLightController::BPL_USER, (LEDId)led,
                                           msg.lights[0].onColor, msg.lights[0].offColor,
                                           msg.lights[0].onFrames, msg.lights[0].offFrames,
                                           msg.lights[0].transitionOnFrames, msg.lights[0].transitionOffFrames);
        
        led = LED_BACKPACK_MIDDLE;
        BackpackLightController::SetParams(BackpackLightController::BPL_USER, (LEDId)led,
                                           msg.lights[1].onColor, msg.lights[1].offColor,
                                           msg.lights[1].onFrames, msg.lights[1].offFrames,
                                           msg.lights[1].transitionOnFrames, msg.lights[1].transitionOffFrames);
        led = LED_BACKPACK_BACK;
        BackpackLightController::SetParams(BackpackLightController::BPL_USER, (LEDId)led,
                                           msg.lights[2].onColor, msg.lights[2].offColor,
                                           msg.lights[2].onFrames, msg.lights[2].offFrames,
                                           msg.lights[2].transitionOnFrames, msg.lights[2].transitionOffFrames);

        BackpackLightController::EnableLayer(BackpackLightController::BPL_USER, true);
      }


#ifdef SIMULATOR
      void Process_otaWrite(const Anki::Cozmo::RobotInterface::OTA::Write& msg)
      {
        
      }
      /// Stub message handlers to satisfy simulator build
      void Process_writeNV(Anki::Cozmo::NVStorage::NVStorageWrite const& msg)
      {
        SimNVStorageSpace::Write(msg);
      }
      void Process_readNV(Anki::Cozmo::NVStorage::NVStorageRead const& msg)
      {
        SimNVStorageSpace::Read(msg);
      }
      void Process_wipeAllNV(Anki::Cozmo::NVStorage::NVWipeAll const& msg)
      {
        SimNVStorageSpace::WipeAll(msg);
      }
      void Process_setHeadlight(RobotInterface::SetHeadlight const&)
      {
        // Nothing to do here
      }
      void Process_bodyRestart(RobotInterface::OTA::BodyRestart const&)
      {
        // Nothing to do here
      }
      void Process_encodedAESKey(Anki::Cozmo::EncodedAESKey const&)
      {
        // Nothing to do here
      }
      void Process_enterPairing(Anki::Cozmo::EnterPairing const&)
      {
        // Nothing to do here
      }
      void Process_oledDisplayNumber(Anki::Cozmo::RobotInterface::DisplayNumber const&)
      {
        // Nothing to do here
      }
      void Process_helloPhoneMessage(Anki::Cozmo::HelloPhone const&)
      {
        // Nothing to do here
      }
      void Process_nvReadToBody(Anki::Cozmo::RobotInterface::NVReadResultToBody const&)
      {
        // Nothing to do here
      }
      void Process_nvOpResultToBody(Anki::Cozmo::RobotInterface::NVOpResultToBody const&)
      {
        // Nothing to do here
      }
      void Process_killBodyCode(Anki::Cozmo::KillBodyCode const&)
      {
        // Nothing to do here
      }
      void Process_setRTTO(Anki::Cozmo::RobotInterface::DebugSetRTTO const&)
      {
        // TODO honor this in simulator
      }
      void Process_setPropSlot(const SetPropSlot& msg)
      {
        HAL::AssignSlot(msg.slot, msg.factory_id);
      }
      void Process_setCubeGamma(const SetCubeGamma& msg)
      {
        // Nothing to do here
      }
      void Process_assignCubeSlots(const CubeSlots& msg)
      {
        HAL::AssignCubeSlots(7, msg.factory_id);
      }
      void Process_setCubeID(const CubeID& msg)
      {
        cubeID_ = msg.objectID;
        rotationPeriod_ = msg.rotationPeriod_frames;
        cubIDSet_ = true;
      }
      void Process_setCubeLights(const CubeLights& msg)
      {
        if(!cubIDSet_)
        {
          return;
        }
        BlockLightController::SetLights(cubeID_, msg.lights, rotationPeriod_);
        cubIDSet_ = false;
      }
      void Process_enterTestMode(const RobotInterface::EnterFactoryTestMode&)
      {
        // nothing to do here//
      }
      /*
      void Process_configureBluetooth(const RobotInterface::ConfigureBluetooth&)
      {
        // nothing to do here
      }
       */
      void Process_setBodyRadioMode(const SetBodyRadioMode&)
      {
        // nothing to do here
      }
      void Process_testState(const RobotInterface::TestState&)
      {
        // Nothing to do here
      }
      
      // These are stubbed out just to get things compiling
      void Process_robotIpInfo(const Anki::Cozmo::RobotInterface::AppConnectRobotIP& msg) {}
      void Process_wifiCfgResult(const Anki::Cozmo::RobotInterface::AppConnectConfigResult& msg) {}
      void Process_appConCfgString(const Anki::Cozmo::RobotInterface::AppConnectConfigString& msg) {}
      void Process_appConCfgFlags(const Anki::Cozmo::RobotInterface::AppConnectConfigFlags& msg) {}
      void Process_appConCfgIPInfo(const Anki::Cozmo::RobotInterface::AppConnectConfigIPInfo& msg) {}
      void Process_appConGetRobotIP(const Anki::Cozmo::RobotInterface::AppConnectGetRobotIP& msg) {}
      void Process_wifiOff(const Anki::Cozmo::RobotInterface::WiFiOff& msg) {}
#endif

// ----------- Send messages -----------------


      Result SendRobotStateMsg(const RobotState* msg)
      {
#ifdef TARGET_K02
        if (sendTestStateMessages)
        {
          RobotInterface::TestState tsm;
          memcpy((void*)tsm.speedsFixed,    (void*)g_dataToHead.speeds,    sizeof(tsm.speedsFixed));
          memcpy((void*)tsm.positionsFixed, (void*)g_dataToHead.positions, sizeof(tsm.positionsFixed));
          memcpy((void*)tsm.gyro, (void*)HAL::IMU::IMUState.gyro, sizeof(tsm.gyro));
          memcpy((void*)tsm.acc,  (void*)HAL::IMU::IMUState.acc,  sizeof(tsm.acc));
          tsm.cliffLevel = g_dataToHead.cliffLevel,
          tsm.battVolt10x = static_cast<uint8_t>(robotState_.batteryVoltage * 10.0f);
          tsm.extVolt10x  = static_cast<uint8_t>(vExt_ * 10.0f);
          tsm.chargeStat  = (onCharger_ << 0) | (isCharging_ << 1);
          RobotInterface::SendMessage(tsm);
        }
#endif
        
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
          
          #ifdef SIMULATOR
          {
            isForcedDelocalizing_ = false;
          }
          #endif
          
          return RESULT_OK;
        } else {
          return RESULT_FAIL;
        }
      }

      
      Result SendMotorCalibrationMsg(MotorID motor, bool calibStarted, bool autoStarted)
      {
        MotorCalibration m;
        m.motorID = motor;
        m.calibStarted = calibStarted;
        m.autoStarted = autoStarted;
        return RobotInterface::SendMessage(m) ? RESULT_OK : RESULT_FAIL;
      }
      
      Result SendMotorAutoEnabledMsg(MotorID motor, bool enabled)
      {
        MotorAutoEnabled m;
        m.motorID = motor;
        m.enabled = enabled;
        return RobotInterface::SendMessage(m) ? RESULT_OK : RESULT_FAIL;
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
#ifdef TARGET_K02
      f32 BatteryGetVoltage()
      {
        return Messages::robotState_.batteryVoltage;
      }
      bool BatteryIsCharging()
      {
        return Messages::isCharging_;
      }
      bool BatteryIsOnCharger()
      {
        return Messages::onCharger_;
      }
#else
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID)
      {
        const bool reliable = msgID < RobotInterface::TO_ENG_UNREL;
        const bool hot = false;
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
  ReliableTransport_FinishConnection(connection); // Accept the connection
  AnkiInfo( 121, "Receiver_OnConnectionRequest", 369, "ReliableTransport new connection", 0);
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
