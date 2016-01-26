/**
 * File: basicActions.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements basic cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "basicActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

namespace Anki {
  
  namespace Cozmo {
    
    TurnInPlaceAction::TurnInPlaceAction(const Radians& angle, const bool isAbsolute)
    :  _targetAngle(angle)
    , _isAbsoluteAngle(isAbsolute)
    {
      
    }
    
    const std::string& TurnInPlaceAction::GetName() const
    {
      static const std::string name("TurnInPlaceAction");
      return name;
    }
    
    void TurnInPlaceAction::SetMaxSpeed(f32 maxSpeed_radPerSec)
    {
      if (std::fabsf(maxSpeed_radPerSec) > MAX_BODY_ROTATION_SPEED_RAD_PER_SEC) {
        PRINT_NAMED_WARNING("TurnInPlaceAction.SetMaxSpeed.SpeedExceedsLimit",
                            "Speed of %f deg/s exceeds limit of %f deg/s. Clamping.",
                            RAD_TO_DEG_F32(maxSpeed_radPerSec), MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
        _maxSpeed_radPerSec = std::copysign(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, maxSpeed_radPerSec);
      } else {
        _maxSpeed_radPerSec = maxSpeed_radPerSec;
      }
    }
    
    void TurnInPlaceAction::SetTolerance(const Radians& angleTol_rad)
    {
      _angleTolerance = angleTol_rad.getAbsoluteVal();
      
      // NOTE: can't be lower than what is used internally on the robot
      if( _angleTolerance.ToFloat() < POINT_TURN_ANGLE_TOL ) {
        PRINT_NAMED_WARNING("TurnInPlaceAction.InvalidTolerance",
                            "Tried to set tolerance of %fdeg, min is %f",
                            RAD_TO_DEG(_angleTolerance.ToFloat()),
                            RAD_TO_DEG(POINT_TURN_ANGLE_TOL));
        _angleTolerance = POINT_TURN_ANGLE_TOL;
      }
    }
    
    ActionResult TurnInPlaceAction::Init()
    {
      // Compute a goal pose rotated by specified angle around robot's
      // _current_ pose, taking into account the current driveCenter offset
      Radians heading = 0;
      if (!_isAbsoluteAngle) {
        heading = _robot->GetPose().GetRotationAngle<'Z'>();
      }
      
      Radians newAngle(heading);
      newAngle += _targetAngle;
      if(_variability != 0) {
        newAngle += GetRNG().RandDblInRange(-_variability.ToDouble(),
                                            _variability.ToDouble());
      }
      
      Pose3d rotatedPose;
      Pose3d dcPose = _robot->GetDriveCenterPose();
      dcPose.SetRotation(newAngle, Z_AXIS_3D());
      _robot->ComputeOriginPose(dcPose, rotatedPose);
      
      _targetAngle = rotatedPose.GetRotation().GetAngleAroundZaxis();
      
      Radians currentAngle;
      _inPosition = IsBodyInPosition(*_robot, currentAngle);
      
      if(!_inPosition) {
        RobotInterface::SetBodyAngle setBodyAngle;
        setBodyAngle.angle_rad             = _targetAngle.ToFloat();
        setBodyAngle.max_speed_rad_per_sec = _maxSpeed_radPerSec;
        setBodyAngle.accel_rad_per_sec2    = _accel_radPerSec2;
        if(RESULT_OK != _robot->SendRobotMessage<RobotInterface::SetBodyAngle>(std::move(setBodyAngle))) {
          return ActionResult::FAILURE_RETRY;
        }
        
        if(_moveEyes)
        {
          // Disable keep face alive if it is enabled and save so we can restore later
          _wasKeepFaceAliveEnabled = _robot->GetAnimationStreamer().GetParam<bool>(LiveIdleAnimationParameter::EnableKeepFaceAlive);
          if(_wasKeepFaceAliveEnabled) {
            _robot->GetAnimationStreamer().SetParam(LiveIdleAnimationParameter::EnableKeepFaceAlive, false);
          }
          
          // Store half the total difference so we know when to remove eye shift
          _halfAngle = 0.5f*(_targetAngle - currentAngle).getAbsoluteVal();
          
          // Move the eyes (only if not in position)
          // Note: assuming screen is about the same x distance from the neck joint as the head cam
          const Radians angleDiff = _targetAngle - currentAngle;
          const f32 x_mm = std::tan(angleDiff.ToFloat()) * HEAD_CAM_POSITION[0];
          const f32 xPixShift = x_mm * (static_cast<f32>(ProceduralFace::WIDTH) / (4*SCREEN_SIZE[0]));
          _robot->ShiftEyes(_eyeShiftTag, xPixShift, 0, 4*IKeyFrame::SAMPLE_LENGTH_MS, "TurnInPlaceEyeDart");
        }
      }
      
      return ActionResult::SUCCESS;
    }
    
    bool TurnInPlaceAction::IsBodyInPosition(const Robot& robot, Radians& currentAngle) const
    {
      currentAngle = _robot->GetPose().GetRotation().GetAngleAroundZaxis();
      const bool inPosition = NEAR(currentAngle-_targetAngle, 0.f, _angleTolerance);
      return inPosition;
    }
    
    ActionResult TurnInPlaceAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      Radians currentAngle;
      
      if(!_inPosition) {
        _inPosition = IsBodyInPosition(*_robot, currentAngle);
      }
      
      // When we've turned at least halfway, remove eye dart
      if(AnimationStreamer::NotAnimatingTag != _eyeShiftTag) {
        if(_inPosition || NEAR(currentAngle-_targetAngle, 0.f, _halfAngle))
        {
          PRINT_NAMED_INFO("TurnInPlaceAction.CheckIfDone.RemovingEyeShift",
                           "Currently at %.1fdeg, on the way to %.1fdeg, within "
                           "half angle of %.1fdeg", currentAngle.getDegrees(),
                           _targetAngle.getDegrees(), _halfAngle.getDegrees());
          _robot->GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag, 3*IKeyFrame::SAMPLE_LENGTH_MS);
          _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
        }
      }
      
      // Wait to get a state message back from the physical robot saying its body
      // is in the commanded position
      // TODO: Is this really necessary in practice?
      if(_inPosition) {
        result = ActionResult::SUCCESS;
      } else {
        PRINT_NAMED_INFO("TurnInPlaceAction.CheckIfDone",
                         "[%d] Waiting for body to reach angle: %.1fdeg vs. %.1fdeg(+/-%.1f) (tol: %f) (pfid: %d)",
                         GetTag(),
                         currentAngle.getDegrees(),
                         _targetAngle.getDegrees(),
                         _variability.getDegrees(),
                         _angleTolerance.ToFloat(),
                         _robot->GetPoseFrameID());
      }
      
      if( _robot->GetMoveComponent().IsMoving() ) {
        _turnStarted = true;
      }
      else if( _turnStarted ) {
        PRINT_NAMED_WARNING("TurnInPlaceAction.StoppedMakingProgress",
                            "[%d] giving up since we stopped moving",
                            GetTag());
        result = ActionResult::FAILURE_RETRY;
      }
      
      return result;
    }
    
    void TurnInPlaceAction::Cleanup()
    {
      if(_moveEyes)
      {
        // Make sure eye shift gets removed no matter what
        if(AnimationStreamer::NotAnimatingTag != _eyeShiftTag) {
          _robot->GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag);
          _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
        }
        // Restore previous keep face alive setting
        if(_wasKeepFaceAliveEnabled) {
          _robot->GetAnimationStreamer().SetParam(LiveIdleAnimationParameter::EnableKeepFaceAlive, true);
        }
      }
    }
    
#pragma mark ---- DriveStraightAction ----
    
    DriveStraightAction::DriveStraightAction(f32 dist_mm, f32 speed_mmps)
    : _dist_mm(dist_mm)
    , _speed_mmps(speed_mmps)
    {
      
    }
    
    ActionResult DriveStraightAction::Init()
    {
      const Radians heading = _robot->GetPose().GetRotation().GetAngleAroundZaxis();
      
      const Vec3f& T = _robot->GetPose().GetTranslation();
      const f32 x_start = T.x();
      const f32 y_start = T.y();
      
      const f32 x_end = x_start + _dist_mm * std::cos(heading.ToFloat());
      const f32 y_end = y_start + _dist_mm * std::sin(heading.ToFloat());
      
      Planning::Path path;
      // TODO: does matID matter? I'm just using 0 below
      if(false  == path.AppendLine(0, x_start, y_start, x_end, y_end,
                                   _speed_mmps, _accel_mmps2, _decel_mmps2))
      {
        PRINT_NAMED_ERROR("DriveStraightAction.Init.AppendLineFailed", "");
        return ActionResult::FAILURE_ABORT;
      }
      
      _name = ("DriveStraight" + std::to_string(_dist_mm) + "mm@" +
               std::to_string(_speed_mmps) + "mmpsAction");
      
      _hasStarted = false;
      
      // Tell robot to execute this simple path
      if(RESULT_OK != _robot->ExecutePath(path, false)) {
        return ActionResult::FAILURE_ABORT;
      }
      
      return ActionResult::SUCCESS;
    }
    
    ActionResult DriveStraightAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      if(!_hasStarted) {
        PRINT_NAMED_INFO("DriveStraightAction.CheckIfDone.WaitingForPathStart", "");
        _hasStarted = _robot->IsTraversingPath();
      } else if(/*hasStarted AND*/ !_robot->IsTraversingPath()) {
        result = ActionResult::SUCCESS;
      }
      
      return result;
    }
    
