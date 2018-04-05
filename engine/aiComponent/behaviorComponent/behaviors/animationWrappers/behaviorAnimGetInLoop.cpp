/**
* File: behaviorAnimGetInLoop.cpp
*
* Author: Kevin M. Karol
* Created: 2/7/18
*
* Description: Behavior which mirrors the animation "Get In Loop" state machine
* Flow: Play GetIn animation followed by Loop animation until EndLoop condition is met
*   followed by GetOut animation
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimGetInLoop.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/events/animationTriggerHelpers.h"
#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {
const char* kGetInAnimationKey  = "getIn";
const char* kLoopAnimationKey   = "loopAnimation";
const char* kGetOutAnimationKey = "getOut";
const char* kEmergencyGetOutAnimationKey = "emergencyGetOut";
const char* kLoopEndConditionKey = "loopEndCondition";
const char* kCheckEndCondKey     = "shouldCheckEndCondDuringAnim";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimGetInLoop::BehaviorAnimGetInLoop(const Json::Value& config)
: ICozmoBehavior(config)
{
  auto loadTrigger = [&config](AnimationTrigger& trigger, const char* key){
    std::string debugStr = "BehaviorAnimGetInLoop.Constructor.MissingParam.";
    trigger = AnimationTriggerFromString(JsonTools::ParseString(config,  key, debugStr + key));
    ANKI_VERIFY(trigger != AnimationTrigger::Count, 
            "BehaviorAnimGetInLoop.Constructor.InvalidAnimTrigger",
            "%s cannot be count",
            key);
  };
  
  loadTrigger(_instanceParams.getInTrigger,  kGetInAnimationKey);
  loadTrigger(_instanceParams.loopTrigger,   kLoopAnimationKey);
  loadTrigger(_instanceParams.getOutTrigger, kGetOutAnimationKey);

  if(config.isMember(kEmergencyGetOutAnimationKey)){
    loadTrigger(_instanceParams.emergencyGetOutTrigger, kEmergencyGetOutAnimationKey);
  }

  if(config.isMember(kLoopEndConditionKey)){
    _instanceParams.endLoopCondition = BEIConditionFactory::CreateBEICondition(config[kLoopEndConditionKey], GetDebugLabel()); 
   ANKI_VERIFY(_instanceParams.endLoopCondition != nullptr,
               "BehaviorAnimGetInLoop.Constructor.InvalidEndLoopCondition",
               "End loop condition specified, but did not build properly");
  }

  if(config.isMember(kCheckEndCondKey)){
    std::string debugStr = "BehaviorAnimGetInLoop.Constructor.CheckEndCondIssue";
    _instanceParams.checkEndConditionDuringAnim = JsonTools::ParseBool(config, kLoopEndConditionKey, debugStr);
  }

}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimGetInLoop::~BehaviorAnimGetInLoop()
{

}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAnimGetInLoop::WantsToBeActivatedBehavior() const
{
  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::InitBehavior()
{
  if(_instanceParams.endLoopCondition != nullptr){
    _instanceParams.endLoopCondition->Init(GetBEI());
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::OnBehaviorActivated()
{
  _instanceParams.endLoopCondition->SetActive(GetBEI(), true);
  _lifetimeParams = LifetimeParams();
  TransitionToGetIn();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::BehaviorUpdate() 
{
  if(!IsActivated()){
    return;
  }
  if(_instanceParams.checkEndConditionDuringAnim){
    _lifetimeParams.shouldLoopEnd |= _instanceParams.endLoopCondition->AreConditionsMet(GetBEI());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::OnBehaviorDeactivated()
{
  _instanceParams.endLoopCondition->SetActive(GetBEI(), false);

  if((_lifetimeParams.stage == BehaviorStage::Loop) &&
     (_instanceParams.emergencyGetOutTrigger != AnimationTrigger::Count)){
    PlayEmergencyGetOut(_instanceParams.emergencyGetOutTrigger);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::TransitionToGetIn()
{
  DelegateIfInControl(new TriggerAnimationAction(_instanceParams.getInTrigger),
                      &BehaviorAnimGetInLoop::TransitionToLoop);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::TransitionToLoop()
{
  if(_instanceParams.endLoopCondition != nullptr){
    _lifetimeParams.shouldLoopEnd |= _instanceParams.endLoopCondition->AreConditionsMet(GetBEI());
  }

  if(_lifetimeParams.shouldLoopEnd){
    TransitionToGetOut();
  }else{
    DelegateIfInControl(new TriggerAnimationAction(_instanceParams.loopTrigger),
                        &BehaviorAnimGetInLoop::TransitionToLoop);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::TransitionToGetOut()
{
  DelegateIfInControl(new TriggerAnimationAction(_instanceParams.getOutTrigger));
}


} // namespace Cozmo
} // namespace Anki
