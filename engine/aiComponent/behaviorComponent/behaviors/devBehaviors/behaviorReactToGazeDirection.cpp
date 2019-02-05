/**
 * File: behaviorReactToGazeDirection.cpp
 *
 * Author: Robert Cosgriff
 * Created: 11/29/2018
 *
 * Description: see header
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
#include <unistd.h>

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorReactToGazeDirection.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"

#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

namespace Anki {
namespace Vector {

namespace {
  // Console vars for tuning
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinXThres_mm,        "Vision.GazeDirection", -25.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxXThres_mm,        "Vision.GazeDirection",  20.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinYThres_mm,        "Vision.GazeDirection", -20.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxYThres_mm,        "Vision.GazeDirection",  20.f);
  CONSOLE_VAR(f32,  kTurnWaitAfterFinalTurn_s,               "Vision.GazeDirection",  1.f);
  CONSOLE_VAR(f32,  kSleepTimeAfterActionCompleted_s,        "Vision.GazeDirection",  2.f);
  CONSOLE_VAR(f32,  kMaxPanSpeed_radPerSec,                  "Vision.GazeDirection",  MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);
  CONSOLE_VAR(f32,  kMaxPanAccel_radPerSec2,                 "Vision.GazeDirection",  10.f);
  CONSOLE_VAR(bool, kUseEyeContact,                          "Vision.GazeDirection",  true);
  CONSOLE_VAR(f32,  kConeFor180TurnForFaceSearch_deg,        "Vision.GazeDirection",  40.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnSideAngle_deg,         "Vision.GazeDirection",  -90.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnAroundAngle_deg,       "Vision.GazeDirection",  180.f);
  CONSOLE_VAR(s32,  kSearchForFaceNumberOfImagesToWait,      "Vision.GazeDirection",  5);
  CONSOLE_VAR(bool, kUseExistingFacesWhenSearchingForFaces,  "Vision.GazeDirection",  false);
  CONSOLE_VAR(s32,  kNumberOfTurnsForSurfacePoint,           "Vision.GazeDirection",  1);
  CONSOLE_VAR(u32,  kMaxTimeSinceTrackedFaceUpdated_ms,      "Vision.GazeDirection",  500);
  CONSOLE_VAR(bool, kUseEyeGazeToLookAtSurfaceorFaces,       "Vision.GazeDirection",  false);
  // TODO disable this use -1
  CONSOLE_VAR(f32,  kMaxDriveToSurfacePointDistance_mm2,     "Vision.GazeDirection",  30000.f*30000.f);
  CONSOLE_VAR(bool, kUseDriveStraightActionForDriveTo,       "Vision.GazeDirection", false);
  CONSOLE_VAR(f32,  kDriveStraightDistance_mm,               "Vision.GazeDirection", 200);
  CONSOLE_VAR(f32,  kMaxDriveStraightDistance_mm,            "Vision.GazeDirection", 300);
  CONSOLE_VAR(bool, kDriveStraightTurnBackToFace,            "Vision.GazeDirection", true);
  CONSOLE_VAR(bool, kUseHandDetectionHack,                   "Vision.GazeDirection", true);
  CONSOLE_VAR(f32,  kProxSensorStopingDistance_mm,           "Vision.GazeDirection", 30);
  CONSOLE_VAR(bool, kUseTranslationDistanceForDriveStraight, "Vision.GazeDirection", true);
  CONSOLE_VAR(f32,  kEyeGazeDirectionXSurfaceThreshold_mm,   "Vision.GazeDirection", 2000.f);
  CONSOLE_VAR(f32,  kEyeGazeDirectionYSurfaceThreshold_mm,   "Vision.GazeDirection", 2000.f);
  CONSOLE_VAR(f32,  kEyeGazeDirectionZSurfaceThreshold_mm,   "Vision.GazeDirection", 20.f);

  CONSOLE_VAR(f32,  kEyeGazeDirectionLowerAngleLinearizationThreshold_deg,   "Vision.GazeDirection",  45.f);
  CONSOLE_VAR(f32,  kEyeGazeDirectionUpperAngleLinearizationThreshold_deg,   "Vision.GazeDirection",  180.f);

  CONSOLE_VAR(f32,  kEyeGazeDirectionLowerAngleGaussianThreshold_deg,   "Vision.GazeDirection",  45.f);
  CONSOLE_VAR(f32,  kEyeGazeDirectionUpperAngleGaussianThreshold_deg,   "Vision.GazeDirection",  180.f);
  CONSOLE_VAR(f32,  kEyeGazeDirectionGaussianMean_deg,   "Vision.GazeDirection",  95.f);
  CONSOLE_VAR(f32,  kEyeGazeDirectionGaussianSTD_deg,   "Vision.GazeDirection",   .5555f);

  CONSOLE_VAR(bool,  kEyeGazeDirectionUseCorrection,   "Vision.GazeDirection",   true);
  CONSOLE_VAR(bool,  kEyeGazeDirectionUseLinearCorrection,   "Vision.GazeDirection",   false);
  CONSOLE_VAR(bool,  kEyeGazeDirectionUseGaussianCorrection,   "Vision.GazeDirection",   true);

  // If this is set to -1 the turns will loop
  CONSOLE_VAR(s32,  kEyeGazeDirectionMaxNumberOfTurns,   "Vision.GazeDirection",   20);
}

namespace {

  // Configuration file keys
  const char* const kSearchForPointsOnSurface = "searchForPointsOnSurface";
}

#define LOG_CHANNEL "Behaviors"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::BehaviorReactToGazeDirection(const Json::Value& config)
 : ICozmoBehavior(config)
, _iConfig(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::InitBehavior()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::BehaviorUpdate()
{

  if( !IsActivated() ) {
    return;
  }
  TransitionToCheckGazeDirection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  const std::string& debugName = "BehaviorReactToBody.InstanceConfig.LoadConfig";
  searchForPointsOnSurface = JsonTools::ParseBool(config, kSearchForPointsOnSurface, debugName);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::DynamicVariables::DynamicVariables()
{
  turnCount = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
      kSearchForPointsOnSurface,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToGazeDirection::WantsToBeActivatedBehavior() const
{
  return GetBEI().GetFaceWorld().AnyStableGazeDirection(kMaxTimeSinceTrackedFaceUpdated_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::OnBehaviorActivated()
{
  TransitionToCheckGazeDirection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToCheckForFace(const Radians& turnAngle)
{

  // Turn to the angle, pick the correct (left or right) animation, and then search for a face.
  CompoundActionSequential* turnAction = new CompoundActionSequential();
  if (turnAngle > 0) {
    turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesGetInLeft));
  } else {
    turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesGetInRight));
  }

  CompoundActionParallel* turnAndAnimate = new CompoundActionParallel();
  if (turnAngle > 0) {
    turnAndAnimate->AddAction(new TurnInPlaceAction(turnAngle.ToFloat(), false));
    // turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtFacesTurnLeft});
    turnAndAnimate->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesTurnLeft));
  } else {
    turnAndAnimate->AddAction(new TurnInPlaceAction(turnAngle.ToFloat(), false));
    // turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtFacesTurnRight});
    turnAndAnimate->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesTurnRight));
  }

  // turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
  turnAction->AddAction(turnAndAnimate);
  turnAction->AddAction(new WaitForImagesAction(kSearchForFaceNumberOfImagesToWait, VisionMode::DetectingFaces));
  turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));
  TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(SmartFaceID(), M_PI_2);
  turnTowardsFace->SetRequireFaceConfirmation(true);
  turnAction->AddAction(turnTowardsFace);
  DelegateIfInControl(turnAction, &BehaviorReactToGazeDirection::FoundNewFace);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToLookAtFace(const SmartFaceID& faceToTurnTowards, const Radians& turnAngle)
{

  // Using a turn towards face action and the face to turn towards, look for the
  // face we think is in the FOV if we were to turn to turn angle. The turn
  // angle is used to choose to play the left or right animation.
  CompoundActionSequential* turnAction = new CompoundActionSequential();
  if (turnAngle > 0) {
    turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesGetInLeft));
  } else {
    turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesGetInRight));
  }

  TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(faceToTurnTowards, M_PI);
  turnTowardsFace->SetRequireFaceConfirmation(true);

  CompoundActionParallel* turnAndAnimate = new CompoundActionParallel();
  turnAndAnimate->AddAction(turnTowardsFace);
  if (turnAngle > 0) {
    turnAndAnimate->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::GazingLookAtFacesTurnLeft));
  } else {
    turnAndAnimate->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::GazingLookAtFacesGetInRight));
  }
  turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
  turnAction->AddAction(turnAndAnimate);

  DelegateIfInControl(turnAction, &BehaviorReactToGazeDirection::FoundNewFace);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToDriveToPointOnSurface(const Pose3d& gazePose)
{
  LOG_WARNING("BehaviorReactToGazeDirection.TransitionToDriveToPointOnSurface",
              "going to try to drive to a point on the surface");
  const auto& translation = gazePose.GetTranslation();
  CompoundActionSequential* turnAction = new CompoundActionSequential();

  if ( Util::IsFltLT(translation.y(), 0.f) ) {
    turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesGetInRight));
  } else {
    turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesGetInLeft));
  }

  TurnTowardsPoseAction* turnTowardsPose = new TurnTowardsPoseAction(gazePose);
  turnTowardsPose->SetMaxPanSpeed(kMaxPanSpeed_radPerSec);
  turnTowardsPose->SetPanAccel(kMaxPanAccel_radPerSec2);
  CompoundActionParallel* turnAndAnimate = new CompoundActionParallel();
  turnAndAnimate->AddAction(turnTowardsPose);
  if ( Util::IsFltLT(translation.y(), 0.f) ) {
    // turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfacesTurnRight});
    turnAndAnimate->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesTurnRight));
  } else {
    // turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfaceTurnLeft});
    turnAndAnimate->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceTurnLeft));
  }
  // turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
  turnAction->AddAction(turnAndAnimate);

  float distanceToDrive_mm = kDriveStraightDistance_mm;
  if (kUseTranslationDistanceForDriveStraight) {
    distanceToDrive_mm = translation.Length();
  }

  if (kMaxDriveStraightDistance_mm > 0.f) {
    if (distanceToDrive_mm > kMaxDriveStraightDistance_mm) {
      distanceToDrive_mm = kMaxDriveStraightDistance_mm;
    }
  }

  if (kUseDriveStraightActionForDriveTo) {
    DriveStraightAction* driveStraightAction = new DriveStraightAction(distanceToDrive_mm);
    turnAction->AddAction(driveStraightAction);

  } else if (kUseHandDetectionHack) {
    Radians gazeAngle = atan2f(translation.y(), translation.x());
    LOG_WARNING("BehaviorReactToGazeDirection.TransitionToDriveToPointOnSurface",
                "Turn pose (x=%.3f, y=%.3f, z=%.3f), Turn Angle %.3f",
                translation.x(), translation.y(), translation.z(),
                RAD_TO_DEG(gazeAngle.ToFloat()));

    CompoundActionParallel* driveAction = new CompoundActionParallel({
      new DriveStraightAction(distanceToDrive_mm),
      //new ReselectingLoopAnimationAction{AnimationTrigger::ExploringReactToHandDrive},
      new WaitForLambdaAction([this](Robot& robot) {
        const bool detectedUnexpectedMovement = (robot.GetMoveComponent().IsUnexpectedMovementDetected());

        bool proxSensorFire = false;
        auto& proxSensor = GetBEI().GetComponentWrapper( BEIComponentID::ProxSensor ).GetComponent<ProxSensorComponent>();
        uint16_t proxDist_mm = 0;
        if ( proxSensor.GetLatestDistance_mm( proxDist_mm) ) {
          if ( proxDist_mm < kProxSensorStopingDistance_mm) {
            proxSensorFire = true;
          }
        }
        return (detectedUnexpectedMovement || proxSensorFire);
      }),
    });
    // The compound drive action will complete either when the DriveStraight finishes or unexpected movement
    // is detected (the looping anim will never finish).
    driveAction->SetShouldEndWhenFirstActionCompletes(true);
    turnAction->AddAction(driveAction);

  } else {

    DriveToPoseAction* driveToAction = new DriveToPoseAction(_dVars.gazeDirectionPose);
    turnAction->AddAction(driveToAction);
  }

  turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceReaction));

  if (kDriveStraightTurnBackToFace) {
    turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
    CompoundActionParallel* returnTurnAndAnimate = new CompoundActionParallel();
    returnTurnAndAnimate->AddAction(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));
    // Reverse the direction of the turn because we're turning back to the point we started at
    if ( Util::IsFltGT(translation.y(), 0.f) ) {
      // turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfacesTurnRight});
      returnTurnAndAnimate->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesTurnRight));
    } else {
      // turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfaceTurnLeft});
      returnTurnAndAnimate->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceTurnLeft));
    }
    turnAction->AddAction(returnTurnAndAnimate);
  }
  LOG_WARNING("BehaviorReactToGazeDirection.TransitionToDriveToPointOnSurface",
              "about to delegate");
  // TODO not sure if this will work but let's try it
  // CancelDelegates(false);
  DelegateIfInControl(turnAction, [this] {
        ++_dVars.turnCount;
        _dVars.turnInProgress = false;
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToCheckForPointOnSurface(const Pose3d& gazePose)
{
  const auto& translation = gazePose.GetTranslation();
  auto makingEyeContact = GetBEI().GetFaceWorld().IsMakingEyeContact(kMaxTimeSinceTrackedFaceUpdated_ms);

  // Check to see if our gaze points is within constraints to be considered
  // "looking" at vector.
  const bool isWithinXConstraints = ( Util::InRange(translation.x(), kFaceDirectedAtRobotMinXThres_mm,
                                                    kFaceDirectedAtRobotMaxXThres_mm) );
  const bool isWithinYConstarints = ( Util::InRange(translation.y(), kFaceDirectedAtRobotMinYThres_mm,
                                                    kFaceDirectedAtRobotMaxYThres_mm) );
  if ( ( isWithinXConstraints && isWithinYConstarints ) || ( makingEyeContact && kUseEyeContact) ) {

    // If we're making eye contact with the robot or looking at the robot, react to that.
    CompoundActionSequential* turnAction = new CompoundActionSequential();
    turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtVectorReaction));
    turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
    turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));
    turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
    DelegateIfInControl(turnAction, [this] {
        ++_dVars.turnCount;
        _dVars.turnInProgress = false;
    });

  } else {
    Pose3d correctedGazePose(gazePose);
    if (kEyeGazeDirectionUseCorrection) {
      if (kEyeGazeDirectionUseLinearCorrection) {
        CorrectPoseForOvershootLinear(gazePose, correctedGazePose);
      } else if (kEyeGazeDirectionUseGaussianCorrection) {
        CorrectPoseForOvershootGaussian(gazePose, correctedGazePose);
      }
    }

    // Check if the gaze pose is too far to drive to. If kMaxDriveToSurfacePointDistance_mm2
    // is set to -1 then driving to the gaze point is disabled.
    LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckForPointOnSurface",
                "length of translation %.3f, max drive surface point distance %.3f",
                translation.LengthSq(), kMaxDriveToSurfacePointDistance_mm2);
    if (translation.LengthSq() < kMaxDriveToSurfacePointDistance_mm2) {
      TransitionToDriveToPointOnSurface(gazePose);
      // We either turn and drive, or just turn
      return;
    }


    // This is the case where we aren't looking at the robot, so we need to turn
    // right or left towards the pose, play the correct animation, and then turn
    // back to the face. This sequence could happen several times.
    CompoundActionSequential* turnAction = new CompoundActionSequential();
    for (int i = 0; i < kNumberOfTurnsForSurfacePoint; ++i) {

      if ( Util::IsFltLT(translation.y(), 0.f) ) {
        turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesGetInRight));
      } else {
        turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesGetInLeft));
      }

      TurnTowardsPoseAction* turnTowardsPose = new TurnTowardsPoseAction(correctedGazePose);
      turnTowardsPose->SetMaxPanSpeed(kMaxPanSpeed_radPerSec);
      turnTowardsPose->SetPanAccel(kMaxPanAccel_radPerSec2);
      CompoundActionParallel* turnAndAnimate = new CompoundActionParallel();
      turnAndAnimate->AddAction(turnTowardsPose);

      
      Radians gazeAngle = atan2f(translation.y(), translation.x());
      LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckForPointOnSurface",
                  "Turn pose (x=%.3f, y=%.3f, z=%.3f), Turn Angle %.3f",
                  translation.x(), translation.y(), translation.z(),
                  RAD_TO_DEG(gazeAngle.ToFloat()));

      if ( Util::IsFltLT(translation.y(), 0.f) ) {
        // turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfacesTurnRight});
        turnAndAnimate->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesTurnRight));
      } else {
        // turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfaceTurnLeft});
        turnAndAnimate->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceTurnLeft));
      }
      // turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
      turnAction->AddAction(turnAndAnimate);
      turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceReaction));
      turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
      turnAction->AddAction(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));

      turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
    }
    DelegateIfInControl(turnAction, [this] {
        ++_dVars.turnCount;
        _dVars.turnInProgress = false;
    });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::CorrectPoseForOvershootLinear(const Pose3d& gazePose, Pose3d& correctedGazePose) 
{

  const Radians lowerAngleLimit_rad(DEG_TO_RAD(kEyeGazeDirectionLowerAngleLinearizationThreshold_deg));
  const Radians upperAngleLimit_rad(DEG_TO_RAD(kEyeGazeDirectionUpperAngleLinearizationThreshold_deg));


  // Find translation
  const auto& translation = gazePose.GetTranslation();

  // Find angle
  Radians gazeAngle = atan2f(translation.y(), translation.x());
  Radians correctedGazeAngle(0.f);

  // TODO add print that logs original angle
  LOG_WARNING("BehaviorReactToGazeDirection.CorrectPoseForOvershootLinear",
              "Original Turn pose (x=%.3f, y=%.3f, z=%.3f), Turn Angle %.3f degrees %.3f radians",
              translation.x(), translation.y(), translation.z(),
              RAD_TO_DEG(gazeAngle.ToFloat()), gazeAngle.ToFloat());

  // Linearlize angle over new domain
  // Check if a "positive angle" is the range we want
  if ( (gazeAngle > lowerAngleLimit_rad) && (gazeAngle < upperAngleLimit_rad) ) {
    correctedGazeAngle = (gazeAngle - lowerAngleLimit_rad) * (upperAngleLimit_rad / (upperAngleLimit_rad - lowerAngleLimit_rad).ToFloat()).ToFloat();
    if (correctedGazeAngle < lowerAngleLimit_rad) {
      correctedGazeAngle = lowerAngleLimit_rad;
    }
  }

  float gazeAngleRaw = gazeAngle.ToFloat();
  if ( (gazeAngleRaw < DEG_TO_RAD(-kEyeGazeDirectionLowerAngleLinearizationThreshold_deg)) &&
       (gazeAngleRaw > DEG_TO_RAD(-kEyeGazeDirectionUpperAngleLinearizationThreshold_deg))) {
      gazeAngleRaw = (gazeAngleRaw - DEG_TO_RAD(-kEyeGazeDirectionLowerAngleLinearizationThreshold_deg))
            * (DEG_TO_RAD(-kEyeGazeDirectionUpperAngleLinearizationThreshold_deg) / (DEG_TO_RAD(-kEyeGazeDirectionUpperAngleLinearizationThreshold_deg) - DEG_TO_RAD(-kEyeGazeDirectionLowerAngleLinearizationThreshold_deg)));
    if (gazeAngleRaw > DEG_TO_RAD(-kEyeGazeDirectionLowerAngleLinearizationThreshold_deg)) {
      LOG_WARNING("BehaviorReactToGazeDirection.CorrectPoseForOvershootLinear",
                  "Hit lower threshold clamping Turn Angle %.3f degrees to %.3f",
                  RAD_TO_DEG(gazeAngleRaw), -kEyeGazeDirectionLowerAngleLinearizationThreshold_deg);
      gazeAngleRaw = DEG_TO_RAD(-kEyeGazeDirectionLowerAngleLinearizationThreshold_deg);
    }
    correctedGazeAngle = Radians(gazeAngleRaw);
  }

  const auto translationMagnitude = translation.Length();
  const float newX = translationMagnitude * std::cos(correctedGazeAngle.ToFloat());
  const float newY = translationMagnitude * std::sin(correctedGazeAngle.ToFloat());
  Radians newGazeAngle = atan2f(newY, newX);
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  correctedGazePose = Pose3d(0.f, Z_AXIS_3D(), {newX, newY, 0.f}, robotPose);
  const auto& newTranslation = correctedGazePose.GetTranslation();

  // TODO add print that logs new angle
  LOG_WARNING("BehaviorReactToGazeDirection.CorrectPoseForOvershootLinear",
              "New Turn pose (x=%.3f, y=%.3f, z=%.3f), Turn Angle %.3f degrees %.3f radians,"
              "The negative bounds are lower: %.3f upper: %.3f",
              newTranslation.x(), newTranslation.y(), newTranslation.z(),
              RAD_TO_DEG(newGazeAngle.ToFloat()), newGazeAngle.ToFloat(),
              -lowerAngleLimit_rad.ToFloat(), -upperAngleLimit_rad.ToFloat());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::CorrectPoseForOvershootGaussian(const Pose3d& gazePose, Pose3d& correctedGazePose) 
{

  const Radians lowerAngleLimit_rad(DEG_TO_RAD(kEyeGazeDirectionLowerAngleGaussianThreshold_deg));
  const Radians upperAngleLimit_rad(DEG_TO_RAD(kEyeGazeDirectionUpperAngleGaussianThreshold_deg));
  const Radians mean(DEG_TO_RAD(kEyeGazeDirectionGaussianMean_deg));
  const Radians std(DEG_TO_RAD(kEyeGazeDirectionGaussianSTD_deg));

  // Find translation
  const auto& translation = gazePose.GetTranslation();

  // Find angle
  Radians gazeAngle = atan2f(translation.y(), translation.x());
  Radians correctedGazeAngle(0.f);

  // TODO add print that logs original angle
  LOG_WARNING("BehaviorReactToGazeDirection.CorrectPoseForOvershootGaussian",
              "Original Turn pose (x=%.3f, y=%.3f, z=%.3f), Turn Angle %.3f degrees %.3f radians",
              translation.x(), translation.y(), translation.z(),
              RAD_TO_DEG(gazeAngle.ToFloat()), gazeAngle.ToFloat());

  // Linearlize angle over new domain
  // Check if a "positive angle" is the range we want
  if ( (gazeAngle > lowerAngleLimit_rad) && (gazeAngle < upperAngleLimit_rad) ) {
    correctedGazeAngle = Radians(DEG_TO_RAD((gazeAngle.ToFloat() - mean.ToFloat()) / (std).ToFloat()));
    if (correctedGazeAngle < lowerAngleLimit_rad) {
      LOG_WARNING("BehaviorReactToGazeDirection.CorrectPoseForOvershootGaussian",
                  "Hit lower threshold clamping Turn Angle %.3f degrees to %.3f,"
                  "using a mean %.3f and a std %.3f",
                  RAD_TO_DEG(correctedGazeAngle.ToFloat()), kEyeGazeDirectionLowerAngleGaussianThreshold_deg,
                  RAD_TO_DEG(mean.ToFloat()), RAD_TO_DEG(std.ToFloat()));
      correctedGazeAngle = lowerAngleLimit_rad;
    }
    if (correctedGazeAngle > upperAngleLimit_rad) {
      LOG_WARNING("BehaviorReactToGazeDirection.CorrectPoseForOvershootGaussian",
                  "Hit upper threshold clamping Turn Angle %.3f degrees to %.3f,"
                  "using a mean %.3f and a std %.3f",
                  RAD_TO_DEG(correctedGazeAngle.ToFloat()), kEyeGazeDirectionUpperAngleGaussianThreshold_deg,
                  RAD_TO_DEG(mean.ToFloat()), RAD_TO_DEG(std.ToFloat()));
      correctedGazeAngle = upperAngleLimit_rad;
    }
  }

  float gazeAngleRaw = gazeAngle.ToFloat();
  if ( (gazeAngleRaw < DEG_TO_RAD(-kEyeGazeDirectionLowerAngleGaussianThreshold_deg)) &&
       (gazeAngleRaw > DEG_TO_RAD(-kEyeGazeDirectionUpperAngleGaussianThreshold_deg))) {
      // I'm not sure why this is needed ... but sigh for some reason it is
      gazeAngleRaw = DEG_TO_RAD(-((-gazeAngleRaw - mean.ToFloat()) / std.ToFloat()));
    if (gazeAngleRaw > DEG_TO_RAD(-kEyeGazeDirectionLowerAngleGaussianThreshold_deg)) {
      LOG_WARNING("BehaviorReactToGazeDirection.CorrectPoseForOvershootGaussian",
                  "Hit lower threshold clamping Turn Angle %.3f degrees to %.3f,"
                  "using a mean %.3f and a std %.3f",
                  RAD_TO_DEG(gazeAngleRaw), -kEyeGazeDirectionLowerAngleGaussianThreshold_deg,
                  RAD_TO_DEG(mean.ToFloat()), RAD_TO_DEG(std.ToFloat()));
      gazeAngleRaw = DEG_TO_RAD(-kEyeGazeDirectionLowerAngleGaussianThreshold_deg);
    }
    if (gazeAngleRaw < DEG_TO_RAD(-kEyeGazeDirectionUpperAngleGaussianThreshold_deg)) {
      LOG_WARNING("BehaviorReactToGazeDirection.CorrectPoseForOvershootGaussian",
                  "Hit upper threshold clamping Turn Angle %.3f degrees to %.3f,"
                  "using a mean %.3f and a std %.3f",
                  RAD_TO_DEG(gazeAngleRaw), -kEyeGazeDirectionUpperAngleGaussianThreshold_deg,
                  RAD_TO_DEG(mean.ToFloat()), RAD_TO_DEG(std.ToFloat()));
      gazeAngleRaw = DEG_TO_RAD(-kEyeGazeDirectionUpperAngleGaussianThreshold_deg);
    }
    correctedGazeAngle = Radians(gazeAngleRaw);
  }

  const auto translationMagnitude = translation.Length();
  const float newX = translationMagnitude * std::cos(correctedGazeAngle.ToFloat());
  const float newY = translationMagnitude * std::sin(correctedGazeAngle.ToFloat());
  Radians newGazeAngle = atan2f(newY, newX);
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  correctedGazePose = Pose3d(0.f, Z_AXIS_3D(), {newX, newY, 0.f}, robotPose);
  const auto& newTranslation = correctedGazePose.GetTranslation();

  // TODO add print that logs new angle
  LOG_WARNING("BehaviorReactToGazeDirection.CorrectPoseForOvershootGaussian",
              "New Turn pose (x=%.3f, y=%.3f, z=%.3f), Turn Angle %.3f degrees %.3f radians,"
              "The negative bounds are lower: %.3f upper: %.3f",
              newTranslation.x(), newTranslation.y(), newTranslation.z(),
              RAD_TO_DEG(newGazeAngle.ToFloat()), newGazeAngle.ToFloat(),
              -lowerAngleLimit_rad.ToFloat(), -upperAngleLimit_rad.ToFloat());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Radians BehaviorReactToGazeDirection::ComputeTurnAngleFromGazePose(const Pose3d& gazePose)
{
  // Compute angle to turn and look for faces. There are three buckets each with a
  // corresponding turn: left, right, turn-around. Put the pose into one on these buckets
  // and return the turn angle associated with that bucket.
  Radians turnAngle;
  const auto& translation = gazePose.GetTranslation();
  Radians gazeAngle = atan2f(translation.y(), translation.x());
  const Radians angleDifference = Radians(DEG_TO_RAD(180)) - gazeAngle;
  if ( (angleDifference <= Radians(DEG_TO_RAD(kConeFor180TurnForFaceSearch_deg/2.f))) &&
       (angleDifference >= -Radians(DEG_TO_RAD(kConeFor180TurnForFaceSearch_deg/2.f))) ) {
    // Turn around
    if (angleDifference < 0) {
      // Turn around by turning right
      turnAngle = DEG_TO_RAD(-kSearchForFaceTurnAroundAngle_deg);
    } else {
      // turn around by turning left
      turnAngle = DEG_TO_RAD(kSearchForFaceTurnAroundAngle_deg);
    }
  } else if (Util::IsFltLTZero(translation.y())) {
    // Turn right
    turnAngle = DEG_TO_RAD(kSearchForFaceTurnSideAngle_deg);
  } else {
    // Turn left
    turnAngle = DEG_TO_RAD(-kSearchForFaceTurnSideAngle_deg);
  }

  return turnAngle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToCheckGazeDirection()
{

  // TODO not sure if I need this outside of the "looping" portion of
  // this but ... who knows. Anyway, don't turn if there is already a
  // turn in progress, or log anything it's too spammy right now.
  if (_dVars.turnInProgress && false) {
    /*
    LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckGazeDirection",
                "early exit because turn in progress");
    */
    return;
  }

  // Put looping eye state into behavior update, and then cancel if conditions are satisfied
  // This could possibly "cut" the animation sort if this is an issue, just have each loop delegate
  // to a call back which determines flow

  if(GetBEI().GetFaceWorld().GetGazeDirectionPose(kMaxTimeSinceTrackedFaceUpdated_ms,
                                                  _dVars.gazeDirectionPose, _dVars.faceIDToTurnBackTo)) {
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    Pose3d gazeDirectionPoseWRTRobot;
    if (_dVars.gazeDirectionPose.GetWithRespectTo(robotPose, gazeDirectionPoseWRTRobot)) {

      SendDASEventForPoseToFollow(gazeDirectionPoseWRTRobot);
      _dVars.turnInProgress = true;
      LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckGazeDirection",
                  "Turn Count %d Number of Allowable Turns %d.",
                  _dVars.turnCount, kEyeGazeDirectionMaxNumberOfTurns);

      // Now that we know we are going to turn clear the history
      GetBEI().GetFaceWorldMutable().ClearGazeDirectionHistory(_dVars.faceIDToTurnBackTo);

      if (_dVars.turnCount < kEyeGazeDirectionMaxNumberOfTurns || kEyeGazeDirectionMaxNumberOfTurns == -1) {

        /*
        // Check how we want to determine whether to search for faces
        bool searchForPointsOnSurface = _iConfig.searchForPointsOnSurface;

        // This serious of if statements is looking to see if we should override
        // searchForPointsOnSurface so if any of this calls return false ... we
        // don't want to do anything thus no else statement
        if (kUseEyeGazeToLookAtSurfaceorFaces) {
          Pose3d eyeDirectionPose;
          if(GetBEI().GetFaceWorld().GetEyeDirectionPose(_dVars.faceIDToTurnBackTo, kMaxTimeSinceTrackedFaceUpdated_ms, eyeDirectionPose)) {
            Pose3d eyeDirectionPoseWRTRobot;
            if (eyeDirectionPose.GetWithRespectTo(robotPose, eyeDirectionPoseWRTRobot)) {
              const auto& translation = eyeDirectionPoseWRTRobot.GetTranslation();

              // If the eye direction pose on the ground plane is too far out then
              // update the searchForPointsOnSurface to be false, otherwise ... update
              // it to be true
              if (std::abs(translation.x()) > kEyeGazeDirectionXSurfaceThreshold_mm &&
                  std::abs(translation.y()) > kEyeGazeDirectionYSurfaceThreshold_mm &&
                  std::abs(translation.z()) > kEyeGazeDirectionZSurfaceThreshold_mm) {
                  searchForPointsOnSurface = false;
              } else {
                  searchForPointsOnSurface = true;
              }
            }
          }
        }
        */

        if (true) {
          // We're looking for points on the surface, similar to looking for faces
          // but the head angle will be pointed towards the ground plane.
          TransitionToCheckForPointOnSurface(gazeDirectionPoseWRTRobot);
        } else {
          // Bucket the angle to turn to look for faces
          const Radians turnAngle = ComputeTurnAngleFromGazePose(gazeDirectionPoseWRTRobot);

          // If we're configured to look for existing faces in face world to turn towards
          // check if there are any existing faces visible from the angle we want to turn to.
          // faces visible from that angle, or we're not configured to do use faces existing in
          // face world then turn to the specificied angle and search for a face.
          SmartFaceID faceToTurnTowards;
          if (GetBEI().GetFaceWorld().FaceInTurnAngle(Radians(turnAngle), _dVars.faceIDToTurnBackTo, robotPose, faceToTurnTowards)
              && kUseExistingFacesWhenSearchingForFaces) {
            // If a face is visible turn towards it instead of the angle.
            TransitionToLookAtFace(faceToTurnTowards, turnAngle);
          } else {
            // If there aren't any faces visible from that angle, or we're not configured to
            // use faces existing in face world then turn to the specificied angle and search
            // for a face.
            TransitionToCheckForFace(turnAngle);
          }
        }
      } else {
        // We have reached the maximum number of turns ... terminate
        // not sure the best way to do this so ... and shouldn't really live in
        // master
        _dVars.turnCount = 0;
        _dVars.turnInProgress = false;
        CancelSelf();
      }
    } else {
      LOG_ERROR("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.GetWithRespectToFailed", "");
      // TODO shouldn't need this but whatever, this should hurt ... i think
      // _dVars.turnInProgress = false;
    }
  } else {
  LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckGazeDirection",
              "Didn't find stable gaze direction");
  }
}

