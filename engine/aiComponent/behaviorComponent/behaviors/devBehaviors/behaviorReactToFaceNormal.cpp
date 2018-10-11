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


namespace Anki {
namespace Vector {

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
    // TransitionToCheckFaceNormalDirectedAtRobot();
    TransitionToCheckFaceDirection();
    //:w_dVars->lastReactionTime_ms = currentTime_sec;
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
void BehaviorReactToFaceNormal::TransitionToCheckFaceNormalDirectedAtRobot()
{
  // TODO set state
  // Check with face world to see if face normal directed at robot
  const bool faceNormDirectedAtRobot = GetBEI().GetFaceWorld().IsFaceDirectedAtRobot(500);

  // TODO we might want to add a delay however that is going to make
  // things more complicated. So for the first pass I'm not going to
  // do that
  if (faceNormDirectedAtRobot) {
    // play place holder animation
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedAtRobot.FaceDirectedAtRobot!!!!!!!", "");
    // TODO this is really heavy handed replace with a subtler animation
    CompoundActionSequential* action = new CompoundActionSequential();
    action->AddAction(new TriggerAnimationAction(AnimationTrigger::ExploringHuhFar));
    action->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));
    DelegateIfInControl(action, &BehaviorReactToFaceNormal::TransitionToCompleted);
  } else {
    TransitionToCheckFaceNormalDirectedLeftOrRight(); 
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::TransitionToCheckFaceDirection()
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
void BehaviorReactToFaceNormal::TransitionToCheckFaceNormalDirectedLeftOrRight()
{
  // TODO set state
  // Check with face world to see if face normal directed at robot
  const int faceNormDirectedLeftOrRight = GetBEI().GetFaceWorld().IsFaceDirectedAtLeftRight(500);

  CompoundActionParallel* turnAction = new CompoundActionParallel();
  turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));

  if (faceNormDirectedLeftOrRight == 0) {
    // Unstable ... do nothing?
    TransitionToCompleted();
  } else if (faceNormDirectedLeftOrRight == 1) {
    // Turn Left
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedLeftOrRight.TurningLLLLLLLeft", "");
    turnAction->AddAction(new TurnInPlaceAction(.7, false));
    DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);
  } else {
    // Turn Right
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedLeftOrRight.TurningRRRRRRRRight", "");
    turnAction->AddAction(new TurnInPlaceAction(-.7, false));
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