#pragma mark ---- MoveHeadToAngleAction ----
    
    MoveHeadToAngleAction::MoveHeadToAngleAction(const Radians& headAngle, const Radians& tolerance, const Radians& variability)
    : _headAngle(headAngle)
    , _angleTolerance(tolerance)
    , _variability(variability)
    , _name("MoveHeadTo" + std::to_string(std::round(RAD_TO_DEG(_headAngle.ToFloat()))) + "DegAction")
    , _inPosition(false)
    {
      if(_headAngle < MIN_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.Constructor",
                            "Requested head angle (%.1fdeg) less than min head angle (%.1fdeg). Clipping.",
                            _headAngle.getDegrees(), RAD_TO_DEG(MIN_HEAD_ANGLE));
        _headAngle = MIN_HEAD_ANGLE;
      } else if(_headAngle > MAX_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.Constructor",
                            "Requested head angle (%.1fdeg) more than max head angle (%.1fdeg). Clipping.",
                            _headAngle.getDegrees(), RAD_TO_DEG(MAX_HEAD_ANGLE));
        _headAngle = MAX_HEAD_ANGLE;
      }
      
      if( _angleTolerance.ToFloat() < HEAD_ANGLE_TOL ) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.InvalidTolerance",
                            "Tried to set tolerance of %fdeg, min is %f",
                            RAD_TO_DEG(_angleTolerance.ToFloat()),
                            RAD_TO_DEG(HEAD_ANGLE_TOL));
        _angleTolerance = HEAD_ANGLE_TOL;
      }
      
      if(_variability > 0) {
        _headAngle += GetRNG().RandDblInRange(-_variability.ToDouble(), _variability.ToDouble());
        _headAngle = CLIP(_headAngle, MIN_HEAD_ANGLE, MAX_HEAD_ANGLE);
      }
    }
    
    bool MoveHeadToAngleAction::IsHeadInPosition(const Robot& robot) const
    {
      const bool inPosition = NEAR(Radians(_robot->GetHeadAngle()) - _headAngle, 0.f, _angleTolerance);
      
      return inPosition;
    }
    
    ActionResult MoveHeadToAngleAction::Init()
    {
      ActionResult result = ActionResult::SUCCESS;
      
      _inPosition = IsHeadInPosition(*_robot);
      
      if(!_inPosition) {
        if(RESULT_OK != _robot->GetMoveComponent().MoveHeadToAngle(_headAngle.ToFloat(),
                                                                   _maxSpeed_radPerSec,
                                                                   _accel_radPerSec2,
                                                                   _duration_sec))
        {
          result = ActionResult::FAILURE_ABORT;
        }
        
        if(_moveEyes)
        {
          // Store initial state of keep face alive so we can restore it
          _wasKeepFaceAliveEnabled = _robot->GetAnimationStreamer().GetParam<bool>(LiveIdleAnimationParameter::EnableKeepFaceAlive);
          if(_wasKeepFaceAliveEnabled) {
            _robot->GetAnimationStreamer().SetParam(LiveIdleAnimationParameter::EnableKeepFaceAlive, false);
          }
          
          // Lead with the eyes, if not in position
          // Note: assuming screen is about the same x distance from the neck joint as the head cam
          Radians angleDiff =  _robot->GetHeadAngle() - _headAngle;
          const f32 y_mm = std::tan(angleDiff.ToFloat()) * HEAD_CAM_POSITION[0];
          const f32 yPixShift = y_mm * (static_cast<f32>(ProceduralFace::HEIGHT/4) / SCREEN_SIZE[1]);
          
          _robot->ShiftEyes(_eyeShiftTag, 0, yPixShift, 4*IKeyFrame::SAMPLE_LENGTH_MS, "MoveHeadToAngleEyeShift");
          
          if(!_holdEyes) {
            // Store the half the angle differene so we know when to remove eye shift
            _halfAngle = 0.5f*(_headAngle - _robot->GetHeadAngle()).getAbsoluteVal();
          }
        }
      }
      
      return result;
    }
    
    ActionResult MoveHeadToAngleAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      if(!_inPosition) {
        _inPosition = IsHeadInPosition(*_robot);
      }
      
      if(!_holdEyes && AnimationStreamer::NotAnimatingTag != _eyeShiftTag)
      {
        // If we're not there yet but at least halfway, and we're not supposed
        // to "hold" the eyes, then remove eye shift
        if(_inPosition || NEAR(Radians(_robot->GetHeadAngle()) - _headAngle, 0.f, _halfAngle))
        {
          PRINT_NAMED_INFO("MoveHeadToAngleAction.CheckIfDone.RemovingEyeShift",
                           "[%d] Currently at %.1fdeg, on the way to %.1fdeg, within "
                           "half angle of %.1fdeg",
                           GetTag(),
                           RAD_TO_DEG(_robot->GetHeadAngle()),
                           _headAngle.getDegrees(),
                           _halfAngle.getDegrees());
          
          _robot->GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag, 3*IKeyFrame::SAMPLE_LENGTH_MS);
          _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
        }
      }
      
      if( _robot->GetMoveComponent().IsHeadMoving() ) {
        _motionStarted = true;
      }
      
      // Wait to get a state message back from the physical robot saying its head
      // is in the commanded position
      // TODO: Is this really necessary in practice?
      if(_inPosition) {
        result = ActionResult::SUCCESS;
      } else {
        PRINT_NAMED_INFO("MoveHeadToAngleAction.CheckIfDone",
                         "[%d] Waiting for head to get in position: %.1fdeg vs. %.1fdeg(+/-%.1f)",
                         GetTag(),
                         RAD_TO_DEG(_robot->GetHeadAngle()), _headAngle.getDegrees(), _variability.getDegrees());
        
        if( _motionStarted && ! _robot->GetMoveComponent().IsHeadMoving() ) {
          PRINT_NAMED_WARNING("MoveHeadToAngleAction.StoppedMakingProgress",
                              "[%d] giving up since we stopped moving",
                              GetTag());
          result = ActionResult::FAILURE_RETRY;
        }
      }
      
      return result;
    }
    
    void MoveHeadToAngleAction::Cleanup()
    {
      if(AnimationStreamer::NotAnimatingTag != _eyeShiftTag)
      {
        // Make sure eye shift gets removed, by this action, or by the MoveComponent if "hold" is enabled
        if(_holdEyes) {
          _robot->GetMoveComponent().RemoveFaceLayerWhenHeadMoves(_eyeShiftTag, 3*IKeyFrame::SAMPLE_LENGTH_MS);
        } else {
          _robot->GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag);
        }
        _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
      }
      if(_moveEyes) {
        // Restore previous keep face alive setting
        if(_wasKeepFaceAliveEnabled) {
          _robot->GetAnimationStreamer().SetParam(LiveIdleAnimationParameter::EnableKeepFaceAlive, true);
        }
      }
    }
    
