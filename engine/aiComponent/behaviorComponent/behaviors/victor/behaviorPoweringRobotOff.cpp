/**
 * File: BehaviorPoweringRobotOff.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-07-18
 *
 * Description: Behavior which plays power on/off animations in response to the power button being held down
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorPoweringRobotOff.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimePowerButtonPressed.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/externalInterface/externalInterface.h"

namespace Anki {
namespace Vector {
  
namespace{
const char* kPowerButtonActivationKey = "powerButtonHeldToActivate_ms";
const char* kPowerOnAnimName          = "powerOnAnimName";
const char* kPowerOffAnimName         = "powerOffAnimName";
const char* const kWaitForAnimMsgKey  = "waitForAnimMsg";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPoweringRobotOff::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  const std::string debugName = "BehaviorPoweringRobotOff.InstanceConfig.MissingKey. ";
  powerOnAnimName    = JsonTools::ParseString(config, kPowerOnAnimName, debugName + kPowerOnAnimName);
  powerOffAnimName   = JsonTools::ParseString(config, kPowerOffAnimName, debugName + kPowerOffAnimName);

  const auto timeActivateBehavior = JsonTools::ParseUInt32(config, kPowerButtonActivationKey, debugName + kPowerButtonActivationKey);
  activateBehaviorCondition = std::shared_ptr<IBEICondition>(new ConditionTimePowerButtonPressed(timeActivateBehavior, "BehaviorPoweringRobotOff"));

  waitForAnimMsg = config.get( kWaitForAnimMsgKey, false ).asBool();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPoweringRobotOff::DynamicVariables::DynamicVariables()
: timeLastPowerAnimStopped_ms(0)
, shouldStartPowerOffAnimaiton(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPoweringRobotOff::BehaviorPoweringRobotOff(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config)
{
  SubscribeToTags({RobotInterface::RobotToEngineTag::startShutdownAnim});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPoweringRobotOff::~BehaviorPoweringRobotOff()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::InitBehavior()
{
  _iConfig.activateBehaviorCondition->Init(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPoweringRobotOff::WantsToBeActivatedBehavior() const
{
  if( _iConfig.waitForAnimMsg ) {
    return _dVars.shouldStartPowerOffAnimaiton;
  } else {
    return _iConfig.activateBehaviorCondition->AreConditionsMet(GetBEI());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kPowerOnAnimName,
    kPowerOffAnimName,
    kPowerButtonActivationKey,
    kWaitForAnimMsgKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::OnBehaviorEnteredActivatableScope()
{
  _iConfig.activateBehaviorCondition->SetActive(GetBEI(), true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  SmartPushResponseToTriggerWord();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::BehaviorUpdate() 
{
  if( !IsActivated() || _dVars.waitingForAnimationCallback ) {
    return;
  }

  _dVars.shouldStartPowerOffAnimaiton &= GetBEI().GetRobotInfo().IsPowerButtonPressed();
  const bool wasPowerOffLastAnimPlayed = _dVars.lastAnimPlayedName == _iConfig.powerOffAnimName;
  const bool wasPowerOnLastAnimPlayed = _dVars.lastAnimPlayedName == _iConfig.powerOnAnimName;

  // Don't do anything until the power button has been held down long enough to start powering down
  if(!_dVars.shouldStartPowerOffAnimaiton &&
     _dVars.lastAnimPlayedName.empty()){
    if(_iConfig.activateBehaviorCondition->AreConditionsMet(GetBEI())){
      return;
    }else{
      CancelSelf();
      return;
    }
  }
  
  if(_dVars.shouldStartPowerOffAnimaiton &&
     (wasPowerOnLastAnimPlayed || _dVars.lastAnimPlayedName.empty())){
    if(IsControlDelegated()){
      CancelDelegates(false);
      _dVars.waitingForAnimationCallback = true;
    }else{
      TransitionToPoweringOff();
    }
  }else if(!_dVars.shouldStartPowerOffAnimaiton && wasPowerOffLastAnimPlayed){
    if(IsControlDelegated()){
      CancelDelegates(false);
      _dVars.waitingForAnimationCallback = true;
    }else{
      TransitionToPoweringOn();
    }
  }
  
  // If user released power button see if eyes are restored and normal behavior should continue
  if(!IsControlDelegated() &&
     !_dVars.shouldStartPowerOffAnimaiton &&
     wasPowerOnLastAnimPlayed){
    CancelSelf();
  }
  
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::OnBehaviorLeftActivatableScope()
{
  _iConfig.activateBehaviorCondition->SetActive(GetBEI(), false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::AlwaysHandleInScope(const RobotToEngineEvent& event)  {
  if(event.GetData().GetTag() == RobotInterface::RobotToEngineTag::startShutdownAnim){
    _dVars.shouldStartPowerOffAnimaiton = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::TransitionToPoweringOff()
{
  const bool havePlayedAnyAnim = _dVars.timeLastPowerAnimStopped_ms > 0;
  const auto startTime_ms = havePlayedAnyAnim ? GetLengthOfAnimation_ms(_iConfig.powerOffAnimName) - _dVars.timeLastPowerAnimStopped_ms : 0;
  
  StartAnimation(_iConfig.powerOffAnimName, startTime_ms);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::TransitionToPoweringOn()
{
  const bool havePlayedAnyAnim = _dVars.timeLastPowerAnimStopped_ms > 0;
  const auto startTime_ms = havePlayedAnyAnim ? GetLengthOfAnimation_ms(_iConfig.powerOnAnimName) - _dVars.timeLastPowerAnimStopped_ms : 0;
  
  StartAnimation(_iConfig.powerOnAnimName, startTime_ms);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::StartAnimation(const std::string& animName, const TimeStamp_t startTime_ms)
{
  _dVars.lastAnimPlayedName = animName;
  const u32 numLoops = 1;
  const bool interruptRunning = true;
  const u8 tracksToLock = (u8)AnimTrackFlag::NO_TRACKS;
  const float timeout_sec = PlayAnimationAction::GetDefaultTimeoutInSeconds();
  
  auto callback = [this](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded) {
    _dVars.waitingForAnimationCallback = false;
    if(res == AnimationComponent::AnimResult::Completed){
      _dVars.timeLastPowerAnimStopped_ms = 0;
    }else{
      _dVars.timeLastPowerAnimStopped_ms = streamTimeAnimEnded;
    }
  };
  DelegateIfInControl(new PlayAnimationAction(animName, numLoops, interruptRunning, 
                                              tracksToLock, timeout_sec, startTime_ms, callback));

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorPoweringRobotOff::GetLengthOfAnimation_ms(const std::string& animName)
{
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  const auto* animContainer = dataAccessorComp.GetCannedAnimationContainer();
  auto length_ms = 0;
  if((animContainer != nullptr) && !_iConfig.powerOffAnimName.empty()){
    auto animPtr = animContainer->GetAnimation(_iConfig.powerOffAnimName);
    if(animPtr != nullptr){
      length_ms = animPtr->GetLastKeyFrameEndTime_ms();
    }else{
      PRINT_NAMED_ERROR("BehaviorPoweringRobotOff.GetLengthOfAnimation_ms.MissingAnimation", 
                        "Animation named %s is not accessible in engine", animName.c_str());
    }
  }
  
  return length_ms;
}


}
}
