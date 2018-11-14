/**
 * File: behaviorReactToFaceNormal.cpp
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

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorReactToFaceNormal.h"

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
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinXThres,           "Vision.FaceNormalDirectedAtRobot3d", -200.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxXThres,           "Vision.FaceNormalDirectedAtRobot3d",  50.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinYThres,           "Vision.FaceNormalDirectedAtRobot3d", -100.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxYThres,           "Vision.FaceNormalDirectedAtRobot3d",  100.f);
  CONSOLE_VAR(bool, kTurnBackToFace,                         "Vision.FaceNormalDirectedAtRobot3d",  false);
  CONSOLE_VAR(f32,  kTurnWaitAfterFinalTurn_s,               "Vision.FaceNormalDirectedAtRobot3d",  1.f);
  CONSOLE_VAR(f32,  kTurnWaitAfterInitialTurn_s,             "Vision.FaceNormalDirectedAtRobot3d",  1.f);
  CONSOLE_VAR(f32,  kTurnWaitAfterInitialLookBackAtFace_s,   "Vision.FaceNormalDirectedAtRobot3d",  2.f);
  CONSOLE_VAR(f32,  kSleepTimeAfterActionCompleted_s,        "Vision.FaceNormalDirectedAtRobot3d",  2.f);
  CONSOLE_VAR(f32,  kMaxPanSpeed_radPerSec,                  "Vision.FaceNormalDirectedAtRobot3d",  MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);
  CONSOLE_VAR(f32,  kMaxPanAccel_radPerSec2,                 "Vision.FaceNormalDirectedAtRobot3d",  10.f);
  CONSOLE_VAR(bool, kUseEyeContact,                          "Vision.FaceNormalDirectedAtRobot3d",  true);
  CONSOLE_VAR(f32,  kDistanceForAboveHorizonSearch_mm2,      "Vision.FaceNormalDirectedAtRobot3d",  1000000.f);
  CONSOLE_VAR(f32,  kConeFor180TurnForFaceSearch_deg,        "Vision.FaceNormalDirectedAtRobot3d",  30.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnRightAngle_deg,        "Vision.FaceNormalDirectedAtRobot3d",  -90.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnLeftAngle_deg,         "Vision.FaceNormalDirectedAtRobot3d",  90.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnAroundAngle_deg,       "Vision.FaceNormalDirectedAtRobot3d",  180.f);
  CONSOLE_VAR(bool, kSearchForFaceUseThreeTurns,             "Vision.FaceNormalDirectedAtRobot3d",  true);
  CONSOLE_VAR(f32,  kConeFor135TurnForFaceSearch_deg,        "Vision.FaceNormalDirectedAtRobot3d",  60.f);
  CONSOLE_VAR(f32,  kSearchForFaceThirdAngle_deg,            "Vision.FaceNormalDirectedAtRobot3d",  135.f);
  CONSOLE_VAR(s32,  kSearchForFaceNumberOfImagesToWait,      "Vision.FaceNormalDirectedAtRobot3d",  5);
  CONSOLE_VAR(bool, kFindSurfacePointsUsingFaceDirection,    "Vision.FaceNormalDirectedAtRobot3d",  false);
}

#define LOG_CHANNEL "Behaviors"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToFaceNormal::BehaviorReactToFaceNormal(const Json::Value& config)
 : ICozmoBehavior(config)
, _iConfig(new InstanceConfig)
, _dVars(new DynamicVariables)
{
  _dVars->lastReactionTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  _iConfig->coolDown_sec = 3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::InitBehavior()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToFaceNormal::WantsToBeActivatedBehavior() const
{
  // const bool faceFound = GetBEI().GetFaceWorld().HasAnyFaces(500);
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::OnBehaviorActivated()
{
  //TransitionToCheckFaceNormalDirectedAtRobot();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::BehaviorUpdate()
{
  //if( !IsActivated() || !IsControlDelegated()) {
  if ( ! IsActivated() ) {
    return;
  }
  /*
  const f32 currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto timeDiff = currentTime_sec - _dVars->lastReactionTime_ms;
  */
  // if (timeDiff > _iConfig->coolDown_sec) {
  if (true) {
    // LOG_INFO("BehaviorReactToFaceNormal.BehaviorUpdate.CooledEnoughTimeForAction", "");
    TransitionToCheckFaceDirection();
    // _dVars->lastReactionTime_ms = currentTime_sec;
  } else {
    LOG_INFO("BehaviorReactToFaceNormal.BehaviorUpdate.StillCooling", "");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToFaceNormal::CheckIfShouldStop()
{
  // It might take a couple of frames to get a stability reading so set a timer here if we
  // need to or something so we don't just wait forever
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::TransitionToCheckFaceDirection()
{
  Pose3d faceFocusPose;
  // SmartFaceID faceID;
  if(GetBEI().GetFaceWorld().GetFaceFocusPose(500, faceFocusPose, _faceIDToTurnBackTo)) {
    auto robotPose = GetBEI().GetRobotInfo().GetPose();
    Pose3d faceFocusPoseWRTRobot;

    if (faceFocusPose.GetWithRespectTo(robotPose, faceFocusPoseWRTRobot)) {
      auto translation = faceFocusPoseWRTRobot.GetTranslation();
      LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.TranslationWRTRobot",
               "x: %.3f, y:%.3f, z:%.3f", translation.x(), translation.y(), translation.z());
      auto makingEyeContact = GetBEI().GetFaceWorld().IsMakingEyeContact(500);

      if (translation.LengthSq() > kDistanceForAboveHorizonSearch_mm2) {
        LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.GazeFarEnoughToLookUp", "");
        LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.DistanceFromRobot",
                 "distance: %.3f", translation.LengthSq());

        // Compute angle
        // If angle is within the turn around cone then turn around and look for face
        Radians turnAngle;
        Radians gazeAngle = atan2f(translation.y(), translation.x());
        LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.GazeAngle",
                 "angle: %.3f", RAD_TO_DEG(gazeAngle.ToFloat()));
        auto angleDifference = Radians(DEG_TO_RAD(180)) - gazeAngle;
        LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.AngleDifference",
                 "angle: %.3f", RAD_TO_DEG(angleDifference.ToFloat()));
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
        LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.TurnAngle",
                 "angle: %.3f", RAD_TO_DEG(turnAngle.ToFloat()));

        CompoundActionSequential* turnAction = new CompoundActionSequential();
        turnAction->AddAction(new TurnInPlaceAction(turnAngle.ToFloat(), false));
        turnAction->AddAction(new WaitForImagesAction(kSearchForFaceNumberOfImagesToWait, VisionMode::DetectingFaces));
        // TODO is this how we set it to find a new face? seems like the face it would try to find
        // is the last face it saw ... how is that not the face it was looking at last ... maybe
        // we need to add a wait
        // There is a wait in TurnTowardsFaceAction and i can change the max frames to wait
        TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(SmartFaceID(), M_PI_2);
        turnTowardsFace->SetRequireFaceConfirmation(true);
        turnAction->AddAction(turnTowardsFace);
        DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::FoundNewFace);

      } else {

        if (!kFindSurfacePointsUsingFaceDirection) {
          return;
        }

        if ( ( ( FLT_GT(translation.x(), kFaceDirectedAtRobotMinXThres) && FLT_LT(translation.x(), kFaceDirectedAtRobotMaxXThres) ) &&
             (FLT_GT(translation.y(), kFaceDirectedAtRobotMinYThres) && FLT_LT(translation.y(), kFaceDirectedAtRobotMaxYThres)) )
              || ( makingEyeContact && kUseEyeContact) ) {

          CompoundActionSequential* turnAction = new CompoundActionSequential();
          turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtVectorGetIn));
          turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtVectorReaction));
          turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
          turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));

          turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
          DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);

        } else {

          if ( FLT_LT(translation.y(), 0.f) ) {
            LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.LookingLeftTest", "");
            faceFocusPoseWRTRobot.Print("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection", "FaceFocusPoseWRTRobot");
            CompoundActionSequential* turnAction = new CompoundActionSequential();
            turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesGetInRight));

            // Do an initial turn if we want to
            if (kTurnBackToFace) {
              turnAction->AddAction(new TurnTowardsPoseAction(faceFocusPoseWRTRobot));
              turnAction->AddAction(new WaitAction(kTurnWaitAfterInitialTurn_s));
              turnAction->AddAction(new TurnTowardsFaceAction(_faceIDToTurnBackTo));
              turnAction->AddAction(new WaitAction(kTurnWaitAfterInitialLookBackAtFace_s));
            }

            // TODO do we want this to be the same point as before
            TurnTowardsPoseAction* turnTowardsPose = new TurnTowardsPoseAction(faceFocusPoseWRTRobot);
            turnTowardsPose->SetMaxPanSpeed(kMaxPanSpeed_radPerSec);
            turnTowardsPose->SetPanAccel(kMaxPanAccel_radPerSec2);
            CompoundActionParallel* turnAndAnimate = new CompoundActionParallel({
              turnTowardsPose,
              new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfacesTurnRight}
            });
            turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
            turnAction->AddAction(turnAndAnimate);
            turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceReaction));
            turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
            turnAction->AddAction(new TurnTowardsFaceAction(_faceIDToTurnBackTo));

            turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
            DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);
          } else {

            LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.LookingRightTest", "");
            faceFocusPoseWRTRobot.Print("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection", "FaceFocusPoseWRTRobot");
            CompoundActionSequential* turnAction = new CompoundActionSequential();
            turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesGetInLeft));

            // Do an initial turn if we want to
            if (kTurnBackToFace) {
              turnAction->AddAction(new TurnTowardsPoseAction(faceFocusPoseWRTRobot));
              turnAction->AddAction(new WaitAction(kTurnWaitAfterInitialTurn_s));
              turnAction->AddAction(new TurnTowardsFaceAction(_faceIDToTurnBackTo));
              turnAction->AddAction(new WaitAction(kTurnWaitAfterInitialLookBackAtFace_s));
            }

            // TODO do we want this to be the same point as before
            TurnTowardsPoseAction* turnTowardsPose = new TurnTowardsPoseAction(faceFocusPoseWRTRobot);
            turnTowardsPose->SetMaxPanSpeed(kMaxPanSpeed_radPerSec);
            turnTowardsPose->SetPanAccel(kMaxPanAccel_radPerSec2);
            CompoundActionParallel* turnAndAnimate = new CompoundActionParallel({
              turnTowardsPose,
              new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfaceTurnLeft}
            });
            turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
            turnAction->AddAction(turnAndAnimate);

            turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceReaction));
            turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
            turnAction->AddAction(new TurnTowardsFaceAction(_faceIDToTurnBackTo));

            turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
            DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);
          }
        }
      }
    } else {
      LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.GetWithRespectToFailed", "");
    }
  } else {
    // LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.NoFaceFocus", "");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::TransitionToCheckFaceDirectionOld()
{
  auto faceDirection = GetBEI().GetFaceWorld().GetFaceDirection(500);

  if (faceDirection == Vision::TrackedFace::FaceDirection::AtRobot) {
    // play place holder animation
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedAtRobot.FaceDirectedAtRobot!!!!!!!", "");
    CompoundActionSequential* action = new CompoundActionSequential();
    action->AddAction(new TriggerAnimationAction(AnimationTrigger::ExploringHuhFar));
    action->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));
    DelegateIfInControl(action, &BehaviorReactToFaceNormal::TransitionToCompleted);
    // TODO this is really heavy handed replace with a subtler animation
  } else if (faceDirection == Vision::TrackedFace::FaceDirection::LeftOfRobot) {
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedLeftOrRight.TurningLLLLLLLeft", "");
    CompoundActionSequential* turnAction = new CompoundActionSequential();
    turnAction->AddAction(new TurnInPlaceAction(.7, false));
    turnAction->AddAction(new WaitAction(1));
    turnAction->AddAction(new TurnInPlaceAction(-.7, false));
    turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));
    DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);
  } else if (faceDirection == Vision::TrackedFace::FaceDirection::RightOfRobot) {
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedLeftOrRight.TurningRRRRRRRRight", "");
    CompoundActionSequential* turnAction = new CompoundActionSequential();
    turnAction->AddAction(new TurnInPlaceAction(-.7, false));
    turnAction->AddAction(new WaitAction(1));
    turnAction->AddAction(new TurnInPlaceAction(.7, false));
    turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));
    DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::TransitionToCompleted()
{
  //CancelDelegates(false);
}

void BehaviorReactToFaceNormal::FoundNewFace(ActionResult result)
{

  if (ActionResult::NO_FACE == result) {
    LOG_INFO("BehaviorReactToFaceNormal.FoundNewFace.Result", "No Face %d", result);
    DelegateIfInControl(new TurnTowardsFaceAction(_faceIDToTurnBackTo), &BehaviorReactToFaceNormal::TransitionToCompleted);
  } else if (ActionResult::SUCCESS == result) {
    LOG_INFO("BehaviorReactToFaceNormal.FoundNewFace.Result", "Success %d", result);
    BehaviorReactToFaceNormal::TransitionToCompleted();
  } else {
    LOG_INFO("BehaviorReactToFaceNormal.FoundNewFace.Result", "Other: %d", result);
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.behaviorAlwaysDelegates = false;

  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingGaze, EVisionUpdateFrequency::High });
}

}
}
