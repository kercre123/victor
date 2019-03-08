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
CONSOLE_VAR(float, kMinTimeAtNominal_s, CONSOLE_GROUP, 4.0f); // >= time for any drive-off-charger anim to clear charger platform

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
  persistent.batteryLevel = BatteryLevel::Unknown;
  persistent.prevBatteryLevel = BatteryLevel::Unknown;
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
  const bool isBatteryFull = (_dVars.persistent.batteryLevel == BatteryLevel::Full);
  const bool needsToCharge = (isOnCharger && !isBatteryFull);

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool onCooldown = (currTime_s < (_dVars.lastTimeCancelled_s + kCooldown_s) );

  // check safeguard timer
  const float onChargerDuration_s = robotInfo.GetOnChargerDurationSec();
  const bool safeguard = (onChargerDuration_s > kSafeguardTimeout_s);
  
  // The battery level can drop from Full to Nominal the moment the robot leaves the contacts, so enfore a minimum time
  const bool droppedToNominal = (_dVars.persistent.batteryLevel == BatteryLevel::Nominal)
                                && (_dVars.persistent.prevBatteryLevel == BatteryLevel::Full);
  const float levelDuration_s = robotInfo.GetTimeAtBatteryLevelSec(_dVars.persistent.batteryLevel);
  const bool briefDropToNominal = droppedToNominal && (levelDuration_s < kMinTimeAtNominal_s);
  
  return (needsToCharge && !onCooldown && !safeguard && !briefDropToNominal);
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
  auto persistent = std::move(_dVars.persistent);
  _dVars = DynamicVariables();
  _dVars.persistent = std::move(persistent);

  LOG_INFO("StayOnChargerUntilCharged.OnBehaviorActivated.Activated", "Vector is charging, staying on charger.");

  // delegate
  ANKI_VERIFY(_iConfig.delegate->WantsToBeActivated(), "StayOnChargerUntilCharged.DelegateDoesNotWantToActivate", "");
  DelegateIfInControl(_iConfig.delegate.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStayOnChargerUntilCharged::BehaviorUpdate() 
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  BatteryLevel batteryLevel = robotInfo.GetBatteryLevel();
  if (batteryLevel != _dVars.persistent.batteryLevel) {
    _dVars.persistent.prevBatteryLevel = _dVars.persistent.batteryLevel;
    _dVars.persistent.batteryLevel = batteryLevel;
  }
  
  if( IsActivated() ) {
    // monitor battery status; if full, cancel delegate and cancel self
    const bool isBatteryFull = (_dVars.persistent.batteryLevel == BatteryLevel::Full);
    bool cancel = false;
    if (isBatteryFull) {
      LOG_INFO("StayOnChargerUntilCharged.BehaviorUpdate.BatteryFull", "Battery is full, canceling self.");
      cancel = true;
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
      cancel = true;
    }
    
    if (cancel) {
      CancelSelf();
    }
  }
}

}
}
