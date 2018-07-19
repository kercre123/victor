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
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimePowerButtonPressed.h"
#include "engine/components/dataAccessorComponent.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const char* kStartPowerOffKey   = "startPowerOffEyes_ms";
const char* kPowerOnAnimName    = "powerOnAnimName";
const char* kPowerOffAnimName   = "powerOffAnimName";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPoweringRobotOff::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  const std::string debugName = "BehaviorPoweringRobotOff.InstanceConfig.MissingKey. ";
  powerOnAnimName    = JsonTools::ParseString(config, kPowerOnAnimName, debugName + kPowerOnAnimName);
  powerOffAnimName   = JsonTools::ParseString(config, kPowerOffAnimName, debugName + kPowerOffAnimName);

  const auto timeStartPowerOff = JsonTools::ParseUInt32(config, kStartPowerOffKey, debugName + kStartPowerOffKey);
  powerButtonHeldCondition = std::shared_ptr<IBEICondition>(new ConditionTimePowerButtonPressed(timeStartPowerOff, "BehaviorPoweringRobotOff"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPoweringRobotOff::DynamicVariables::DynamicVariables()
: behaviorStage(BehaviorStage::PoweringOff)
, timeLastPowerAnimStopped_ms(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPoweringRobotOff::BehaviorPoweringRobotOff(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPoweringRobotOff::~BehaviorPoweringRobotOff()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::InitBehavior()
{
  _iConfig.powerButtonHeldCondition->Init(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPoweringRobotOff::WantsToBeActivatedBehavior() const
{
  return _iConfig.powerButtonHeldCondition->AreConditionsMet(GetBEI());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
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
    kStartPowerOffKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::OnBehaviorEnteredActivatableScope()
{
  _iConfig.powerButtonHeldCondition->SetActive(GetBEI(), true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  TransitionToPoweringOff();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }

  const auto isPowerButtonPressed = GetBEI().GetRobotInfo().IsPowerButtonPressed();

  // Switch back and forth between power on/power off
  if(!isPowerButtonPressed &&
     ((_dVars.behaviorStage == BehaviorStage::PoweringOff) ||
      (_dVars.behaviorStage == BehaviorStage::AnimationComplete))){
    if(IsControlDelegated()){
      CancelDelegates(false);
      _dVars.behaviorStage = BehaviorStage::WaitingForAnimationCallback;
    }else{
      TransitionToPoweringOn();
    }
  }
  
  // The behavior wants to "re-start" and turn the power off again
  if(_dVars.behaviorStage == BehaviorStage::PoweringOn &&
     _iConfig.powerButtonHeldCondition->AreConditionsMet(GetBEI())){
    if(IsControlDelegated()){
      CancelDelegates(false);
      _dVars.behaviorStage = BehaviorStage::WaitingForAnimationCallback;
    }else{
      TransitionToPoweringOff();
    }
  }

  if(!IsControlDelegated()){
    // We've restored the robot to nuetral eyes,  done
    if(_dVars.behaviorStage == BehaviorStage::PoweringOn){
      CancelSelf();
    }
    
    if(_dVars.behaviorStage == BehaviorStage::AnimationInterruptionRecieved){
       if(_iConfig.powerButtonHeldCondition->AreConditionsMet(GetBEI())){
         TransitionToPoweringOff();
       }else{
         TransitionToPoweringOn();
       }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::OnBehaviorLeftActivatableScope()
{
  _iConfig.powerButtonHeldCondition->SetActive(GetBEI(), false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::TransitionToPoweringOff()
{
  _dVars.behaviorStage = BehaviorStage::PoweringOff;
  const bool havePlayedAnyAnim = _dVars.timeLastPowerAnimStopped_ms > 0;
  const auto startTime_ms = havePlayedAnyAnim ? GetLengthOfAnimation_ms(_iConfig.powerOffAnimName) - _dVars.timeLastPowerAnimStopped_ms : 0;
  
  StartAnimation(_iConfig.powerOffAnimName, startTime_ms);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::TransitionToPoweringOn()
{
  _dVars.behaviorStage = BehaviorStage::PoweringOn;
  const bool havePlayedAnyAnim = _dVars.timeLastPowerAnimStopped_ms > 0;
  const auto startTime_ms = havePlayedAnyAnim ? GetLengthOfAnimation_ms(_iConfig.powerOnAnimName) - _dVars.timeLastPowerAnimStopped_ms : 0;
  
  StartAnimation(_iConfig.powerOnAnimName, startTime_ms);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPoweringRobotOff::StartAnimation(const std::string& animName, const TimeStamp_t startTime_ms)
{
  const u32 numLoops = 1;
  const bool interruptRunning = true;
  const u8 tracksToLock = (u8)AnimTrackFlag::NO_TRACKS;
  const float timeout_sec = PlayAnimationAction::GetDefaultTimeoutInSeconds();
  
  auto callback = [this](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded) {
    if(res != AnimationComponent::AnimResult::Completed){
      _dVars.timeLastPowerAnimStopped_ms = streamTimeAnimEnded;
      _dVars.behaviorStage = BehaviorStage::AnimationInterruptionRecieved;
    }else{
      _dVars.behaviorStage = BehaviorStage::AnimationComplete;
      _dVars.timeLastPowerAnimStopped_ms = 0;
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
