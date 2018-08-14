/**
 * File: sleepTracker.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-08-15
 *
 * Description: Behavior component to track sleep and sleepiness
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#define LOG_CHANNEL "Behaviors"

#include "engine/aiComponent/behaviorComponent/sleepTracker.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/cozmoContext.h"
#include "engine/wallTime.h"
#include "lib/util/source/anki/util/entityComponent/dependencyManagedEntity.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "webServerProcess/src/webService.h"
#include "webServerProcess/src/webVizSender.h"

namespace Anki {
namespace Vector {

#define CONSOLE_GROUP "Sleeping"

CONSOLE_VAR_RANGED(int, kSleepTracker_moning_hour, CONSOLE_GROUP, 7, 0, 23);
CONSOLE_VAR_RANGED(int, kSleepTracker_moning_minute, CONSOLE_GROUP, 0, 0, 59);

// NOTE: Because of a lazy developer, night time must be before midnight
CONSOLE_VAR_RANGED(int, kSleepTracker_night_hour, CONSOLE_GROUP, 21, 0, 23);
CONSOLE_VAR_RANGED(int, kSleepTracker_night_minute, CONSOLE_GROUP, 0, 0, 59);

// gain sleep debt at this rate when awake
CONSOLE_VAR(float, kSleepTracker_awakeSleepDebtRate, CONSOLE_GROUP, 20.0 / 4.0);
CONSOLE_VAR(float, kSleepTracker_maxSleepDebt_hours, CONSOLE_GROUP, 20.0);

CONSOLE_VAR(float, kSleepTracker_debtToConsiderSleepy_awake, CONSOLE_GROUP, 60.0f * 60.0f);
CONSOLE_VAR(float, kSleepTracker_debtToConsiderSleepy_fromSleep, CONSOLE_GROUP, 30.0f * 60.0f);


CONSOLE_VAR(float, kSleepTracker_updatePeriod_s, CONSOLE_GROUP, 60.0);


namespace {
#if REMOTE_CONSOLE_ENABLED
  static float sForceSleepDebtDelta_s = 0.0f;
  void ForceAddSleepDebtHours(ConsoleFunctionContextRef context)
  {
    sForceSleepDebtDelta_s = ConsoleArg_Get_Float(context, "deltaHours") * 60.0f * 60.0f;
  }
#endif

  static const char* kWebVizModuleNameSleeping = "sleeping";
}

CONSOLE_FUNC(ForceAddSleepDebtHours, CONSOLE_GROUP, float deltaHours);

SleepTracker::SleepTracker()
  : IDependencyManagedComponent(this, BCComponentID::SleepTracker )
{
}

void SleepTracker::SetIsSleeping(bool sleeping)
{
  LOG_INFO("SleepTracker.SetSleeping",
           "%s",
           sleeping ? "asleep" : "awake");

  _asleep = sleeping;
}

void SleepTracker::InitDependent( Robot* robot, const BCCompMap& dependentComps )
{
  const auto* context = dependentComps.GetComponent<BEIRobotInfo>().GetContext();
  if( context != nullptr ) {
    auto* webService = context->GetWebService();
    if( webService != nullptr ) {

      auto onWebVizSubscribed = [this](const std::function<void(const Json::Value&)>& sendToClient) {
        // just got a subscription, send now
        Json::Value data;
        PopulateWebVizJson(data);
        sendToClient(data);
      };
      _eventHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleNameSleeping ).ScopedSubscribe(
                                    onWebVizSubscribed ));
    }
  }
}

void SleepTracker::UpdateDependent(const BCCompMap& dependentComps)
{
  bool consoleFuncUsed = false;
#if REMOTE_CONSOLE_ENABLED
  consoleFuncUsed = sForceSleepDebtDelta_s != 0.0f;
#endif

  const float currBSTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( consoleFuncUsed || currBSTime_s > _nextUpdateTime_s ) {
    _nextUpdateTime_s = currBSTime_s + kSleepTracker_updatePeriod_s;

    if( _asleep ) {
      _sleepDebt_s -= kSleepTracker_updatePeriod_s;
    }
    else {
      _sleepDebt_s += kSleepTracker_updatePeriod_s * kSleepTracker_awakeSleepDebtRate;
    }

#if REMOTE_CONSOLE_ENABLED
    if( consoleFuncUsed ) {
      PRINT_NAMED_WARNING("SleepTracker.ConsoleFunc.AddSleepDebt",
                          "Add %f seconds of sleep debt",
                          sForceSleepDebtDelta_s);
      _sleepDebt_s += sForceSleepDebtDelta_s;
      sForceSleepDebtDelta_s = 0.0f;
    }
#endif

    _sleepDebt_s = Util::Clamp(_sleepDebt_s, 0.0f, kSleepTracker_maxSleepDebt_hours * 60 * 60);

    const auto* context = dependentComps.GetComponent<BEIRobotInfo>().GetContext();
    if( context ) {
      if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameSleeping, context->GetWebService()) ) {
        PopulateWebVizJson(webSender->Data());
      }
    }
  }
}

bool SleepTracker::IsSleepy(const bool fromSleep) const
{
  const float debtToConsiderSleepy = fromSleep ?
    kSleepTracker_debtToConsiderSleepy_fromSleep :
    kSleepTracker_debtToConsiderSleepy_awake;

  const bool isSleepy = _sleepDebt_s > debtToConsiderSleepy;
  return isSleepy;
}

bool SleepTracker::IsNightTime() const
{
  struct tm localTime;
  if(!WallTime::getInstance()->GetLocalTime(localTime)){
    // arbitrarily default to "night"
    return true;
  }

  const int currMins = localTime.tm_min;
  const int currHours = localTime.tm_hour;

  // convert to decimal hours to make life a bit easier
  const float currDecimalHours = currHours + currMins / 60.0f;

  const float morningDecimalHours = kSleepTracker_moning_hour + kSleepTracker_moning_minute / 60.0f;
  const float nightDecimalHours = kSleepTracker_night_hour + kSleepTracker_night_minute / 60.0f;

  DEV_ASSERT(morningDecimalHours < nightDecimalHours, "SleepTracker.MorningMustBeBeforeNight");

  if( Util::InRange( currDecimalHours, morningDecimalHours, nightDecimalHours ) ) {
    return false;
  }
  else {
    return true;
  }
}

void SleepTracker::EnsureSleepDebtAtLeast(const float minDebt_s)
{
  if(minDebt_s < kSleepTracker_debtToConsiderSleepy_fromSleep) {
    PRINT_NAMED_WARNING("SleepTracker.EnsureSleepDebtAtLeast.MinDebtProbablyTooLow",
                        "You specified a min sleep debt of %f, but the min to wake up from sleep is higher at %f",
                        minDebt_s,
                        kSleepTracker_debtToConsiderSleepy_fromSleep);
  }

  if( _sleepDebt_s < minDebt_s ) {
    // set to min but also make sure it's in range
    _sleepDebt_s = Util::Clamp(minDebt_s, 0.0f, kSleepTracker_maxSleepDebt_hours * 60 * 60);
  }
}

void SleepTracker::PopulateWebVizJson(Json::Value& data) const
{
  data["sleep_debt_hours"] = _sleepDebt_s / (3600.0f);
}

}
}
