#include "messages.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/hal.h"
#include <math.h>

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

#include <string.h>

#include "backpackLightController.h"
#include "dockingController.h"
#include "headController.h"
#include "imuFilter.h"
#include "liftController.h"
#include "localization.h"
#include "pathFollower.h"
#include "pickAndPlaceController.h"
#include "powerModeManager.h"
#include "proxSensors.h"
#include "speedController.h"
#include "steeringController.h"
#include "testModeController.h"
#include "wheelController.h"

#include <stdio.h>

#include "anki/cozmo/robot/logging.h"

#define SEND_TEXT_REDIRECT_TO_STDOUT 0

namespace Anki {
  namespace Vector {
    namespace Messages {

      namespace {


        constexpr auto IS_MOVING = EnumToUnderlyingType(RobotStatusFlag::IS_MOVING);
        constexpr auto IS_CARRYING_BLOCK = EnumToUnderlyingType(RobotStatusFlag::IS_CARRYING_BLOCK);
        constexpr auto IS_PICKING_OR_PLACING = EnumToUnderlyingType(RobotStatusFlag::IS_PICKING_OR_PLACING);
        constexpr auto IS_BUTTON_PRESSED = EnumToUnderlyingType(RobotStatusFlag::IS_BUTTON_PRESSED);
        constexpr auto IS_PICKED_UP = EnumToUnderlyingType(RobotStatusFlag::IS_PICKED_UP);
        constexpr auto IS_FALLING = EnumToUnderlyingType(RobotStatusFlag::IS_FALLING);
        constexpr auto IS_PATHING = EnumToUnderlyingType(RobotStatusFlag::IS_PATHING);
        constexpr auto LIFT_IN_POS = EnumToUnderlyingType(RobotStatusFlag::LIFT_IN_POS);
        constexpr auto HEAD_IN_POS = EnumToUnderlyingType(RobotStatusFlag::HEAD_IN_POS);
        constexpr auto CALM_POWER_MODE = EnumToUnderlyingType(RobotStatusFlag::CALM_POWER_MODE);
        constexpr auto IS_BATTERY_DISCONNECTED = EnumToUnderlyingType(RobotStatusFlag::IS_BATTERY_DISCONNECTED);
        constexpr auto IS_ON_CHARGER = EnumToUnderlyingType(RobotStatusFlag::IS_ON_CHARGER);
        constexpr auto IS_CHARGING = EnumToUnderlyingType(RobotStatusFlag::IS_CHARGING);
        constexpr auto CLIFF_DETECTED = EnumToUnderlyingType(RobotStatusFlag::CLIFF_DETECTED);
        constexpr auto ARE_WHEELS_MOVING = EnumToUnderlyingType(RobotStatusFlag::ARE_WHEELS_MOVING);
        constexpr auto IS_BEING_HELD = EnumToUnderlyingType(RobotStatusFlag::IS_BEING_HELD);
        constexpr auto IS_MOTION_DETECTED = EnumToUnderlyingType(RobotStatusFlag::IS_MOTION_DETECTED);

        u8 pktBuffer_[2048];

        static RobotState robotState_;

        // Flag for receipt of sync message
        bool syncRobotReceived_ = false;
        bool syncRobotAckSent_ = false;

#ifdef SIMULATOR
        bool isForcedDelocalizing_ = false;
#endif
      } // private namespace

// #pragma mark --- Messages Method Implementations ---

      Result Init()
      {
        return RESULT_OK;
      }

      void ProcessMessage(RobotInterface::EngineToRobot& msg)
      {
        switch(msg.tag)
        {
          #include "clad/robotInterface/messageEngineToRobot_switch_from_0x01_to_0x4F.def"

          default:
            AnkiWarn( "Messages.ProcessBadTag_EngineToRobot.Recvd", "Received message with bad tag %x", msg.tag);
        }
      } // ProcessMessage()

