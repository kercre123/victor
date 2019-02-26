/**
 * File: BehaviorStayOnChargerUntilCharged.cpp
 *
 * Author: Andrew Stout
 * Created: 2019-01-31
 *
 * Description: WantsToBeActivated if on the charger and battery is not full; delegates to a parameterized behavior in the meantime.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/behaviorStayOnChargerUntilCharged.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "Behaviors.StayOnChargerUntilCharged"

#define CONSOLE_GROUP "StayOnCargerUntilCharged"

CONSOLE_VAR(float, kSafeguardTimeout_s, CONSOLE_GROUP, 30*60.0f); // TODO: get better intuition for reasonable defaults
CONSOLE_VAR(float, kCooldown_s, CONSOLE_GROUP, 20*60.0f);

namespace Anki {
namespace Vector {

namespace {
  const char* kDelegateKey = "delegate";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStayOnChargerUntilCharged::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStayOnChargerUntilCharged::DynamicVariables::DynamicVariables():
  lastTimeCancelled_s(-1.0*kCooldown_s)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStayOnChargerUntilCharged::BehaviorStayOnChargerUntilCharged(const Json::Value& config)
 : ICozmoBehavior(config)
{
  auto debugStr = "BehaviorStayOnChargerUntilCharged.Constructor.MissingDelegateID";
  _iConfig.delegateName = JsonTools::ParseString(config, kDelegateKey, debugStr);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStayOnChargerUntilCharged::InitBehavior()
{
  _iConfig.delegate = FindBehavior(_iConfig.delegateName);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStayOnChargerUntilCharged::~BehaviorStayOnChargerUntilCharged()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorStayOnChargerUntilCharged::WantsToBeActivatedBehavior() const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const bool isOnCharger = robotInfo.IsOnChargerPlatform();
  const bool isBatteryFull = (robotInfo.GetBatteryLevel() == BatteryLevel::Full);
  const bool needsToCharge = (isOnCharger && !isBatteryFull);

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool onCooldown = (currTime_s < (_dVars.lastTimeCancelled_s + kCooldown_s) );

  // check safeguard timer
  const float onChargerDuration_s = robotInfo.GetOnChargerDurationSec();
  const bool safeguard = (onChargerDuration_s > kSafeguardTimeout_s);
  
  return (needsToCharge && !onCooldown && !safeguard);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStayOnChargerUntilCharged::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStayOnChargerUntilCharged::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.delegate.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStayOnChargerUntilCharged::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kDelegateKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStayOnChargerUntilCharged::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  LOG_INFO("StayOnChargerUntilCharged.OnBehaviorActivated.Activated", "Vector is charging, staying on charger.");

  // delegate
  ANKI_VERIFY(_iConfig.delegate->WantsToBeActivated(), "StayOnChargerUntilCharged.DelegateDoesNotWantToActivate", "");
  DelegateIfInControl(_iConfig.delegate.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStayOnChargerUntilCharged::BehaviorUpdate() 
{
  if( IsActivated() ) {
    // monitor battery status; if full, cancel delegate and cancel self
    const auto& robotInfo = GetBEI().GetRobotInfo();
    const bool isBatteryFull = (robotInfo.GetBatteryLevel() == BatteryLevel::Full);
    if (isBatteryFull) {
      LOG_INFO("StayOnChargerUntilCharged.BehaviorUpdate.BatteryFull", "Battery is full, canceling self.");
      CancelSelf(); // (also cancels delegates)
      // TODO: opinions wanted: should we do a cooldown in this case?
    }

    // check safeguard timer
    const float onChargerDuration_s = robotInfo.GetOnChargerDurationSec();
    if (onChargerDuration_s > kSafeguardTimeout_s) {
      LOG_INFO("StayOnChargerUntilCharged.BehaviorUpdate.SafeguardTimeout",
          "Timeout of %f seconds reached, cancelling self", kSafeguardTimeout_s);
      // set the cooldown
      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _dVars.lastTimeCancelled_s = currTime_s;
      CancelSelf();
    }
  }
}

}
}
