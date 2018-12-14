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
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinYThres_mm,        "Vision.GazeDirection", -100.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxYThres_mm,        "Vision.GazeDirection",  100.f);
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
  CONSOLE_VAR(f32,  kMaxDriveToSurfacePointDistance_mm2,     "Vision.GazeDirection",  1000000.f);
  CONSOLE_VAR(bool, kUseRelativePoseForDriveTo,              "Vision.GazeDirection", false);
  CONSOLE_VAR(f32,  kAngleToleranceForDrive_rad,             "Vision.GazeDirection", M_PI_F);
  CONSOLE_VAR(f32,  kDistanceToleranceForDrive_mm,           "Vision.GazeDirection", DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM);
  CONSOLE_VAR(bool, kUseDriveStraightActionForDriveTo,       "Vision.GazeDirection", false);
  CONSOLE_VAR(f32,  kDriveStraightDistance_mm,               "Vision.GazeDirection", 200);
  CONSOLE_VAR(bool, kDriveStraightTurnBackToFace,            "Vision.GazeDirection", true);
  CONSOLE_VAR(bool, kUseHandDetectionHack,                   "Vision.GazeDirection", true);
  CONSOLE_VAR(f32,  kProxSensorStopingDistance_mm,           "Vision.GazeDirection", 30);
  CONSOLE_VAR(bool, kUseTranslationDistanceForDriveStraight, "Vision.GazeDirection", true);
}

namespace {

