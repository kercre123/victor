/**
 * File: behaviorCantHandleTallStack.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2016-08-29
 *
 * Description: Behavior to unstack a large stack of cubes before knock over cubes is unlocked
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/oneShots/behaviorCantHandleTallStack.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorSystem/behaviorExternalInterface.h"
#include "engine/blockWorld/blockConfigurationManager.h"
#include "engine/blockWorld/blockConfigurationStack.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/events/animationTriggerHelpers.h"
#include "anki/common/basestation/jsonTools.h"
#include "engine/robot.h"

namespace {
static const char* kLookingWaitInitial = "lookingInitialWait_s";
static const char* kLookingDownWait = "lookingDownWait_s";
static const char* kLookingUpWait = "lookingUpWait_s";
static const char* kMinBlockMovedThreshold = "minBlockMovedThreshold_mm";
static const char* const kMinimumStackHeight = "minimumStackHeight";

const float kLookingDown_rad = DEG_TO_RAD(-25);
const float kLookingUp_rad = DEG_TO_RAD(45);
const float kZToleranceStackMoved = 10.0f;
}

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCantHandleTallStack::BehaviorCantHandleTallStack(const Json::Value& config)
: IBehavior(config)
, _isLastReactionPoseValid(false)
{
    
  SubscribeToTags({
    EngineToGameTag::RobotDelocalized
  });
  
  _lookingInitialWait_s = config.get(kLookingWaitInitial, 3).asFloat();
  _lookingDownWait_s = config.get(kLookingDownWait, 3).asFloat();
  _lookingTopWait_s = config.get(kLookingUpWait, 3).asFloat();
  _minBlockMovedThreshold_mm = config.get(kMinBlockMovedThreshold, 3).asFloat();

  _minStackHeight = config.get(kMinimumStackHeight, 3).asInt();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCantHandleTallStack::IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool forFreeplay = true;
  auto progressionUnlockComp = behaviorExternalInterface.GetProgressionUnlockComponent().lock();
  if(progressionUnlockComp!= nullptr){
    if(!progressionUnlockComp->IsUnlocked(UnlockId::KnockOverThreeCubeStack, forFreeplay)){
      UpdateTargetStack(behaviorExternalInterface);
      if(auto tallestStack = _currentTallestStack.lock()){
        const bool tallEnoughStack = tallestStack->GetStackHeight() >= _minStackHeight;
        if(tallEnoughStack){
          auto bottomBlock = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(tallestStack->GetBottomBlockID());
          if(bottomBlock == nullptr){
            return false;
          }
          auto tolerance = Point3f(_minBlockMovedThreshold_mm, _minBlockMovedThreshold_mm, kZToleranceStackMoved);
          const bool hasStackMovedEnough = !_isLastReactionPoseValid ||
                                            !_lastReactionBasePose.IsSameAs(bottomBlock->GetPose(), tolerance, M_PI);
          return hasStackMovedEnough;
        }
      }
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorCantHandleTallStack::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(auto tallestStack = _currentTallestStack.lock()){
    auto bottomBlock = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(tallestStack->GetBottomBlockID());
    if(bottomBlock == nullptr){
      return Result::RESULT_FAIL;
    }
    
    _lastReactionBasePose = bottomBlock->GetPose();
    _isLastReactionPoseValid = true;
    TransitionToLookingUpAndDown(behaviorExternalInterface);
    
    return Result::RESULT_OK;
  }else{
    return Result::RESULT_FAIL;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  ClearStack();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::TransitionToLookingUpAndDown(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(DebugState::LookingUpAndDown);
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  StartActing(new CompoundActionSequential(robot, {
                new WaitAction(robot, _lookingInitialWait_s),
                new MoveHeadToAngleAction(robot, kLookingDown_rad),
                new WaitAction(robot, _lookingDownWait_s),
                new MoveHeadToAngleAction(robot, kLookingUp_rad),
                new WaitAction(robot, _lookingTopWait_s)
              }),
              &BehaviorCantHandleTallStack::TransitionToDisapointment);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::TransitionToDisapointment(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(DebugState::Disapiontment);
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::CantHandleTallStack));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::ClearStack()
{
  _currentTallestStack = {};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::UpdateTargetStack(const BehaviorExternalInterface& behaviorExternalInterface) const
{
  _currentTallestStack = behaviorExternalInterface.GetBlockWorld().GetBlockConfigurationManager().GetStackCache().GetTallestStack();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::RobotDelocalized:
    {
      _isLastReactionPoseValid = false;
      ClearStack();
      break;
    }
    default:
      PRINT_NAMED_ERROR("BehaviorCantHandleTallStack.AlwaysHandleInternal.InvalidEvent", "");
      break;
  }
}

  
}
}
