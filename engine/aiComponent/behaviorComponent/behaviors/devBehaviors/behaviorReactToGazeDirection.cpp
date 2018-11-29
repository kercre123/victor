/**
 * File: behaviorReactToGazeDirection.cpp
 *
 * Author: Robert Cosgriff
 * Created: 2018-09-04
 *
 * Description: see header
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
#include <unistd.h>

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorReactToGazeDirection.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorSearchWithinBoundingBox.h"
#include "engine/aiComponent/beiConditions/conditions/conditionSalientPointDetected.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {

namespace {
  // This used to be -200 just a note
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinXThres_mm,        "Vision.GazeDirection", -25.f);
  // This used to 50 just a note
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxXThres_mm,        "Vision.GazeDirection",  20.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinYThres_mm,        "Vision.GazeDirection", -100.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxYThres_mm,        "Vision.GazeDirection",  100.f);
  CONSOLE_VAR(bool, kTurnBackToFace,                         "Vision.GazeDirection",  false);
  CONSOLE_VAR(f32,  kTurnWaitAfterFinalTurn_s,               "Vision.GazeDirection",  1.f);
  CONSOLE_VAR(f32,  kTurnWaitAfterInitialTurn_s,             "Vision.GazeDirection",  1.f);
  CONSOLE_VAR(f32,  kTurnWaitAfterInitialLookBackAtFace_s,   "Vision.GazeDirection",  2.f);
  CONSOLE_VAR(f32,  kSleepTimeAfterActionCompleted_s,        "Vision.GazeDirection",  2.f);
  CONSOLE_VAR(f32,  kMaxPanSpeed_radPerSec,                  "Vision.GazeDirection",  MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);
  CONSOLE_VAR(f32,  kMaxPanAccel_radPerSec2,                 "Vision.GazeDirection",  10.f);
  CONSOLE_VAR(bool, kUseEyeContact,                          "Vision.GazeDirection",  true);
  CONSOLE_VAR(f32,  kDistanceForAboveHorizonSearch_mm2,      "Vision.GazeDirection",  0.f);
  CONSOLE_VAR(f32,  kConeFor180TurnForFaceSearch_deg,        "Vision.GazeDirection",  40.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnRightAngle_deg,        "Vision.GazeDirection",  -90.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnLeftAngle_deg,         "Vision.GazeDirection",  90.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnAroundAngle_deg,       "Vision.GazeDirection",  180.f);
  CONSOLE_VAR(bool, kSearchForFaceUseThreeTurns,             "Vision.GazeDirection",  false);
  CONSOLE_VAR(f32,  kConeFor135TurnForFaceSearch_deg,        "Vision.GazeDirection",  60.f);
  CONSOLE_VAR(f32,  kSearchForFaceThirdAngle_deg,            "Vision.GazeDirection",  135.f);
  CONSOLE_VAR(s32,  kSearchForFaceNumberOfImagesToWait,      "Vision.GazeDirection",  5);
  CONSOLE_VAR(bool, kFindSurfacePointsUsingFaceDirection,    "Vision.GazeDirection",  false);
  CONSOLE_VAR(bool, kFindFacesUsingFaceDirection,            "Vision.GazeDirection",  true);
  CONSOLE_VAR(bool, kUseExistingFacesWhenSearchingForFaces,  "Vision.GazeDirection",  false);
  CONSOLE_VAR(s32,  kNumberOfTurnsForSurfacePoint,           "Vision.GazeDirection",  1);
}

namespace {
  const char* const kSearchForFaces = "searchForFaces";
}

#define LOG_CHANNEL "Behaviors"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::BehaviorReactToGazeDirection(const Json::Value& config)
 : ICozmoBehavior(config)
, _iConfig(new InstanceConfig(config))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::InitBehavior()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  const std::string& debugName = "BehaviorReactToBody.InstanceConfig.LoadConfig";
  searchForFaces = JsonTools::ParseBool(config, kSearchForFaces, debugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::DynamicVariables::DynamicVariables()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToGazeDirection::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::BehaviorUpdate()
{
  if ( ! IsActivated() ) {
    return;
  }
  TransitionToCheckFaceDirection();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToGazeDirection::CheckIfShouldStop()
{
  // It might take a couple of frames to get a stability reading so set a timer here if we
  // need to or something so we don't just wait forever
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToCheckForFace(const Radians& turnAngle)
{
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
Radians BehaviorReactToGazeDirection::ComputeTurnAngleFromGazePose(const Pose3d& gazePose)
{
  // Compute angle
  // If angle is within the turn around cone then turn around and look for face
  Radians turnAngle;
  auto translation = gazePose.GetTranslation();
  Radians gazeAngle = atan2f(translation.y(), translation.x());
  auto angleDifference = Radians(DEG_TO_RAD(180)) - gazeAngle;
  if ( (angleDifference <= Radians(DEG_TO_RAD(kConeFor180TurnForFaceSearch_deg/2.f))) &&
       (angleDifference >= -Radians(DEG_TO_RAD(kConeFor180TurnForFaceSearch_deg/2.f))) ) {
    if (angleDifference < 0) {
      turnAngle = DEG_TO_RAD(-kSearchForFaceTurnAroundAngle_deg);
    } else {
      turnAngle = DEG_TO_RAD(kSearchForFaceTurnAroundAngle_deg);
    }
  } else if ( (angleDifference <= Radians(DEG_TO_RAD(kConeFor135TurnForFaceSearch_deg/2.f))) &&
              (angleDifference >= -Radians(DEG_TO_RAD(kConeFor135TurnForFaceSearch_deg/2.f))) &&
              kSearchForFaceUseThreeTurns) {
    if (angleDifference < 0) {
      turnAngle = DEG_TO_RAD(-kSearchForFaceThirdAngle_deg);
    } else {
      turnAngle = DEG_TO_RAD(kSearchForFaceThirdAngle_deg);
    }
  } else if (translation.y() < 0) {
    turnAngle = DEG_TO_RAD(kSearchForFaceTurnRightAngle_deg);
  } else {
    turnAngle = DEG_TO_RAD(kSearchForFaceTurnLeftAngle_deg);
  }

  return turnAngle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToCheckFaceDirection()
{
  Pose3d faceFocusPose;
  if(GetBEI().GetFaceWorld().GetGazeDirectionPose(500, faceFocusPose, _dVars.faceIDToTurnBackTo)) {
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    Pose3d gazeDirectionPoseWRTRobot;

    if (faceFocusPose.GetWithRespectTo(robotPose, gazeDirectionPoseWRTRobot)) {

      if (_iConfig->searchForFaces) {
        const Radians turnAngle = ComputeTurnAngleFromGazePose(gazeDirectionPoseWRTRobot);

        // Now that we know we are going to turn clear the history
        GetBEI().GetFaceWorldMutable().ClearGazeDirectionHistory(_dVars.faceIDToTurnBackTo);

        // If angle is within the turn around cone then turn around and look for face
        SmartFaceID faceToTurnTowards;
        if (GetBEI().GetFaceWorld().FaceInTurnAngle(Radians(turnAngle), _dVars.faceIDToTurnBackTo, robotPose, faceToTurnTowards)
            && kUseExistingFacesWhenSearchingForFaces) {
          TransitionToLookAtFace(faceToTurnTowards, turnAngle);
        } else {
          TransitionToCheckForFace(turnAngle);
        }
      } else {
        // Search on the sruface for stuff and things
      }
    } else {
      LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.GetWithRespectToFailed", "");
    }
    LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.FaceNotFocused", "");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToCompleted()
{
}

void BehaviorReactToGazeDirection::FoundNewFace(ActionResult result)
{
  if (ActionResult::NO_FACE == result) {
    DelegateIfInControl(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo), &BehaviorReactToGazeDirection::TransitionToCompleted);
  } else if (ActionResult::SUCCESS == result) {
    BehaviorReactToGazeDirection::TransitionToCompleted();
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

}
}
