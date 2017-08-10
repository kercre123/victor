/**
 * File: localNotifications
 *
 * Author: Paul Terry
 * Created: 08/02/2017
 *
 * Description: Local notifications for Cozmo's app
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/needsSystem/localNotifications.h"
#include "engine/needsSystem/needsManager.h"
#include "anki/common/basestation/jsonTools.h"


namespace Anki {
namespace Cozmo {

static const std::string kLocalNotificationConfigsKey = "localNotificationConfigs";

using namespace std::chrono;


LocalNotifications::LocalNotifications(NeedsManager& needsManager)
: _needsManager(needsManager)
, _localNotificationConfig()
{
}

void LocalNotifications::Init(const Json::Value& json, Util::RandomGenerator* rng)
{
  _rng = rng;

  _localNotificationConfig.items.clear();
  const auto& jsonLocalNotifications = json[kLocalNotificationConfigsKey];
  if (jsonLocalNotifications.isArray())
  {
    const s32 numEntries = jsonLocalNotifications.size();
    _localNotificationConfig.items.reserve(numEntries);

    for (int i = 0; i < numEntries; i++)
    {
      const Json::Value& jsonEntry = jsonLocalNotifications[i];
      LocalNotificationItem item;
      item.SetFromJSON(jsonEntry);
      _localNotificationConfig.items.emplace_back(item);
    }
  }
}


void LocalNotifications::CancelAll()
{
  // TODO:  Send ETG message that Scott's wrapper will handle
}


void LocalNotifications::Generate()
{
  CancelAll();

  const auto& needsState = _needsManager.GetCurNeedsState();
  const auto& needsConfig = _needsManager.GetNeedsConfig();

  NeedsMultipliers multipliers;
  needsState.GetDecayMultipliers(needsConfig._decayUnconnected, multipliers);

  const Time now = system_clock::now();
  const float timeSinceAppOpen_m = (duration_cast<seconds>(now -
                                    needsState._timeLastAppUnBackgrounded).count()) /
                                    60.0f;

  for (const auto& config : _localNotificationConfig.items)
  {
    if (ShouldBeRegistered(config))
    {
      const float duration_m = DetermineTimeToNotify(config, multipliers,
                                                     timeSinceAppOpen_m, now);

      PRINT_CH_INFO(NeedsManager::kLogChannelName,
                    "LocalNotifications.Generate",
                    "Registering: %10.1f minutes; key: \"%s\"",
                    duration_m, config.textKeys[0].c_str());

      // TODO:  Send ETG message that Scott's wrapper will handle, to register this notification with OS
    }
  }
}


bool LocalNotifications::ShouldBeRegistered(const LocalNotificationItem& config) const
{
  bool shouldBeRegistered = true;

  // TODO:  Filter on Connection

  switch (config.notificationMainUnion.GetTag())
  {
    case NotificationUnionTag::notificationGeneral:
    {
      // Nothing to check
      break;
    }

    case NotificationUnionTag::notificationNeedLevel:
    {
      const auto& needLevelConfig = config.notificationMainUnion.Get_notificationNeedLevel();
      const NeedId needId = needLevelConfig.needId;
      const float threshold = needLevelConfig.level;
      const float curLevel = _needsManager.GetCurNeedsState().GetNeedLevel(needId);
      if (curLevel < threshold)
      {
        // The current need level is already below the threshold for this notification,
        // so it's too late to send
        shouldBeRegistered = false;
      }
      break;
    }

    case NotificationUnionTag::notificationNeedBracket:
    {
      const auto& needBracketConfig = config.notificationMainUnion.Get_notificationNeedBracket();
      const NeedId needId = needBracketConfig.needId;
      const NeedBracketId needBracketId = needBracketConfig.needBracketId;
      const NeedBracketId curNeedBracketId = _needsManager.GetCurNeedsStateMutable().GetNeedBracket(needId);
      if (curNeedBracketId >= needBracketId)
      {
        // The current need bracket is already at or below the bracket for this notification,
        // so it's too late to send
        shouldBeRegistered = false;
      }
      break;
    }

    case NotificationUnionTag::notificationDailyTokensToGo:
    {
      const NeedsState state = _needsManager.GetCurNeedsState();
      const int tokensToGo = state._numStarsForNextUnlock - state._numStarsAwarded;
      const auto& needTokensToGoConfig = config.notificationMainUnion.Get_notificationDailyTokensToGo();
      if (tokensToGo != needTokensToGoConfig.numTokensToGo)
      {
        shouldBeRegistered = false;
      }
      break;
    }

    default:
      break;
  }

  return shouldBeRegistered;
}


float LocalNotifications::DetermineTimeToNotify(const LocalNotificationItem& config,
                                                const NeedsMultipliers& multipliers,
                                                const float timeSinceAppOpen_m,
                                                const Time now) const
{
  float minutesInFuture = 0.0f;

  switch (config.notificationMainUnion.GetTag())
  {
    case NotificationUnionTag::notificationGeneral:
    {
      minutesInFuture = CalculateMinutesInFuture(config, timeSinceAppOpen_m, now);
      break;
    }

    case NotificationUnionTag::notificationNeedLevel:
    {
      const auto& needLevelConfig = config.notificationMainUnion.Get_notificationNeedLevel();
      const NeedId needId = needLevelConfig.needId;
      const float threshold = needLevelConfig.level;

      // How long will it take to decay to the given level?
      minutesInFuture = CalculateMinutesToThreshold(needId, threshold, multipliers);
      break;
    }

    case NotificationUnionTag::notificationNeedBracket:
    {
      const auto& needBracketConfig = config.notificationMainUnion.Get_notificationNeedBracket();
      const NeedId needId = needBracketConfig.needId;
      const NeedBracketId bracketId = needBracketConfig.needBracketId;

      // How long will it take to decay to the TOP of the given bracket?
      // (e.g. when Cozmo's Energy level BECOMES 'critical')
      const int prevBracketIndex = static_cast<int>(bracketId) - 1;
      if (prevBracketIndex >= 0)
      {
        const NeedBracketId prevBracketId = static_cast<NeedBracketId>(prevBracketIndex);
        const auto& needsConfig = _needsManager.GetNeedsConfig();
        const float threshold = needsConfig.NeedLevelForNeedBracket(needId, prevBracketId);

        minutesInFuture = CalculateMinutesToThreshold(needId, threshold, multipliers);
      }
      else
      {
        PRINT_CH_INFO(NeedsManager::kLogChannelName,
                      "LocalNotifications.DetermineTimeToNotify",
                      "Need Bracket notification specifies 'full' which doesn't make sense");
      }
      break;
    }

    case NotificationUnionTag::notificationDailyTokensToGo:
    {
      minutesInFuture = CalculateMinutesInFuture(config, timeSinceAppOpen_m, now);
      break;
    }

    default:
      break;
  }
  
  // Allow for a random uniform distribution if desired
  const float range = config.rangeEarly + config.rangeLate;
  const float randDist = _rng->RandDbl(range) - config.rangeEarly;
  minutesInFuture += randDist;

  // Enforce minimum duration
  if (minutesInFuture < config.minimumDuration)
  {
    minutesInFuture = config.minimumDuration;
  }

  // Enforce maximum duration
  const float maxMinutes = _needsManager.GetNeedsConfig()._localNotificationMaxFutureMinutes;
  if (minutesInFuture > maxMinutes)
  {
    PRINT_CH_INFO(NeedsManager::kLogChannelName,
                  "LocalNotifications.DetermineTimeToNotify",
                  "Clamping local notification time at the configured maximum of %.1f minutes",
                  maxMinutes);
    minutesInFuture = maxMinutes;
  }

  return minutesInFuture;
}


float LocalNotifications::CalculateMinutesInFuture(const LocalNotificationItem& config,
                                                   const float timeSinceAppOpen_m,
                                                   const Time now) const
{
  float minutesInFuture = 0.0f;

  switch (config.whenType)
  {
    case WhenType::NotApplicable:
    {
      DEV_ASSERT_MSG(config.whenType != WhenType::NotApplicable,
                     "LocalNotifications.CalculateMinutesInFuture",
                     "Notification config has 'when type' of NotApplicable but wants an applicable when type");
      break;
    }

    case WhenType::AfterAppOpen:
    {
      minutesInFuture = config.whenParam - timeSinceAppOpen_m;
      break;
    }

    case WhenType::AfterAppClose:
    {
      minutesInFuture = config.whenParam;
      break;
    }

    case WhenType::ClockTime:
    {
      // First, create a Time for today, at the config's clock time:
      const std::time_t nowTimeT = system_clock::to_time_t(now);
      std::tm targetLocalTime;
      localtime_r(&nowTimeT, &targetLocalTime);
      const int clockTimeHours = static_cast<int>(config.whenParam / 60.0f);
      const int clockTimeMinutes = static_cast<int>(fmodf(config.whenParam, 60.0f));
      targetLocalTime.tm_hour = clockTimeHours;
      targetLocalTime.tm_min = clockTimeMinutes;
      std::time_t targetTimeT = mktime(&targetLocalTime);

      // If we've already passed this time of day, make it the NEXT day
      if (targetTimeT <= nowTimeT)
      {
        targetTimeT += (24 * 60 * 60);
      }
      minutesInFuture = (targetTimeT - nowTimeT) / 60.0f;

      // Finally, enforce the 'minimum duration' by bumping ahead a day if needed
      if (minutesInFuture < config.minimumDuration)
      {
        minutesInFuture += (24 * 60);
      }
      break;
    }
  }

  return minutesInFuture;
}


float LocalNotifications::CalculateMinutesToThreshold(const NeedId needId, const float targetLevel,
                                                      const NeedsMultipliers& multipliers) const
{
  // Calculate how long it will take (in minutes) for curLevel to decay
  // (with unconnected decay rates) to the target level
  const auto& needsState = _needsManager.GetCurNeedsState();
  const auto& needsConfig = _needsManager.GetNeedsConfig();

  const float minutesInFuture = needsState.TimeForDecayToLevel(needsConfig._decayUnconnected,
                                                               static_cast<int>(needId),
                                                               targetLevel,
                                                               multipliers);

  if (minutesInFuture == std::numeric_limits<float>::max())
  {
    // If we calculated "infinite" (which could be the case when a
    // decay bracket has a rate of zero), report this
    PRINT_CH_INFO(NeedsManager::kLogChannelName,
                  "LocalNotifications.CalculateMinutesToThreshold",
                  "Notification set to infinite future");
  }

  return minutesInFuture;
}



} // namespace Cozmo
} // namespace Anki