  // Configuration file keys
  const char* const kSearchForFaces = "searchForPointsOnSurface";
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
  TransitionToCheckGazeDirection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  const std::string& debugName = "BehaviorReactToBody.InstanceConfig.LoadConfig";
  searchForPointsOnSurface = JsonTools::ParseBool(config, kSearchForFaces, debugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::DynamicVariables::DynamicVariables()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
      kSearchForFaces,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToGazeDirection::WantsToBeActivatedBehavior() const
{
  // return GetBEI().GetFaceWorld().AnyStableGazeDirection(kMaxTimeSinceTrackedFaceUpdated_ms);
  return true;
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
    turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtFacesTurnLeft});
  } else {
    turnAndAnimate->AddAction(new TurnInPlaceAction(turnAngle.ToFloat(), false));
    turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtFacesTurnRight});
  }

  turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
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
    turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfacesTurnRight});
  } else {
    turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfaceTurnLeft});
  }
  turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
  turnAction->AddAction(turnAndAnimate);
  turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceReaction));

  // Now that we're looking at the point actually drive to it
  // TODO need something better than this. The duplciate code is awful.
  if (kUseDriveStraightActionForDriveTo) {
    LOG_WARNING("BehaviorReactToGazeDirection.TransitionToDriveToPointOnSurface.UsingOnlyDriveStraight", "");
    DriveStraightAction* driveStraightAction = new DriveStraightAction(kDriveStraightDistance_mm);
    turnAction->AddAction(driveStraightAction);
    if (kDriveStraightTurnBackToFace) {
      turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
      turnAction->AddAction(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));
    }
  } else if (kUseHandDetectionHack) {
    float distanceToDrive_mm = kDriveStraightDistance_mm;
    if (kUseTranslationDistanceForDriveStraight) {
      distanceToDrive_mm = translation.Length();
    }
    LOG_WARNING("BehaviorReactToGazeDirection.TransitionToDriveToPointOnSurface.UsingHandDetectionHack", "");
    CompoundActionParallel* driveAction = new CompoundActionParallel({
      new DriveStraightAction(distanceToDrive_mm),
      new ReselectingLoopAnimationAction{AnimationTrigger::ExploringReactToHandDrive},
      new WaitForLambdaAction([this](Robot& robot) {
        const bool detectedUnexpectedMovement = (robot.GetMoveComponent().IsUnexpectedMovementDetected());
        if(detectedUnexpectedMovement) {
          LOG_WARNING("BehaviorReactToGazeDirection.TransitionToDriveToPointOnSurface.GotUnexpectedMovement", "");
        }
        bool proxSensorFire = false;
        auto& proxSensor = GetBEI().GetComponentWrapper( BEIComponentID::ProxSensor ).GetComponent<ProxSensorComponent>();
        uint16_t proxDist_mm = 0;
        if ( proxSensor.GetLatestDistance_mm( proxDist_mm) ) {
          if ( proxDist_mm < kProxSensorStopingDistance_mm) {
            LOG_WARNING("BehaviorReactToGazeDirection.TransitionToDriveToPointOnSurface.GotProxSensorReadingThatWereClose", "");
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
    if (kDriveStraightTurnBackToFace) {
      turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
      turnAction->AddAction(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));
    }

  }else {
    LOG_WARNING("BehaviorReactToGazeDirection.TransitionToDriveToPointOnSurface.UsingDriveToPoseAction", "");
    if (kUseRelativePoseForDriveTo) {
      DriveToPoseAction* driveToAction = new DriveToPoseAction(gazePose);
      turnAction->AddAction(driveToAction);
    } else {
      DriveToPoseAction* driveToAction = new DriveToPoseAction(_dVars.gazeDirectionPose);
      turnAction->AddAction(driveToAction);
    }
  }
  DelegateIfInControl(turnAction);
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
    DelegateIfInControl(turnAction);

  } else {
    if (translation.LengthSq() < kMaxDriveToSurfacePointDistance_mm2) {
      LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckForPointOnSurface.Drive",
                  "Translation distance squared %.3f, kMaxDriveToSurfacePointDistance_mm2 %.3f",
                  translation.LengthSq(), kMaxDriveToSurfacePointDistance_mm2);
      TransitionToDriveToPointOnSurface(gazePose);
    } else {
      LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckForPointOnSurface.NoDrive",
                  "Translation distance squared %.3f, kMaxDriveToSurfacePointDistance_mm2 %.3f",
                  translation.LengthSq(), kMaxDriveToSurfacePointDistance_mm2);
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

      TurnTowardsPoseAction* turnTowardsPose = new TurnTowardsPoseAction(gazePose);
      turnTowardsPose->SetMaxPanSpeed(kMaxPanSpeed_radPerSec);
      turnTowardsPose->SetPanAccel(kMaxPanAccel_radPerSec2);
      CompoundActionParallel* turnAndAnimate = new CompoundActionParallel();
      turnAndAnimate->AddAction(turnTowardsPose);
      if ( Util::IsFltLT(translation.y(), 0.f) ) {
        turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfacesTurnRight});
      } else {
        turnAndAnimate->AddAction(new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfaceTurnLeft});
      }
      turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
      turnAction->AddAction(turnAndAnimate);
      turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceReaction));
      turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
      turnAction->AddAction(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));

      turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
    }
    DelegateIfInControl(turnAction);
  }
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
  if(GetBEI().GetFaceWorld().GetGazeDirectionPose(kMaxTimeSinceTrackedFaceUpdated_ms,
                                                  _dVars.gazeDirectionPose, _dVars.faceIDToTurnBackTo)) {
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    Pose3d gazeDirectionPoseWRTRobot;
    if (_dVars.gazeDirectionPose.GetWithRespectTo(robotPose, gazeDirectionPoseWRTRobot)) {

      SendDASEventForPoseToFollow(gazeDirectionPoseWRTRobot);

      // Now that we know we are going to turn clear the history
      GetBEI().GetFaceWorldMutable().ClearGazeDirectionHistory(_dVars.faceIDToTurnBackTo);

      // Check how we want to determine whether to search for faces
      bool searchForPointsOnSurface = _iConfig.searchForPointsOnSurface;
      if (kUseEyeGazeToLookAtSurfaceorFaces) {
        // TODO need to have better naming
        searchForPointsOnSurface = GetBEI().GetFaceWorld().GetFaceEyesDirectedAtSurface(_dVars.faceIDToTurnBackTo);
      }

      if (searchForPointsOnSurface) {

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
      } else {
        // We're looking for points on the surface, similar to looking for faces
        // but the head angle will be pointed towards the ground plane.
        TransitionToCheckForPointOnSurface(gazeDirectionPoseWRTRobot);
      }
    } else {
      LOG_ERROR("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.GetWithRespectToFailed", "");
    }
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
