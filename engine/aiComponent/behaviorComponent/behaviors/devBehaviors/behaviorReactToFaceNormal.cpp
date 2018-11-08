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
  CONSOLE_VAR(f32, kMaxPanAccel_radPerSec2,                  "Vision.FaceNormalDirectedAtRobot3d",  10.f);
  CONSOLE_VAR(bool, kUseEyeContact,                          "Vision.FaceNormalDirectedAtRobot3d",  true);
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
  SmartFaceID faceID;
  if(GetBEI().GetFaceWorld().GetFaceFocusPose(500, faceFocusPose, faceID)) {
    auto translation = faceFocusPose.GetTranslation();
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.Translation",
             "x: %.3f, y:%.3f, z:%.3f", translation.x(), translation.y(), translation.z());
    auto makingEyeContact = GetBEI().GetFaceWorld().IsMakingEyeContact(500);
    if ( ( ( FLT_GT(translation.x(), kFaceDirectedAtRobotMinXThres) && FLT_LT(translation.x(), kFaceDirectedAtRobotMaxXThres) ) &&
         (FLT_GT(translation.y(), kFaceDirectedAtRobotMinYThres) && FLT_LT(translation.y(), kFaceDirectedAtRobotMaxYThres)) )
          || ( makingEyeContact && kUseEyeContact) ) {

      LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.LookingAtRobot",
               "min x: %.3f, max x: %.3f, min y: %.3f, max y: %.3f", kFaceDirectedAtRobotMinXThres,
               kFaceDirectedAtRobotMaxXThres, kFaceDirectedAtRobotMinYThres, kFaceDirectedAtRobotMaxYThres);
      CompoundActionSequential* turnAction = new CompoundActionSequential();
      turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtVectorGetIn));
      turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtVectorReaction));
      turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
      turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));

      turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
      DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);

    } else {
      auto robotPose = GetBEI().GetRobotInfo().GetPose();
      Pose3d faceFocusPoseWRTRobot;

      if (faceFocusPose.GetWithRespectTo(robotPose, faceFocusPoseWRTRobot)) {

        if ( FLT_LT(translation.y(), 0.f) ) {
          LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.LookingLeftTest", "");
          faceFocusPoseWRTRobot.Print("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection", "FaceFocusPoseWRTRobot");
          CompoundActionSequential* turnAction = new CompoundActionSequential();
          turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesGetInRight));

          // Do an initial turn if we want to
          if (kTurnBackToFace) {
            turnAction->AddAction(new TurnTowardsPoseAction(faceFocusPoseWRTRobot));
            turnAction->AddAction(new WaitAction(kTurnWaitAfterInitialTurn_s));
            turnAction->AddAction(new TurnTowardsFaceAction(faceID));
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
          turnAction->AddAction(new TurnTowardsFaceAction(faceID));

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
            turnAction->AddAction(new TurnTowardsFaceAction(faceID));
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
          turnAction->AddAction(new TurnTowardsFaceAction(faceID));

          turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
          DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);
        }
      } else {
        LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceDirection.GetWithRespectToFailed", "");
      }
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
