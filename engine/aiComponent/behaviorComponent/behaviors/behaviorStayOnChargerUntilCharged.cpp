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
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
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
  const BatteryLevel batteryLevel = robotInfo.GetBatteryLevel();
  const BatteryLevel prevBatteryLevel = robotInfo.GetPrevBatteryLevel();
  
  const bool isOnCharger = robotInfo.IsOnChargerPlatform();
  const bool isBatteryFull = (batteryLevel == BatteryLevel::Full);
  const bool needsToCharge = (isOnCharger && !isBatteryFull);

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool onCooldown = (currTime_s < (_dVars.lastTimeCancelled_s + kCooldown_s) );

  // check safeguard timer
  const float onChargerDuration_s = robotInfo.GetOnChargerDurationSec();
  const bool safeguard = (onChargerDuration_s > kSafeguardTimeout_s);
  
  // the battery level can drop from Full to Nominal the moment the robot leaves the contacts, so enfore a minimum time
  const bool droppedToNominal = (batteryLevel == BatteryLevel::Nominal) && (prevBatteryLevel == BatteryLevel::Full);
  const float levelDuration_s = robotInfo.GetTimeAtBatteryLevelSec(batteryLevel);
  const bool briefDropToNominal = droppedToNominal && (levelDuration_s < kMinTimeAtNominal_s);

  // if a voice command is pending or active, it may be handled at a lower priority level than this behavior,
  // so don't activate this one (e.g. exploring)
  const auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool hasIntent = uic.IsAnyUserIntentPending() || uic.IsAnyUserIntentActive();
  
  return (needsToCharge && !onCooldown && !safeguard && !briefDropToNominal && !hasIntent);
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

    // if nay condition wants to cancel, set this bool instead of directly cancelling so that all checks will
    // be performed (and the cooldown dVar can be set, if needed)
    bool cancel = false;

    // if we have a pending or active voice intent, voice commands should take priority, so cancel this NOTE:
    // as of this writing, the usage of this behavior is such that most voice commands are higher priority,
    // but "exploring" is handled lower down in HLAI, hence the need for this
    const auto& uic = GetBehaviorComp<UserIntentComponent>();
    const bool hasIntent = uic.IsAnyUserIntentPending() || uic.IsAnyUserIntentActive();
    if( hasIntent ) {
      LOG_INFO("StayOnChargerUntilCharged.BehaviorUpdate.CancelDueToIntent",
               "Cancelling because of a voice intent");
      cancel = true;
    }

    // monitor battery status; if full, cancel delegate and cancel self
    const auto& robotInfo = GetBEI().GetRobotInfo();
    const bool isBatteryFull = (robotInfo.GetBatteryLevel() == BatteryLevel::Full);
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
