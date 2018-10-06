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
{
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
  const bool faceFound = GetBEI().GetFaceWorld().HasAnyFaces(500);
  if (faceFound) {
    LOG_INFO("BehaviorReactToFaceNormal.WantsToBeActivatedBehavior.FaceFound", "");
  }
  return faceFound;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::OnBehaviorActivated()
{
  LOG_INFO("BehaviorReactToFaceNormal.OnBehaviorActivated.TransitionToCheckFaceNormalDirectedAtRobot", "");
  TransitionToCheckFaceNormalDirectedAtRobot();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::BehaviorUpdate()
{
  // Not sure if I need this last
  /*
  if (CheckIfShouldStop()) {
    TransitionToCompleted();
  }

  if( ! IsActivated() || ! IsControlDelegated()) {
    return;
  }
  */
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
  LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedAtRobot.AboutToLeftRight", "");
  // TODO set state
  // Check with face world to see if face normal directed at robot
  const bool faceNormDirectedAtRobot = GetBEI().GetFaceWorld().IsFaceDirectedAtRobot(500);

  // TODO we might want to add a delay however that is going to make
  // things more complicated. So for the first pass I'm not going to
  // do that
  if (faceNormDirectedAtRobot) {
    // play place holder animation
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedAtRobot.FaceDirectedAtRobot!!!!!!!", "");
    TransitionToCompleted();
  } else {
    TransitionToCheckFaceNormalDirectedLeftOrRight(); 
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
    // Unstable ... play some animation
    TransitionToCompleted();
  } else if (faceNormDirectedLeftOrRight == 1) {
    // Turn Left
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedLeftOrRight.TurningLeft", "");
    turnAction->AddAction(new TurnInPlaceAction(.7, false));
    DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);
  } else {
    // Turn Right
    LOG_INFO("BehaviorReactToFaceNormal.TransitionToCheckFaceNormalDirectedLeftOrRight.TurningRight", "");
    turnAction->AddAction(new TurnInPlaceAction(-.7, false));
    DelegateIfInControl(turnAction, &BehaviorReactToFaceNormal::TransitionToCompleted);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::TransitionToCompleted()
{
  CancelDelegates(false);
  LOG_INFO("BehaviorReactToFaceNormal.TransitionToCompleted.Finished","");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFaceNormal::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.behaviorAlwaysDelegates = true;

  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingGaze, EVisionUpdateFrequency::High });
}

}
}
