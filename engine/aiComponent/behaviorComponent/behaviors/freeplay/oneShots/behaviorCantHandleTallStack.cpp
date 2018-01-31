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

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/oneShots/behaviorCantHandleTallStack.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/blockWorld/blockConfigurationManager.h"
#include "engine/blockWorld/blockConfigurationStack.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/events/animationTriggerHelpers.h"
#include "coretech/common/engine/jsonTools.h"

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
: ICozmoBehavior(config)
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
bool BehaviorCantHandleTallStack::WantsToBeActivatedBehavior() const
{
  const bool forFreeplay = true;
  if(GetBEI().HasProgressionUnlockComponent()){
    auto& progressionUnlockComp = GetBEI().GetProgressionUnlockComponent();
    if(!progressionUnlockComp.IsUnlocked(UnlockId::KnockOverThreeCubeStack, forFreeplay)){
      UpdateTargetStack();
      if(auto tallestStack = _currentTallestStack.lock()){
        const bool tallEnoughStack = tallestStack->GetStackHeight() >= _minStackHeight;
        if(tallEnoughStack){
          auto bottomBlock = GetBEI().GetBlockWorld().GetLocatedObjectByID(tallestStack->GetBottomBlockID());
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
void BehaviorCantHandleTallStack::OnBehaviorActivated()
{
  if(auto tallestStack = _currentTallestStack.lock()){
    auto bottomBlock = GetBEI().GetBlockWorld().GetLocatedObjectByID(tallestStack->GetBottomBlockID());
    if(bottomBlock == nullptr){
      return;
    }
    
    _lastReactionBasePose = bottomBlock->GetPose();
    _isLastReactionPoseValid = true;
    TransitionToLookingUpAndDown();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::OnBehaviorDeactivated()
{
  ClearStack();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::TransitionToLookingUpAndDown()
{
  DEBUG_SET_STATE(DebugState::LookingUpAndDown);
  
  DelegateIfInControl(new CompoundActionSequential({
                new WaitAction(_lookingInitialWait_s),
                new MoveHeadToAngleAction(kLookingDown_rad),
                new WaitAction(_lookingDownWait_s),
                new MoveHeadToAngleAction(kLookingUp_rad),
                new WaitAction(_lookingTopWait_s)
              }),
              &BehaviorCantHandleTallStack::TransitionToDisapointment);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::TransitionToDisapointment()
{
  DEBUG_SET_STATE(DebugState::Disapiontment);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CantHandleTallStack));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::ClearStack()
{
  _currentTallestStack = {};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::UpdateTargetStack() const
{
  _currentTallestStack = GetBEI().GetBlockWorld().GetBlockConfigurationManager().GetStackCache().GetTallestStack();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCantHandleTallStack::AlwaysHandleInScope(const EngineToGameEvent& event)
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
