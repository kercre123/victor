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
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Vector {

namespace {
const char* kGetInAnimationKey           = "getIn";
const char* kLoopAnimationKey            = "loopAnimation";
const char* kGetOutAnimationKey          = "getOut";
const char* kEmergencyGetOutAnimationKey = "emergencyGetOut";
const char* kLoopEndConditionKey         = "loopEndCondition";
const char* kLoopIntervalKey             = "loopInterval_s"; // If less than length of animation, will not interrupt
const char* kCheckEndCondKey             = "shouldCheckEndCondDuringAnim";
const char* kLockTreadsKey               = "lockTreads";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimGetInLoop::InstanceConfig::InstanceConfig()
{
  getInTrigger                = AnimationTrigger::Count;
  loopTrigger                 = AnimationTrigger::Count;
  getOutTrigger               = AnimationTrigger::Count;
  emergencyGetOutTrigger      = AnimationTrigger::Count;
  loopInterval_s              = 0;
  checkEndConditionDuringAnim = true;
  tracksToLock                = static_cast<uint8_t>(AnimTrackFlag::NO_TRACKS);
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimGetInLoop::DynamicVariables::DynamicVariables() 
{
  stage = BehaviorStage::GetIn;
  shouldLoopEnd = false;
  nextLoopTime_s = 0;
};


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
  
  loadTrigger(_iConfig.getInTrigger,  kGetInAnimationKey);
  loadTrigger(_iConfig.loopTrigger,   kLoopAnimationKey);
  loadTrigger(_iConfig.getOutTrigger, kGetOutAnimationKey);

  if(config.isMember(kEmergencyGetOutAnimationKey)){
    loadTrigger(_iConfig.emergencyGetOutTrigger, kEmergencyGetOutAnimationKey);
  }
  
  bool lockTreads = false;
  JsonTools::GetValueOptional( config, kLockTreadsKey, lockTreads );
  _iConfig.tracksToLock = lockTreads
                          ? static_cast<uint8_t>(AnimTrackFlag::BODY_TRACK)
                          : static_cast<uint8_t>(AnimTrackFlag::NO_TRACKS);

  if(config.isMember(kLoopEndConditionKey)){
    _iConfig.endLoopCondition = BEIConditionFactory::CreateBEICondition(config[kLoopEndConditionKey], GetDebugLabel()); 
   ANKI_VERIFY(_iConfig.endLoopCondition != nullptr,
               "BehaviorAnimGetInLoop.Constructor.InvalidEndLoopCondition",
               "End loop condition specified, but did not build properly");
  }

  if(config.isMember(kCheckEndCondKey)){
    std::string debugStr = "BehaviorAnimGetInLoop.Constructor.CheckEndCondIssue";
    _iConfig.checkEndConditionDuringAnim = JsonTools::ParseBool(config, kCheckEndCondKey, debugStr);
  }

  if(config.isMember(kLoopIntervalKey)){
    std::string debugStr = "BehaviorAnimGetInLoop.Constructor.LoopInterval";
    _iConfig.loopInterval_s = JsonTools::ParseFloat(config, kLoopIntervalKey, debugStr);

  }

}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kGetInAnimationKey,
    kLoopAnimationKey,
    kGetOutAnimationKey,
    kEmergencyGetOutAnimationKey,
    kLoopEndConditionKey,
    kCheckEndCondKey,
    kLoopIntervalKey,
    kLockTreadsKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
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
  if(_iConfig.endLoopCondition != nullptr){
    _iConfig.endLoopCondition->Init(GetBEI());
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::OnBehaviorActivated()
{
  if(_iConfig.endLoopCondition != nullptr){
    _iConfig.endLoopCondition->SetActive(GetBEI(), true);
  }
  _dVars = DynamicVariables();
  TransitionToGetIn();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::BehaviorUpdate() 
{
  AnimBehaviorUpdate();
  if(!IsActivated()){
    return;
  }

  if(_iConfig.checkEndConditionDuringAnim &&
     (_iConfig.endLoopCondition != nullptr)){
    _dVars.shouldLoopEnd |= _iConfig.endLoopCondition->AreConditionsMet(GetBEI());
  }

  if(IsControlDelegated() || (_dVars.stage == BehaviorStage::GetOut)){
    return;
  }

  if(_dVars.shouldLoopEnd){
    TransitionToGetOut();
    return;
  }
  
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(_dVars.nextLoopTime_s  < currentTime_sec){
    _dVars.nextLoopTime_s = currentTime_sec + _iConfig.loopInterval_s;
    TransitionToLoop();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::OnBehaviorDeactivated()
{
  if(_iConfig.endLoopCondition != nullptr){
    _iConfig.endLoopCondition->SetActive(GetBEI(), false);
  }

  if((_dVars.stage == BehaviorStage::Loop || 
      _dVars.stage == BehaviorStage::GetIn) &&
     (_iConfig.emergencyGetOutTrigger != AnimationTrigger::Count)){
    PlayEmergencyGetOut(_iConfig.emergencyGetOutTrigger);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::TransitionToGetIn()
{
  _dVars.stage = BehaviorStage::GetIn;
  DelegateIfInControl(new TriggerAnimationAction(_iConfig.getInTrigger, 1, true, _iConfig.tracksToLock));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::TransitionToLoop()
{
  _dVars.stage = BehaviorStage::Loop;
  DelegateIfInControl(new TriggerAnimationAction(_iConfig.loopTrigger, 1, true, _iConfig.tracksToLock));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimGetInLoop::TransitionToGetOut()
{
  _dVars.stage = BehaviorStage::GetOut;
  DelegateIfInControl(new TriggerAnimationAction(_iConfig.getOutTrigger, 1, true, _iConfig.tracksToLock), [this](){
                        CancelSelf();
                      });
}


} // namespace Vector
} // namespace Anki
