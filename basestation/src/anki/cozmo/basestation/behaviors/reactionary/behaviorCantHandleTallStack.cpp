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

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorCantHandleTallStack.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/robot.h"

static const char* kLookingWaitInitial = "lookingInitialWait_s";
static const char* kLookingDownWait = "lookingDownWait_s";
static const char* kLookingUpWait = "lookingUpWait_s";
static const char* kMinBlockMovedThreshold = "minBlockMovedThreshold_mm_sqr";
static const char* const kMinimumStackHeight = "minimumStackHeight";

const float kLookingDown_rad = DEG_TO_RAD(-25);
const float kLookingUp_rad = DEG_TO_RAD(45);

namespace Anki {
namespace Cozmo {
   
  
BehaviorCantHandleTallStack::BehaviorCantHandleTallStack(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _stackHeight(0)
  , _objectObservedChanged(false)
  , _baseBlockPoseValid(false)
  , _lastObservedObject(-1)
{
  SetDefaultName("CantHandleTallStack");
  
  SubscribeToTags({{
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::RobotDelocalized
  }});
  
  _lookingInitialWait_s = config.get(kLookingWaitInitial, 3).asFloat();
  _lookingDownWait_s = config.get(kLookingDownWait, 3).asFloat();
  _lookingTopWait_s = config.get(kLookingUpWait, 3).asFloat();
  _minBlockMovedThreshold_mm_sqr = config.get(kMinBlockMovedThreshold, 3).asFloat();

  _minStackHeight = config.get(kMinimumStackHeight, 3).asInt();
  
}
  
bool BehaviorCantHandleTallStack::IsRunnableInternal(const Robot& robot) const
{
  const bool forFreeplay = true;
  if(!robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::KnockOverThreeCubeStack, forFreeplay)){
    UpdateTargetStack(robot);
    return _stackHeight >= _minStackHeight;
  }
  
  return false;
}

  
Result BehaviorCantHandleTallStack::InitInternal(Robot& robot)
{
  TransitionToReactingToStack(robot);
  return Result::RESULT_OK;
}

  
void BehaviorCantHandleTallStack::StopInternal(Robot& robot)
{
  ResetBehavior();
}
  
void BehaviorCantHandleTallStack::TransitionToReactingToStack(Robot& robot)
{
  DEBUG_SET_STATE(DebugState::ReactiongToStack);
  
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::AcknowledgeObject),
              &BehaviorCantHandleTallStack::TransitionToLookingAtStack);
}


void BehaviorCantHandleTallStack::TransitionToLookingAtStack(Robot& robot)
{
  DEBUG_SET_STATE(DebugState::LookingAtStack);
  
  StartActing(new TurnTowardsObjectAction(robot, _baseBlockID, M_PI),
              &BehaviorCantHandleTallStack::TransitionToLookingUpAndDown);
  
}

  
void BehaviorCantHandleTallStack::TransitionToLookingUpAndDown(Robot& robot)
{
  DEBUG_SET_STATE(DebugState::LookingUpAndDown);
  
  StartActing(new CompoundActionSequential(robot, {
                new WaitAction(robot, _lookingInitialWait_s),
                new MoveHeadToAngleAction(robot, kLookingDown_rad),
                new WaitAction(robot, _lookingDownWait_s),
                new MoveHeadToAngleAction(robot, kLookingUp_rad),
                new WaitAction(robot, _lookingTopWait_s)
              }),
              &BehaviorCantHandleTallStack::TransitionToDisapointment);
  
}
  
void BehaviorCantHandleTallStack::TransitionToDisapointment(Robot& robot)
{
  DEBUG_SET_STATE(DebugState::Disapiontment);
  
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::CantHandleTallStack));
}
  
void BehaviorCantHandleTallStack::ResetBehavior()
{
  _baseBlockID.UnSet();
  _stackHeight = 0;
}

void BehaviorCantHandleTallStack::UpdateTargetStack(const Robot& robot) const
{
   _stackHeight = robot.GetBlockWorld().GetTallestStack(_baseBlockID);
}
  
void BehaviorCantHandleTallStack::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::RobotObservedObject:
    {
      if(event.GetData().Get_RobotObservedObject().objectID != _lastObservedObject){
        _lastObservedObject = event.GetData().Get_RobotObservedObject().objectID;
        _objectObservedChanged = true;
      }
      break;
    }
    case ExternalInterface::MessageEngineToGameTag::RobotDelocalized:
    {
      _baseBlockPoseValid = false;
      ResetBehavior();
      break;
    }
    default:
      PRINT_NAMED_ERROR("BehaviorCantHandleTallStack.AlwaysHandleInternal.InvalidEvent", "");
      break;
  }
}

  
}
}
