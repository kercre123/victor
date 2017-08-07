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


#include "anki/cozmo/basestation/needsSystem/localNotifications.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/common/basestation/jsonTools.h"


namespace Anki {
namespace Cozmo {

static const std::string kLocalNotificationConfigsKey = "localNotificationConfigs";


LocalNotifications::LocalNotifications(NeedsManager& needsManager)
: _needsManager(needsManager)
, _localNotificationConfig()
{
}

void LocalNotifications::Init(const Json::Value& json)
{
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

  for (const auto& config : _localNotificationConfig.items)
  {
    if (ShouldBeRegistered(config))
    {
      const float duration_m = DetermineTimeToNotify(config);

      PRINT_CH_INFO(NeedsManager::kLogChannelName,
                    "LocalNotifications.Generate",
                    "Registering: %10.1f minutes; key: \"%s\"",
                    duration_m, config.textKeys[0].c_str());

      // TODO:  Send ETG message that Scott's wrapper will handle, to register this notification with OS
    }
  }
}


bool LocalNotifications::ShouldBeRegistered(const LocalNotificationItem& config)
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
      // todo:  Ensure number of tokens to go is appropriate
      break;
    }

    default:
      break;
  }

  return shouldBeRegistered;
}


float LocalNotifications::DetermineTimeToNotify(const LocalNotificationItem& config)
{
  float minutesInFuture = 0.0f;

  switch (config.notificationMainUnion.GetTag())
  {
    case NotificationUnionTag::notificationGeneral:
    {
      // TODO: use WhenType and WhenParam
      break;
    }

    case NotificationUnionTag::notificationNeedLevel:
    {
      const auto& needLevelConfig = config.notificationMainUnion.Get_notificationNeedLevel();
      const NeedId needId = needLevelConfig.needId;
      const float threshold = needLevelConfig.level;

      // How long will it take to decay to the given level?
      minutesInFuture = CalculateMinutesToThreshold(needId, threshold);
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

        minutesInFuture = CalculateMinutesToThreshold(needId, threshold);
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
      // TODO: use WhenType and WhenParam
      break;
    }

    default:
      break;
  }
  
  // TODO:  RangeEarly, RangeLate

  // Enforce minimum duration
  if (minutesInFuture < config.minimumDuration)
  {
    minutesInFuture = config.minimumDuration;
  }

  return minutesInFuture;
}


float LocalNotifications::CalculateMinutesToThreshold(const NeedId needId, const float targetLevel)
{
  // Calculate how long it will take (in minutes) for curLevel to decay
  // (with unconnected decay rates) to the target level
  const auto& needsState = _needsManager.GetCurNeedsState();
  const auto& needsConfig = _needsManager.GetNeedsConfig();

  NeedsMultipliers multipliers;
  needsState.SetDecayMultipliers(needsConfig._decayUnconnected, multipliers);

  const float minutesInFuture = needsState.TimeForDecayToLevel(needsConfig._decayUnconnected,
                                                              static_cast<int>(needId),
                                                              targetLevel,
                                                              multipliers);

  if (minutesInFuture == std::numeric_limits<float>::max())
  {
    // If we calculated "infinite" (which could be the case when a
    // decay bracket has a rate of zero), report this
    PRINT_CH_INFO(NeedsManager::kLogChannelName,
                  "LocalNotifications.DetermineTimeToNotify",
                  "Notification set to infinite future");
  }

  return minutesInFuture;
}



} // namespace Cozmo
} // namespace Anki