      void UpdateRobotStateMsg()
      {
        robotState_.timestamp = HAL::GetTimeStamp();

        robotState_.pose_frame_id = Localization::GetPoseFrameId();
        robotState_.pose_origin_id = Localization::GetPoseOriginId();

        robotState_.pose.x = Localization::GetCurrPose_x();
        robotState_.pose.y = Localization::GetCurrPose_y();
        robotState_.pose.z = 0;
        robotState_.pose.angle = Localization::GetCurrPose_angle().ToFloat();
        robotState_.pose.pitch_angle = IMUFilter::GetPitch();
        WheelController::GetFilteredWheelSpeeds(robotState_.lwheel_speed_mmps, robotState_.rwheel_speed_mmps);
        robotState_.headAngle  = HeadController::GetAngleRad();
        robotState_.liftAngle  = LiftController::GetAngleRad();

        HAL::IMU_DataStructure imuData = IMUFilter::GetLatestRawData();
        robotState_.accel.x = imuData.acc_x;
        robotState_.accel.y = imuData.acc_y;
        robotState_.accel.z = imuData.acc_z;
        robotState_.gyro.x = IMUFilter::GetBiasCorrectedGyroData()[0];
        robotState_.gyro.y = IMUFilter::GetBiasCorrectedGyroData()[1];
        robotState_.gyro.z = IMUFilter::GetBiasCorrectedGyroData()[2];

        for (int i=0 ; i < HAL::CLIFF_COUNT ; i++) {
          robotState_.cliffDataRaw[i] = ProxSensors::GetCliffValue(i);
        }
        robotState_.proxData = ProxSensors::GetProxData();

        robotState_.backpackTouchSensorRaw = HAL::GetButtonState(HAL::BUTTON_CAPACITIVE);

        robotState_.cliffDetectedFlags = ProxSensors::GetCliffDetectedFlags();
        
        robotState_.whiteDetectedFlags = ProxSensors::GetWhiteDetectedFlags();

        robotState_.currPathSegment = PathFollower::GetCurrPathSegment();

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
        robotState_.status |= (IMUFilter::IsBeingHeld() ? IS_BEING_HELD : 0);
        robotState_.status |= (IMUFilter::IsMotionDetected() ? IS_MOTION_DETECTED : 0);
        robotState_.status |= (HAL::GetButtonState(HAL::BUTTON_POWER) > 0 ? IS_BUTTON_PRESSED : 0 );
        robotState_.status |= (PathFollower::IsTraversingPath() ? IS_PATHING : 0);
        robotState_.status |= (LiftController::IsInPosition() ? LIFT_IN_POS : 0);
        robotState_.status |= (HeadController::IsInPosition() ? HEAD_IN_POS : 0);
        robotState_.status |= HAL::PowerGetMode() == HAL::POWER_MODE_CALM ? CALM_POWER_MODE : 0;
        robotState_.status |= HAL::BatteryIsDisconnected() ? IS_BATTERY_DISCONNECTED : 0;
        robotState_.status |= HAL::BatteryIsOnCharger() ? IS_ON_CHARGER : 0;
        robotState_.status |= HAL::BatteryIsCharging() ? IS_CHARGING : 0;
        robotState_.status |= ProxSensors::IsAnyCliffDetected() ? CLIFF_DETECTED : 0;
        robotState_.status |= IMUFilter::IsFalling() ? IS_FALLING : 0;
        robotState_.batteryVoltage = HAL::BatteryGetVoltage();
        robotState_.chargerVoltage = HAL::ChargerGetVoltage();
        robotState_.battTemp_C = HAL::BatteryGetTemperature_C();
#ifdef  SIMULATOR
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

      void Process_syncRobot(const RobotInterface::SyncRobot& msg)
      {
        AnkiInfo( "Messages.Process_syncRobot.Recvd", "");

        // Set SyncRobot received flag
        // Acknowledge in Update()
        syncRobotReceived_ = true;

        // TODO: Compare message ID to robot ID as a handshake?

        // Reset pose history and frameID to zero
        Localization::ResetPoseFrame();

        AnkiInfo("watchdog_reset_count", "%d", HAL::GetWatchdogResetCounter());
      } // ProcessRobotInit()


      void Process_shutdown(const RobotInterface::Shutdown& msg)
      {
        HAL::Shutdown();
      }

      void Process_calmPowerMode(const RobotInterface::CalmPowerMode& msg)
      {
        AnkiInfo("Messages.Process_calmPowerMode.enable", "enable: %d, calib: %d", msg.enable, msg.calibOnDisable);
        PowerModeManager::EnableActiveMode(!msg.enable, msg.calibOnDisable);
      }

      void Process_absLocalizationUpdate(const RobotInterface::AbsoluteLocalizationUpdate& msg)
      {
        // Don't modify localization while running path following test.
        // The point of the test is to see how well it follows a path
        // assuming perfect localization.
        if (TestModeController::GetMode() == TestMode::TM_PATH_FOLLOW) {
          return;
        }

        // TODO: Double-check that size matches expected size?

        // TODO: take advantage of timestamp

        f32 currentMatX       = msg.xPosition;
        f32 currentMatY       = msg.yPosition;
        Radians currentMatHeading = msg.headingAngle;
        /*Result res =*/ Localization::UpdatePoseWithKeyframe(msg.origin_id, msg.pose_frame_id, msg.timestamp, currentMatX, currentMatY, currentMatHeading.ToFloat());

        /*
        AnkiInfo( "Messages.Process_absLocalizationUpdate.Recvd", "Result %d, currTime=%d, updated frame time=%d: (%.3f,%.3f) at %.1f degrees (frame = %d)\n",
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

      void Update()
      {
        // Send ACK of SyncRobot message when system is ready
        if (!syncRobotAckSent_) {
          if (syncRobotReceived_ &&
              IMUFilter::IsBiasFilterComplete() &&
              LiftController::IsCalibrated() &&
              HeadController::IsCalibrated()) {
            RobotInterface::SyncRobotAck syncRobotAckMsg;
            while (RobotInterface::SendMessage(syncRobotAckMsg) == false);
            syncRobotAckSent_ = true;

            // Send up gyro calibration
            // Since the bias is typically calibrate before the robot is even connected,
            // this is the time when the data can actually be sent up to engine.
            AnkiInfo("Messages.Update.GyroCalibrated", "%f %f %f",
                     RAD_TO_DEG_F32(IMUFilter::GetGyroBias()[0]),
                     RAD_TO_DEG_F32(IMUFilter::GetGyroBias()[1]),
                     RAD_TO_DEG_F32(IMUFilter::GetGyroBias()[2]));
          }
        }

        // Process incoming messages
        u32 dataLen;

        // Each packet is a single message
        while((dataLen = HAL::RadioGetNextPacket(pktBuffer_)) > 0)
        {
          Anki::Vector::RobotInterface::EngineToRobot msgBuf;

          // Copy into structured memory
          memcpy(msgBuf.GetBuffer(), pktBuffer_, dataLen);
          if (!msgBuf.IsValid())
          {
            AnkiWarn( "Receiver.ReceiveData.Invalid", "Receiver got %02x[%d] invalid", pktBuffer_[0], dataLen);
          }
          else if (msgBuf.Size() != dataLen)
          {
            AnkiWarn( "Receiver.ReceiveData.SizeError", "Parsed message size error %d != %d", dataLen, msgBuf.Size());
          }
          else
          {
            Anki::Vector::Messages::ProcessMessage(msgBuf);
          }
        }

      }

      void Process_clearPath(const RobotInterface::ClearPath& msg) {
        SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
        PathFollower::ClearPath();
        //SteeringController::ExecuteDirectDrive(0,0);
      }

      void Process_appendPathSegArc(const RobotInterface::AppendPathSegmentArc& msg) {
        PathFollower::AppendPathSegment_Arc(msg.x_center_mm, msg.y_center_mm,
                                            msg.radius_mm, msg.startRad, msg.sweepRad,
                                            msg.speed.target, msg.speed.accel, msg.speed.decel);
      }

      void Process_appendPathSegLine(const RobotInterface::AppendPathSegmentLine& msg) {
        PathFollower::AppendPathSegment_Line(msg.x_start_mm, msg.y_start_mm,
                                             msg.x_end_mm, msg.y_end_mm,
                                             msg.speed.target, msg.speed.accel, msg.speed.decel);
      }

      void Process_appendPathSegPointTurn(const RobotInterface::AppendPathSegmentPointTurn& msg) {
        PathFollower::AppendPathSegment_PointTurn(msg.x_center_mm, msg.y_center_mm, msg.startRad, msg.targetRad,
                                                  msg.speed.target, msg.speed.accel, msg.speed.decel,
                                                  msg.angleTolerance, msg.useShortestDir);
      }

      void Process_trimPath(const RobotInterface::TrimPath& msg) {
        PathFollower::TrimPath(msg.numPopFrontSegments, msg.numPopBackSegments);
      }

      void Process_executePath(const RobotInterface::ExecutePath& msg) {
        AnkiInfo( "Messages.Process_executePath.StartingPath", "%d", msg.pathID);
        PathFollower::StartPathTraversal(msg.pathID);
      }

      void Process_dockWithObject(const DockWithObject& msg)
      {
        AnkiInfo( "Messages.Process_dockWithObject.Recvd", "action %hhu, dockMethod %hhu, doLiftLoadCheck %d, speed %f, accel %f, decel %f",
                 msg.action, msg.dockingMethod, msg.doLiftLoadCheck, msg.speed_mmps, msg.accel_mmps2, msg.decel_mmps2);

        DockingController::SetDockingMethod(msg.dockingMethod);

        // Currently passing in default values for rel_x, rel_y, and rel_angle
        PickAndPlaceController::DockToBlock(msg.action,
                                            msg.doLiftLoadCheck,
                                            msg.speed_mmps,
                                            msg.accel_mmps2,
                                            msg.decel_mmps2,
                                            0, 0, 0,
                                            msg.numRetries);
      }

      void Process_placeObjectOnGround(const PlaceObjectOnGround& msg)
      {
        //AnkiInfo( "Messages.Process_placeObjectOnGround.Recvd", "");
        PickAndPlaceController::PlaceOnGround(msg.speed_mmps,
                                              msg.accel_mmps2,
                                              msg.decel_mmps2,
                                              msg.rel_x_mm,
                                              msg.rel_y_mm,
                                              msg.rel_angle);
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
          AnkiWarn( "Messages.Process_drive.IgnoringBecauseAlreadyOnPath", "");
          return;
        }

        //AnkiInfo( "Messages.Process_drive.Executing", "left=%f mm/s, right=%f mm/s", msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);

        //PathFollower::ClearPath();
        SteeringController::ExecuteDirectDrive(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps,
                                               msg.lwheel_accel_mmps2, msg.rwheel_accel_mmps2);
      }

      void Process_driveCurvature(const RobotInterface::DriveWheelsCurvature& msg) {
        SteeringController::ExecuteDriveCurvature(msg.speed,
                                                  msg.curvatureRadius_mm,
                                                  msg.accel);
      }

      void Process_moveLift(const RobotInterface::MoveLift& msg) {
        LiftController::SetAngularVelocity(msg.speed_rad_per_sec, MAX_LIFT_ACCEL_RAD_PER_S2);
      }

      void Process_moveHead(const RobotInterface::MoveHead& msg) {
        HeadController::SetAngularVelocity(msg.speed_rad_per_sec, MAX_HEAD_ACCEL_RAD_PER_S2);
      }

      // Send ack of head motor action
      void AckMotorCommand(u8 actionID) {
        if (actionID != 0) {
          RobotInterface::MotorActionAck ack;
          ack.actionID = actionID;
          RobotInterface::SendMessage(ack);
        }
      }

      void Process_liftHeight(const RobotInterface::SetLiftHeight& msg) {
        //AnkiInfo( "Messages.Process_liftHeight.Recvd", "height %f, maxSpeed %f, duration %f", msg.height_mm, msg.max_speed_rad_per_sec, msg.duration_sec);
        if (msg.duration_sec > 0) {
          LiftController::SetDesiredHeightByDuration(msg.height_mm, 0.1f, 0.1f, msg.duration_sec);
        } else {
          LiftController::SetDesiredHeight(msg.height_mm, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        }
        AckMotorCommand(msg.actionID);
      }

      void Process_headAngle(const RobotInterface::SetHeadAngle& msg) {
        //AnkiInfo( "Messages.Process_headAngle.Recvd", "angle %f, maxSpeed %f, duration %f", msg.angle_rad, msg.max_speed_rad_per_sec, msg.duration_sec);
        if (msg.duration_sec > 0) {
          HeadController::SetDesiredAngleByDuration(msg.angle_rad, 0.1f, 0.1f, msg.duration_sec);
        } else {
          HeadController::SetDesiredAngle(msg.angle_rad, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
        }
        AckMotorCommand(msg.actionID);
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
                                             msg.use_shortest_direction,
                                             msg.num_half_revolutions);
        AckMotorCommand(msg.actionID);
      }

      void Process_setCarryState(const CarryStateUpdate& update)
      {
        PickAndPlaceController::SetCarryState(update.state);
      }

      void Process_imuRequest(const IMURequest& msg)
      {
        IMUFilter::RecordAndSend(msg.length_ms);
      }

      void Process_turnInPlaceAtSpeed(const RobotInterface::TurnInPlaceAtSpeed& msg) {
        //AnkiInfo( "Messages.Process_turnInPlaceAtSpeed.Recvd", "speed %f rad/s, accel %f rad/s2", msg.speed_rad_per_sec, msg.accel_rad_per_sec2);
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

      void Process_cameraFOVInfo(const CameraFOVInfo& msg)
      {
        DockingController::SetCameraFieldOfView(msg.horizontalFOV, msg.verticalFOV);
      }

      void Process_rollActionParams(const RobotInterface::RollActionParams& msg) {
        PickAndPlaceController::SetRollActionParams(msg.liftHeight_mm,
                                                    msg.driveSpeed_mmps,
                                                    msg.driveAccel_mmps2,
                                                    msg.driveDuration_ms,
                                                    msg.backupDist_mm);
      }

      void Process_playpenStart(const RobotInterface::PlaypenStart& msg) {
      }

      void Process_setControllerGains(const RobotInterface::ControllerGains& msg) {
        switch (msg.controller)
        {
          case ControllerChannel::controller_wheel:
          {
            WheelController::SetGains(msg.kp, msg.ki, msg.maxIntegralError);
            break;
          }
          case ControllerChannel::controller_head:
          {
            HeadController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
            break;
          }
          case ControllerChannel::controller_lift:
          {
            LiftController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
            break;
          }
          case ControllerChannel::controller_steering:
          {
            SteeringController::SetGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError); // Coopting structure
            break;
          }
          case ControllerChannel::controller_pointTurn:
          {
            SteeringController::SetPointTurnGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
            break;
          }
          default:
          {
            AnkiWarn( "Messages.Process_setControllerGains.InvalidController", "controller: %hhu",  msg.controller);
          }
        }
      }

      void Process_setMotionModelParams(const RobotInterface::SetMotionModelParams& msg)
      {
        Localization::SetMotionModelParams(msg.slipFactor);
      }

      void Process_abortDocking(const AbortDocking& msg)
      {
        DockingController::StopDocking();
      }

      void Process_checkLiftLoad(const RobotInterface::CheckLiftLoad& msg)
      {
        LiftController::CheckForLoad();
      }

      void Process_enableMotorPower(const RobotInterface::EnableMotorPower& msg)
      {
        switch(msg.motorID) {
          case MotorID::MOTOR_HEAD:
          {
            if (msg.enable) {
              HeadController::Enable();
            } else {
              HeadController::Disable();
            }
            break;
          }
          case MotorID::MOTOR_LIFT:
          {
            if (msg.enable) {
              LiftController::Enable();
            } else {
              LiftController::Disable();
            }
            break;
          }
          default:
          {
            AnkiWarn( "Messages.enableMotorPower.UnhandledMotorID", "%hhu", msg.motorID);
            break;
          }
        }
      }

      void Process_robotStoppedAck(const RobotInterface::RobotStoppedAck& msg)
      {
        AnkiInfo("Messages.Process_robotStoppedAck", "");
        SteeringController::Enable();
      }

      void Process_enableStopOnCliff(const RobotInterface::EnableStopOnCliff& msg)
      {
        ProxSensors::EnableStopOnCliff(msg.enable);
      }

      void Process_enableStopOnWhite(const RobotInterface::EnableStopOnWhite& msg)
      {
        ProxSensors::EnableStopOnWhite(msg.enable);
      }
      
      void Process_setCliffDetectThresholds(const SetCliffDetectThresholds& msg)
      {
        for (int i = 0 ; i < HAL::CLIFF_COUNT ; i++) {
          ProxSensors::SetCliffDetectThreshold(i, msg.thresholds[i]);
        }
      }

      void Process_cliffAlignToWhiteAction(const RobotInterface::CliffAlignToWhiteAction& msg)
      {
        if (msg.start) {
          PickAndPlaceController::CliffAlignToWhite();
        } else {
          PickAndPlaceController::StopCliffAlignToWhite();
        }
      }

      void Process_enableBraceWhenFalling(const RobotInterface::EnableBraceWhenFalling& msg)
      {
        IMUFilter::EnableBraceWhenFalling(msg.enable);
      }

      void Process_recordHeading(RobotInterface::RecordHeading const& msg)
      {
        SteeringController::RecordHeading();
      }

      void Process_turnToRecordedHeading(RobotInterface::TurnToRecordedHeading const& msg)
      {
        SteeringController::ExecutePointTurnToRecordedHeading(DEG_TO_RAD_F32(msg.offset_deg),
                                                              DEG_TO_RAD_F32(msg.speed_degPerSec),
                                                              DEG_TO_RAD_F32(msg.accel_degPerSec2),
                                                              DEG_TO_RAD_F32(msg.decel_degPerSec2),
                                                              DEG_TO_RAD_F32(msg.tolerance_deg),
                                                              msg.numHalfRevs,
                                                              msg.useShortestDir);
      }

      void Process_setBackpackLights(RobotInterface::SetBackpackLights const& msg)
      {
        BackpackLightController::SetParams(msg);
      }

      void Process_setSystemLight(RobotInterface::SetSystemLight const& msg)
      {
        BackpackLightController::SetParams(msg);
      }

      void Process_getMfgInfo(const RobotInterface::GetManufacturingInfo& msg)
      {
        RobotInterface::SendMessage(RobotInterface::ManufacturingID());
      }

      void Process_setBackpackLayer(const RobotInterface::BackpackSetLayer& msg) {
        BackpackLightController::EnableLayer((BackpackLightLayer)msg.layer);
      }
      
// ----------- Send messages -----------------

      Result SendRobotStateMsg()
      {
        // Don't send robot state updates unless the init message was received
        if (!syncRobotReceived_) {
          return RESULT_FAIL;
        }


        if(RobotInterface::SendMessage(robotState_) == true) {
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

      Result SendMicDataFunction(const s16* latestMicData, uint32_t numSamples)
      {
        RobotInterface::MicData micData{};
        micData.timestamp = HAL::GetTimeStamp();
        micData.robotStatusFlags = robotState_.status;
        micData.robotRotationAngle = robotState_.pose.angle;
        std::copy(latestMicData, latestMicData + numSamples, micData.data);
        return RobotInterface::SendMessage(micData) ? RESULT_OK : RESULT_FAIL;
      }

      Result SendMicDataMsgs()
      {
        while (HAL::HandleLatestMicData(&SendMicDataFunction))
        {
          // Keep calling HandleLatestMicData until it returns false
        }
        return RESULT_OK;
      }

      bool ReceivedInit()
      {
        return syncRobotReceived_;
      }

      void ResetInit()
      {
        syncRobotReceived_ = false;
        syncRobotAckSent_ = false;
      }

    } // namespace Messages

    namespace HAL {
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID)
      {
        //Stuff msgID up front
        size_t newSize = size + 1;
        u8 buf[newSize];

        memcpy(buf, &msgID, 1);
        memcpy(buf + 1, buffer, size);

        //fprintf(stderr, "RadioSendMsg: %02x [%d]", msgID, newSize);

        return HAL::RadioSendPacket(buf, newSize);
      }


    } // namespace HAL
  } // namespace Vector
} // namespace Anki
