/**
 * File: BehaviorReactToBatteryTooHotToCharge.cpp
 *
 * Author: Arjun Menon
 * Created: 2019-02-01
 *
 * Description: simple animation reaction behavior that listens for when the robot 
 * disconnects its battery upon being placed on the charger (due to high temperature) 
 * and needs to inform the user of the high temp warning
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToBatteryTooHotToCharge.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBatteryTooHotToCharge::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBatteryTooHotToCharge::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBatteryTooHotToCharge::BehaviorReactToBatteryTooHotToCharge(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBatteryTooHotToCharge::~BehaviorReactToBatteryTooHotToCharge()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToBatteryTooHotToCharge::WantsToBeActivatedBehavior() const
{
  return _dVars.persistent.animationPending;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBatteryTooHotToCharge::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBatteryTooHotToCharge::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBatteryTooHotToCharge::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBatteryTooHotToCharge::OnBehaviorActivated() 
{
  auto action = new TriggerLiftSafeAnimationAction(AnimationTrigger::HighTemperatureWarningFace);
  DelegateIfInControl(action, [this](){
    _dVars.persistent.animationPending = false; // blocks successive reactions
    CancelSelf();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBatteryTooHotToCharge::OnBehaviorDeactivated() 
{
  auto copy = _dVars.persistent;
  _dVars = BehaviorReactToBatteryTooHotToCharge::DynamicVariables();
  _dVars.persistent = copy;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBatteryTooHotToCharge::BehaviorUpdate()
{
  
  const bool isDisconnected = GetBEI().GetRobotInfo().GetBatteryComponent().IsBatteryDisconnectedFromCharger();
  const bool isCharging = GetBEI().GetRobotInfo().GetBatteryComponent().IsCharging();
  const bool disconnectedBecauseTooHot = isDisconnected && isCharging;

  if(!IsActivated() && disconnectedBecauseTooHot && !_dVars.persistent.lastDisconnectedBecauseTooHot) {
    _dVars.persistent.animationPending = true;
  }
  _dVars.persistent.lastDisconnectedBecauseTooHot = disconnectedBecauseTooHot;
}

}
}
