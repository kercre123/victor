/**
 * File: behaviorReactToRobotOnSide.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: Cozmo reacts to being placed on his side
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnSide.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"

namespace Anki {
namespace Vector {
  
using namespace ExternalInterface;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotOnSide::BehaviorReactToRobotOnSide(const Json::Value& config)
: ICozmoBehavior(config)
{
  _offTreadsConditions.emplace_back( std::make_shared<ConditionOffTreadsState>(OffTreadsState::OnLeftSide, GetDebugLabel()) );
  _offTreadsConditions.emplace_back( std::make_shared<ConditionOffTreadsState>(OffTreadsState::OnRightSide,GetDebugLabel()) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnSide::WantsToBeActivatedBehavior() const
{
  const bool wantsToBeActivated = std::any_of(_offTreadsConditions.begin(),
                                              _offTreadsConditions.end(),
                                              [this](const IBEIConditionPtr& condition) {
                                                return condition->AreConditionsMet(GetBEI());
                                              });
  return wantsToBeActivated;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::InitBehavior()
{
  for (auto& condition : _offTreadsConditions) {
    condition->Init(GetBEI());
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::OnBehaviorEnteredActivatableScope() {
  for (auto& condition : _offTreadsConditions) {
    condition->SetActive(GetBEI(), true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::OnBehaviorLeftActivatableScope()
{
  for (auto& condition : _offTreadsConditions) {
    condition->SetActive(GetBEI(), false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::OnBehaviorActivated()
{
  ICozmoBehavior::SmartRequestPowerSaveMode();
  
  ReactToBeingOnSide();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::ReactToBeingOnSide()
{
  AnimationTrigger anim = AnimationTrigger::Count;
  
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnLeftSide){
    anim = AnimationTrigger::ReactToOnLeftSide;
  }
  
  if(GetBEI().GetOffTreadsState() == OffTreadsState::OnRightSide) {
    anim = AnimationTrigger::ReactToOnRightSide;
  }
  
  if(anim != AnimationTrigger::Count){
    const u32 numLoops = 1;
    const bool interruptRunning = true;
    // lock the body to avoid weird motion
    const u8 tracksToLock = (u8)AnimTrackFlag::BODY_TRACK;
    DelegateIfInControl(new TriggerAnimationAction(anim,
                                                   numLoops,
                                                   interruptRunning,
                                                   tracksToLock),
                        &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::HoldingLoop()
{
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnRightSide
     || GetBEI().GetOffTreadsState() == OffTreadsState::OnLeftSide) {
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::WaitOnSideLoop),
                        &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::OnBehaviorDeactivated()
{
}

} // namespace Vector
} // namespace Anki