void BehaviorReactToGazeDirection::FoundNewFace(ActionResult result)
{
  switch (result)
  {
    case ActionResult::NO_FACE: {
      DASMSG(behavior_react_to_gaze_direction_found_no_new_face,
             "behavior.react_to_gaze_direction.found_no_new_face",
             "No new face found by following the gaze direction of another face.");
      DASMSG_SEND();
      // No face was found so turn back to the face we started with
      DelegateIfInControl(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));
      break;
    }
    case ActionResult::SUCCESS: {
      DASMSG(behavior_react_to_gaze_direction_found_new_face,
             "behavior.react_to_gaze_direction.found_new_face",
             "Found a new face by following the gaze direction of another face.");
      DASMSG_SEND();
      break;
    }
    default: {
      LOG_ERROR("BehaviorReactToGazeDirection.FoundNewFace.UnexpectedResultFromTurnTowardsFace",
                "Result: %d", result);
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.behaviorAlwaysDelegates = false;

  // This will result in running facial part detection whenever is behavior is an activatable scope
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingGaze, EVisionUpdateFrequency::High });
  modifiers.visionModesForActivatableScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
  modifiers.visionModesForActivatableScope->insert({ VisionMode::DetectingGaze, EVisionUpdateFrequency::High });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::SendDASEventForPoseToFollow(const Pose3d& gazePose) const
{
  const auto& translation = gazePose.GetTranslation();
  std::string type;
  if (_iConfig.searchForPointsOnSurface) {
    type = "Surface";
  } else {
    type = "Face";
  }
  DASMSG(behavior_react_to_gaze_direction_pose_to_follow,
         "behavior.react_to_gaze_direction.pose_to_follow",
         "Gaze point to turn towards.");
  DASMSG_SET(s1, type.c_str(), "The type of point we are turning towards");
  DASMSG_SET(i1, static_cast<int>(translation.x()), "X coordinate in mm of point to turn towards");
  DASMSG_SET(i2, static_cast<int>(translation.y()), "Y coordinate in mm of point to turn towards");
  DASMSG_SET(i3, static_cast<int>(translation.z()), "Z coordinate in mm of point to turn towards");
  DASMSG_SEND();
}

u32 BehaviorReactToGazeDirection::GetMaxTimeSinceTrackedFaceUpdated_ms() const
{
  return kMaxTimeSinceTrackedFaceUpdated_ms;
}

} // namespace Vector
} // namespace Anki
