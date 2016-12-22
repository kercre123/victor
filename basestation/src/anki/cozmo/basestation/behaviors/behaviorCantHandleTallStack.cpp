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

#include "anki/cozmo/basestation/behaviors/behaviorCantHandleTallStack.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationStack.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/robot.h"

namespace {
static const char* kLookingWaitInitial = "lookingInitialWait_s";
static const char* kLookingDownWait = "lookingDownWait_s";
static const char* kLookingUpWait = "lookingUpWait_s";
static const char* kMinBlockMovedThreshold = "minBlockMovedThreshold_mm";
static const char* const kMinimumStackHeight = "minimumStackHeight";

const float kLookingDown_rad = DEG_TO_RAD(-25);
const float kLookingUp_rad = DEG_TO_RAD(45);
const float kZTolerenceStackMoved = 10.0f;
}

namespace Anki {
namespace Cozmo {
   
  
BehaviorCantHandleTallStack::BehaviorCantHandleTallStack(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _isLastReactionPoseValid(false)
{
  SetDefaultName("CantHandleTallStack");
  
  SubscribeToTags({
    EngineToGameTag::RobotDelocalized
  });
  
  _lookingInitialWait_s = config.get(kLookingWaitInitial, 3).asFloat();
  _lookingDownWait_s = config.get(kLookingDownWait, 3).asFloat();
  _lookingTopWait_s = config.get(kLookingUpWait, 3).asFloat();
  _minBlockMovedThreshold_mm = config.get(kMinBlockMovedThreshold, 3).asFloat();

  _minStackHeight = config.get(kMinimumStackHeight, 3).asInt();
}
  
bool BehaviorCantHandleTallStack::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();
  const bool forFreeplay = true;
  
  if(!robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::KnockOverThreeCubeStack, forFreeplay)){
    UpdateTargetStack(robot);
    if(auto tallestStack = _currentTallestStack.lock()){
      const bool tallEnoughStack = tallestStack->GetStackHeight() >= _minStackHeight;
      if(tallEnoughStack){
        auto bottomBlock = robot.GetBlockWorld().GetLocatedObjectByID(tallestStack->GetBottomBlockID());
        if(bottomBlock == nullptr){
          return false;
        }
        auto tolerence = Point3f(_minBlockMovedThreshold_mm, _minBlockMovedThreshold_mm, kZTolerenceStackMoved);
        const bool hasStackMovedEnough = !_isLastReactionPoseValid ||
                                          !_lastReactionBasePose.IsSameAs(bottomBlock->GetPose(), tolerence, M_PI);
        return hasStackMovedEnough;
      }
    }
  }
  
  return false;
}

  
Result BehaviorCantHandleTallStack::InitInternal(Robot& robot)
{
  if(auto tallestStack = _currentTallestStack.lock()){
    auto bottomBlock = robot.GetBlockWorld().GetLocatedObjectByID(tallestStack->GetBottomBlockID());
    if(bottomBlock == nullptr){
      return Result::RESULT_FAIL;
    }
    
    _lastReactionBasePose = bottomBlock->GetPose();
    _isLastReactionPoseValid = true;
    TransitionToLookingUpAndDown(robot);
    
    return Result::RESULT_OK;
  }else{
    return Result::RESULT_FAIL;
  }
}

  
void BehaviorCantHandleTallStack::StopInternal(Robot& robot)
{
  ClearStack();
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
  
void BehaviorCantHandleTallStack::ClearStack()
{
  _currentTallestStack = {};
}

void BehaviorCantHandleTallStack::UpdateTargetStack(const Robot& robot) const
{
  _currentTallestStack = robot.GetBlockWorld().GetBlockConfigurationManager().GetStackCache().GetTallestStack();
}
  
void BehaviorCantHandleTallStack::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
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