#pragma mark ---- MoveLiftToHeightAction ----
    
    MoveLiftToHeightAction::MoveLiftToHeightAction(const f32 height_mm, const f32 tolerance_mm, const f32 variability)
    : _height_mm(height_mm)
    , _heightTolerance(tolerance_mm)
    , _variability(variability)
    , _name("MoveLiftTo" + std::to_string(_height_mm) + "mmAction")
    , _inPosition(false)
    {
      
    }
    
    MoveLiftToHeightAction::MoveLiftToHeightAction(const Preset preset, const f32 tolerance_mm)
    : MoveLiftToHeightAction(GetPresetHeight(preset), tolerance_mm, 0.f)
    {
      _name = "MoveLiftTo";
      _name += GetPresetName(preset);
    }
    
    
    f32 MoveLiftToHeightAction::GetPresetHeight(Preset preset)
    {
      static const std::map<Preset, f32> LUT = {
        {Preset::LOW_DOCK,   LIFT_HEIGHT_LOWDOCK},
        {Preset::HIGH_DOCK,  LIFT_HEIGHT_HIGHDOCK},
        {Preset::CARRY,      LIFT_HEIGHT_CARRY},
        {Preset::OUT_OF_FOV, -1.f},
      };
      
      return LUT.at(preset);
    }
    
    const std::string& MoveLiftToHeightAction::GetPresetName(Preset preset)
    {
      static const std::map<Preset, std::string> LUT = {
        {Preset::LOW_DOCK,   "LowDock"},
        {Preset::HIGH_DOCK,  "HighDock"},
        {Preset::CARRY,      "HeightCarry"},
        {Preset::OUT_OF_FOV, "OutOfFOV"},
      };
      
      static const std::string unknown("UnknownPreset");
      
      auto iter = LUT.find(preset);
      if(iter == LUT.end()) {
        return unknown;
      } else {
        return iter->second;
      }
    }
    
    bool MoveLiftToHeightAction::IsLiftInPosition(const Robot& robot) const
    {
      const bool inPosition = (NEAR(_heightWithVariation, _robot->GetLiftHeight(), _heightTolerance) &&
                               !_robot->GetMoveComponent().IsLiftMoving());
      
      return inPosition;
    }
    
    ActionResult MoveLiftToHeightAction::Init()
    {
      ActionResult result = ActionResult::SUCCESS;
      
      if (_height_mm >= 0 && (_height_mm < LIFT_HEIGHT_LOWDOCK || _height_mm > LIFT_HEIGHT_CARRY)) {
        PRINT_NAMED_WARNING("MoveLiftToHeightAction.Init.InvalidHeight",
                            "%f mm. Clipping to be in range.", _height_mm);
        _height_mm = CLIP(_height_mm, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
      }
      
      if(_height_mm < 0.f) {
        // Choose whatever is closer to current height, LOW or CARRY:
        const f32 currentHeight = _robot->GetLiftHeight();
        const f32 low   = GetPresetHeight(Preset::LOW_DOCK);
        const f32 carry = GetPresetHeight(Preset::CARRY);
        // Absolute values here shouldn't be necessary, since these are supposed
        // to be the lowest and highest possible lift settings, but just in case...
        if( std::abs(currentHeight-low) < std::abs(carry-currentHeight)) {
          _heightWithVariation = low;
        } else {
          _heightWithVariation = carry;
        }
      } else {
        _heightWithVariation = _height_mm;
        if(_variability > 0.f) {
          _heightWithVariation += GetRNG().RandDblInRange(-_variability, _variability);
        }
        _heightWithVariation = CLIP(_heightWithVariation, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
      }
      
      
      // Convert height tolerance to angle tolerance and make sure that it's larger
      // than the tolerance that the liftController uses.
      
      // Convert target height, height - tol, and height + tol to angles.
      f32 heightLower = _heightWithVariation - _heightTolerance;
      f32 heightUpper = _heightWithVariation + _heightTolerance;
      f32 targetAngle = Robot::ConvertLiftHeightToLiftAngleRad(_heightWithVariation);
      f32 targetAngleLower = Robot::ConvertLiftHeightToLiftAngleRad(heightLower);
      f32 targetAngleUpper = Robot::ConvertLiftHeightToLiftAngleRad(heightUpper);
      
      // Neither of the angular differences between targetAngle and its associated
      // lower and upper tolerance limits should be smaller than LIFT_ANGLE_TOL.
      // That is, unless the limits exceed the physical limits of the lift.
      f32 minAngleDiff = std::numeric_limits<f32>::max();
      if (heightLower > LIFT_HEIGHT_LOWDOCK) {
        minAngleDiff = targetAngle - targetAngleLower;
      }
      if (heightUpper < LIFT_HEIGHT_CARRY) {
        minAngleDiff = std::min(minAngleDiff, targetAngleUpper - targetAngle);
      }
      
      if (minAngleDiff < LIFT_ANGLE_TOL) {
        // Tolerance is too small. Clip to be within range.
        f32 desiredHeightLower = Robot::ConvertLiftAngleToLiftHeightMM(targetAngle - LIFT_ANGLE_TOL);
        f32 desiredHeightUpper = Robot::ConvertLiftAngleToLiftHeightMM(targetAngle + LIFT_ANGLE_TOL);
        f32 newHeightTolerance = std::max(_height_mm - desiredHeightLower, desiredHeightUpper - _height_mm);
        
        PRINT_NAMED_WARNING("MoveLiftToHeightAction.Init.TolTooSmall",
                            "HeightTol %f mm == AngleTol %f rad near height of %f mm. Clipping tol to %f mm",
                            _heightTolerance, minAngleDiff, _heightWithVariation, newHeightTolerance);
        _heightTolerance = newHeightTolerance;
      }
      
      _inPosition = IsLiftInPosition(*_robot);
      
      if(!_inPosition) {
        if(_robot->GetMoveComponent().MoveLiftToHeight(_heightWithVariation,
                                                       _maxLiftSpeedRadPerSec,
                                                       _liftAccelRacPerSec2,
                                                       _duration) != RESULT_OK) {
          result = ActionResult::FAILURE_ABORT;
        }
      }
      
      return result;
    }
    
    ActionResult MoveLiftToHeightAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      if(!_inPosition) {
        _inPosition = IsLiftInPosition(*_robot);
      }
      
      // TODO: Somehow verify robot got command to move lift before declaring success
      /*
       // Wait for the lift to start moving (meaning robot received command) and
       // then stop moving
       static bool liftStartedMoving = false;
       if(!liftStartedMoving) {
       liftStartedMoving = _robot->IsLiftMoving();
       }
       else
       */
      
      if( _robot->GetMoveComponent().IsLiftMoving() ) {
        _motionStarted = true;
      }
      
      if(_inPosition) {
        result = ActionResult::SUCCESS;
      } else {
        PRINT_NAMED_INFO("MoveLiftToHeightAction.CheckIfDone",
                         "[%d] Waiting for lift to get in position: %.1fmm vs. %.1fmm (tol: %f)",
                         GetTag(),
                         _robot->GetLiftHeight(), _heightWithVariation, _heightTolerance);
        
        if( _motionStarted && ! _robot->GetMoveComponent().IsLiftMoving() ) {
          PRINT_NAMED_WARNING("MoveLiftToHeightAction.StoppedMakingProgress",
                              "[%d] giving up since we stopped moving",
                              GetTag());
          result = ActionResult::FAILURE_RETRY;
        }
      }
      
      return result;
    }
    
#pragma mark ---- PanAndTiltAction ----
    
    PanAndTiltAction::PanAndTiltAction(Radians bodyPan, Radians headTilt,
                                       bool isPanAbsolute, bool isTiltAbsolute)
    : _compoundAction{}
    , _bodyPanAngle(bodyPan)
    , _headTiltAngle(headTilt)
    , _isPanAbsolute(isPanAbsolute)
    , _isTiltAbsolute(isTiltAbsolute)
    {

    }
    
    void PanAndTiltAction::SetMaxPanSpeed(f32 maxSpeed_radPerSec)
    {
      if (maxSpeed_radPerSec == 0.f) {
        _maxPanSpeed_radPerSec = _kDefaultMaxPanSpeed;
      } else if (std::fabsf(maxSpeed_radPerSec) > MAX_BODY_ROTATION_SPEED_RAD_PER_SEC) {
        PRINT_NAMED_WARNING("PanAndTiltAction.SetMaxSpeed.PanSpeedExceedsLimit",
                            "Speed of %f deg/s exceeds limit of %f deg/s. Clamping.",
                            RAD_TO_DEG_F32(maxSpeed_radPerSec), MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
        _maxPanSpeed_radPerSec = std::copysign(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, maxSpeed_radPerSec);
      } else {
        _maxPanSpeed_radPerSec = maxSpeed_radPerSec;
      }
    }
    
    void PanAndTiltAction::SetPanAccel(f32 accel_radPerSec2)
    {
      // If 0, use default value
      if (accel_radPerSec2 == 0.f) {
        _panAccel_radPerSec2 = _kDefaultPanAccel;
      } else {
        _panAccel_radPerSec2 = accel_radPerSec2;
      }
    }
    
    void PanAndTiltAction::SetPanTolerance(const Radians& angleTol_rad)
    {
      if (angleTol_rad == 0.f) {
        _panAngleTol = _kDefaultPanAngleTol;
        return;
      }
      
      _panAngleTol = angleTol_rad.getAbsoluteVal();
      
      // NOTE: can't be lower than what is used internally on the robot
      if( _panAngleTol.ToFloat() < POINT_TURN_ANGLE_TOL ) {
        PRINT_NAMED_WARNING("PanAndTiltAction.InvalidTolerance",
                            "Tried to set tolerance of %fdeg, min is %f",
                            RAD_TO_DEG(_panAngleTol.ToFloat()),
                            RAD_TO_DEG(POINT_TURN_ANGLE_TOL));
        _panAngleTol = POINT_TURN_ANGLE_TOL;
      }
    }
    
    void PanAndTiltAction::SetMaxTiltSpeed(f32 maxSpeed_radPerSec)
    {
      if (maxSpeed_radPerSec == 0.f) {
        _maxTiltSpeed_radPerSec = _kDefaultMaxTiltSpeed;
      } else {
        _maxTiltSpeed_radPerSec = maxSpeed_radPerSec;
      }
    }
    
    void PanAndTiltAction::SetTiltAccel(f32 accel_radPerSec2)
    {
      if (accel_radPerSec2 == 0.f) {
        _tiltAccel_radPerSec2 = _kDefaultTiltAccel;
      } else {
        _tiltAccel_radPerSec2 = accel_radPerSec2;
      }
    }
    
    void PanAndTiltAction::SetTiltTolerance(const Radians& angleTol_rad)
    {
      // If 0, use default value
      if (angleTol_rad == 0.f) {
        _tiltAngleTol = _kDefaultTiltAngleTol;
        return;
      }
      
      _tiltAngleTol = angleTol_rad.getAbsoluteVal();
      
      // NOTE: can't be lower than what is used internally on the robot
      if( _tiltAngleTol.ToFloat() < HEAD_ANGLE_TOL ) {
        PRINT_NAMED_WARNING("PanAndTiltAction.InvalidTolerance",
                            "Tried to set tolerance of %fdeg, min is %f",
                            RAD_TO_DEG(_tiltAngleTol.ToFloat()),
                            RAD_TO_DEG(HEAD_ANGLE_TOL));
        _tiltAngleTol = HEAD_ANGLE_TOL;
      }
    }
    
    ActionResult PanAndTiltAction::Init()
    {
      CompoundActionParallel* newCompoundParallel = new CompoundActionParallel();
      _compoundAction = newCompoundParallel;
      RegisterSubAction(_compoundAction);
      
      newCompoundParallel->EnableMessageDisplay(IsMessageDisplayEnabled());
      
      TurnInPlaceAction* action = new TurnInPlaceAction(_bodyPanAngle, _isPanAbsolute);
      action->SetTolerance(_panAngleTol);
      action->SetMaxSpeed(_maxPanSpeed_radPerSec);
      action->SetAccel(_panAccel_radPerSec2);
      newCompoundParallel->AddAction(action);
      
      const Radians newHeadAngle = _isTiltAbsolute ? _headTiltAngle : _robot->GetHeadAngle() + _headTiltAngle;
      MoveHeadToAngleAction* headAction = new MoveHeadToAngleAction(newHeadAngle, _tiltAngleTol);
      headAction->SetMaxSpeed(_maxTiltSpeed_radPerSec);
      headAction->SetAccel(_tiltAccel_radPerSec2);
      newCompoundParallel->AddAction(headAction);
      
      // Put the angles in the name for debugging
      _name = ("Pan" + std::to_string(std::round(_bodyPanAngle.getDegrees())) +
               "AndTilt" + std::to_string(std::round(_headTiltAngle.getDegrees())) +
               "Action");
      
      // Prevent the compound action from signaling completion
      newCompoundParallel->SetEmitCompletionSignal(false);
      
      // Prevent the compound action from locking tracks (the PanAndTiltAction handles it itself)
      newCompoundParallel->SetSuppressTrackLocking(true);
      
      // Go ahead and do the first Update for the compound action so we don't
      // "waste" the first CheckIfDone call doing so. Proceed so long as this
      // first update doesn't _fail_
      ActionResult compoundResult = newCompoundParallel->Update();
      if(ActionResult::SUCCESS == compoundResult ||
         ActionResult::RUNNING == compoundResult)
      {
        return ActionResult::SUCCESS;
      } else {
        return compoundResult;
      }
      
    } // PanAndTiltAction::Init()
    
    
    ActionResult PanAndTiltAction::CheckIfDone()
    {
      return _compoundAction->Update();
    }
    
#pragma mark ---- FaceObjectAction ----
    
    FaceObjectAction::FaceObjectAction(ObjectID objectID,
                                       Radians maxTurnAngle,
                                       bool visuallyVerifyWhenDone,
                                       bool headTrackWhenDone)
    : FaceObjectAction(objectID, Vision::Marker::ANY_CODE,
                       maxTurnAngle, visuallyVerifyWhenDone, headTrackWhenDone)
    {
      
    }
    
    FaceObjectAction::FaceObjectAction(ObjectID objectID,
                                       Vision::Marker::Code whichCode,
                                       Radians maxTurnAngle,
                                       bool visuallyVerifyWhenDone,
                                       bool headTrackWhenDone)
    : FacePoseAction(maxTurnAngle)
    , _facePoseCompoundActionDone(false)
    , _visuallyVerifyAction()
    , _objectID(objectID)
    , _whichCode(whichCode)
    , _visuallyVerifyWhenDone(visuallyVerifyWhenDone)
    , _headTrackWhenDone(headTrackWhenDone)
    {
      
    }
    
    Radians FaceObjectAction::GetHeadAngle(f32 heightDiff)
    {
      // TODO: Just commanding fixed head angle depending on height of object.
      //       Verify this is ok with the wide angle lens. If not, dynamically compute
      //       head angle so that it is at the bottom (for high blocks) or top (for low blocks)
      //       of the image.
      Radians headAngle = DEG_TO_RAD_F32(-15);
      if (heightDiff > 0) {
        headAngle = DEG_TO_RAD_F32(17);
      }
      return headAngle;
    }
    
    ActionResult FaceObjectAction::Init()
    {
      _visuallyVerifyAction = new VisuallyVerifyObjectAction(_objectID, _whichCode);
      RegisterSubAction(_visuallyVerifyAction);
      
      ObservableObject* object = _robot->GetBlockWorld().GetObjectByID(_objectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("FaceObjectAction.Init.ObjectNotFound",
                          "Object with ID=%d no longer exists in the world.",
                          _objectID.GetValue());
        return ActionResult::FAILURE_ABORT;
      }
      
      Pose3d objectPoseWrtRobot;
      if(_whichCode == Vision::Marker::ANY_CODE) {
        if(false == object->GetPose().GetWithRespectTo(_robot->GetPose(), objectPoseWrtRobot)) {
          PRINT_NAMED_ERROR("FaceObjectAction.Init.ObjectPoseOriginProblem",
                            "Could not get pose of object %d w.r.t. robot pose.",
                            _objectID.GetValue());
          return ActionResult::FAILURE_ABORT;
        }
      } else {
        // Use the closest marker with the specified code:
        std::vector<Vision::KnownMarker*> const& markers = object->GetMarkersWithCode(_whichCode);
        
        if(markers.empty()) {
          PRINT_NAMED_ERROR("FaceObjectAction.Init.NoMarkersWithCode",
                            "Object %d does not have any markers with code %d.",
                            _objectID.GetValue(), _whichCode);
          return ActionResult::FAILURE_ABORT;
        }
        
        Vision::KnownMarker* closestMarker = nullptr;
        if(markers.size() == 1) {
          closestMarker = markers.front();
          if(false == closestMarker->GetPose().GetWithRespectTo(_robot->GetPose(), objectPoseWrtRobot)) {
            PRINT_NAMED_ERROR("FaceObjectAction.Init.MarkerOriginProblem",
                              "Could not get pose of marker with code %d of object %d "
                              "w.r.t. robot pose.", _whichCode, _objectID.GetValue() );
            return ActionResult::FAILURE_ABORT;
          }
        } else {
          f32 closestDist = std::numeric_limits<f32>::max();
          Pose3d markerPoseWrtRobot;
          for(auto marker : markers) {
            if(false == marker->GetPose().GetWithRespectTo(_robot->GetPose(), markerPoseWrtRobot)) {
              PRINT_NAMED_ERROR("FaceObjectAction.Init.MarkerOriginProblem",
                                "Could not get pose of marker with code %d of object %d "
                                "w.r.t. robot pose.", _whichCode, _objectID.GetValue() );
              return ActionResult::FAILURE_ABORT;
            }
            
            const f32 currentDist = markerPoseWrtRobot.GetTranslation().Length();
            if(currentDist < closestDist) {
              closestDist = currentDist;
              closestMarker = marker;
              objectPoseWrtRobot = markerPoseWrtRobot;
            }
          }
        }
        
        if(closestMarker == nullptr) {
          PRINT_NAMED_ERROR("FaceObjectAction.Init.NoClosestMarker",
                            "No closest marker found for object %d.", _objectID.GetValue());
          return ActionResult::FAILURE_ABORT;
        }
      }
      
      // Have to set the parent class's pose before calling its Init()
      SetPose(objectPoseWrtRobot);
      
      ActionResult facePoseInitResult = FacePoseAction::Init();
      if(ActionResult::SUCCESS != facePoseInitResult) {
        return facePoseInitResult;
      }
      
      _facePoseCompoundActionDone = false;
      
      // Disable completion signals since this is inside another action
      _visuallyVerifyAction->SetEmitCompletionSignal(false);
      
      return ActionResult::SUCCESS;
    } // FaceObjectAction::Init()
    
    
    ActionResult FaceObjectAction::CheckIfDone()
    {
      // Tick the compound action until it completes
      if(!_facePoseCompoundActionDone) {
        ActionResult compoundResult = FacePoseAction::CheckIfDone();
        
        if(compoundResult != ActionResult::SUCCESS) {
          return compoundResult;
        } else {
          _facePoseCompoundActionDone = true;
          
          // Go ahead and do a first tick of visual verification's Update, to
          // get it initialized
          ActionResult verificationResult = _visuallyVerifyAction->Update();
          if(ActionResult::SUCCESS != verificationResult) {
            return verificationResult;
          }
        }
      }
      PRINT_NAMED_INFO("FaceObjectAction.CheckIfDone", "_compoundAction returned success");
      // If we get here, _compoundAction completed returned SUCCESS. So we can
      // can continue with our additional checks:
      if (_visuallyVerifyWhenDone) {
        ActionResult verificationResult = _visuallyVerifyAction->Update();
        if (verificationResult != ActionResult::SUCCESS) {
          return verificationResult;
        } else {
          _visuallyVerifyWhenDone = false;
        }
      }
      
      if(_headTrackWhenDone) {
        ActionList::SlotHandle inSlot = GetSlotHandle();
        if(ActionList::UnknownSlot == inSlot) {
          PRINT_NAMED_WARNING("FaceObjectAction.CheckIfDone.UnknownSlot",
                              "Queuing TrackObjectAction because headTrackWhenDone==true, but "
                              "slot unknown. Using DriveAndManipulateSlot");
          inSlot = Robot::DriveAndManipulateSlot;
        }
         PRINT_NAMED_INFO("FaceObjectAction.CheckIfDone", "Queueing new action");
        _robot->GetActionList().QueueActionNext(inSlot, new TrackObjectAction(_objectID));
      }
      PRINT_NAMED_INFO("FaceObjectAction.CheckIfDone", "End");
      return ActionResult::SUCCESS;
    } // FaceObjectAction::CheckIfDone()
    
    
    const std::string& FaceObjectAction::GetName() const
    {
      static const std::string name("FaceObjectAction");
      return name;
    }
    
    void FaceObjectAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      info.numObjects = 1;
      info.objectIDs[0] = _objectID;
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
    }
    
#pragma mark ---- TraverseObjectAction ----
    
    TraverseObjectAction::TraverseObjectAction(ObjectID objectID, const bool useManualSpeed)
    : _objectID(objectID)
    , _useManualSpeed(useManualSpeed)
    {
  
    }
    
    const std::string& TraverseObjectAction::GetName() const
    {
      static const std::string name("TraverseObjectAction");
      return name;
    }
    
    void TraverseObjectAction::SetSpeedAndAccel(f32 speed_mmps, f32 accel_mmps2) {
      _speed_mmps = speed_mmps;
      _accel_mmps2 = accel_mmps2;
    }
    
    ActionResult TraverseObjectAction::UpdateInternal()
    {
      // Select the chosen action based on the object's type, if we haven't
      // already
      if(_chosenAction == nullptr) {
        ActionableObject* object = dynamic_cast<ActionableObject*>(_robot->GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("TraverseObjectAction.Init.ObjectNotFound",
                            "Could not get actionable object with ID = %d from world.", _objectID.GetValue());
          return ActionResult::FAILURE_ABORT;
        }
        
        if(object->GetType() == ObjectType::Bridge_LONG ||
           object->GetType() == ObjectType::Bridge_SHORT)
        {
          CrossBridgeAction* bridgeAction = new CrossBridgeAction(_objectID, _useManualSpeed);
          bridgeAction->SetSpeedAndAccel(_speed_mmps, _accel_mmps2);
          _chosenAction = bridgeAction;
          RegisterSubAction(_chosenAction);
        }
        else if(object->GetType() == ObjectType::Ramp_Basic) {
          AscendOrDescendRampAction* rampAction = new AscendOrDescendRampAction(_objectID, _useManualSpeed);
          rampAction->SetSpeedAndAccel(_speed_mmps, _accel_mmps2);
          _chosenAction = rampAction;
          RegisterSubAction(_chosenAction);
        }
        else {
          PRINT_NAMED_ERROR("TraverseObjectAction.Init.CannotTraverseObjectType",
                            "Robot %d was asked to traverse object ID=%d of type %s, but "
                            "that traversal is not defined.", _robot->GetID(),
                            object->GetID().GetValue(), ObjectTypeToString(object->GetType()));
          
          return ActionResult::FAILURE_ABORT;
        }
      }
      
      // Now just use chosenAction's Update()
      assert(_chosenAction != nullptr);
      return _chosenAction->Update();
      
    } // Update()
    
#pragma mark ---- FacePoseAction ----
    
    FacePoseAction::FacePoseAction(const Pose3d& pose, Radians maxTurnAngle)
    : PanAndTiltAction(0,0,false,true)
    , _poseWrtRobot(pose)
    , _isPoseSet(true)
    , _maxTurnAngle(maxTurnAngle.getAbsoluteVal())
    {
    }
    
    FacePoseAction::FacePoseAction(Radians maxTurnAngle)
    : PanAndTiltAction(0,0,false,true)
    , _isPoseSet(false)
    , _maxTurnAngle(maxTurnAngle.getAbsoluteVal())
    {
    }
    
    Radians FacePoseAction::GetHeadAngle(f32 heightDiff)
    {
      const f32 distanceXY = Point2f(_poseWrtRobot.GetTranslation()).Length();
      const Radians headAngle = std::atan2(heightDiff, distanceXY);
      return headAngle;
    }
    
    void FacePoseAction::SetPose(const Pose3d& pose)
    {
      _poseWrtRobot = pose;
      _isPoseSet = true;
    }
    
    ActionResult FacePoseAction::Init()
    {
      if(!_isPoseSet) {
        PRINT_NAMED_ERROR("FacePoseAction.Init.PoseNotSet", "");
        return ActionResult::FAILURE_ABORT;
      }
      
      if(_poseWrtRobot.GetParent() == nullptr) {
        PRINT_NAMED_INFO("FacePoseAction.SetPose.AssumingRobotOriginAsParent", "");
        _poseWrtRobot.SetParent(_robot->GetWorldOrigin());
      }
      else if(false == _poseWrtRobot.GetWithRespectTo(_robot->GetPose(), _poseWrtRobot))
      {
        PRINT_NAMED_ERROR("FacePoseAction.Init.PoseOriginFailure",
                          "Could not get pose w.r.t. robot pose.");
        return ActionResult::FAILURE_ABORT;
      }
      
      if(_maxTurnAngle > 0)
      {
        // Compute the required angle to face the object
        const Radians turnAngle = std::atan2(_poseWrtRobot.GetTranslation().y(),
                                             _poseWrtRobot.GetTranslation().x());
        
        PRINT_NAMED_INFO("FacePoseAction.Init.TurnAngle",
                         "Computed turn angle = %.1fdeg", turnAngle.getDegrees());
        
        if(turnAngle.getAbsoluteVal() <= _maxTurnAngle) {
          SetBodyPanAngle(turnAngle);
        } else {
          PRINT_NAMED_ERROR("FacePoseAction.Init.RequiredTurnTooLarge",
                            "Required turn angle of %.1fdeg is larger than max angle of %.1fdeg.",
                            turnAngle.getDegrees(), _maxTurnAngle.getDegrees());
          return ActionResult::FAILURE_ABORT;
        }
      }
      
      // Compute the required head angle to face the object
      // NOTE: It would be more accurate to take head tilt into account, but I'm
      //  just using neck joint height as an approximation for the camera's
      //  current height, since its actual height changes slightly as the head
      //  rotates around the neck.
      const f32 heightDiff = _poseWrtRobot.GetTranslation().z() - NECK_JOINT_POSITION[2];
      Radians headAngle = GetHeadAngle(heightDiff);
      
      SetHeadTiltAngle(headAngle);
      
      // Proceed with base class's Init()
      return PanAndTiltAction::Init();
      
    } // FacePoseAction::Init()
    
    const std::string& FacePoseAction::GetName() const
    {
      static const std::string name("FacePoseAction");
      return name;
    }
    
#pragma mark ---- VisuallyVerifyObjectAction ----
    
    VisuallyVerifyObjectAction::VisuallyVerifyObjectAction(ObjectID objectID,
                                                           Vision::Marker::Code whichCode)
    : _objectID(objectID)
    , _whichCode(whichCode)
    , _waitToVerifyTime(-1)
    , _moveLiftToHeightAction()
    , _moveLiftToHeightActionDone(false)
    {
      
    }
    
    const std::string& VisuallyVerifyObjectAction::GetName() const
    {
      static const std::string name("VisuallyVerifyObject" + std::to_string(_objectID.GetValue())
                                    + "Action");
      return name;
    }
    
    ActionResult VisuallyVerifyObjectAction::Init()
    {
      using namespace ExternalInterface;
      
      _objectSeen = false;
      
      auto obsObjLambda = [this](const AnkiEvent<MessageEngineToGame>& event)
      {
        auto objectObservation = event.GetData().Get_RobotObservedObject();
        // ID has to match and we have to actually have seen a marker (not just
        // saying part of the object is in FOV due to assumed projection)
        if(!_objectSeen && objectObservation.objectID == _objectID && objectObservation.markersVisible)
        {
          _objectSeen = true;
        }
      };
      
      _observedObjectHandle = _robot->GetExternalInterface()->Subscribe(MessageEngineToGameTag::RobotObservedObject, obsObjLambda);
      
      if(_whichCode == Vision::Marker::ANY_CODE) {
        _markerSeen = true;
      } else {
        _markerSeen = false;
      }
      
      // Get lift out of the way
      _moveLiftToHeightAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::OUT_OF_FOV);
      RegisterSubAction(_moveLiftToHeightAction);
      _moveLiftToHeightAction->SetEmitCompletionSignal(false);
      _moveLiftToHeightActionDone = false;
      _waitToVerifyTime = -1.f;
      
      // Go ahead and do the first update on moving the lift, so we don't "waste"
      // the first tick of CheckIfDone initializing the sub-action.
      ActionResult moveLiftInitResult = _moveLiftToHeightAction->Update();
      if(ActionResult::SUCCESS == moveLiftInitResult ||
         ActionResult::RUNNING == moveLiftInitResult)
      {
        // Continue to CheckIfDone as long as the first Update didn't _fail_
        return ActionResult::SUCCESS;
      } else {
        return moveLiftInitResult;
      }
    }
    
    ActionResult VisuallyVerifyObjectAction::CheckIfDone()
    {
      ActionResult actionRes = ActionResult::RUNNING;
      
      const f32 currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      
      if(_objectSeen)
      {
        if(!_markerSeen)
        {
          // We've seen the object, check if we've seen the correct marker if one was
          // specified and we haven't seen it yet
          ObservableObject* object = _robot->GetBlockWorld().GetObjectByID(_objectID);
          if(object == nullptr) {
            PRINT_NAMED_ERROR("VisuallyVerifyObjectAction.CheckIfDone.ObjectNotFound",
                              "[%d] Object with ID=%d no longer exists in the world.",
                              GetTag(),
                              _objectID.GetValue());
            return ActionResult::FAILURE_ABORT;
          }
          
          // Look for which markers were seen since (and including) last observation time
          std::vector<const Vision::KnownMarker*> observedMarkers;
          object->GetObservedMarkers(observedMarkers, object->GetLastObservedTime());
          
          for(auto marker : observedMarkers) {
            if(marker->GetCode() == _whichCode) {
              _markerSeen = true;
              break;
            }
          }
          
          if(!_markerSeen) {
            // Seeing wrong marker(s). Log this for help in debugging
            std::string observedMarkerNames;
            for(auto marker : observedMarkers) {
              observedMarkerNames += Vision::MarkerTypeStrings[marker->GetCode()];
              observedMarkerNames += " ";
            }
            
            PRINT_NAMED_INFO("VisuallyVerifyObjectAction.CheckIfDone.WrongMarker",
                             "Have seen object %d, but not marker code %d. Have seen: %s",
                             _objectID.GetValue(), _whichCode, observedMarkerNames.c_str());
          }
        } // if(!_markerSeen)
        
        if(_markerSeen) {
          // We've seen the object and the correct marker: we're good to go!
          return ActionResult::SUCCESS;
        }
        
      } else {
        // Still waiting to see the object: keep moving head/lift
        if (!_moveLiftToHeightActionDone) {
          ActionResult liftActionRes = _moveLiftToHeightAction->Update();
          if (liftActionRes != ActionResult::SUCCESS) {
            if (liftActionRes != ActionResult::RUNNING) {
              PRINT_NAMED_WARNING("VisuallyVerifyObjectAction.CheckIfDone.CompoundActionFailed",
                                  "Failed to move lift out of FOV. Action result = %s\n",
                                  EnumToString(actionRes));
            }
            return liftActionRes;
          }
          _moveLiftToHeightActionDone = true;
        }
        
        // While head is moving to verification angle, this shouldn't count towards the waitToVerifyTime
        // TODO: Should this check if it's moving at all?
        if (_robot->GetMoveComponent().IsHeadMoving()) {
          _waitToVerifyTime = -1;
        }
        
        if(_waitToVerifyTime < 0.f) {
          _waitToVerifyTime = currentTime + GetWaitToVerifyTime();
        }
        
      } // if/else(objectSeen)
      
      if(currentTime > _waitToVerifyTime)
      {
        PRINT_NAMED_WARNING("VisuallyVerifyObjectAction.CheckIfDone.TimedOut",
                            "Did not see object %d and current time > waitUntilTime (%.3f>%.3f)",
                            _objectID.GetValue(), currentTime, _waitToVerifyTime);
        return ActionResult::FAILURE_ABORT;
      }
      
      return actionRes;
      
    } // VisuallyVerifyObjectAction::CheckIfDone()
    
#pragma mark ---- WaitAction ----
    
    WaitAction::WaitAction(f32 waitTimeInSeconds)
    :  _waitTimeInSeconds(waitTimeInSeconds)
    , _doneTimeInSeconds(-1.f)
    {
      // Put the wait time with two decimals of precision in the action's name
      char tempBuffer[32];
      snprintf(tempBuffer, 32, "Wait%.2fSecondsAction", _waitTimeInSeconds);
      _name = tempBuffer;
    }
    
    ActionResult WaitAction::Init()
    {
      _doneTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _waitTimeInSeconds;
      return ActionResult::SUCCESS;
    }
    
    ActionResult WaitAction::CheckIfDone()
    {
      assert(_doneTimeInSeconds > 0.f);
      if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _doneTimeInSeconds) {
        return ActionResult::SUCCESS;
      } else {
        return ActionResult::RUNNING;
      }
    }
  }
}