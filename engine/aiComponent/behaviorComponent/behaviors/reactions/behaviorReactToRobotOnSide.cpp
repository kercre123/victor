/**
 * File: behaviorReactToRobotOnSide.cpp
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

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"

namespace Anki {
namespace Vector {

namespace {
  const char* kAskForHelpAfter_key = "askForHelpAfter_sec";
  const char* kAskForHelpBehavior_key = "askForHelpBehavior";
}

using namespace ExternalInterface;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotOnSide::BehaviorReactToRobotOnSide(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config, "Behavior" + GetDebugLabel() + ".LoadConfig")
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotOnSide::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{
  const bool hasAskForHelpTime = JsonTools::GetValueOptional(config, kAskForHelpAfter_key, askForHelpAfter_sec);
  const bool hasAskForHelpBehavior = JsonTools::GetValueOptional(config, kAskForHelpBehavior_key, askForHelpBehaviorStr);
 
  DEV_ASSERT(hasAskForHelpTime == hasAskForHelpBehavior, "BehaviorReactToRobotOnSide.InstanceConfig.InvalidConfig");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnSide::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  if (!_iConfig.askForHelpBehaviorStr.empty()) {
    _iConfig.askForHelpBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.askForHelpBehaviorStr));
    DEV_ASSERT(_iConfig.askForHelpBehavior != nullptr,
               "BehaviorReactToRobotOnSide.InitBehavior.NullBehavior");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kAskForHelpAfter_key,
    kAskForHelpBehavior_key,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if (_iConfig.askForHelpBehavior != nullptr) {
    delegates.insert(_iConfig.askForHelpBehavior.get());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  ReactToBeingOnSide();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::BehaviorUpdate()
{
  if (!IsActivated()) {
    return;
  }
  
  // If we are no longer on our side (but have not been cancelled externally yet), then play the get-out and exit
  const bool onSide = (GetBEI().GetOffTreadsState() == OffTreadsState::OnLeftSide) ||
                      (GetBEI().GetOffTreadsState() == OffTreadsState::OnRightSide);
  if (!onSide && !_dVars.getOutPlayed) {
    DelegateNow(new TriggerAnimationAction(AnimationTrigger::ReactToOnSideGetOut));
    _dVars.getOutPlayed = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::ReactToBeingOnSide()
{
  if (GetBEI().GetOffTreadsState() == OffTreadsState::OnLeftSide) {
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ReactToOnLeftSideGetIn),
                        &BehaviorReactToRobotOnSide::HoldingLoop);
  } else if (GetBEI().GetOffTreadsState() == OffTreadsState::OnRightSide) {
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ReactToOnRightSideGetIn),
                        &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::HoldingLoop()
{
  // Check if we have been activated for longer than the timeout. If so, play the getout then transition to the
  // "ask for help" behavior
  if (_iConfig.askForHelpAfter_sec > 0.f &&
      GetActivatedDuration() > _iConfig.askForHelpAfter_sec) {
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ReactToOnSideGetOut),
                        [this](){
                          if (_iConfig.askForHelpBehavior.get()->WantsToBeActivated()) {
                            DelegateIfInControl(_iConfig.askForHelpBehavior.get());
                          }
                        });
    return;
  }
  
  auto loopAnim = AnimationTrigger::Count;
  
  if (GetBEI().GetOffTreadsState() == OffTreadsState::OnLeftSide) {
    loopAnim = AnimationTrigger::ReactToOnLeftSideLoop;
  } else if (GetBEI().GetOffTreadsState() == OffTreadsState::OnRightSide) {
    loopAnim = AnimationTrigger::ReactToOnRightSideLoop;
  }
  
  if (loopAnim != AnimationTrigger::Count) {
    const auto effortAnim = AnimationTrigger::ReactToOnSideEffort;
    
    // Alternate between 'loop' animation and 'effort' animation
    auto* action = new CompoundActionSequential();
    action->AddAction(new TriggerAnimationAction(loopAnim));
    action->AddAction(new TriggerAnimationAction(effortAnim));
    
    DelegateIfInControl(action,
                        &BehaviorReactToRobotOnSide::HoldingLoop);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnSide::OnBehaviorDeactivated()
{
}

} // namespace Vector
} // namespace Anki
